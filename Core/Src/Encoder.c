#include "Encoder.h"
#include "ModbusMap.h"
#include "main.h"
#include "MotorControl.h"

#include <math.h>

// ═══════════════════════════════════════════════════════════════════════════════
// ENCODER CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════
// Hardware: 8-slot optical encoder (8 gaps/slots per revolution)
// STM32 Mode: TIM_ENCODERMODE_TI12 (quadrature decoding on both edges)
// Counting: 4x per encoder pulse (rising/falling edges of both channels A & B)
// Result: 8 PPR × 4 = 32 counts per revolution
// ═══════════════════════════════════════════════════════════════════════════════

#define ENCODER_SLOTS           8       // Physical slots on encoder disk
#define ENCODER_PPR             8       // Pulses Per Revolution (same as slots for optical encoder)
#define ENCODER_CPR             (ENCODER_PPR * 4)  // Counts Per Revolution = 32 counts/rev
#define ENCODER_RESOLUTION      (360.0f / ENCODER_CPR)  // Degrees per count = 11.25°/count

// ═══════════════════════════════════════════════════════════════════════════════
// SPOOL GEOMETRY CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════
// These values MUST be measured and calibrated for your specific hardware
// ═══════════════════════════════════════════════════════════════════════════════

#define WIRE_LENGTH_CM          300         // Total wire length in cm (3 meters)
#define WIRE_LENGTH_MM          3000.0f     // Total wire length in mm (for calculation)
#define SPOOL_RADIUS_FULL_MM    35.0f       // Radius when spool is full (mm) - MEASURE THIS!
#define SPOOL_RADIUS_EMPTY_MM   10.0f       // Radius of empty core (mm) - MEASURE THIS!
#define WIRE_DIAMETER_MM        0.5f        // Wire diameter (mm) - optional for advanced model

// Derived constants
#define SPOOL_CIRCUMFERENCE_MAX (2.0f * M_PI * SPOOL_RADIUS_FULL_MM)   // Max circumference
#define SPOOL_CIRCUMFERENCE_MIN (2.0f * M_PI * SPOOL_RADIUS_EMPTY_MM)  // Min circumference

// ═══════════════════════════════════════════════════════════════════════════════
// PRIVATE STATE VARIABLES
// ═══════════════════════════════════════════════════════════════════════════════
// These maintain the encoder position tracking state across function calls
// ═══════════════════════════════════════════════════════════════════════════════

typedef struct {
    float unrolled_length_mm;       // Accumulated linear position (mm)
    float current_radius_mm;        // Current spool radius (mm)
    uint16_t last_encoder_count;    // Previous encoder reading for delta calculation
    int32_t total_encoder_ticks;    // Cumulative encoder ticks (can be negative)
    float filtered_length_mm;       // Low-pass filtered length for noise reduction
    bool initialized;               // Initialization flag
} EncoderState_t;

static EncoderState_t encoder_state = {
    .unrolled_length_mm = 0.0f,
    .current_radius_mm = SPOOL_RADIUS_FULL_MM,
    .last_encoder_count = 0,
    .total_encoder_ticks = 0,
    .filtered_length_mm = 0.0f,
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
    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_1 | TIM_CHANNEL_2);
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    
    // Initialize encoder state tracking
    encoder_state.unrolled_length_mm = 0.0f;
    encoder_state.current_radius_mm = SPOOL_RADIUS_FULL_MM;
    encoder_state.last_encoder_count = 0;
    encoder_state.total_encoder_ticks = 0;
    encoder_state.filtered_length_mm = 0.0f;
    encoder_state.initialized = true;
}

void Encoder_Read(Encoder_t* encoder){
    if(encoder->Encoder_Reset == true){
        __HAL_TIM_SET_COUNTER(&htim2, 0);
        encoder->Encoder_Count = 0;
        encoder->Encoder_Reset = 0; // ✅ Reset flag sau khi xử lý
    }
    
    // Đọc giá trị counter từ TIM2 (16-bit: 0-65535)
    uint16_t current_count = __HAL_TIM_GET_COUNTER(&htim2);
    encoder->Encoder_Count = current_count;
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
 * ALGORITHM OVERVIEW:
 * ──────────────────────────────────────────────────────────────────────────────
 * This function implements a bidirectional encoder position tracking system that
 * converts rotary encoder counts into linear wire displacement (unrolled_length).
 * 
 * KEY FEATURES:
 * 1. Bidirectional tracking (forward and backward rotation)
 * 2. 16-bit counter overflow/underflow handling
 * 3. Dynamic spool radius compensation
 * 4. Low-pass filtering for noise reduction
 * 
 * DIRECTION HANDLING:
 * ──────────────────────────────────────────────────────────────────────────────
 * - FORWARD rotation (motor unrolls wire):
 *   → Encoder count increases → delta_count > 0 → unrolled_length increases
 * 
 * - BACKWARD rotation (motor rewinds wire):
 *   → Encoder count decreases → delta_count < 0 → unrolled_length decreases
 * 
 * The STM32 TIM2 in quadrature mode automatically handles direction:
 * - CW rotation: counter increments (0 → 1 → 2 → ...)
 * - CCW rotation: counter decrements (... → 2 → 1 → 0 → 65535)
 * 
 * POSITION MAPPING:
 * ──────────────────────────────────────────────────────────────────────────────
 * Step 1: Read current encoder count from hardware (16-bit, 0-65535)
 * Step 2: Calculate delta = current_count - last_count (with overflow handling)
 * Step 3: Convert delta_count to delta_revolutions = delta / ENCODER_CPR
 * Step 4: Calculate linear displacement = circumference × delta_revolutions
 * Step 5: Update unrolled_length += displacement (can be positive or negative)
 * Step 6: Update spool radius based on remaining wire
 * Step 7: Apply low-pass filter to reduce noise
 * 
 * SPOOL RADIUS MODEL:
 * ──────────────────────────────────────────────────────────────────────────────
 * As wire unrolls, the effective spool radius decreases linearly:
 * 
 *   R(L) = R_max - (R_max - R_min) × (L / L_total)
 * 
 * Where:
 *   R(L) = current radius at length L
 *   R_max = radius when spool is full (35mm)
 *   R_min = radius of empty core (10mm)
 *   L = current unrolled length
 *   L_total = total wire length (3000mm)
 * 
 * Example:
 *   L = 0mm    → R = 35mm (full spool)
 *   L = 1500mm → R = 22.5mm (half empty)
 *   L = 3000mm → R = 10mm (empty core)
 * 
 * @param encoder Pointer to encoder structure containing hardware count
 * @return Current unrolled wire length in millimeters (0 to WIRE_LENGTH_MM)
 * 
 * @note This function must be called periodically (e.g., every 250ms)
 * @note Direction is automatically determined from encoder count changes
 * @note Negative deltas (backward rotation) correctly decrease unrolled_length
 * ═══════════════════════════════════════════════════════════════════════════════
 */
uint16_t Encoder_MeasureLength(Encoder_t* encoder) {
    
    // ───────────────────────────────────────────────────────────────────────────
    // STEP 1: Calculate encoder count delta with overflow handling
    // ───────────────────────────────────────────────────────────────────────────
    // The STM32 TIM2 counter is 16-bit (0-65535). When it overflows or underflows,
    // we need to detect and correct the delta calculation.
    
    uint16_t current_count = encoder->Encoder_Count;
    int32_t delta_count = (int32_t)current_count - (int32_t)encoder_state.last_encoder_count;
    
    // Handle 16-bit counter wrap-around:
    // - If delta < -32768: Counter overflowed (65535 → 0), actual delta is positive
    // - If delta > +32768: Counter underflowed (0 → 65535), actual delta is negative
    if (delta_count < -32768) {
        delta_count += 65536;  // Overflow correction: e.g., (5 - 65530) = -65525 → +11
    } else if (delta_count > 32768) {
        delta_count -= 65536;  // Underflow correction: e.g., (65530 - 5) = +65525 → -11
    }
    
    encoder_state.last_encoder_count = current_count;
    encoder_state.total_encoder_ticks += delta_count;  // Accumulate total ticks (can be negative)
    
    // ───────────────────────────────────────────────────────────────────────────
    // STEP 2: Convert encoder counts to linear displacement
    // ───────────────────────────────────────────────────────────────────────────
    // Only recalculate if encoder has moved
    if (delta_count != 0) {
        
        // Convert encoder counts to revolutions
        // For 8-slot encoder with TIM_ENCODERMODE_TI12: 32 counts = 1 revolution
        // Positive delta_count = forward rotation = wire unrolling
        // Negative delta_count = backward rotation = wire rewinding
        float delta_revolutions = (float)delta_count / (float)ENCODER_CPR;
        
        // Calculate current spool circumference
        float current_circumference = 2.0f * M_PI * encoder_state.current_radius_mm;
        
        // Calculate linear displacement (positive or negative)
        // This is the key step where rotation direction affects position:
        // - Forward rotation: delta_revolutions > 0 → delta_length > 0 → length increases
        // - Backward rotation: delta_revolutions < 0 → delta_length < 0 → length decreases
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
        
        // ───────────────────────────────────────────────────────────────────────
        // STEP 3: Update spool radius based on remaining wire
        // ───────────────────────────────────────────────────────────────────────
        // Linear model: radius decreases proportionally to unrolled length
        // This compensates for the changing mechanical advantage as wire depletes
        
        float length_ratio = encoder_state.unrolled_length_mm / WIRE_LENGTH_MM;
        encoder_state.current_radius_mm = SPOOL_RADIUS_FULL_MM - 
                                          (SPOOL_RADIUS_FULL_MM - SPOOL_RADIUS_EMPTY_MM) * length_ratio;
        
        // Clamp radius to physical limits
        if (encoder_state.current_radius_mm < SPOOL_RADIUS_EMPTY_MM) {
            encoder_state.current_radius_mm = SPOOL_RADIUS_EMPTY_MM;
        }
        if (encoder_state.current_radius_mm > SPOOL_RADIUS_FULL_MM) {
            encoder_state.current_radius_mm = SPOOL_RADIUS_FULL_MM;
        }
    }
    
    // ───────────────────────────────────────────────────────────────────────────
    // STEP 4: Apply low-pass filter to reduce measurement noise
    // ───────────────────────────────────────────────────────────────────────────
    // First-order IIR filter: y[n] = α × x[n] + (1-α) × y[n-1]
    // α = 0.2 provides good balance between responsiveness and noise rejection
    
    const float FILTER_ALPHA = 0.2f;
    encoder_state.filtered_length_mm = FILTER_ALPHA * encoder_state.unrolled_length_mm + 
                                       (1.0f - FILTER_ALPHA) * encoder_state.filtered_length_mm;
    
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
    encoder_state.last_encoder_count = encoder->Encoder_Count;
    encoder_state.total_encoder_ticks = 0;
    encoder_state.filtered_length_mm = 0.0f;
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
    
    // Calculate corresponding radius
    float length_ratio = length_mm / WIRE_LENGTH_MM;
    encoder_state.current_radius_mm = SPOOL_RADIUS_FULL_MM - 
                                      (SPOOL_RADIUS_FULL_MM - SPOOL_RADIUS_EMPTY_MM) * length_ratio;
    
    // Sync last encoder count to prevent jumps on next read
    encoder_state.last_encoder_count = encoder->Encoder_Count;
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

void Encoder_Check_Calib_Origin(Encoder_t* encoder){
    bool Calib_Origin_Status = false;
    Calib_Origin_Status = HAL_GPIO_ReadPin(GPIOA, IN1_Pin);
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