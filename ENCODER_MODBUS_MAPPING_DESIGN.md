# 🔌 ENCODER-MODBUS MAPPING DESIGN

## Professional Register Mapping for STM32 Motor Control System

---

## 📋 OVERVIEW

This document defines the **complete Modbus register mapping** for the `Encoder_t` structure, ensuring seamless integration between encoder hardware, firmware logic, and Modbus communication protocol.

### Design Goals

✅ **Preserve structure integrity** - `Encoder_t` remains unchanged  
✅ **Modbus RTU compliance** - Standard 16-bit holding registers  
✅ **Efficient data flow** - Minimize unnecessary reads/writes  
✅ **Type safety** - Proper handling of bool in Modbus context  
✅ **Industrial compatibility** - Works with standard SCADA/HMI systems  

---

## 🏗️ ENCODER STRUCTURE DEFINITION

### Complete Structure

```c
typedef struct {
    uint16_t Encoder_Count;                    // Raw encoder count (0-65535)
    uint16_t Encoder_Config;                   // Configuration flags
    uint16_t Encoder_Reset;                    // Reset command flag
    uint16_t Encoder_Calib_Sensor_Status;      // External sensor status
    uint16_t Encoder_Calib_Length_CM_Max;      // Maximum calibration length (cm)
    uint16_t Encoder_Calib_Start;              // Start calibration command
    uint16_t Encoder_Calib_Status;             // Calibration status
    uint16_t Encoder_Calib_Current_Length_CM;  // Current measured length (cm)
    bool     Calib_Origin_Status;              // Origin/home sensor status
} Encoder_t;

extern Encoder_t encoder1;
```

### Memory Layout

```
┌─────────────────────────────────────────────────────────────────┐
│ Offset │ Field Name                      │ Type    │ Size      │
├────────┼─────────────────────────────────┼─────────┼───────────┤
│ +0     │ Encoder_Count                   │ uint16  │ 2 bytes   │
│ +2     │ Encoder_Config                  │ uint16  │ 2 bytes   │
│ +4     │ Encoder_Reset                   │ uint16  │ 2 bytes   │
│ +6     │ Encoder_Calib_Sensor_Status     │ uint16  │ 2 bytes   │
│ +8     │ Encoder_Calib_Length_CM_Max     │ uint16  │ 2 bytes   │
│ +10    │ Encoder_Calib_Start             │ uint16  │ 2 bytes   │
│ +12    │ Encoder_Calib_Status            │ uint16  │ 2 bytes   │
│ +14    │ Encoder_Calib_Current_Length_CM │ uint16  │ 2 bytes   │
│ +16    │ Calib_Origin_Status             │ bool    │ 1 byte    │
│ +17    │ (padding)                       │ -       │ 1 byte    │
└─────────────────────────────────────────────────────────────────┘
Total: 18 bytes (9 × uint16_t + 1 × bool + padding)
```

---

## 📊 MODBUS REGISTER MAPPING

### Register Address Allocation

```c
// ═══════════════════════════════════════════════════════════════════════════
// ENCODER MODBUS REGISTER MAP (Motor 1)
// Base Address: 0x0026 - 0x002E (9 registers)
// ═══════════════════════════════════════════════════════════════════════════

#define REG_M1_ENCODER_COUNT              0x0026  // R    | Raw encoder count
#define REG_M1_ENCODER_CONFIG             0x0027  // R/W  | Configuration flags
#define REG_M1_ENCODER_RESET              0x0028  // W    | Write 1 to reset
#define REG_M1_CALIB_SENSOR_STATUS        0x0029  // R    | External sensor
#define REG_M1_CALIB_DISTANCE_CM          0x002A  // R/W  | Max calib length
#define REG_M1_CALIB_START                0x002B  // W    | Start calibration
#define REG_M1_CALIB_STATUS               0x002C  // R    | Calibration status
#define REG_M1_UNROLLED_WIRE_LENGTH_CM    0x002D  // R    | Current length (cm)
#define REG_M1_CALIB_ORIGIN_STATUS        0x002E  // R    | Origin sensor (bool)
```

### Register Details Table

| Address | Register Name | Access | Data Type | Range | Unit | Description |
|---------|---------------|--------|-----------|-------|------|-------------|
| 0x0026 | ENCODER_COUNT | Read | uint16 | 0-65535 | counts | Raw TIM2 counter value |
| 0x0027 | ENCODER_CONFIG | R/W | uint16 | 0-65535 | - | Configuration flags (reserved) |
| 0x0028 | ENCODER_RESET | Write | uint16 | 0-1 | bool | Write 1 to reset encoder |
| 0x0029 | CALIB_SENSOR_STATUS | Read | uint16 | 0-1 | bool | External calibration sensor |
| 0x002A | CALIB_DISTANCE_CM | R/W | uint16 | 0-65535 | cm | Maximum wire length |
| 0x002B | CALIB_START | Write | uint16 | 0-1 | bool | Write 1 to start calibration |
| 0x002C | CALIB_STATUS | Read | uint16 | 0-1 | bool | Calibration in progress |
| 0x002D | UNROLLED_WIRE_LENGTH_CM | Read | uint16 | 0-3000 | cm | Current measured length |
| 0x002E | CALIB_ORIGIN_STATUS | Read | uint16 | 0-1 | bool | Home/origin sensor status |

---

## 🔄 DATA FLOW ARCHITECTURE

### Three-Layer Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    LAYER 1: HARDWARE                            │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐     │
│  │ TIM2 Encoder │    │ Origin Sensor│    │ Calib Sensor │     │
│  │   (32 CPR)   │    │   (GPIO)     │    │   (GPIO)     │     │
│  └──────┬───────┘    └──────┬───────┘    └──────┬───────┘     │
└─────────┼────────────────────┼────────────────────┼─────────────┘
          │                    │                    │
          ▼                    ▼                    ▼
┌─────────────────────────────────────────────────────────────────┐
│                    LAYER 2: FIRMWARE                            │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                    Encoder_t Structure                    │  │
│  │  • Encoder_Count (from TIM2)                             │  │
│  │  • Encoder_Config                                        │  │
│  │  • Encoder_Reset                                         │  │
│  │  • Calib_Sensor_Status (from GPIO)                       │  │
│  │  • Calib_Length_CM_Max                                   │  │
│  │  • Calib_Start                                           │  │
│  │  • Calib_Status                                          │  │
│  │  • Calib_Current_Length_CM (calculated)                  │  │
│  │  • Calib_Origin_Status (from GPIO)                       │  │
│  └──────────────────────────────────────────────────────────┘  │
│           │                                        ▲             │
│           │ Encoder_Save()                         │             │
│           ▼                                        │             │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │              g_holdingRegisters[] Array                   │  │
│  │  [0x0026] = Encoder_Count                                │  │
│  │  [0x0027] = Encoder_Config                               │  │
│  │  [0x0028] = Encoder_Reset                                │  │
│  │  [0x0029] = Calib_Sensor_Status                          │  │
│  │  [0x002A] = Calib_Length_CM_Max                          │  │
│  │  [0x002B] = Calib_Start                                  │  │
│  │  [0x002C] = Calib_Status                                 │  │
│  │  [0x002D] = Calib_Current_Length_CM                      │  │
│  │  [0x002E] = Calib_Origin_Status                          │  │
│  └──────────────────────────────────────────────────────────┘  │
│           │                                        ▲             │
└───────────┼────────────────────────────────────────┼─────────────┘
            │ Modbus Response                        │ Modbus Request
            ▼                                        │
┌─────────────────────────────────────────────────────────────────┐
│                    LAYER 3: COMMUNICATION                       │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │              Modbus RTU Protocol (UART)                   │  │
│  │  • Function 03: Read Holding Registers                   │  │
│  │  • Function 06: Write Single Register                    │  │
│  │  • Function 16: Write Multiple Registers                 │  │
│  └──────────────────────────────────────────────────────────┘  │
│                            │                                    │
└────────────────────────────┼────────────────────────────────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │  Modbus Master  │
                    │  (PLC/SCADA/HMI)│
                    └─────────────────┘
```

---

## 🔧 IMPLEMENTATION DETAILS

### 1. Encoder_Load() - Read from Modbus to Structure

```c
/**
 * @brief Load encoder configuration from Modbus registers into structure
 * 
 * DATA FLOW: Modbus Registers → Encoder_t Structure
 * 
 * DESIGN PRINCIPLES:
 * ─────────────────────────────────────────────────────────────────
 * ✅ Load ONLY configuration/command registers (writable by master)
 * ❌ DO NOT load measured values (they come from hardware/calculation)
 * 
 * RATIONALE:
 * ─────────────────────────────────────────────────────────────────
 * • Encoder_Count: Read from TIM2 hardware, not from Modbus
 * • Calib_Current_Length_CM: Calculated by firmware, not from Modbus
 * • Calib_Origin_Status: Read from GPIO, not from Modbus
 * 
 * These are OUTPUT values that firmware writes TO Modbus, not reads FROM it.
 */
void Encoder_Load(Encoder_t* encoder) {
    // ═══════════════════════════════════════════════════════════════
    // CONFIGURATION REGISTERS (Master can write these)
    // ═══════════════════════════════════════════════════════════════
    encoder->Encoder_Config = g_holdingRegisters[REG_M1_ENCODER_CONFIG];
    encoder->Encoder_Reset = g_holdingRegisters[REG_M1_ENCODER_RESET];
    encoder->Encoder_Calib_Sensor_Status = g_holdingRegisters[REG_M1_CALIB_SENSOR_STATUS];
    encoder->Encoder_Calib_Length_CM_Max = g_holdingRegisters[REG_M1_CALIB_DISTANCE_CM];
    encoder->Encoder_Calib_Start = g_holdingRegisters[REG_M1_CALIB_START];
    
    // ═══════════════════════════════════════════════════════════════
    // MEASURED VALUES (DO NOT load from Modbus)
    // ═══════════════════════════════════════════════════════════════
    // ❌ encoder->Encoder_Count = ... (read from TIM2 in Encoder_Read())
    // ❌ encoder->Calib_Current_Length_CM = ... (calculated in Encoder_MeasureLength())
    // ❌ encoder->Calib_Origin_Status = ... (read from GPIO in Encoder_Check_Calib_Origin())
    
    // Note: Calib_Status is firmware-controlled, not loaded from Modbus
    // It will be set by calibration logic
}
```

### 2. Encoder_Save() - Write from Structure to Modbus

```c
/**
 * @brief Save encoder state from structure to Modbus registers
 * 
 * DATA FLOW: Encoder_t Structure → Modbus Registers
 * 
 * DESIGN PRINCIPLES:
 * ─────────────────────────────────────────────────────────────────
 * ✅ Save ALL fields to Modbus for master visibility
 * ✅ Include both measured values and configuration
 * ✅ Convert bool to uint16_t (0 or 1)
 * 
 * RATIONALE:
 * ─────────────────────────────────────────────────────────────────
 * Master needs to read:
 * • Current encoder count (for monitoring)
 * • Current wire length (for position control)
 * • Sensor statuses (for safety/diagnostics)
 * • Configuration echo-back (for verification)
 */
void Encoder_Save(Encoder_t* encoder) {
    // ═══════════════════════════════════════════════════════════════
    // MEASURED VALUES (Firmware → Modbus)
    // ═══════════════════════════════════════════════════════════════
    g_holdingRegisters[REG_M1_ENCODER_COUNT] = encoder->Encoder_Count;
    g_holdingRegisters[REG_M1_UNROLLED_WIRE_LENGTH_CM] = encoder->Encoder_Calib_Current_Length_CM;
    g_holdingRegisters[REG_M1_CALIB_STATUS] = encoder->Encoder_Calib_Status;
    
    // ═══════════════════════════════════════════════════════════════
    // BOOLEAN TO MODBUS CONVERSION
    // ═══════════════════════════════════════════════════════════════
    // Modbus standard: 0 = FALSE, non-zero = TRUE
    // Best practice: Use 0 or 1 explicitly for clarity
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
```

### 3. Boolean Field Handling - Best Practices

```c
/**
 * ═══════════════════════════════════════════════════════════════════════════
 * BOOLEAN IN MODBUS: DESIGN RATIONALE
 * ═══════════════════════════════════════════════════════════════════════════
 * 
 * PROBLEM:
 * ────────────────────────────────────────────────────────────────────────
 * • C bool type: 1 byte (true/false)
 * • Modbus register: 16-bit (0-65535)
 * • Need safe, unambiguous conversion
 * 
 * SOLUTION:
 * ────────────────────────────────────────────────────────────────────────
 * Use explicit ternary operator for clarity and safety:
 * 
 *   g_holdingRegisters[REG_XXX] = encoder->BoolField ? 1 : 0;
 * 
 * ALTERNATIVES (NOT RECOMMENDED):
 * ────────────────────────────────────────────────────────────────────────
 * 
 * ❌ Direct cast: (uint16_t)encoder->BoolField
 *    Problem: Undefined behavior if bool contains garbage value
 * 
 * ❌ Implicit conversion: encoder->BoolField
 *    Problem: Relies on compiler behavior, not explicit
 * 
 * ✅ Ternary operator: encoder->BoolField ? 1 : 0
 *    Benefit: Explicit, safe, readable, guaranteed 0 or 1
 * 
 * MODBUS STANDARD:
 * ────────────────────────────────────────────────────────────────────────
 * • 0x0000 = FALSE
 * • 0xFF00 = TRUE (for coils)
 * • 0x0001 = TRUE (for holding registers) ← We use this
 * 
 * Our choice (0 or 1) is:
 * • Compatible with all Modbus masters
 * • Easy to interpret in HMI/SCADA
 * • Consistent with digital I/O conventions
 */

// ═══════════════════════════════════════════════════════════════════════════
// EXAMPLE: Reading Boolean from Modbus Master
// ═══════════════════════════════════════════════════════════════════════════

// Python (pymodbus)
origin_status = client.read_holding_registers(0x002E, 1).registers[0]
if origin_status == 1:
    print("At origin position")
else:
    print("Not at origin")

// C# (NModbus)
ushort[] registers = master.ReadHoldingRegisters(1, 0x002E, 1);
bool isAtOrigin = (registers[0] != 0);

// Ladder Logic (PLC)
IF REG_M1_CALIB_ORIGIN_STATUS = 1 THEN
    SET Origin_Reached_Flag
END_IF
```

---

## 🔄 COMPLETE USAGE EXAMPLE

### Firmware Side (STM32)

```c
// ═══════════════════════════════════════════════════════════════════════════
// ENCODER TASK - Periodic Update (250ms)
// ═══════════════════════════════════════════════════════════════════════════

void StartEncoderTask(void *argument) {
    uint32_t previousTick = osKernelGetTickCount();
    
    for(;;) {
        // ───────────────────────────────────────────────────────────────────
        // STEP 1: Load configuration from Modbus
        // ───────────────────────────────────────────────────────────────────
        // Master may have written new commands (reset, calib start, etc.)
        Encoder_Load(&encoder1);
        
        // ───────────────────────────────────────────────────────────────────
        // STEP 2: Process encoder logic
        // ───────────────────────────────────────────────────────────────────
        Encoder_Process(&encoder1);
        // This internally calls:
        // • Encoder_Check_Calib_Origin() → reads GPIO → updates Calib_Origin_Status
        // • Encoder_Read() → reads TIM2 → updates Encoder_Count
        // • Encoder_MeasureLength() → calculates → updates Calib_Current_Length_CM
        
        // ───────────────────────────────────────────────────────────────────
        // STEP 3: Save results to Modbus
        // ───────────────────────────────────────────────────────────────────
        // Make all values visible to Modbus master
        Encoder_Save(&encoder1);
        
        // ───────────────────────────────────────────────────────────────────
        // STEP 4: Handle reset command
        // ───────────────────────────────────────────────────────────────────
        if (encoder1.Encoder_Reset == 1) {
            Encoder_Reset(&encoder1);
            encoder1.Encoder_Reset = 0;  // Clear flag
        }
        
        // ───────────────────────────────────────────────────────────────────
        // STEP 5: Wait for next cycle
        // ───────────────────────────────────────────────────────────────────
        osDelayUntil(previousTick += 250);
    }
}
```

### Modbus Master Side (Python Example)

```python
from pymodbus.client import ModbusSerialClient

# ═══════════════════════════════════════════════════════════════════════════
# CONNECT TO STM32 VIA MODBUS RTU
# ═══════════════════════════════════════════════════════════════════════════
client = ModbusSerialClient(
    port='/dev/ttyUSB0',
    baudrate=115200,
    parity='N',
    stopbits=1,
    bytesize=8,
    timeout=1
)
client.connect()

SLAVE_ID = 3  # STM32 device ID

# ═══════════════════════════════════════════════════════════════════════════
# EXAMPLE 1: Read Current Wire Length
# ═══════════════════════════════════════════════════════════════════════════
result = client.read_holding_registers(
    address=0x002D,  # REG_M1_UNROLLED_WIRE_LENGTH_CM
    count=1,
    slave=SLAVE_ID
)
wire_length_cm = result.registers[0]
print(f"Current wire length: {wire_length_cm} cm")

# ═══════════════════════════════════════════════════════════════════════════
# EXAMPLE 2: Check Origin Sensor Status
# ═══════════════════════════════════════════════════════════════════════════
result = client.read_holding_registers(
    address=0x002E,  # REG_M1_CALIB_ORIGIN_STATUS
    count=1,
    slave=SLAVE_ID
)
is_at_origin = (result.registers[0] == 1)
print(f"At origin: {is_at_origin}")

# ═══════════════════════════════════════════════════════════════════════════
# EXAMPLE 3: Reset Encoder
# ═══════════════════════════════════════════════════════════════════════════
client.write_register(
    address=0x0028,  # REG_M1_ENCODER_RESET
    value=1,         # Write 1 to reset
    slave=SLAVE_ID
)
print("Encoder reset command sent")

# Wait for firmware to process
time.sleep(0.5)

# Verify reset
result = client.read_holding_registers(
    address=0x0026,  # REG_M1_ENCODER_COUNT
    count=1,
    slave=SLAVE_ID
)
encoder_count = result.registers[0]
print(f"Encoder count after reset: {encoder_count}")  # Should be 0

# ═══════════════════════════════════════════════════════════════════════════
# EXAMPLE 4: Read Multiple Registers at Once
# ═══════════════════════════════════════════════════════════════════════════
result = client.read_holding_registers(
    address=0x0026,  # Start from REG_M1_ENCODER_COUNT
    count=9,         # Read all 9 encoder registers
    slave=SLAVE_ID
)

if result.isError():
    print("Error reading registers")
else:
    encoder_count = result.registers[0]
    encoder_config = result.registers[1]
    encoder_reset = result.registers[2]
    calib_sensor = result.registers[3]
    max_length = result.registers[4]
    calib_start = result.registers[5]
    calib_status = result.registers[6]
    current_length = result.registers[7]
    origin_status = result.registers[8]
    
    print(f"""
    Encoder Status:
    ─────────────────────────────────────
    Raw Count:       {encoder_count}
    Current Length:  {current_length} cm
    Max Length:      {max_length} cm
    Origin Sensor:   {'YES' if origin_status else 'NO'}
    Calib Status:    {'ACTIVE' if calib_status else 'IDLE'}
    """)

client.close()
```

---

## 📝 REGISTER ACCESS PATTERNS

### Read-Only Registers (Firmware → Master)

```c
// These registers are updated by firmware only
// Master should READ them, not WRITE

REG_M1_ENCODER_COUNT              // Hardware measurement
REG_M1_UNROLLED_WIRE_LENGTH_CM    // Calculated value
REG_M1_CALIB_STATUS               // Firmware state
REG_M1_CALIB_ORIGIN_STATUS        // Hardware sensor
```

**Master Usage:**
```python
# Periodic monitoring (every 1 second)
length = read_register(REG_M1_UNROLLED_WIRE_LENGTH_CM)
update_display(length)
```

### Write-Only Registers (Master → Firmware)

```c
// These registers are commands from master
// Firmware reads them, executes action, then may clear them

REG_M1_ENCODER_RESET              // Command: reset encoder
REG_M1_CALIB_START                // Command: start calibration
```

**Master Usage:**
```python
# Send command
write_register(REG_M1_ENCODER_RESET, 1)

# Wait for firmware to process
time.sleep(0.1)

# Verify result
count = read_register(REG_M1_ENCODER_COUNT)
assert count == 0, "Reset failed"
```

### Read/Write Registers (Bidirectional)

```c
// These registers can be both read and written
// Master writes configuration, firmware echoes back

REG_M1_ENCODER_CONFIG             // Configuration flags
REG_M1_CALIB_DISTANCE_CM          // Maximum wire length
REG_M1_CALIB_SENSOR_STATUS        // External sensor config
```

**Master Usage:**
```python
# Write new configuration
write_register(REG_M1_CALIB_DISTANCE_CM, 300)  # 3 meters

# Read back to verify
max_length = read_register(REG_M1_CALIB_DISTANCE_CM)
assert max_length == 300, "Configuration not applied"
```

---

## ⚠️ IMPORTANT DESIGN CONSIDERATIONS

### 1. Data Direction Clarity

```c
/**
 * CRITICAL RULE: Avoid bidirectional data flow conflicts
 * ────────────────────────────────────────────────────────────────
 * 
 * ❌ BAD: Same register written by both firmware AND master
 * 
 *   // In firmware:
 *   encoder->Encoder_Count = read_from_TIM2();
 *   
 *   // In Encoder_Load():
 *   encoder->Encoder_Count = g_holdingRegisters[REG_M1_ENCODER_COUNT];
 *   
 *   // Result: Race condition! Who owns this value?
 * 
 * ✅ GOOD: Clear ownership
 * 
 *   // Firmware owns Encoder_Count (reads from hardware)
 *   // Master can only READ this register, never WRITE
 *   
 *   // Encoder_Load() does NOT load Encoder_Count
 *   // Encoder_Save() DOES save Encoder_Count
 */
```

### 2. Boolean Conversion Safety

```c
/**
 * ALWAYS use explicit conversion for bool → uint16_t
 * ────────────────────────────────────────────────────────────────
 */

// ✅ CORRECT: Explicit ternary
g_holdingRegisters[REG_XXX] = encoder->BoolField ? 1 : 0;

// ✅ ALSO CORRECT: Explicit cast with !!
g_holdingRegisters[REG_XXX] = (uint16_t)!!encoder->BoolField;

// ❌ AVOID: Direct cast (may not be 0 or 1)
g_holdingRegisters[REG_XXX] = (uint16_t)encoder->BoolField;

// ❌ AVOID: Implicit conversion
g_holdingRegisters[REG_XXX] = encoder->BoolField;
```

### 3. Register Reset Handling

```c
/**
 * PATTERN: Self-clearing command registers
 * ────────────────────────────────────────────────────────────────
 * 
 * For command registers (Reset, Calib_Start), use this pattern:
 */

// In EncoderTask:
if (encoder1.Encoder_Reset == 1) {
    Encoder_Reset(&encoder1);           // Execute command
    encoder1.Encoder_Reset = 0;         // Clear flag
    // Next Encoder_Save() will write 0 back to Modbus
}

// Master sees:
// 1. Write 1 to REG_M1_ENCODER_RESET
// 2. Wait 250ms (one task cycle)
// 3. Read REG_M1_ENCODER_RESET → should be 0 (command executed)
```

---

## 🧪 TESTING & VALIDATION

### Unit Test: Boolean Conversion

```c
void test_bool_to_modbus_conversion(void) {
    Encoder_t test_encoder = {0};
    
    // Test TRUE
    test_encoder.Calib_Origin_Status = true;
    Encoder_Save(&test_encoder);
    assert(g_holdingRegisters[REG_M1_CALIB_ORIGIN_STATUS] == 1);
    
    // Test FALSE
    test_encoder.Calib_Origin_Status = false;
    Encoder_Save(&test_encoder);
    assert(g_holdingRegisters[REG_M1_CALIB_ORIGIN_STATUS] == 0);
    
    printf("✅ Boolean conversion test passed\n");
}
```

### Integration Test: Round-Trip

```c
void test_load_save_roundtrip(void) {
    // Setup: Write test values to Modbus registers
    g_holdingRegisters[REG_M1_ENCODER_CONFIG] = 0x1234;
    g_holdingRegisters[REG_M1_CALIB_DISTANCE_CM] = 300;
    
    // Load from Modbus to structure
    Encoder_Load(&encoder1);
    
    // Verify
    assert(encoder1.Encoder_Config == 0x1234);
    assert(encoder1.Encoder_Calib_Length_CM_Max == 300);
    
    // Modify structure
    encoder1.Encoder_Count = 1000;
    encoder1.Calib_Origin_Status = true;
    
    // Save back to Modbus
    Encoder_Save(&encoder1);
    
    // Verify
    assert(g_holdingRegisters[REG_M1_ENCODER_COUNT] == 1000);
    assert(g_holdingRegisters[REG_M1_CALIB_ORIGIN_STATUS] == 1);
    
    printf("✅ Round-trip test passed\n");
}
```

---

## 📖 SUMMARY

### Design Principles

1. **Clear Data Ownership**
   - Hardware values: Firmware writes, Master reads
   - Configuration: Master writes, Firmware reads
   - Commands: Master writes, Firmware executes and clears

2. **Type Safety**
   - Always use explicit bool → uint16_t conversion
   - Use ternary operator: `value ? 1 : 0`
   - Avoid implicit casts

3. **Modbus Compliance**
   - All registers are 16-bit
   - Boolean represented as 0 or 1
   - Standard function codes (03, 06, 16)

4. **Efficient Updates**
   - Load only configuration registers
   - Save all registers for visibility
   - Minimize unnecessary data transfers

### Register Summary

| Category | Registers | Direction | Update Frequency |
|----------|-----------|-----------|------------------|
| Measured Values | 3 | Firmware → Master | Every 250ms |
| Configuration | 3 | Master → Firmware | On change |
| Commands | 2 | Master → Firmware | On demand |
| Status | 1 | Firmware → Master | Every 250ms |

---

**Version:** 1.0  
**Last Updated:** 2025-11-19  
**Compatible With:** STM32 HAL, Modbus RTU, Industrial SCADA/HMI systems  




