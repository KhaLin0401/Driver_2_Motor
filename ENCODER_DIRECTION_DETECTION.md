# 🔄 XÁC ĐỊNH CHIỀU QUAY ENCODER DỰA TRÊN MOTOR DIRECTION

## 🎯 VẤN ĐỀ

**Encoder 1 kênh (TIM_ENCODERMODE_TI1) chỉ đếm lên, không thể tự phát hiện chiều quay!**

### ❌ **Hạn chế của Encoder 1 kênh:**

```
Encoder 1 kênh (chỉ có Channel A):
┌────────────────────────────────────────┐
│ Quay thuận (CW):  Counter tăng 0→100  │
│ Quay ngược (CCW): Counter vẫn tăng 0→100 │  ❌ Không phân biệt được!
└────────────────────────────────────────┘
```

**Kết quả:** Encoder luôn đếm lên bất kể chiều quay → Không biết dây đang kéo ra hay cuộn vào!

---

## ✅ GIẢI PHÁP: Sử dụng Motor Direction

### **Ý tưởng:**

Motor biết chiều quay của nó (FORWARD/REVERSE) → Sử dụng thông tin này để xác định dấu của delta_count!

```
┌──────────────────────────────────────────────────────────────┐
│  Motor Direction  │  Encoder Count  │  Delta Sign  │  Ý nghĩa  │
├──────────────────────────────────────────────────────────────┤
│  FORWARD (1)      │  Tăng 0→100    │  +100        │  Kéo dây ra  │
│  REVERSE (2)      │  Tăng 0→100    │  -100        │  Cuộn dây vào │
│  IDLE (0)         │  Không đổi     │  0           │  Dừng        │
└──────────────────────────────────────────────────────────────┘
```

---

## 🔧 TRIỂN KHAI

### **1. Đọc Motor Direction trong Encoder_MeasureLength()**

```c
// Import motor1 từ MotorControl
extern MotorRegisterMap_t motor1;

// Xác định dấu dựa trên Direction
int8_t direction_sign = 0;

if (motor1.Direction == 1) {  // FORWARD - kéo dây ra
    direction_sign = +1;
} else if (motor1.Direction == 2) {  // REVERSE - cuộn dây vào
    direction_sign = -1;
} else {  // IDLE - không chuyển động
    direction_sign = 0;
}
```

### **2. Tính delta có dấu**

```c
// Encoder chỉ đếm lên → delta_count_abs luôn dương
uint16_t delta_count_abs = current_count - last_encoder_count;

// Áp dụng dấu từ motor direction
int32_t delta_count = (int32_t)delta_count_abs * direction_sign;

// Bây giờ delta_count có dấu đúng:
// - FORWARD: delta_count > 0 → unrolled_length tăng ✅
// - REVERSE: delta_count < 0 → unrolled_length giảm ✅
// - IDLE: delta_count = 0 → unrolled_length không đổi ✅
```

### **3. Tính toán độ dài dây**

```c
// Chuyển đổi counts → revolutions → mm
float delta_revolutions = (float)delta_count / (float)ENCODER_CPR;
float delta_length_mm = current_circumference * delta_revolutions;

// Cập nhật độ dài tích lũy
encoder_state.unrolled_length_mm += delta_length_mm;
// - FORWARD: delta_length_mm > 0 → length tăng ✅
// - REVERSE: delta_length_mm < 0 → length giảm ✅
```

---

## 📊 LUỒNG DỮ LIỆU

```
┌─────────────────────────────────────────────────────────────────┐
│                    MOTOR CONTROL                                │
│  motor1.Direction = FORWARD (1) / REVERSE (2) / IDLE (0)       │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│              ENCODER HARDWARE (TIM2)                            │
│  Counter tăng: 0 → 100 (không phân biệt chiều)                 │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│         Encoder_MeasureLength() - LOGIC MỚI                     │
│  1. Đọc delta_count_abs = current - last (luôn dương)         │
│  2. Đọc motor1.Direction                                        │
│  3. Tính direction_sign:                                        │
│     - FORWARD (1) → +1                                          │
│     - REVERSE (2) → -1                                          │
│     - IDLE (0) → 0                                              │
│  4. delta_count = delta_count_abs × direction_sign             │
│  5. Tính delta_length_mm (có dấu)                              │
│  6. unrolled_length_mm += delta_length_mm                      │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🎯 VÍ DỤ THỰC TẾ

### **Scenario 1: Kéo dây ra (FORWARD)**

```
Thời điểm t=0:
  motor1.Direction = FORWARD (1)
  encoder->Encoder_Count = 0
  unrolled_length_mm = 0 mm

Thời điểm t=1s: (Motor chạy FORWARD)
  motor1.Direction = FORWARD (1)
  encoder->Encoder_Count = 100
  delta_count_abs = 100 - 0 = 100
  direction_sign = +1 (vì FORWARD)
  delta_count = 100 × (+1) = +100 ✅
  delta_revolutions = 100 / 16 = 6.25 vòng
  delta_length_mm = 6.25 × 220mm = +1375 mm ✅
  unrolled_length_mm = 0 + 1375 = 1375 mm ✅
  
→ Dây kéo ra 1375mm (137.5cm) ✅
```

### **Scenario 2: Cuộn dây vào (REVERSE)**

```
Thời điểm t=0:
  motor1.Direction = REVERSE (2)
  encoder->Encoder_Count = 0
  unrolled_length_mm = 1375 mm (từ scenario 1)

Thời điểm t=1s: (Motor chạy REVERSE)
  motor1.Direction = REVERSE (2)
  encoder->Encoder_Count = 50
  delta_count_abs = 50 - 0 = 50
  direction_sign = -1 (vì REVERSE) ✅
  delta_count = 50 × (-1) = -50 ✅
  delta_revolutions = -50 / 16 = -3.125 vòng
  delta_length_mm = -3.125 × 220mm = -687.5 mm ✅
  unrolled_length_mm = 1375 + (-687.5) = 687.5 mm ✅
  
→ Dây cuộn vào 687.5mm, còn lại 687.5mm (68.75cm) ✅
```

### **Scenario 3: Motor dừng (IDLE)**

```
Thời điểm t=0:
  motor1.Direction = IDLE (0)
  encoder->Encoder_Count = 100
  unrolled_length_mm = 1000 mm

Thời điểm t=1s: (Motor IDLE, nhưng có nhiễu làm counter nhảy)
  motor1.Direction = IDLE (0)
  encoder->Encoder_Count = 105 (nhiễu)
  delta_count_abs = 105 - 100 = 5
  direction_sign = 0 (vì IDLE) ✅
  delta_count = 5 × 0 = 0 ✅
  delta_length_mm = 0 mm
  unrolled_length_mm = 1000 + 0 = 1000 mm ✅
  
→ Độ dài không đổi (bỏ qua nhiễu khi motor dừng) ✅
```

---

## ⚠️ LƯU Ý QUAN TRỌNG

### **1. Đồng bộ Direction và Encoder**

```c
// ✅ ĐÚNG: Set Direction TRƯỚC khi motor chạy
motor1.Direction = FORWARD;
Motor1_Set_Direction(DIRECTION_FORWARD);
Motor1_OutputPWM(motor, 50);  // Bắt đầu chạy

// ❌ SAI: Motor đã chạy nhưng Direction chưa set
Motor1_OutputPWM(motor, 50);  // Motor chạy
motor1.Direction = FORWARD;   // Set Direction sau → Encoder đọc sai!
```

### **2. Xử lý trường hợp Direction thay đổi đột ngột**

```c
// Motor đổi chiều đột ngột:
t=0: Direction = FORWARD, Count = 100
t=1: Direction = REVERSE, Count = 150

// Không cần xử lý đặc biệt!
// delta_count_abs = 150 - 100 = 50
// direction_sign = -1 (REVERSE)
// delta_count = 50 × (-1) = -50 ✅
// Encoder tự động tính đúng!
```

### **3. Reset Encoder khi đổi chiều**

**KHÔNG CẦN reset encoder khi đổi chiều!**

```c
// ❌ SAI: Reset encoder khi đổi chiều
if (motor1.Direction != last_direction) {
    Encoder_Reset(&encoder1);  // KHÔNG CẦN!
}

// ✅ ĐÚNG: Để encoder tiếp tục đếm
// Logic direction_sign sẽ tự động xử lý dấu
```

---

## 📋 SO SÁNH TRƯỚC VÀ SAU

| Tiêu chí | TRƯỚC (Encoder 2 kênh) | SAU (Encoder 1 kênh + Direction) |
|----------|------------------------|----------------------------------|
| **Phát hiện chiều** | Tự động (hardware) | Từ motor Direction (software) |
| **Encoder Mode** | TIM_ENCODERMODE_TI12 | TIM_ENCODERMODE_TI1 |
| **Số kênh cần** | 2 (A + B) | 1 (A only) |
| **Delta calculation** | Hardware tự tính dấu | Software áp dụng dấu từ Direction |
| **Độ chính xác** | Cao (hardware) | Phụ thuộc đồng bộ Direction |
| **Ưu điểm** | Chính xác, tự động | Đơn giản, ít dây |
| **Nhược điểm** | Cần 2 kênh | Phải đồng bộ Direction |

---

## ✅ CHECKLIST TRIỂN KHAI

- [x] Import `motor1` từ `MotorControl.h` ✅
- [x] Đọc `motor1.Direction` trong `Encoder_MeasureLength()` ✅
- [x] Tính `direction_sign` dựa trên Direction ✅
- [x] Áp dụng dấu: `delta_count = delta_abs × direction_sign` ✅
- [x] Cập nhật comment giải thích logic mới ✅
- [x] Xóa code xử lý wrap-around không cần thiết ✅
- [x] Test với cả 3 trường hợp: FORWARD, REVERSE, IDLE ✅

---

## 🎯 KẾT LUẬN

✅ **Encoder 1 kênh giờ có thể phát hiện chiều quay bằng cách sử dụng thông tin Direction từ motor!**

✅ **Logic đơn giản:**
- FORWARD (1) → delta_count dương → dây kéo ra
- REVERSE (2) → delta_count âm → dây cuộn vào
- IDLE (0) → delta_count = 0 → không đổi

✅ **Lợi ích:**
- Giảm số dây encoder (chỉ cần 1 kênh)
- Logic rõ ràng, dễ debug
- Tương thích với hardware hiện tại

⚠️ **Lưu ý:**
- Phải đảm bảo `motor1.Direction` được set ĐÚNG và ĐỒNG BỘ với motor thực tế
- Nếu Direction sai → Encoder tính ngược chiều!


