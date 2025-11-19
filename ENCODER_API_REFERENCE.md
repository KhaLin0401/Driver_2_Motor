# 📚 ENCODER API REFERENCE

## Quick Reference Guide for Encoder Functions

---

## 🔧 INITIALIZATION

### `void Encoder_Init(void)`

**Purpose:** Initialize encoder hardware and reset all tracking variables.

**When to call:** Once during system startup, before main loop.

**Example:**
```c
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM2_Init();
    
    Encoder_Init();  // ← Initialize encoder
    
    osKernelStart();
}
```

**What it does:**
- Starts STM32 TIM2 in quadrature encoder mode
- Resets hardware counter to 0
- Initializes position tracking state
- Sets spool radius to maximum (full spool)

---

## 📖 READING POSITION

### `uint16_t Encoder_MeasureLength(Encoder_t* encoder)`

**Purpose:** Calculate current unrolled wire length from encoder position.

**Parameters:**
- `encoder`: Pointer to encoder structure (use `&encoder1`)

**Returns:** Current unrolled length in **millimeters** (0 to 3000mm)

**Call frequency:** Every 100-500ms (recommended: 250ms)

**Example:**
```c
void StartEncoderTask(void *argument) {
    uint32_t previousTick = osKernelGetTickCount();
    
    for(;;) {
        Encoder_Load(&encoder1);
        Encoder_Process(&encoder1);  // Calls Encoder_MeasureLength() internally
        Encoder_Save(&encoder1);
        
        // Result available in:
        // - encoder1.Encoder_Calib_Current_Length_CM (in cm)
        // - g_holdingRegisters[REG_M1_UNROLLED_WIRE_LENGTH_CM]
        
        osDelayUntil(previousTick += 250);
    }
}
```

**Direction handling:**
- ✅ **Forward rotation** (motor unrolls wire) → length **increases**
- ✅ **Backward rotation** (motor rewinds wire) → length **decreases**
- ✅ **Automatic** - no manual direction checking needed!

**Accuracy:**
- ±2-5% with proper calibration
- ±10-15% without calibration

---

## 🔄 RESET & CALIBRATION

### `void Encoder_ResetWireLength(Encoder_t* encoder)`

**Purpose:** Reset position tracking to zero (treat current position as "home").

**When to use:**
- After manually rewinding wire to full spool
- After calibration procedure
- When starting a new measurement cycle

**Example:**
```c
// User pressed "Home" button
if (home_button_pressed) {
    Encoder_ResetWireLength(&encoder1);
    // Now current position is treated as 0mm (full spool)
}
```

**What it does:**
- Sets `unrolled_length = 0mm`
- Resets `current_radius = 35mm` (full spool)
- Clears accumulated encoder ticks
- Resets filter state

---

### `void Encoder_SetWireLength(Encoder_t* encoder, float length_cm)`

**Purpose:** Manually set current wire length (for calibration).

**Parameters:**
- `encoder`: Pointer to encoder structure
- `length_cm`: Desired length in **centimeters**

**When to use:**
- During calibration procedure
- After measuring actual wire length manually
- To correct accumulated error

**Example:**
```c
// Calibration: User measured actual length = 150cm
Encoder_SetWireLength(&encoder1, 150.0f);

// Now encoder tracking is synchronized with actual position
```

---

## 📊 READING RAW DATA

### `void Encoder_Read(Encoder_t* encoder)`

**Purpose:** Read current encoder count from hardware (TIM2).

**Example:**
```c
Encoder_Read(&encoder1);
uint16_t raw_count = encoder1.Encoder_Count;  // 0 to 65535

// Convert to revolutions:
float revolutions = (float)raw_count / 32.0f;  // 32 CPR for 8-slot encoder
```

**Note:** This is a low-level function. Normally you should use `Encoder_MeasureLength()` instead.

---

## 🔢 MODBUS INTEGRATION

### Available Registers

| Register | Address | Type | Unit | Description |
|----------|---------|------|------|-------------|
| `REG_M1_ENCODER_COUNT` | 0x0026 | Read | counts | Raw encoder count (0-65535) |
| `REG_M1_ENCODER_CONFIG` | 0x0027 | R/W | - | Configuration register |
| `REG_M1_ENCODER_RESET` | 0x0028 | Write | - | Write 1 to reset |
| `REG_M1_UNROLLED_WIRE_LENGTH_CM` | 0x002D | Read | cm | Unrolled length in centimeters |
| `REG_M1_CALIB_ORIGIN_STATUS` | 0x002E | Read | bool | Origin sensor status |

### Example: Read Wire Length via Modbus

```c
// Master reads wire length
uint16_t length_cm = g_holdingRegisters[REG_M1_UNROLLED_WIRE_LENGTH_CM];

printf("Wire unrolled: %d cm\n", length_cm);
```

### Example: Reset Encoder via Modbus

```c
// Master writes 1 to reset register
g_holdingRegisters[REG_M1_ENCODER_RESET] = 1;

// Next encoder task cycle will reset the encoder
```

---

## 🎛️ CONFIGURATION CONSTANTS

### Encoder Hardware

```c
#define ENCODER_SLOTS           8       // Physical slots on disk
#define ENCODER_PPR             8       // Pulses per revolution
#define ENCODER_CPR             32      // Counts per revolution (8 × 4)
#define ENCODER_RESOLUTION      11.25f  // Degrees per count
```

### Spool Geometry (⚠️ MUST CALIBRATE!)

```c
#define SPOOL_RADIUS_FULL_MM    35.0f   // Measure with caliper!
#define SPOOL_RADIUS_EMPTY_MM   10.0f   // Measure with caliper!
#define WIRE_LENGTH_MM          3000.0f // Total wire length
```

**How to measure:**
1. Use digital caliper to measure spool diameter when full
2. Divide by 2 to get radius → update `SPOOL_RADIUS_FULL_MM`
3. Measure empty core diameter
4. Divide by 2 → update `SPOOL_RADIUS_EMPTY_MM`

---

## 🧪 TESTING & DEBUGGING

### Test Procedure

```c
// 1. Reset encoder
Encoder_ResetWireLength(&encoder1);

// 2. Unroll exactly 1 meter (measure with ruler)
// ... motor runs ...

// 3. Read encoder
uint16_t measured_mm = Encoder_MeasureLength(&encoder1);
printf("Expected: 1000mm, Measured: %d mm\n", measured_mm);

// 4. Calculate error
float error_percent = ((float)measured_mm - 1000.0f) / 1000.0f * 100.0f;
printf("Error: %.2f%%\n", error_percent);

// 5. Rewind 50cm
// ... motor reverses ...

// 6. Read again
measured_mm = Encoder_MeasureLength(&encoder1);
printf("Expected: 500mm, Measured: %d mm\n", measured_mm);
```

### Expected Results

| Test | Expected | Acceptable Range |
|------|----------|------------------|
| 1m unroll | 1000mm | 950-1050mm (±5%) |
| 50cm rewind | 500mm | 475-525mm (±5%) |
| Repeatability | Same value | ±10mm |

---

## ⚠️ COMMON ISSUES

### Issue 1: Position Jumps Randomly

**Symptoms:** Encoder value jumps up and down erratically.

**Causes:**
- Electrical noise on encoder signals
- Loose connections
- Missing pull-up resistors

**Solutions:**
```c
// 1. Add hardware filtering in TIM2 config:
sConfig.IC1Filter = 5;  // Increase from 0 to 5
sConfig.IC2Filter = 5;

// 2. Check wiring:
// - Use shielded cable for encoder
// - Keep encoder wires away from motor power wires
// - Add 10kΩ pull-up resistors on A and B channels
```

---

### Issue 2: Wrong Direction

**Symptoms:** Position decreases when motor unrolls (should increase).

**Causes:**
- Encoder channels A and B swapped

**Solutions:**
```c
// Option 1: Swap encoder wires physically
// Connect A to B pin, B to A pin

// Option 2: Invert delta in software:
delta_count = -delta_count;  // Add this line after delta calculation
```

---

### Issue 3: Inaccurate Measurements

**Symptoms:** Measured length is consistently off by 10-50%.

**Causes:**
- Wrong `ENCODER_PPR` value
- Wrong `SPOOL_RADIUS` values
- Not calibrated

**Solutions:**
```c
// 1. Verify ENCODER_PPR:
// - Manually rotate shaft exactly 1 revolution
// - Read encoder count
// - Should be 32 for 8-slot encoder
// - If different, update ENCODER_PPR

// 2. Measure spool radii with caliper
// 3. Run calibration procedure (see ENCODER_CALIBRATION_GUIDE.md)
```

---

### Issue 4: Position Drifts Over Time

**Symptoms:** After many cycles, position is no longer accurate.

**Causes:**
- Wire slipping on spool
- Encoder mechanical wear
- Accumulated floating-point error

**Solutions:**
```c
// 1. Periodic recalibration:
if (cycle_count % 100 == 0) {
    // Every 100 cycles, recalibrate
    Encoder_SetWireLength(&encoder1, measured_actual_length);
}

// 2. Use origin sensor for absolute reference:
if (origin_sensor_triggered) {
    Encoder_ResetWireLength(&encoder1);
}
```

---

## 📖 COMPLETE EXAMPLE

### Full Integration Example

```c
// ═══════════════════════════════════════════════════════════════
// INITIALIZATION (in main.c)
// ═══════════════════════════════════════════════════════════════

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM2_Init();
    MX_USART2_UART_Init();
    
    // Initialize Modbus
    initializeModbusRegisters();
    
    // Initialize encoder
    Encoder_Init();
    
    // Start RTOS
    osKernelInitialize();
    osThreadNew(StartEncoderTask, NULL, &EncoderTask_attributes);
    osKernelStart();
    
    while (1) { }
}

// ═══════════════════════════════════════════════════════════════
// ENCODER TASK (periodic update)
// ═══════════════════════════════════════════════════════════════

void StartEncoderTask(void *argument) {
    uint32_t previousTick = osKernelGetTickCount();
    
    for(;;) {
        // 1. Load configuration from Modbus
        Encoder_Load(&encoder1);
        
        // 2. Process encoder (read hardware, calculate position)
        Encoder_Process(&encoder1);
        
        // 3. Save results to Modbus
        Encoder_Save(&encoder1);
        
        // 4. Check for origin sensor
        if (encoder1.Calib_Origin_Status) {
            // At home position - reset tracking
            Encoder_ResetWireLength(&encoder1);
        }
        
        // 5. Wait 250ms
        osDelayUntil(previousTick += 250);
    }
}

// ═══════════════════════════════════════════════════════════════
// MOTOR CONTROL (uses encoder position)
// ═══════════════════════════════════════════════════════════════

void Motor_PositionControl(void) {
    // Get current position from encoder
    uint16_t current_pos_cm = encoder1.Encoder_Calib_Current_Length_CM;
    
    // Get target position from Modbus
    uint16_t target_pos_cm = motor1.Position_Target;
    
    // Calculate error
    int16_t error = (int16_t)target_pos_cm - (int16_t)current_pos_cm;
    
    // Determine direction
    if (error > 5) {
        // Need to unroll more wire
        motor1.Direction = DIRECTION_FORWARD;
        motor1.Command_Speed = 50;  // 50% speed
    }
    else if (error < -5) {
        // Need to rewind wire
        motor1.Direction = DIRECTION_REVERSE;
        motor1.Command_Speed = 50;
    }
    else {
        // At target position
        motor1.Direction = DIRECTION_IDLE;
        motor1.Command_Speed = 0;
    }
}

// ═══════════════════════════════════════════════════════════════
// CALIBRATION ROUTINE
// ═══════════════════════════════════════════════════════════════

void Encoder_Calibrate(void) {
    // 1. Reset encoder
    Encoder_ResetWireLength(&encoder1);
    
    // 2. Unroll all wire slowly
    motor1.Direction = DIRECTION_FORWARD;
    motor1.Command_Speed = 20;  // Slow speed
    
    // 3. Wait until origin sensor triggers (end of travel)
    while (!encoder1.Calib_Origin_Status) {
        osDelay(100);
    }
    
    // 4. Stop motor
    motor1.Direction = DIRECTION_IDLE;
    motor1.Command_Speed = 0;
    
    // 5. Read encoder measurement
    uint16_t measured_length_cm = encoder1.Encoder_Calib_Current_Length_CM;
    
    // 6. Compare with actual length (measured manually)
    uint16_t actual_length_cm = 300;  // 3 meters
    
    // 7. Calculate correction factor
    float correction = (float)actual_length_cm / (float)measured_length_cm;
    
    // 8. Apply correction (update in code or store in EEPROM)
    printf("Calibration factor: %.3f\n", correction);
    printf("Measured: %d cm, Actual: %d cm\n", measured_length_cm, actual_length_cm);
}
```

---

## 🔗 SEE ALSO

- **ENCODER_CALIBRATION_GUIDE.md** - Detailed calibration procedures
- **ENCODER_ALGORITHM_EXPLAINED.md** - Algorithm internals and theory
- **ModbusMap.h** - Register definitions
- **MotorControl.c** - Motor control integration

---

**Last Updated:** 2025-11-19  
**Version:** 1.0  

