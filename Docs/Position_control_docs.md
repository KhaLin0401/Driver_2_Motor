# Hướng Dẫn Sử Dụng Hàm Motor_HandlePosition

## 📋 Mục Lục
1. [Tổng Quan](#tổng-quan)
2. [Cấu Hình Registers](#cấu-hình-registers)
3. [Chu Trình Hoạt Động](#chu-trình-hoạt-động)
4. [Hướng Dẫn Sử Dụng](#hướng-dẫn-sử-dụng)
5. [Ví Dụ Cụ Thể](#ví-dụ-cụ-thể)
6. [Lưu Ý Quan Trọng](#lưu-ý-quan-trọng)
7. [Xử Lý Lỗi](#xử-lý-lỗi)

---

## 📖 Tổng Quan

### Chức Năng
`Motor_HandlePosition` là hàm điều khiển motor ở **chế độ vị trí (Position Control Mode)**, cho phép motor di chuyển đến một vị trí mục tiêu cụ thể và dừng lại tại đó.

### Đặc Điểm
- **Mode**: `CONTROL_MODE_POSITION` (giá trị = 3)
- **Đầu vào**: Vị trí mục tiêu (cm)
- **Đầu ra**: Điều khiển PWM và hướng quay motor
- **Thuật toán**: PID Controller với feedback từ encoder
- **Độ chính xác**: ±1 cm (có thể điều chỉnh)

### Nguyên Lý Hoạt Động
```
┌─────────────────┐
│ Position_Target │ (Vị trí mục tiêu)
└────────┬────────┘
         │
         ▼
    ┌────────────┐
    │  Position  │
    │   Error    │ = Target - Current
    └─────┬──────┘
          │
          ▼
    ┌─────────────┐
    │ PID Control │ → Tính toán tốc độ cần thiết
    └─────┬───────┘
          │
          ▼
    ┌──────────────┐
    │ PWM + Direction │ → Điều khiển motor
    └──────┬─────────┘
           │
           ▼
    ┌──────────────┐
    │   Encoder    │ → Đo vị trí thực tế
    └──────┬───────┘
           │
           ▼
    ┌─────────────────┐
    │ Position_Current│ (Feedback)
    └─────────────────┘
```

---

## ⚙️ Cấu Hình Registers

### Registers Cần Thiết (Modbus)

| Register | Địa Chỉ | Tên | Mô Tả | Đơn Vị | Giá Trị Mặc Định |
|----------|---------|-----|-------|--------|------------------|
| **Control_Mode** | 0x0000 | Chế độ điều khiển | Phải đặt = 3 (Position) | - | 1 |
| **Enable** | 0x0001 | Bật/Tắt motor | 1 = Bật, 0 = Tắt | - | 0 |
| **Command_Speed** | 0x0002 | Tốc độ tối đa | Tốc độ khi di chuyển | % | 0 |
| **Direction** | 0x0004 | Hướng quay | Tự động điều chỉnh | - | 0 |
| **Max_Speed** | 0x0005 | Giới hạn tốc độ max | Không vượt quá giá trị này | % | 100 |
| **Min_Speed** | 0x0006 | Giới hạn tốc độ min | Tốc độ tối thiểu | % | 0 |
| **PID_Kp** | 0x0007 | Hệ số P | Tỉ lệ (×100) | - | 100 |
| **PID_Ki** | 0x0008 | Hệ số I | Tích phân (×100) | - | 10 |
| **PID_Kd** | 0x0009 | Hệ số D | Vi phân (×100) | - | 5 |
| **Max_Acc** | 0x000A | Gia tốc tối đa | Giới hạn tăng tốc | %/s | 5 |
| **Position_Current** | 0x000E | Vị trí hiện tại | Đọc từ encoder | cm | 0 |
| **Position_Target** | 0x000F | Vị trí mục tiêu | Vị trí cần đến | cm | 0 |

### Giải Thích Các Tham Số

#### 1. **Command_Speed** (Tốc độ di chuyển)
- Đây là tốc độ tối đa mà motor sẽ chạy khi di chuyển đến vị trí mục tiêu
- Giá trị: 0-100 (%)
- Ví dụ: Đặt = 50 → motor chạy tối đa 50% PWM

#### 2. **PID Gains** (Hệ số PID)
- **Kp (Proportional)**: Phản ứng với sai số hiện tại
  - Càng lớn → phản ứng càng nhanh
  - Quá lớn → dao động (overshoot)
  - Giá trị thực = `PID_Kp / 100` (vì lưu ×100)
  
- **Ki (Integral)**: Loại bỏ sai số tích lũy
  - Giúp đạt chính xác vị trí mục tiêu
  - Quá lớn → dao động chậm
  - Giá trị thực = `PID_Ki / 100`
  
- **Kd (Derivative)**: Giảm dao động
  - Làm mượt chuyển động
  - Quá lớn → nhạy với nhiễu
  - Giá trị thực = `PID_Kd / 100`

#### 3. **Max_Acc** (Gia tốc tối đa)
- Giới hạn tốc độ thay đổi của PWM
- Đơn vị: %/giây
- Ví dụ: Max_Acc = 5 → PWM chỉ tăng tối đa 5%/giây

#### 4. **Position_Current** vs **Position_Target**
- **Position_Current**: Vị trí hiện tại (đọc từ encoder) - READ ONLY
- **Position_Target**: Vị trí mục tiêu (do người dùng đặt) - WRITE

---

## 🔄 Chu Trình Hoạt Động

### Sơ Đồ Khối

```
START
  │
  ▼
┌─────────────────────────────────┐
│ 1. Kiểm tra Enable & Mode       │
│    - Enable = 1?                │
│    - Control_Mode = 3?          │
└────────┬────────────────────────┘
         │ NO → Dừng motor, return 0
         │ YES
         ▼
┌─────────────────────────────────┐
│ 2. Đọc Vị Trí Hiện Tại          │
│    current_position =           │
│    motor->Position_Current      │
└────────┬────────────────────────┘
         │
         ▼
┌─────────────────────────────────┐
│ 3. Đọc Vị Trí Mục Tiêu          │
│    target_position =            │
│    motor->Position_Target       │
└────────┬────────────────────────┘
         │
         ▼
┌─────────────────────────────────┐
│ 4. Tính Sai Số Vị Trí           │
│    position_error =             │
│    target - current             │
└────────┬────────────────────────┘
         │
         ▼
┌─────────────────────────────────┐
│ 5. Kiểm tra Đã Đến Đích?        │
│    |error| <= 1 cm?             │
└────────┬────────────────────────┘
         │ YES → Dừng motor
         │ NO
         ▼
┌─────────────────────────────────┐
│ 6. Xác Định Hướng Quay          │
│    error > 0 → FORWARD          │
│    error < 0 → REVERSE          │
└────────┬────────────────────────┘
         │
         ▼
┌─────────────────────────────────┐
│ 7. Tính Toán PID                │
│    output = PID_Compute_        │
│    Position(target, current)    │
└────────┬────────────────────────┘
         │
         ▼
┌─────────────────────────────────┐
│ 8. Giới Hạn Tốc Độ              │
│    - Clamp to Max_Speed         │
│    - Clamp to Min_Speed         │
│    - Apply 0.98 factor          │
└────────┬────────────────────────┘
         │
         ▼
┌─────────────────────────────────┐
│ 9. Xuất PWM                     │
│    Motor_OutputPWM(duty)        │
└────────┬────────────────────────┘
         │
         ▼
END (Lặp lại chu kỳ tiếp theo)
```

### Chi Tiết Từng Bước

#### **Bước 1: Kiểm Tra Điều Kiện**
```c
if (motor->Enable == 0 || motor->Control_Mode != CONTROL_MODE_POSITION) {
    // Reset PID state
    // Dừng motor
    return 0;
}
```
- Kiểm tra motor có được bật không
- Kiểm tra có đúng chế độ Position không
- Nếu không → dừng motor và thoát

#### **Bước 2-3: Đọc Vị Trí**
```c
uint16_t current_position = motor->Position_Current;
uint16_t target_position = motor->Position_Target;
```
- `Position_Current`: Được cập nhật tự động từ encoder
- `Position_Target`: Do người dùng đặt qua Modbus

#### **Bước 4: Tính Sai Số**
```c
int16_t position_error = (int16_t)target_position - (int16_t)current_position;
```
- Sai số dương (+) → cần di chuyển FORWARD
- Sai số âm (-) → cần di chuyển REVERSE

#### **Bước 5: Kiểm Tra Đến Đích**
```c
uint16_t abs_position_error = (position_error > 0) ? position_error : -position_error;

if (abs_position_error <= 1) {
    // Đã đến đích → dừng motor
    motor->Direction = DIRECTION_IDLE;
    motor->Actual_Speed = 0;
    Motor_OutputPWM(motor, 0);
    return 0;
}
```
- Ngưỡng chính xác: ±1 cm
- Có thể điều chỉnh giá trị này nếu cần

#### **Bước 6: Xác Định Hướng**
```c
if (position_error > 0) {
    motor->Direction = DIRECTION_FORWARD;
    Motor_Set_Direction(DIRECTION_FORWARD);
} else if(position_error < 0) {
    motor->Direction = DIRECTION_REVERSE;
    Motor_Set_Direction(DIRECTION_REVERSE);
}
```
- Hướng được tự động điều chỉnh dựa trên sai số
- Người dùng KHÔNG cần đặt Direction thủ công

#### **Bước 7: Tính Toán PID**
```c
float output = PID_Compute_Position(motor_id, 
                                     (float)target_position, 
                                     (float)current_position);
```
- PID tính toán tốc độ cần thiết dựa trên sai số vị trí
- Sai số lớn → tốc độ cao
- Sai số nhỏ → tốc độ thấp (giảm tốc khi gần đích)

#### **Bước 8: Giới Hạn Tốc Độ**
```c
uint8_t duty = (uint8_t)output;

if (duty > motor->Max_Speed) duty = motor->Max_Speed;
if (duty < motor->Min_Speed && duty > 0) duty = motor->Min_Speed;
duty = duty * 0.98;  // Safety factor
```
- Đảm bảo không vượt quá Max_Speed
- Đảm bảo không thấp hơn Min_Speed
- Nhân với 0.98 để an toàn

#### **Bước 9: Xuất PWM**
```c
Motor_OutputPWM(motor, duty);
```
- Điều khiển motor với tốc độ đã tính toán

---

## 📝 Hướng Dẫn Sử Dụng

### Bước 1: Cấu Hình Ban Đầu

```c
// Qua Modbus hoặc code
motor1.Control_Mode = CONTROL_MODE_POSITION;  // = 3
motor1.Enable = 0;  // Chưa bật
motor1.Command_Speed = 50;  // Tốc độ di chuyển 50%
motor1.Max_Speed = 80;      // Giới hạn tối đa 80%
motor1.Min_Speed = 10;      // Tốc độ tối thiểu 10%

// Cấu hình PID (giá trị ×100)
motor1.PID_Kp = 100;  // Kp = 1.00
motor1.PID_Ki = 10;   // Ki = 0.10
motor1.PID_Kd = 5;    // Kd = 0.05

motor1.Max_Acc = 5;   // Gia tốc 5%/s
```

### Bước 2: Đặt Vị Trí Mục Tiêu

```c
// Ví dụ: Di chuyển đến vị trí 150 cm
motor1.Position_Target = 150;  // cm
```

### Bước 3: Bật Motor

```c
motor1.Enable = 1;
```

### Bước 4: Motor Tự Động Di Chuyển

Motor sẽ:
1. Tự động xác định hướng (FORWARD hoặc REVERSE)
2. Tăng tốc đến `Command_Speed`
3. Giảm tốc khi gần đích
4. Dừng chính xác tại vị trí mục tiêu (±1 cm)

### Bước 5: Theo Dõi Tiến Trình

```c
// Đọc vị trí hiện tại
uint16_t current = motor1.Position_Current;

// Đọc tốc độ hiện tại
uint8_t speed = motor1.Actual_Speed;

// Kiểm tra đã đến đích chưa
if (abs(motor1.Position_Target - motor1.Position_Current) <= 1) {
    // Đã đến đích
}
```

---

## 💡 Ví Dụ Cụ Thể

### Ví Dụ 1: Di Chuyển Từ 0 → 200 cm

```c
// Giả sử vị trí hiện tại = 0 cm
motor1.Position_Current = 0;

// Cấu hình
motor1.Control_Mode = CONTROL_MODE_POSITION;
motor1.Command_Speed = 60;  // 60% tốc độ
motor1.Position_Target = 200;  // Đến 200 cm
motor1.Enable = 1;

// Chu trình hoạt động:
// t=0s:   Position = 0 cm,   Error = +200 cm → FORWARD, Speed = 60%
// t=2s:   Position = 50 cm,  Error = +150 cm → FORWARD, Speed = 60%
// t=4s:   Position = 100 cm, Error = +100 cm → FORWARD, Speed = 60%
// t=6s:   Position = 150 cm, Error = +50 cm  → FORWARD, Speed = 40% (PID giảm tốc)
// t=7s:   Position = 180 cm, Error = +20 cm  → FORWARD, Speed = 20%
// t=7.5s: Position = 195 cm, Error = +5 cm   → FORWARD, Speed = 10%
// t=8s:   Position = 200 cm, Error = 0 cm    → IDLE, Speed = 0% (DỪNG)
```

### Ví Dụ 2: Di Chuyển Ngược 200 → 50 cm

```c
// Giả sử vị trí hiện tại = 200 cm
motor1.Position_Current = 200;

// Cấu hình
motor1.Position_Target = 50;  // Quay về 50 cm
motor1.Enable = 1;

// Chu trình hoạt động:
// t=0s:   Position = 200 cm, Error = -150 cm → REVERSE, Speed = 60%
// t=2s:   Position = 150 cm, Error = -100 cm → REVERSE, Speed = 60%
// t=4s:   Position = 100 cm, Error = -50 cm  → REVERSE, Speed = 40%
// t=5s:   Position = 75 cm,  Error = -25 cm  → REVERSE, Speed = 25%
// t=5.5s: Position = 55 cm,  Error = -5 cm   → REVERSE, Speed = 10%
// t=6s:   Position = 50 cm,  Error = 0 cm    → IDLE, Speed = 0% (DỪNG)
```

### Ví Dụ 3: Di Chuyển Liên Tiếp Nhiều Vị Trí

```c
// Vị trí 1: 0 → 100 cm
motor1.Position_Target = 100;
motor1.Enable = 1;
// Đợi đến đích...
while(abs(motor1.Position_Target - motor1.Position_Current) > 1) {
    osDelay(100);
}

// Vị trí 2: 100 → 200 cm
motor1.Position_Target = 200;
// Đợi đến đích...
while(abs(motor1.Position_Target - motor1.Position_Current) > 1) {
    osDelay(100);
}

// Vị trí 3: 200 → 0 cm (về gốc)
motor1.Position_Target = 0;
// Đợi đến đích...
while(abs(motor1.Position_Target - motor1.Position_Current) > 1) {
    osDelay(100);
}

// Tắt motor
motor1.Enable = 0;
```

---

## ⚠️ Lưu Ý Quan Trọng

### 1. **Calibration Encoder Trước Khi Sử Dụng**
```c
// Phải chạy calibration để encoder có giá trị chính xác
motor1.Control_Mode = CONTROL_MODE_CALIB;  // Mode 10
motor1.Enable = 1;
// Đợi calibration hoàn thành...
```

### 2. **Không Đặt Direction Thủ Công**
- Trong Position Mode, `Direction` được tự động điều chỉnh
- KHÔNG đặt `motor1.Direction` thủ công
- Hệ thống sẽ tự động chọn FORWARD/REVERSE dựa trên sai số

### 3. **Giới Hạn Vật Lý**
```c
// Đảm bảo Position_Target nằm trong phạm vi hợp lệ
if (motor1.Position_Target > encoder1.Encoder_Calib_Length_CM_Max) {
    motor1.Position_Target = encoder1.Encoder_Calib_Length_CM_Max;
}
if (motor1.Position_Target < 0) {
    motor1.Position_Target = 0;
}
```

### 4. **Tốc Độ Quét (Scan Rate)**
- Hàm `Motor_HandlePosition` được gọi mỗi 10ms (100 Hz)
- PID được tính toán với `SAMPLE_TIME = 0.01s`
- KHÔNG thay đổi tốc độ quét nếu không hiểu rõ

### 5. **Tuning PID**

#### Phương Pháp Ziegler-Nichols:
1. Đặt Ki = 0, Kd = 0
2. Tăng Kp từ 0 cho đến khi dao động
3. Ghi lại Kp_critical và chu kỳ dao động T
4. Tính toán:
   - Kp = 0.6 × Kp_critical
   - Ki = 1.2 × Kp_critical / T
   - Kd = 0.075 × Kp_critical × T

#### Phương Pháp Thử Nghiệm:
1. **Kp quá nhỏ**: Phản ứng chậm, không đến đích
2. **Kp quá lớn**: Dao động, overshoot
3. **Ki quá nhỏ**: Sai số tĩnh (không đến chính xác)
4. **Ki quá lớn**: Dao động chậm
5. **Kd quá nhỏ**: Overshoot
6. **Kd quá lớn**: Nhạy nhiễu, rung

### 6. **Command_Speed vs Max_Speed**
- `Command_Speed`: Tốc độ mục tiêu khi di chuyển
- `Max_Speed`: Giới hạn tuyệt đối (không bao giờ vượt quá)
- Thường đặt: `Command_Speed < Max_Speed`

### 7. **Min_Speed**
- Tốc độ tối thiểu để motor có thể quay
- Nếu PID tính ra tốc độ < Min_Speed → đặt = Min_Speed
- Tránh trường hợp motor "rung" ở tốc độ quá thấp

### 8. **Safety Factor 0.98**
```c
duty = duty * 0.98;
```
- Giảm 2% để an toàn
- Tránh motor chạy quá tốc độ tối đa
- Có thể điều chỉnh nếu cần

---

## 🐛 Xử Lý Lỗi

### Lỗi 1: Motor Không Chạy

**Nguyên nhân:**
- `Enable = 0`
- `Control_Mode != 3`
- `Position_Target = Position_Current` (đã ở đích)

**Giải pháp:**
```c
// Kiểm tra
if (motor1.Enable == 0) {
    motor1.Enable = 1;
}
if (motor1.Control_Mode != CONTROL_MODE_POSITION) {
    motor1.Control_Mode = CONTROL_MODE_POSITION;
}
if (motor1.Position_Target == motor1.Position_Current) {
    motor1.Position_Target = motor1.Position_Current + 50;  // Di chuyển 50 cm
}
```

### Lỗi 2: Motor Dao Động Quanh Vị Trí Mục Tiêu

**Nguyên nhân:**
- Kp quá lớn
- Kd quá nhỏ
- Min_Speed quá cao

**Giải pháp:**
```c
// Giảm Kp
motor1.PID_Kp = 50;  // Giảm từ 100 xuống 50

// Tăng Kd
motor1.PID_Kd = 20;  // Tăng từ 5 lên 20

// Giảm Min_Speed
motor1.Min_Speed = 5;  // Giảm từ 10 xuống 5
```

### Lỗi 3: Motor Không Đến Đích Chính Xác

**Nguyên nhân:**
- Ki quá nhỏ (sai số tĩnh)
- Min_Speed quá cao (không đủ chính xác)
- Encoder chưa calibration

**Giải pháp:**
```c
// Tăng Ki
motor1.PID_Ki = 20;  // Tăng từ 10 lên 20

// Giảm Min_Speed
motor1.Min_Speed = 5;

// Chạy calibration
motor1.Control_Mode = CONTROL_MODE_CALIB;
motor1.Enable = 1;
```

### Lỗi 4: Motor Chạy Quá Nhanh/Chậm

**Nguyên nhân:**
- `Command_Speed` không phù hợp
- `Max_Acc` quá lớn/nhỏ

**Giải pháp:**
```c
// Điều chỉnh Command_Speed
motor1.Command_Speed = 40;  // Giảm tốc độ

// Điều chỉnh Max_Acc
motor1.Max_Acc = 3;  // Giảm gia tốc
```

### Lỗi 5: Motor Chạy Sai Hướng

**Nguyên nhân:**
- Encoder đấu ngược
- Motor đấu ngược

**Giải pháp:**
```c
// Kiểm tra định nghĩa FORWARD/REVERSE trong code
// Hoặc đổi dây motor
```

---

## 📊 Bảng Tham Số Khuyến Nghị

| Tham Số | Giá Trị Khởi Đầu | Phạm Vi | Ghi Chú |
|---------|-------------------|---------|---------|
| Command_Speed | 50 | 20-80 | Tốc độ trung bình |
| Max_Speed | 80 | 50-100 | Giới hạn an toàn |
| Min_Speed | 10 | 5-20 | Đủ để motor quay |
| PID_Kp | 100 | 50-200 | Phản ứng nhanh |
| PID_Ki | 10 | 5-30 | Loại bỏ sai số |
| PID_Kd | 5 | 0-20 | Giảm dao động |
| Max_Acc | 5 | 3-10 | Tăng tốc mượt |

---

## 🔍 Debug và Giám Sát

### Đọc Trạng Thái Qua Modbus

```c
// Đọc vị trí hiện tại
uint16_t current_pos = g_holdingRegisters[REG_M1_POSITION_CURRENT];

// Đọc vị trí mục tiêu
uint16_t target_pos = g_holdingRegisters[REG_M1_POSITION_TARGET];

// Đọc tốc độ hiện tại
uint8_t speed = g_holdingRegisters[REG_M1_ACTUAL_SPEED];

// Đọc hướng
uint8_t direction = g_holdingRegisters[REG_M1_DIRECTION];

// Tính sai số
int16_t error = target_pos - current_pos;
```

### Debug PID Values

```c
// Hàm PID_DebugPrint() lưu giá trị PID vào registers
PID_DebugPrint(1);  // Motor 1

// Đọc debug values
uint16_t pid_error = g_holdingRegisters[0x00E0];      // Error ×10
uint16_t pid_integral = g_holdingRegisters[0x00E1];   // Integral ×10
uint16_t pid_output = g_holdingRegisters[0x00E2];     // Output
```

---

## 📚 Tài Liệu Tham Khảo

- **MotorControl.c**: Dòng 359-462 (hàm `Motor_HandlePosition`)
- **MotorControl.c**: Dòng 718-824 (hàm `PID_Compute_Position`)
- **Encoder.c**: Dòng 446-557 (hàm `Encoder_MeasureLength`)
- **ModbusMap.h**: Định nghĩa registers và constants

---

## 📞 Hỗ Trợ

Nếu gặp vấn đề, kiểm tra:
1. ✅ Encoder đã calibration chưa?
2. ✅ Enable = 1?
3. ✅ Control_Mode = 3?
4. ✅ Position_Target khác Position_Current?
5. ✅ PID gains đã tuning chưa?
6. ✅ Max_Speed/Min_Speed hợp lý?

---

**Phiên bản**: 1.0  
**Ngày cập nhật**: 2025-11-22  
**Tác giả**: DC Motor Driver Team

