# 🚀 Acceleration Limiting Implementation

## 📋 Tổng quan

Đã implement **Acceleration Limiting** vào hàm `Motor_HandlePositionControl()` để giải quyết vấn đề tốc độ nhảy trực tiếp lên `Command_Speed` khi set target position mới.

---

## ⚠️ Vấn đề trước khi fix

### Hiện tượng:
Khi set `Position_Target` mới, motor nhảy trực tiếp lên tốc độ `Command_Speed` thay vì tăng dần.

### Nguyên nhân:
1. **P-term quá lớn**: Với error lớn (ví dụ 90cm), `P_term = Kp × error = 5.0 × 90 = 450%`
2. **PID output bị clamp ngay**: Output 450% bị giới hạn xuống `Command_Speed (80%)` ngay lập tức
3. **Không có ramping**: Thiếu cơ chế giới hạn tốc độ thay đổi giữa các chu kỳ

### Ví dụ cụ thể:
```
Position_Current = 10 cm
Position_Target  = 100 cm
Command_Speed    = 80%
Max_Acc          = 1000 (%/s)
PID_Kp           = 500 (thực tế = 5.0)

Chu kỳ 1:
├─ error = 90 cm
├─ P_term = 5.0 × 90 = 450%
├─ PID output = 450% → clamp xuống 80%
└─ Actual_Speed = 80% ← NHẢY TRỰC TIẾP!
```

---

## ✅ Giải pháp đã implement

### 1. Thêm Acceleration Limiting Logic

**Vị trí**: `Core/Src/MotorControl.c` - Hàm `Motor_HandlePositionControl()`

**Code đã thêm** (sau dòng 452):

```c
// ═══════════════════════════════════════════════════════════════════════════════
// ✅ ACCELERATION LIMITING - Giới hạn tốc độ thay đổi output
// ═══════════════════════════════════════════════════════════════════════════════
// Prevents sudden jumps in motor speed by limiting acceleration/deceleration
// Max_Acc is in %/s, we need %/cycle (cycle = 10ms = 0.01s)
static float previous_output1 = 0.0f;
static float previous_output2 = 0.0f;
float* prev_output = (motor_id == 1) ? &previous_output1 : &previous_output2;

// Calculate max allowed change per cycle (Max_Acc is in %/second)
// Sample time = 10ms = 0.01s
float max_delta_per_cycle = pid_state->acceleration_limit * 0.01f;

// Apply acceleration limiting
if (output > *prev_output + max_delta_per_cycle) {
    // Acceleration too high - limit increase
    output = *prev_output + max_delta_per_cycle;
} else if (output < *prev_output - max_delta_per_cycle) {
    // Deceleration too high - limit decrease
    output = *prev_output - max_delta_per_cycle;
}

// Store current output for next cycle
*prev_output = output;
// ═══════════════════════════════════════════════════════════════════════════════
```

### 2. Reset Acceleration State khi Disable

**Vị trí**: `Core/Src/MotorControl.c` - Dòng 372-380

**Code đã thêm**:

```c
// Reset acceleration limiting state (declared as static in code below)
// This ensures smooth start from 0 when re-enabled
static float previous_output1 = 0.0f;
static float previous_output2 = 0.0f;
if (motor_id == 1) {
    previous_output1 = 0.0f;
} else {
    previous_output2 = 0.0f;
}
```

---

## 🔧 Cách hoạt động

### Công thức tính toán:

```
max_delta_per_cycle = Max_Acc × sample_time
                    = Max_Acc × 0.01 (10ms)
                    
Ví dụ: Max_Acc = 1000 %/s
       max_delta_per_cycle = 1000 × 0.01 = 10% per cycle
```

### Luồng xử lý mỗi chu kỳ (10ms):

```
1. PID tính toán output dựa trên position error
   ├─ output_pid = PID_Compute_Position(...)
   └─ Ví dụ: output_pid = 80%

2. So sánh với output chu kỳ trước
   ├─ previous_output = 0%
   ├─ delta = 80% - 0% = 80%
   └─ max_delta_per_cycle = 10%

3. Áp dụng giới hạn
   ├─ if (80% > 0% + 10%) → TRUE
   ├─ output = 0% + 10% = 10%
   └─ Actual_Speed = 10% ← TĂNG DẦN!

4. Lưu trữ cho chu kỳ tiếp theo
   └─ previous_output = 10%
```

### Ví dụ chi tiết:

**Điều kiện ban đầu:**
- `Position_Current` = 10 cm
- `Position_Target` = 100 cm
- `Command_Speed` = 80%
- `Max_Acc` = 1000 %/s → 10% per 10ms cycle
- `PID_Kp` = 500

**Quá trình tăng tốc:**

| Chu kỳ | Error (cm) | PID Output | Prev Output | Max Delta | Limited Output | Actual Speed |
|---------|-----------|------------|-------------|-----------|----------------|--------------|
| 1       | 90        | 80%        | 0%          | 10%       | 10%            | 10%          |
| 2       | 89        | 80%        | 10%         | 10%       | 20%            | 20%          |
| 3       | 88        | 80%        | 20%         | 10%       | 30%            | 30%          |
| 4       | 87        | 80%        | 30%         | 10%       | 40%            | 40%          |
| 5       | 86        | 80%        | 40%         | 10%       | 50%            | 50%          |
| 6       | 85        | 80%        | 50%         | 10%       | 60%            | 60%          |
| 7       | 84        | 80%        | 60%         | 10%       | 70%            | 70%          |
| 8       | 83        | 80%        | 70%         | 10%       | 80%            | 80%          |
| 9+      | <83       | 80%        | 80%         | 10%       | 80%            | 80%          |

**Thời gian tăng tốc từ 0 → 80%:**
- Số chu kỳ cần = 80% / 10% = 8 chu kỳ
- Thời gian = 8 × 10ms = **80ms**

---

## 📊 So sánh trước và sau

### ❌ Trước khi fix:

```
Time (ms)  |  0  | 10  | 20  | 30  | 40  | 50  |
Speed (%)  |  0  | 80  | 80  | 80  | 80  | 80  |
           └─────┘ ← NHẢY ĐỘT NGỘT!
```

### ✅ Sau khi fix:

```
Time (ms)  |  0  | 10  | 20  | 30  | 40  | 50  | 60  | 70  | 80  |
Speed (%)  |  0  | 10  | 20  | 30  | 40  | 50  | 60  | 70  | 80  |
           └─────────────────────────────────────────────────────┘
                        TĂNG DẦN MƯỢT MÀ!
```

---

## ⚙️ Cấu hình tham số

### Max_Acc (Register 0x001A / 0x002A)

**Đơn vị**: %/second  
**Phạm vi**: 0 - 65535  
**Ý nghĩa**: Tốc độ tăng/giảm tốc tối đa

**Ví dụ cấu hình:**

| Max_Acc | Thời gian 0→100% | Ứng dụng |
|---------|------------------|----------|
| 500     | 200ms            | Chuyển động mượt, ít rung |
| 1000    | 100ms            | Cân bằng tốc độ/mượt |
| 2000    | 50ms             | Phản ứng nhanh |
| 5000    | 20ms             | Rất nhanh, có thể rung |
| 10000   | 10ms             | Gần như không giới hạn |

**Khuyến nghị:**
- Ứng dụng cần mượt: `Max_Acc = 500-1000`
- Ứng dụng cần nhanh: `Max_Acc = 2000-3000`
- Test và điều chỉnh dựa trên cơ học hệ thống

---

## 🧪 Testing

### Test Case 1: Tăng tốc từ 0

```python
# Modbus commands
write_register(0x001F, 100)   # Position_Target = 100 cm
write_register(0x0012, 80)    # Command_Speed = 80%
write_register(0x001A, 1000)  # Max_Acc = 1000 %/s
write_register(0x0011, 1)     # Enable = 1

# Expected: Speed tăng dần 0→10→20→...→80% trong 80ms
```

### Test Case 2: Giảm tốc

```python
# Motor đang chạy 80%
write_register(0x001F, current_position)  # Target = current (stop)

# Expected: Speed giảm dần 80→70→60→...→0% trong 80ms
```

### Test Case 3: Reset khi disable

```python
write_register(0x0011, 0)     # Disable motor
# Wait...
write_register(0x0011, 1)     # Re-enable

# Expected: Speed bắt đầu từ 0%, không nhảy đột ngột
```

---

## 📝 Lưu ý quan trọng

### 1. Static Variables
- `previous_output1` và `previous_output2` là **static** → giữ giá trị giữa các lần gọi hàm
- Được reset về 0 khi motor disable hoặc đổi mode

### 2. Sample Time
- Code giả định chu kỳ gọi hàm = **10ms**
- Nếu thay đổi chu kỳ, cần update công thức: `max_delta_per_cycle = Max_Acc × new_sample_time`

### 3. Tương tác với PID
- Acceleration limiting hoạt động **sau** PID
- PID vẫn tính toán bình thường, chỉ output bị giới hạn tốc độ thay đổi
- Không ảnh hưởng đến integral và derivative terms

### 4. Min_Speed
- Acceleration limiting áp dụng **trước** Min_Speed clamp
- Nếu `Min_Speed = 20%` và `Max_Acc = 1000`, motor sẽ tăng từ 0→10% (bị clamp lên 20%) → 20%+

---

## 🔍 Troubleshooting

### Vấn đề: Tốc độ vẫn nhảy đột ngột

**Nguyên nhân có thể:**
1. `Max_Acc` quá lớn (>5000)
2. Chu kỳ gọi hàm không đúng 10ms
3. Static variables bị reset không đúng cách

**Giải pháp:**
- Giảm `Max_Acc` xuống 500-1000
- Verify sample time trong code
- Kiểm tra logic reset trong phần disable

### Vấn đề: Motor phản ứng quá chậm

**Nguyên nhân:**
- `Max_Acc` quá nhỏ (<500)

**Giải pháp:**
- Tăng `Max_Acc` lên 1000-2000
- Cân bằng giữa tốc độ phản ứng và độ mượt

---

## 📚 Tài liệu liên quan

- `modbus_map.md` - Định nghĩa register Max_Acc
- `Position_control_docs.md` - Chi tiết về Position Control mode
- `MotorControl.c` - Source code implementation

---

**Ngày cập nhật**: 2025-11-23  
**Version**: 1.0  
**Tác giả**: AI Assistant



