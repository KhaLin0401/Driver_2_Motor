#include "Encoder.h"
#include "ModbusMap.h"
#include "main.h"
#include "MotorControl.h"

#include <math.h>
#include <stdint.h>
#include <stdbool.h>

// ═══════════════════════════════════════════════════════════════════════════════
// ENCODER CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════
// ⚠️ HARDWARE CONFIGURATION:
// - Encoder: 1-channel optical encoder (chỉ có kênh A)
// - Timer Mode: TIM_ENCODERMODE_TI1 (CH1 đếm xung, CH2 xác định chiều)
// - Problem: CH2 không có tín hiệu → đọc noise → counter nhảy lung tung
// 
// ✅ SOLUTION APPLIED:
// 1. Hardware: IC2Filter = 15 (MAX) để chặn noise trên CH2
// 2. Software: Treat counter as ABSOLUTE position (always increasing)
// 3. Direction: Use Motor_Direction để xác định chiều thực tế
// ═══════════════════════════════════════════════════════════════════════════════

#define ENCODER_SLOTS           8       // Physical slots on encoder disk
#define ENCODER_PPR             8       // Pulses Per Revolution (same as slots for optical encoder)
#define ENCODER_CPR             16      // Counts Per Revolution = 16 counts/rev (8 slots × 2 edges)
#define ENCODER_RESOLUTION      (360.0f / ENCODER_CPR)  // Degrees per count = 22.5°/count

// ═══════════════════════════════════════════════════════════════════════════════
// SPOOL GEOMETRY CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════
// These values MUST be measured and calibrated for your specific hardware
// ═══════════════════════════════════════════════════════════════════════════════

#define WIRE_LENGTH_CM          300         // Total wire length in cm (3 meters)
#define WIRE_LENGTH_MM          3000.0f     // Total wire length in mm (for calculation)
#define SPOOL_RADIUS_FULL_MM    35.0f       // Radius when spool is full (mm) - MEASURE THIS!
#define SPOOL_RADIUS_EMPTY_MM   10.0f       // Radius of empty core (mm) - MEASURE THIS!
#define WIRE_DIAMETER_MM        0.5f        // Wire diameter (mm) - for advanced model

// ═══════════════════════════════════════════════════════════════════════════════
// NOISE FILTERING & STABILITY CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════

#define FILTER_ALPHA            0.15f       // Low-pass filter coefficient (0.0-1.0)
                                            // Lower = smoother but slower response
                                            // Higher = faster but more noise
                                            
#define NOISE_THRESHOLD_TICKS   2           // Ignore changes smaller than this
                                            // Helps reject electrical noise
                                            
#define MAX_DELTA_PER_CYCLE     200         // Maximum expected count change per read cycle
                                            // Reject values above this as noise/error
                                            
#define AUTO_RESET_THRESHOLD    32000       // Auto-reset counter before Modbus int16 overflow

// ═══════════════════════════════════════════════════════════════════════════════
// PRIVATE STATE VARIABLES
// ═══════════════════════════════════════════════════════════════════════════════

typedef struct {
    // Position tracking
    float unrolled_length_mm;       // Accumulated linear position (mm)
    float current_radius_mm;        // Current spool radius (mm)
    int32_t total_encoder_ticks;    // Cumulative encoder ticks (signed, for bidirectional)
    
    // Hardware state
    uint16_t last_hardware_count;   // Previous raw hardware counter value
    uint16_t stable_count;          // Debounced/validated counter value
    uint16_t last_encoder_count;    // Previous encoder reading for delta calculation
    // Filtering
    float filtered_length_mm;       // Low-pass filtered length for noise reduction
    
    // Diagnostics
    uint32_t noise_reject_count;    // Number of rejected noisy readings
    uint32_t overflow_count;        // Number of counter overflows handled
    
    bool initialized;               // Initialization flag
} EncoderState_t;

static EncoderState_t encoder_state = {
    .unrolled_length_mm = 0.0f,
    .current_radius_mm = SPOOL_RADIUS_FULL_MM,
    .total_encoder_ticks = 0,
    .last_hardware_count = 0,
    .stable_count = 0,
    .last_encoder_count = 0,
    .filtered_length_mm = 0.0f,
    .noise_reject_count = 0,
    .overflow_count = 0,
    .initialized = false
};

Encoder_t encoder1;
//Encoder_t encoder2;

/**
 * @brief Initialize encoder hardware and state variables
 * 
 * This function:
 * 1. Starts the STM32 hardware encoder interface (TIM2)
 * 2. Resets all encoder counters to zero
 * 3. Initializes the encoder state structure
 * 4. Sets the initial spool radius to maximum (full spool)
 */
void Encoder_Init(void){
    // Initialize encoder hardware structure
    encoder1.Calib_Origin_Status = false;
    encoder1.Encoder_Count = 0;
    encoder1.Encoder_Config = 100;
    encoder1.Encoder_Reset = 0;
    encoder1.Encoder_Calib_Sensor_Status = 0;
    encoder1.Encoder_Calib_Length_CM_Max = WIRE_LENGTH_CM;
    encoder1.Encoder_Calib_Start = 0;
    encoder1.Encoder_Calib_Status = 0;
    encoder1.Encoder_Calib_Current_Length_CM = 0;
    
    // Start STM32 hardware encoder (TIM2 in quadrature mode)
    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_1);
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    
    // Initialize encoder state tracking
    encoder_state.unrolled_length_mm = 0.0f;
    encoder_state.current_radius_mm = SPOOL_RADIUS_FULL_MM;
    encoder_state.last_hardware_count = 0;
    encoder_state.last_encoder_count = 0;
    encoder_state.total_encoder_ticks = 0;
    encoder_state.filtered_length_mm = 0.0f;
    encoder_state.initialized = true;
}

/**
 * ═══════════════════════════════════════════════════════════════════════════════
 * @brief Read and validate encoder hardware counter with noise rejection
 * ═══════════════════════════════════════════════════════════════════════════════
 * 
 * PROBLEM ANALYSIS:
 * ─────────────────────────────────────────────────────────────────────────────
 * 1. TIM_ENCODERMODE_TI1 still uses CH2 for direction → noise on CH2 causes
 *    random up/down counting even though we only have 1-channel encoder
 * 
 * 2. Counter can jump up/down unpredictably → negative values appear
 * 
 * 3. Hardware filter (IC1Filter=5) helps but not enough for noisy environments
 * 
 * SOLUTION:
 * ─────────────────────────────────────────────────────────────────────────────
 * 1. Treat hardware counter as ABSOLUTE position (ignore hardware direction)
 * 2. Calculate delta as UNSIGNED difference (always positive)
 * 3. Use Motor_Direction to determine actual movement direction
 * 4. Apply software noise filtering:
 *    - Reject unreasonably large jumps (>MAX_DELTA_PER_CYCLE)
 *    - Ignore very small changes (<NOISE_THRESHOLD_TICKS)
 * 5. Auto-reset counter before Modbus int16 overflow (>32000)
 * 
 * ═══════════════════════════════════════════════════════════════════════════════
 */
void Encoder_Read(Encoder_t* encoder){
    // ───────────────────────────────────────────────────────────────────────────
    // Handle manual reset request
    // ───────────────────────────────────────────────────────────────────────────
    if(encoder->Encoder_Reset == true){
        __HAL_TIM_SET_COUNTER(&htim2, 0);
        encoder->Encoder_Count = 0;
        encoder->Encoder_Reset = 0;
        
        // Reset tracking state to prevent large delta on next read
        encoder_state.last_hardware_count = 0;
        encoder_state.stable_count = 0;
        encoder_state.total_encoder_ticks = 0;
        
        return;  // Skip processing this cycle
    }
    
    // ───────────────────────────────────────────────────────────────────────────
    // Read raw hardware counter (16-bit: 0-65535)
    // ───────────────────────────────────────────────────────────────────────────
    uint16_t raw_count = __HAL_TIM_GET_COUNTER(&htim2);
    
    // ───────────────────────────────────────────────────────────────────────────
    // Calculate UNSIGNED delta (always positive increment)
    // ───────────────────────────────────────────────────────────────────────────
    // Since we're treating counter as absolute position, we only care about
    // how much it increased, not whether hardware thinks it went up or down
    
    uint16_t delta_abs = 0;
    bool counter_wrapped = false;
    
    if (raw_count >= encoder_state.last_hardware_count) {
        // Normal case: counter increased
        delta_abs = raw_count - encoder_state.last_hardware_count;
    } else {
        // Counter wrapped around (overflow or auto-reset)
        // Assume it wrapped from 65535→0 or from AUTO_RESET_THRESHOLD→0
        delta_abs = raw_count;  // Just use the new value as delta
        counter_wrapped = true;
        encoder_state.overflow_count++;
    }
    
    // ───────────────────────────────────────────────────────────────────────────
    // NOISE REJECTION: Filter out unrealistic jumps
    // ───────────────────────────────────────────────────────────────────────────
    // If delta is too large, it's likely noise or a glitch - reject it
    
    if (delta_abs > MAX_DELTA_PER_CYCLE && !counter_wrapped) {
        // Reject this reading - keep previous value
        encoder_state.noise_reject_count++;
        return;  // Don't update anything
    }
    
    // ───────────────────────────────────────────────────────────────────────────
    // NOISE REJECTION: Ignore tiny changes (likely electrical noise)
    // ───────────────────────────────────────────────────────────────────────────
    
    if (delta_abs < NOISE_THRESHOLD_TICKS && delta_abs > 0) {
        // Too small to be real movement - likely noise
        encoder_state.noise_reject_count++;
        return;  // Keep previous value
    }
    
    // ───────────────────────────────────────────────────────────────────────────
    // Valid reading - update state
    // ───────────────────────────────────────────────────────────────────────────
    
    encoder_state.last_hardware_count = raw_count;
    encoder_state.stable_count = raw_count;
    encoder->Encoder_Count = raw_count;
    
    // ───────────────────────────────────────────────────────────────────────────
    // AUTO-RESET: Prevent Modbus int16 overflow
    // ───────────────────────────────────────────────────────────────────────────
    // Modbus uses signed int16 (-32768 to +32767)
    // If counter exceeds 32000, reset it to prevent negative values in Modbus
    
    if(raw_count >= AUTO_RESET_THRESHOLD){
        __HAL_TIM_SET_COUNTER(&htim2, 0);
        encoder->Encoder_Count = 0;
        encoder_state.last_hardware_count = 0;
        encoder_state.stable_count = 0;
        // Note: Don't reset total_encoder_ticks - it tracks cumulative position
    }
}

void Encoder_Write(Encoder_t* encoder, uint16_t value){
    encoder->Encoder_Calib_Current_Length_CM = value;
}

void Encoder_Reset(Encoder_t* encoder){
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    encoder->Encoder_Count = 0;
    encoder->Encoder_Calib_Current_Length_CM = 0;
    
    // ✅ Reset wire length calculation để tránh tính toán sai
    Encoder_ResetWireLength(encoder);
}

/**
 * @brief Load encoder configuration from Modbus registers into structure
 * 
 * DATA FLOW: Modbus Registers → Encoder_t Structure
 * 
 * This function loads ONLY configuration and command registers that the
 * Modbus master can write. Measured values (encoder count, wire length,
 * sensor status) are NOT loaded because they come from hardware/firmware.
 * 
 * DESIGN RATIONALE:
 * ─────────────────────────────────────────────────────────────────────
 * ✅ Load: Configuration registers (master writes these)
 * ❌ Skip: Measured values (firmware generates these)
 * 
 * This prevents data flow conflicts and ensures clear ownership:
 * • Master owns: Configuration, Commands
 * • Firmware owns: Measurements, Status
 */
void Encoder_Load(Encoder_t* encoder){
    // ═══════════════════════════════════════════════════════════════
    // CONFIGURATION REGISTERS (Modbus Master → Firmware)
    // ═══════════════════════════════════════════════════════════════
    encoder->Encoder_Config = g_holdingRegisters[REG_M1_ENCODER_CONFIG];
    encoder->Encoder_Reset = g_holdingRegisters[REG_M1_ENCODER_RESET];
    encoder->Encoder_Calib_Sensor_Status = g_holdingRegisters[REG_M1_CALIB_SENSOR_STATUS];
    encoder->Encoder_Calib_Length_CM_Max = g_holdingRegisters[REG_M1_CALIB_DISTANCE_CM];
    encoder->Encoder_Calib_Start = g_holdingRegisters[REG_M1_CALIB_START];
    
    // ═══════════════════════════════════════════════════════════════
    // MEASURED VALUES (DO NOT load from Modbus)
    // ═══════════════════════════════════════════════════════════════
    // These are OUTPUT values generated by firmware:
    //
    // ❌ encoder->Encoder_Count
    //    Source: TIM2 hardware counter (read in Encoder_Read())
    //
    // ❌ encoder->Calib_Current_Length_CM
    //    Source: Calculated by Encoder_MeasureLength()
    //
    // ❌ encoder->Calib_Origin_Status
    //    Source: GPIO pin (read in Encoder_Check_Calib_Origin())
    //
    // ❌ encoder->Calib_Status
    //    Source: Firmware calibration state machine
}

/**
 * @brief Save encoder state from structure to Modbus registers
 * 
 * DATA FLOW: Encoder_t Structure → Modbus Registers
 * 
 * This function saves ALL encoder fields to Modbus registers, making them
 * visible to the Modbus master for monitoring and control.
 * 
 * DESIGN RATIONALE:
 * ─────────────────────────────────────────────────────────────────────
 * ✅ Save ALL fields (both measured and configured)
 * ✅ Provide complete visibility to master
 * ✅ Enable configuration echo-back for verification
 * ✅ Convert bool to uint16_t safely (0 or 1)
 * 
 * BOOLEAN HANDLING:
 * ─────────────────────────────────────────────────────────────────────
 * Use explicit ternary operator for bool → uint16_t conversion:
 *   value ? 1 : 0
 * 
 * This ensures:
 * • Always 0 or 1 (never garbage values)
 * • Modbus standard compliance
 * • Clear, readable code
 * • Safe across all compilers
 */
void Encoder_Save(Encoder_t* encoder){
    // ═══════════════════════════════════════════════════════════════
    // MEASURED VALUES (Firmware → Modbus Master)
    // ═══════════════════════════════════════════════════════════════
    g_holdingRegisters[REG_M1_ENCODER_COUNT] = encoder->Encoder_Count;
    g_holdingRegisters[REG_M1_UNROLLED_WIRE_LENGTH_CM] = encoder->Encoder_Calib_Current_Length_CM;
    g_holdingRegisters[REG_M1_CALIB_STATUS] = encoder->Encoder_Calib_Status;
    
    // ═══════════════════════════════════════════════════════════════
    // BOOLEAN TO MODBUS CONVERSION (CRITICAL!)
    // ═══════════════════════════════════════════════════════════════
    // Modbus standard: 0 = FALSE, 1 = TRUE (for holding registers)
    // Use explicit ternary operator for safety and clarity
    g_holdingRegisters[REG_M1_CALIB_ORIGIN_STATUS] = encoder->Calib_Origin_Status ? 1 : 0;
    
    // ═══════════════════════════════════════════════════════════════
    // CONFIGURATION ECHO-BACK (for master verification)
    // ═══════════════════════════════════════════════════════════════
    g_holdingRegisters[REG_M1_ENCODER_CONFIG] = encoder->Encoder_Config;
    g_holdingRegisters[REG_M1_ENCODER_RESET] = encoder->Encoder_Reset;
    g_holdingRegisters[REG_M1_CALIB_SENSOR_STATUS] = encoder->Encoder_Calib_Sensor_Status;
    g_holdingRegisters[REG_M1_CALIB_DISTANCE_CM] = encoder->Encoder_Calib_Length_CM_Max;
    g_holdingRegisters[REG_M1_CALIB_START] = encoder->Encoder_Calib_Start;
}

void Encoder_Process(Encoder_t* encoder){
    
    Encoder_Check_Calib_Origin(encoder);

    Encoder_Read(encoder);  // Collect encoder count
    
    // ✅ FIX: Lưu kết quả đo độ dài vào struct encoder
    uint16_t measured_length = Encoder_MeasureLength(encoder);
    encoder->Encoder_Calib_Current_Length_CM = measured_length / 10; // Convert mm to cm

}

/**
 * ═══════════════════════════════════════════════════════════════════════════════
 * @brief Calculate unrolled wire length from encoder position
 * ═══════════════════════════════════════════════════════════════════════════════
 * 
 * ALGORITHM IMPROVEMENTS:
 * ──────────────────────────────────────────────────────────────────────────────
 * 1. ✅ Use validated stable_count (already noise-filtered in Encoder_Read)
 * 2. ✅ Calculate UNSIGNED delta (counter only increases)
 * 3. ✅ Apply motor direction to determine actual movement
 * 4. ✅ Use NONLINEAR spool radius model (more accurate than linear)
 * 5. ✅ Apply low-pass filter for smooth output
 * 
 * DIRECTION HANDLING:
 * ──────────────────────────────────────────────────────────────────────────────
 * Since hardware counter always increases (1-channel encoder), we use motor
 * direction to determine actual wire movement:
 * 
 * - FORWARD (Direction = 1): Wire unrolling → length increases
 * - REVERSE (Direction = 2): Wire rewinding → length decreases  
 * - IDLE (Direction = 0): No movement → length unchanged
 * 
 * SPOOL RADIUS MODEL (NONLINEAR - MORE ACCURATE):
 * ──────────────────────────────────────────────────────────────────────────────
 * Wire wraps in layers around the core. As wire unrolls, radius decreases
 * according to the cross-sectional area of remaining wire:
 * 
 *   A_wire = π × d² / 4                    (area of wire cross-section)
 *   A_spool(L) = A_core + A_wire × N_wraps (total spool area)
 *   
 *   where N_wraps = (L_total - L) / (π × d)  (number of wire wraps remaining)
 *   
 *   R(L) = sqrt(A_spool(L) / π)            (radius from area)
 * 
 * For simplicity, we use a hybrid model:
 *   R(L) = sqrt(R_max² - (R_max² - R_min²) × (L / L_total))
 * 
 * This accounts for the quadratic relationship between radius and wire volume.
 * 
 * Example (35mm → 10mm over 3000mm):
 *   L = 0mm    → R = 35.0mm (full spool)
 *   L = 1500mm → R = 25.5mm (half empty - not 22.5mm as in linear model!)
 *   L = 3000mm → R = 10.0mm (empty core)
 * 
 * @param encoder Pointer to encoder structure containing hardware count
 * @return Current unrolled wire length in millimeters (0 to WIRE_LENGTH_MM)
 * 
 * @note Call this periodically (e.g., every 100-250ms)
 * @note Encoder_Read() must be called first to validate counts
 * ═══════════════════════════════════════════════════════════════════════════════
 */
uint16_t Encoder_MeasureLength(Encoder_t* encoder) {
    
    // ───────────────────────────────────────────────────────────────────────────
    // STEP 1: Calculate encoder count delta (UNSIGNED)
    // ───────────────────────────────────────────────────────────────────────────
    // Use stable_count which has already been noise-filtered in Encoder_Read()
    
    uint16_t current_count = encoder_state.stable_count;
    uint16_t last_count = encoder_state.last_encoder_count;  // ✅ FIX: Use separate variable
    uint16_t delta_abs = 0;
    
    // Calculate unsigned delta (counter only increases)
    if (current_count >= last_count) {
        delta_abs = current_count - last_count;
    } else {
        // Counter wrapped (auto-reset or overflow)
        delta_abs = current_count;  // Use new value as delta
    }
    
    // Skip if no movement detected
    if (delta_abs == 0) {
        // Return current filtered value (no change)
        return (uint16_t)encoder_state.filtered_length_mm;
    }
    
    // ───────────────────────────────────────────────────────────────────────────
    // STEP 2: Determine movement direction from motor
    // ───────────────────────────────────────────────────────────────────────────
    
    extern MotorRegisterMap_t motor1;
    int8_t direction_sign = 0;
    
    if (motor1.Direction == FORWARD) {  // FORWARD - unrolling
        direction_sign = +1;
    } else if (motor1.Direction == REVERSE) {  // REVERSE - rewinding
        direction_sign = -1;
    } else {  // IDLE - should not happen if delta > 0, but handle it
        // If encoder moved but motor is idle, assume forward (inertia/external force)
        direction_sign = +1;
    }
    
    // Apply direction to delta
    int32_t delta_signed = (int32_t)delta_abs * direction_sign;
    
    // Update cumulative tick counter
    encoder_state.total_encoder_ticks += delta_signed;
    
    // ───────────────────────────────────────────────────────────────────────────
    // STEP 3: Convert encoder counts to linear displacement
    // ───────────────────────────────────────────────────────────────────────────
    
    // Convert ticks to revolutions
    float delta_revolutions = (float)delta_signed / (float)ENCODER_CPR;
    
    // Calculate current spool circumference
    float current_circumference = 2.0f * M_PI * encoder_state.current_radius_mm;
    
    // Calculate linear displacement (can be positive or negative)
    float delta_length_mm = current_circumference * delta_revolutions;
    
    // Update accumulated unrolled length
    encoder_state.unrolled_length_mm += delta_length_mm;
    
    // Clamp to valid range [0, WIRE_LENGTH_MM]
    if (encoder_state.unrolled_length_mm < 0.0f) {
        encoder_state.unrolled_length_mm = 0.0f;
    }
    if (encoder_state.unrolled_length_mm > WIRE_LENGTH_MM) {
        encoder_state.unrolled_length_mm = WIRE_LENGTH_MM;
    }
    
    // ───────────────────────────────────────────────────────────────────────────
    // STEP 4: Update spool radius (NONLINEAR MODEL)
    // ───────────────────────────────────────────────────────────────────────────
    // This model accounts for the fact that wire volume is proportional to
    // the difference in spool area (π×R²), not radius directly
    
    float length_ratio = encoder_state.unrolled_length_mm / WIRE_LENGTH_MM;
    
    // Nonlinear model: R² decreases linearly with unrolled length
    float radius_squared = SPOOL_RADIUS_FULL_MM * SPOOL_RADIUS_FULL_MM - 
                          (SPOOL_RADIUS_FULL_MM * SPOOL_RADIUS_FULL_MM - 
                           SPOOL_RADIUS_EMPTY_MM * SPOOL_RADIUS_EMPTY_MM) * length_ratio;
    
    encoder_state.current_radius_mm = sqrtf(radius_squared);
    
    // Clamp radius to physical limits (safety check)
    if (encoder_state.current_radius_mm < SPOOL_RADIUS_EMPTY_MM) {
        encoder_state.current_radius_mm = SPOOL_RADIUS_EMPTY_MM;
    }
    if (encoder_state.current_radius_mm > SPOOL_RADIUS_FULL_MM) {
        encoder_state.current_radius_mm = SPOOL_RADIUS_FULL_MM;
    }
    
    // ───────────────────────────────────────────────────────────────────────────
    // STEP 5: Apply low-pass filter to reduce measurement noise
    // ───────────────────────────────────────────────────────────────────────────
    // First-order IIR filter: y[n] = α × x[n] + (1-α) × y[n-1]
    // FILTER_ALPHA = 0.15 provides good balance between responsiveness and smoothness
    
    encoder_state.filtered_length_mm = FILTER_ALPHA * encoder_state.unrolled_length_mm + 
                                       (1.0f - FILTER_ALPHA) * encoder_state.filtered_length_mm;
    
    // ───────────────────────────────────────────────────────────────────────────
    // STEP 6: Update last_encoder_count for next delta calculation
    // ───────────────────────────────────────────────────────────────────────────
    // ✅ CRITICAL: Update AFTER all calculations are done
    encoder_state.last_encoder_count = current_count;
    
    // Return filtered result in millimeters
    return (uint16_t)encoder_state.filtered_length_mm;
}

/**
 * @brief Reset encoder position tracking to initial state (full spool)
 * 
 * This function resets all position tracking variables to their initial values,
 * effectively treating the current position as the "home" position with a full spool.
 * 
 * Use cases:
 * - After manual wire rewinding
 * - After calibration
 * - When starting a new measurement cycle
 * 
 * @param encoder Pointer to encoder structure
 */
void Encoder_ResetWireLength(Encoder_t* encoder){
    encoder_state.unrolled_length_mm = 0.0f;
    encoder_state.current_radius_mm = SPOOL_RADIUS_FULL_MM;
    encoder_state.last_hardware_count = encoder_state.stable_count;
    encoder_state.last_encoder_count = encoder_state.stable_count;  // ✅ FIX: Reset both
    encoder_state.total_encoder_ticks = 0;
    encoder_state.filtered_length_mm = 0.0f;
    
    // Reset diagnostic counters
    encoder_state.noise_reject_count = 0;
    encoder_state.overflow_count = 0;
}

/**
 * @brief Set current wire length manually (for calibration)
 * 
 * This function allows manual setting of the current wire position,
 * useful during calibration procedures or when synchronizing with
 * an external measurement.
 * 
 * @param encoder Pointer to encoder structure
 * @param length_mm Desired wire length in millimeters
 */
void Encoder_SetWireLength(Encoder_t* encoder, float length_mm){
    // Clamp to valid range
    if(length_mm < 0.0f) length_mm = 0.0f;
    if(length_mm > WIRE_LENGTH_MM) length_mm = WIRE_LENGTH_MM;
    
    // Update encoder state
    encoder_state.unrolled_length_mm = length_mm;
    encoder_state.filtered_length_mm = length_mm;  // Reset filter to match
    
    // Update structure (convert mm to cm)
    encoder->Encoder_Calib_Current_Length_CM = (uint16_t)(length_mm / 10.0f);
    
    // Calculate corresponding radius using NONLINEAR model
    float length_ratio = length_mm / WIRE_LENGTH_MM;
    float radius_squared = SPOOL_RADIUS_FULL_MM * SPOOL_RADIUS_FULL_MM - 
                          (SPOOL_RADIUS_FULL_MM * SPOOL_RADIUS_FULL_MM - 
                           SPOOL_RADIUS_EMPTY_MM * SPOOL_RADIUS_EMPTY_MM) * length_ratio;
    encoder_state.current_radius_mm = sqrtf(radius_squared);
    
    // Clamp radius
    if (encoder_state.current_radius_mm < SPOOL_RADIUS_EMPTY_MM) {
        encoder_state.current_radius_mm = SPOOL_RADIUS_EMPTY_MM;
    }
    if (encoder_state.current_radius_mm > SPOOL_RADIUS_FULL_MM) {
        encoder_state.current_radius_mm = SPOOL_RADIUS_FULL_MM;
    }
    
    // Sync last encoder count to prevent jumps on next read
    encoder_state.last_hardware_count = encoder_state.stable_count;
    encoder_state.last_encoder_count = encoder_state.stable_count;  // ✅ FIX: Sync both
}

/**
 * @brief Get current spool radius
 * 
 * Returns the current effective radius of the spool, which decreases
 * as wire is unrolled. This value is useful for:
 * • Monitoring spool status
 * • Advanced control algorithms
 * • Diagnostics and debugging
 * 
 * @return Current spool radius in millimeters (10.0 to 35.0mm)
 */
float Encoder_GetCurrentRadius(void){
    return encoder_state.current_radius_mm;
}

/**
 * @brief Get total cumulative encoder ticks (signed)
 * 
 * Returns the cumulative encoder position including direction.
 * Useful for debugging and advanced position tracking.
 * 
 * @return Total encoder ticks (can be negative if rewound past zero)
 */
int32_t Encoder_GetTotalTicks(void){
    return encoder_state.total_encoder_ticks;
}

/**
 * @brief Get noise rejection statistics
 * 
 * Returns the number of noisy readings that were rejected.
 * High values indicate electrical noise or mechanical vibration.
 * 
 * @return Number of rejected readings since last reset
 */
uint32_t Encoder_GetNoiseRejectCount(void){
    return encoder_state.noise_reject_count;
}

/**
 * @brief Get counter overflow count
 * 
 * Returns the number of times the hardware counter wrapped around.
 * 
 * @return Number of overflows since initialization
 */
uint32_t Encoder_GetOverflowCount(void){
    return encoder_state.overflow_count;
}

/**
 * @brief Reset diagnostic counters
 * 
 * Resets noise rejection and overflow counters for fresh statistics.
 */
void Encoder_ResetDiagnostics(void){
    encoder_state.noise_reject_count = 0;
    encoder_state.overflow_count = 0;
}

void Encoder_Check_Calib_Origin(Encoder_t* encoder){

    bool Calib_Origin_Status = HAL_GPIO_ReadPin(GPIOA, IN1_Pin);
    if(Calib_Origin_Status == true){
        encoder->Calib_Origin_Status = true;
        Encoder_Reset(encoder);
    }
    else{
        encoder->Calib_Origin_Status = false;
    }
}

void Encoder_Process_Calib(Encoder_t* encoder){
    if(encoder->Encoder_Calib_Sensor_Status == true){
        encoder->Encoder_Calib_Status = true;
    }
    else{
        encoder->Encoder_Calib_Status = false;
    }
}