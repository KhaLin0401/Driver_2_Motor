# 📊 LUỒNG DỮ LIỆU POSITION CONTROL - ĐƠN VỊ CM

## 🔄 TỔNG QUAN LUỒNG DỮ LIỆU

```
┌─────────────────────────────────────────────────────────────────────┐
│                    ENCODER HARDWARE (TIM2)                          │
│  8 khe × 2 cạnh = 16 counts/vòng                                   │
│  Encoder_Count: 0-32767 (uint16_t, auto-reset tại 32768)          │
└────────────────────────┬────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────────┐
│              Encoder_Read() - Core/Src/Encoder.c:99                 │
│  • Đọc TIM2 counter                                                 │
│  • Software debounce (bỏ qua delta > 1000)                         │
│  • Auto-reset nếu >= 32768                                          │
│  → encoder->Encoder_Count (uint16_t, 0-32767)                      │
└────────────────────────┬────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────────┐
│         Encoder_MeasureLength() - Core/Src/Encoder.c:321            │
│  • Tính delta_count (có xử lý overflow)                            │
│  • Chuyển đổi: counts → revolutions → mm                           │
│  • CPR = 16 counts/rev                                              │
│  • Spool circumference = 2π × radius_mm                            │
│  • Low-pass filter (α = 0.2)                                       │
│  → Return: filtered_length_mm (uint16_t, đơn vị MM)                │
└────────────────────────┬────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────────┐
│          Encoder_Process() - Core/Src/Encoder.c:245                 │
│  measured_length = Encoder_MeasureLength(&encoder1);  // mm         │
│  encoder->Encoder_Calib_Current_Length_CM = measured_length / 10;   │
│  → Chuyển đổi MM → CM ✅                                            │
│  → encoder1.Encoder_Calib_Current_Length_CM (uint16_t, CM)         │
└────────────────────────┬────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────────┐
│         Encoder_Save() - Core/Src/Encoder.c:220                     │
│  g_holdingRegisters[REG_M1_UNROLLED_WIRE_LENGTH_CM] =              │
│      encoder->Encoder_Calib_Current_Length_CM;                      │
│  → Modbus Register 0x002D (CM) ✅                                   │
└────────────────────────┬────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────────┐
│      Motor_UpdatePosition() - Core/Src/MotorControl.c:786           │
│  motor->Position_Current = encoder1.Encoder_Calib_Current_Length_CM;│
│  → motor1.Position_Current (uint16_t, CM) ✅                        │
└────────────────────────┬────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────────┐
│     MotorRegisters_Save() - Core/Src/MotorControl.c:90              │
│  g_holdingRegisters[REG_M1_POSITION_CURRENT] =                      │
│      motor->Position_Current;                                       │
│  → Modbus Register 0x000E (CM) ✅                                   │
└────────────────────────┬────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────────┐
│    MotorRegisters_Load() - Core/Src/MotorControl.c:57               │
│  motor->Position_Target = g_holdingRegisters[REG_M1_POSITION_TARGET];│
│  → motor1.Position_Target (uint16_t, CM) ✅                         │
│  ← Modbus Register 0x000F (CM, từ Master)                           │
└────────────────────────┬────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────────┐
│      Motor_HandlePosition() - Core/Src/MotorControl.c:359           │
│  current_position = motor->Position_Current;  // CM ✅              │
│  target_position = motor->Position_Target;    // CM ✅              │
│  position_error = target - current;           // CM ✅              │
│  abs_position_error = abs(position_error);    // CM ✅              │
└────────────────────────┬────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────────┐
│    PID_Compute_Position() - Core/Src/MotorControl.c:718             │
│  Input:  setpoint_cm (target position, CM) ✅                       │
│          feedback_cm (current position, CM) ✅                      │
│  Process:                                                            │
│    • position_error_cm = setpoint_cm - feedback_cm                  │
│    • abs_error = |position_error_cm|                                │
│    • P term = Kp × abs_error                                        │
│    • I term = Ki × ∫(error × dt)                                    │
│    • D term = Kd × d(error)/dt                                      │
│    • output = P + I + D                                             │
│    • Clamp: 0 ≤ output ≤ Command_Speed                             │
│    • If abs_error ≤ 1cm → output = 0                               │
│  Output: motor speed (0-100%) ✅                                    │
└────────────────────────┬────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────────┐
│              Motor_HandlePosition() (continued)                      │
│  duty = (uint8_t)output;                                            │
│  Clamp: Min_Speed ≤ duty ≤ Max_Speed                               │
│  Motor1_OutputPWM(motor, duty);                                     │
│  → PWM Output (0-100%) ✅                                           │
└─────────────────────────────────────────────────────────────────────┘
```

---

## ✅ KIỂM TRA ĐƠN VỊ TẠI CÁC ĐIỂM QUAN TRỌNG

| Vị trí | Biến | Đơn vị | Kiểu dữ liệu | Giá trị hợp lệ |
|--------|------|--------|--------------|----------------|
| **TIM2 Hardware** | Counter | counts | uint16_t | 0-32767 |
| **Encoder_Count** | encoder->Encoder_Count | counts | uint16_t | 0-32767 |
| **Encoder_MeasureLength()** | Return value | **MM** | uint16_t | 0-3000 |
| **Encoder_Calib_Current_Length_CM** | encoder1.Encoder_Calib_Current_Length_CM | **CM** ✅ | uint16_t | 0-300 |
| **REG_M1_UNROLLED_WIRE_LENGTH_CM** | g_holdingRegisters[0x002D] | **CM** ✅ | uint16_t | 0-300 |
| **Position_Current** | motor1.Position_Current | **CM** ✅ | uint16_t | 0-300 |
| **REG_M1_POSITION_CURRENT** | g_holdingRegisters[0x000E] | **CM** ✅ | uint16_t | 0-300 |
| **Position_Target** | motor1.Position_Target | **CM** ✅ | uint16_t | 0-300 |
| **REG_M1_POSITION_TARGET** | g_holdingRegisters[0x000F] | **CM** ✅ | uint16_t | 0-300 |
| **position_error** | position_error | **CM** ✅ | int16_t | -300 to +300 |
| **PID Input** | setpoint_cm, feedback_cm | **CM** ✅ | float | 0-300 |
| **PID Output** | Return value | **%** (speed) ✅ | float | 0-100 |

---

## 🔍 CHUYỂN ĐỔI ĐƠN VỊ QUAN TRỌNG

### 1️⃣ **MM → CM** (Encoder.c:253)
```c
uint16_t measured_length = Encoder_MeasureLength(encoder);  // MM
encoder->Encoder_Calib_Current_Length_CM = measured_length / 10; // → CM ✅
```

### 2️⃣ **Encoder Counts → MM** (Encoder.c:355-364)
```c
// ENCODER_CPR = 16 counts/rev
float delta_revolutions = (float)delta_count / (float)ENCODER_CPR;
float current_circumference = 2.0f * M_PI * encoder_state.current_radius_mm;
float delta_length_mm = current_circumference * delta_revolutions;  // → MM
```

### 3️⃣ **Position Error (CM) → Speed (%)** (MotorControl.c:718)
```c
float position_error_cm = setpoint_cm - feedback_cm;  // CM
float abs_error = fabs(position_error_cm);            // CM
float p_term = kp * abs_error;                        // % (Kp scaled)
float output_speed = p_term + i_term + d_term;       // → % (0-100)
```

---

## 📋 MODBUS REGISTERS - ĐƠN VỊ CM

| Register | Địa chỉ | Tên | Đơn vị | R/W | Mô tả |
|----------|---------|-----|--------|-----|-------|
| **REG_M1_ENCODER_COUNT** | 0x0026 | Encoder Count | counts | R | Raw encoder count (0-32767) |
| **REG_M1_UNROLLED_WIRE_LENGTH_CM** | 0x002D | Wire Length | **CM** ✅ | R | Độ dài dây đã kéo ra |
| **REG_M1_POSITION_CURRENT** | 0x000E | Current Position | **CM** ✅ | R | Vị trí hiện tại |
| **REG_M1_POSITION_TARGET** | 0x000F | Target Position | **CM** ✅ | R/W | Vị trí đích (set từ Master) |

---

## 🎯 VÍ DỤ TÍNH TOÁN THỰC TẾ

### **Scenario: Di chuyển từ 10cm → 60cm**

```
1. Khởi tạo:
   Position_Current = 10 cm
   Position_Target = 60 cm (set từ Modbus Master)
   
2. Tính toán:
   position_error = 60 - 10 = 50 cm ✅
   abs_error = 50 cm
   
3. PID tính tốc độ:
   Giả sử: Kp = 1.0, Command_Speed = 80%
   p_term = 1.0 × 50 = 50%
   output_speed = min(50%, 80%) = 50% ✅
   
4. Motor chạy với PWM = 50%
   
5. Encoder đo được di chuyển:
   Encoder_Count tăng từ 0 → 800 counts (giả sử)
   Encoder_MeasureLength() = 500 mm
   Encoder_Calib_Current_Length_CM = 500 / 10 = 50 cm ✅
   Position_Current = 50 cm
   
6. Tính lại:
   position_error = 60 - 50 = 10 cm
   p_term = 1.0 × 10 = 10%
   output_speed = 10% (chậm lại khi gần đích) ✅
   
7. Đến đích:
   Position_Current = 60 cm
   position_error = 60 - 60 = 0 cm
   abs_error ≤ 1 cm → output_speed = 0% → DỪNG ✅
```

---

## ✅ CHECKLIST ĐỒNG BỘ ĐƠN VỊ

- [x] Encoder_MeasureLength() trả về **MM** ✅
- [x] Chuyển đổi MM → CM tại Encoder_Process() ✅
- [x] Encoder_Calib_Current_Length_CM đơn vị **CM** ✅
- [x] Position_Current đơn vị **CM** ✅
- [x] Position_Target đơn vị **CM** ✅
- [x] position_error đơn vị **CM** ✅
- [x] PID input (setpoint, feedback) đơn vị **CM** ✅
- [x] PID output đơn vị **%** (speed) ✅
- [x] Modbus registers position đơn vị **CM** ✅
- [x] Tolerance check (≤ 1cm) đơn vị **CM** ✅

---

## 🔧 ĐÃ SỬA CÁC LỖI

### **Lỗi 1: Tham số PID sai (MotorControl.c:442)**
```c
// ❌ CŨ (SAI):
float output = PID_Compute_Position(motor_id, current_position, (float)abs_position_error);
// Tham số 2: current_position (sai, đáng lẽ là target)
// Tham số 3: abs_position_error (sai, mất dấu của error)

// ✅ MỚI (ĐÚNG):
float output = PID_Compute_Position(motor_id, (float)target_position, (float)current_position);
// Tham số 2: target_position (setpoint, CM) ✅
// Tham số 3: current_position (feedback, CM) ✅
```

### **Lỗi 2: PID output không đúng logic (MotorControl.c:718)**
```c
// ✅ ĐÃ SỬA:
// - PID tính toán dựa trên position_error (CM)
// - Output là tốc độ (%), không phải position
// - P term = Kp × abs_error → tốc độ tỷ lệ với khoảng cách
// - Clamp output ≤ Command_Speed (max speed)
// - Stop nếu abs_error ≤ 1cm
```

---

## 🎯 KẾT LUẬN

✅ **TẤT CẢ ĐƠN VỊ ĐÃ ĐỒNG BỘ SANG CM**

✅ **LUỒNG DỮ LIỆU CHÍNH XÁC:**
- Encoder đo → MM → chuyển sang CM → Position_Current (CM)
- Master set → Position_Target (CM)
- PID tính → position_error (CM) → output speed (%)
- Motor chạy với tốc độ tỷ lệ với khoảng cách còn lại

✅ **REGISTERS MODBUS ĐÚNG ĐƠN VỊ CM:**
- 0x000E: Position_Current (CM)
- 0x000F: Position_Target (CM)
- 0x002D: Wire_Length (CM)

---

## 📊 TEST CASE ĐỂ KIỂM TRA

```python
# Test 1: Kiểm tra đơn vị CM
current = client.read_holding_registers(0x000E, 1, slave=3).registers[0]
wire_length = client.read_holding_registers(0x002D, 1, slave=3).registers[0]
print(f"Position_Current: {current} cm")
print(f"Wire_Length: {wire_length} cm")
# Expected: Cả 2 giá trị phải giống nhau (cùng đơn vị CM) ✅

# Test 2: Di chuyển 50cm
target = current + 50
client.write_register(0x000F, target, slave=3)
# Expected: Motor di chuyển đúng 50cm ✅

# Test 3: Kiểm tra PID output
# Khi error = 50cm, Kp = 1.0 → speed ≈ 50%
# Khi error = 10cm, Kp = 1.0 → speed ≈ 10%
# Khi error ≤ 1cm → speed = 0% (stop) ✅
```

