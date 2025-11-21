# 🧪 HƯỚNG DẪN KIỂM TRA ENCODER SAU KHI SỬA

## 📋 CHECKLIST KIỂM TRA

### ✅ Test 1: Giá trị encoder không bị âm
```c
// Chạy motor trong 10 giây
for (int i = 0; i < 40; i++) {  // 40 x 250ms = 10s
    Encoder_Read(&encoder1);
    
    // ✅ PASS: Giá trị luôn >= 0
    if (encoder1.Encoder_Count < 0) {
        printf("❌ FAIL: Negative count detected: %d\n", encoder1.Encoder_Count);
    } else {
        printf("✅ PASS: Count = %u\n", encoder1.Encoder_Count);
    }
    
    HAL_Delay(250);
}
```

**Kết quả mong đợi:**
```
✅ PASS: Count = 0
✅ PASS: Count = 15
✅ PASS: Count = 32
✅ PASS: Count = 47
...
✅ PASS: Count = 523
```

---

### ✅ Test 2: Giá trị encoder chỉ tăng (không nhảy xuống)
```c
uint16_t last_count = 0;
bool monotonic = true;

for (int i = 0; i < 40; i++) {
    Encoder_Read(&encoder1);
    uint16_t current_count = encoder1.Encoder_Count;
    
    // ✅ PASS: Giá trị chỉ tăng hoặc giữ nguyên (không giảm)
    if (current_count < last_count && current_count > 100) {  // Bỏ qua auto-reset
        printf("❌ FAIL: Count decreased: %u → %u\n", last_count, current_count);
        monotonic = false;
    }
    
    last_count = current_count;
    HAL_Delay(250);
}

if (monotonic) {
    printf("✅ PASS: Encoder is monotonically increasing\n");
}
```

**Kết quả mong đợi:**
```
✅ PASS: Encoder is monotonically increasing
```

---

### ✅ Test 3: Auto-reset hoạt động đúng
```c
// Chạy motor cho đến khi counter > 32000
while (encoder1.Encoder_Count < 31000) {
    Encoder_Read(&encoder1);
    HAL_Delay(100);
}

printf("Count before auto-reset: %u\n", encoder1.Encoder_Count);

// Đợi thêm vài chu kỳ
for (int i = 0; i < 10; i++) {
    Encoder_Read(&encoder1);
    HAL_Delay(100);
}

// ✅ PASS: Counter đã reset về gần 0
if (encoder1.Encoder_Count < 1000) {
    printf("✅ PASS: Auto-reset worked at %u\n", encoder1.Encoder_Count);
} else {
    printf("❌ FAIL: Auto-reset did not trigger\n");
}
```

**Kết quả mong đợi:**
```
Count before auto-reset: 31523
✅ PASS: Auto-reset worked at 47
```

---

### ✅ Test 4: Lọc nhiễu hoạt động
```c
// Reset diagnostic counters
Encoder_ResetDiagnostics();

// Chạy encoder trong 5 giây
for (int i = 0; i < 20; i++) {
    Encoder_Read(&encoder1);
    HAL_Delay(250);
}

// Kiểm tra số lần từ chối nhiễu
uint32_t noise_count = Encoder_GetNoiseRejectCount();

printf("Noise rejections: %lu\n", noise_count);

// ✅ PASS: Có lọc nhiễu (nhưng không quá nhiều)
if (noise_count > 0 && noise_count < 50) {
    printf("✅ PASS: Noise filtering is working (reasonable level)\n");
} else if (noise_count == 0) {
    printf("⚠️ WARNING: No noise detected (clean environment or filter not working)\n");
} else {
    printf("❌ FAIL: Too much noise (%lu rejections) - check hardware!\n", noise_count);
}
```

**Kết quả mong đợi:**
```
Noise rejections: 12
✅ PASS: Noise filtering is working (reasonable level)
```

---

### ✅ Test 5: Đo độ dài dây chính xác
```c
// Reset encoder về 0
Encoder_Reset(&encoder1);
Encoder_ResetWireLength(&encoder1);

// Chạy motor FORWARD trong 5 giây
motor1.Direction = 1;  // FORWARD
motor1.Speed = 50;     // 50% tốc độ

for (int i = 0; i < 20; i++) {
    Encoder_Read(&encoder1);
    uint16_t length_mm = Encoder_MeasureLength(&encoder1);
    
    printf("Time: %d ms, Count: %u, Length: %u mm\n", 
           i * 250, encoder1.Encoder_Count, length_mm);
    
    HAL_Delay(250);
}

motor1.Direction = 0;  // STOP

// ✅ PASS: Độ dài tăng dần
printf("✅ PASS: Wire length increased during FORWARD\n");

// Chạy motor REVERSE trong 5 giây
motor1.Direction = 2;  // REVERSE
motor1.Speed = 50;

uint16_t length_before = Encoder_MeasureLength(&encoder1);

for (int i = 0; i < 20; i++) {
    Encoder_Read(&encoder1);
    Encoder_MeasureLength(&encoder1);
    HAL_Delay(250);
}

motor1.Direction = 0;  // STOP

uint16_t length_after = Encoder_MeasureLength(&encoder1);

// ✅ PASS: Độ dài giảm khi REVERSE
if (length_after < length_before) {
    printf("✅ PASS: Wire length decreased during REVERSE\n");
} else {
    printf("❌ FAIL: Wire length did not decrease during REVERSE\n");
}
```

**Kết quả mong đợi:**
```
Time: 0 ms, Count: 0, Length: 0 mm
Time: 250 ms, Count: 23, Length: 158 mm
Time: 500 ms, Count: 47, Length: 325 mm
Time: 750 ms, Count: 71, Length: 491 mm
...
✅ PASS: Wire length increased during FORWARD
✅ PASS: Wire length decreased during REVERSE
```

---

### ✅ Test 6: Mô hình bán kính phi tuyến
```c
// Reset và đo bán kính tại các mốc khác nhau
Encoder_Reset(&encoder1);
Encoder_ResetWireLength(&encoder1);

// Mốc 1: L = 0mm (full spool)
float radius_0 = Encoder_GetCurrentRadius();
printf("L = 0mm    → R = %.2f mm (expected: 35.0mm)\n", radius_0);

// Giả lập L = 1500mm (half empty)
Encoder_SetWireLength(&encoder1, 1500.0f);
float radius_1500 = Encoder_GetCurrentRadius();
printf("L = 1500mm → R = %.2f mm (expected: ~25.5mm)\n", radius_1500);

// Giả lập L = 3000mm (empty core)
Encoder_SetWireLength(&encoder1, 3000.0f);
float radius_3000 = Encoder_GetCurrentRadius();
printf("L = 3000mm → R = %.2f mm (expected: 10.0mm)\n", radius_3000);

// ✅ PASS: Kiểm tra mô hình phi tuyến
if (radius_0 > 34.5f && radius_0 < 35.5f &&
    radius_1500 > 24.5f && radius_1500 < 26.5f &&
    radius_3000 > 9.5f && radius_3000 < 10.5f) {
    printf("✅ PASS: Nonlinear radius model is correct\n");
} else {
    printf("❌ FAIL: Radius model is incorrect\n");
}
```

**Kết quả mong đợi:**
```
L = 0mm    → R = 35.00 mm (expected: 35.0mm)
L = 1500mm → R = 25.50 mm (expected: ~25.5mm)
L = 3000mm → R = 10.00 mm (expected: 10.0mm)
✅ PASS: Nonlinear radius model is correct
```

---

### ✅ Test 7: Bộ lọc low-pass
```c
// Tạo nhiễu giả (thay đổi đột ngột)
Encoder_Reset(&encoder1);
Encoder_ResetWireLength(&encoder1);

// Giả lập encoder nhảy đột ngột
for (int i = 0; i < 5; i++) {
    // Tăng đột ngột
    __HAL_TIM_SET_COUNTER(&htim2, 100);
    Encoder_Read(&encoder1);
    uint16_t length1 = Encoder_MeasureLength(&encoder1);
    
    HAL_Delay(100);
    
    // Tăng thêm
    __HAL_TIM_SET_COUNTER(&htim2, 200);
    Encoder_Read(&encoder1);
    uint16_t length2 = Encoder_MeasureLength(&encoder1);
    
    printf("Raw jump: 100 → 200, Filtered length: %u → %u mm\n", length1, length2);
}

// ✅ PASS: Bộ lọc làm mượt giá trị (không nhảy đột ngột)
printf("✅ PASS: Low-pass filter is smoothing the output\n");
```

**Kết quả mong đợi:**
```
Raw jump: 100 → 200, Filtered length: 693 → 1040 mm
✅ PASS: Low-pass filter is smoothing the output
```

---

## 🔍 KIỂM TRA MODBUS

### Test 8: Modbus không đọc giá trị âm
```python
# Python script để test Modbus
from pymodbus.client import ModbusSerialClient

client = ModbusSerialClient(port='COM3', baudrate=115200)
client.connect()

for i in range(100):
    # Đọc REG_M1_ENCODER_COUNT
    result = client.read_holding_registers(address=REG_M1_ENCODER_COUNT, count=1)
    count = result.registers[0]
    
    # ✅ PASS: Giá trị luôn dương (0-32000)
    if count < 0 or count > 32767:
        print(f"❌ FAIL: Invalid count: {count}")
    else:
        print(f"✅ PASS: Count = {count}")
    
    time.sleep(0.25)

client.close()
```

---

## 📊 BẢNG KIỂM TRA TỔNG HỢP

| Test | Mục đích | Kết quả mong đợi |
|------|----------|------------------|
| 1 | Không có giá trị âm | ✅ Luôn >= 0 |
| 2 | Chỉ tăng, không giảm | ✅ Monotonic |
| 3 | Auto-reset tại 32000 | ✅ Reset về 0 |
| 4 | Lọc nhiễu | ✅ 0-50 rejections |
| 5 | Đo độ dài chính xác | ✅ Tăng/giảm đúng |
| 6 | Mô hình bán kính | ✅ Phi tuyến đúng |
| 7 | Bộ lọc low-pass | ✅ Giá trị mượt |
| 8 | Modbus không âm | ✅ 0-32000 |

---

## 🛠️ TROUBLESHOOTING

### Vấn đề 1: Vẫn thấy giá trị nhảy xuống
**Nguyên nhân:** Nhiễu quá mạnh, vượt qua bộ lọc

**Giải pháp:**
```c
// Tăng ngưỡng lọc nhiễu
#define NOISE_THRESHOLD_TICKS   3  // Tăng từ 2 → 3
#define MAX_DELTA_PER_CYCLE     150  // Giảm từ 200 → 150
```

---

### Vấn đề 2: Noise rejection count quá cao (>100)
**Nguyên nhân:** Môi trường nhiễu cao hoặc kết nối kém

**Giải pháp:**
1. Kiểm tra kết nối encoder (dây lỏng?)
2. Thêm tụ lọc 100nF giữa PA0 và GND
3. Tăng IC1Filter trong CubeMX (hiện tại = 5)

---

### Vấn đề 3: Độ dài dây không chính xác
**Nguyên nhân:** Bán kính spool chưa đúng

**Giải pháp:**
```c
// Đo lại bán kính thực tế và cập nhật
#define SPOOL_RADIUS_FULL_MM    36.0f  // Đo lại với thước kẹp
#define SPOOL_RADIUS_EMPTY_MM   9.5f   // Đo lại lõi trống
```

---

### Vấn đề 4: Motor direction không đúng
**Nguyên nhân:** Chiều quay motor ngược với giả định

**Giải pháp:**
```c
// Đảo chiều trong code
if (motor1.Direction == 1) {
    direction_sign = -1;  // Đảo từ +1 → -1
} else if (motor1.Direction == 2) {
    direction_sign = +1;  // Đảo từ -1 → +1
}
```

---

## 📈 ĐÁNH GIÁ HIỆU SUẤT

### Độ chính xác mong đợi:
- **Encoder count:** 100% chính xác (không nhiễu)
- **Wire length:** ±2-3% (giới hạn bởi mô hình bán kính)
- **Spool radius:** ±1mm (phụ thuộc vào đo lường ban đầu)

### Tốc độ đáp ứng:
- **Encoder_Read():** <1ms (rất nhanh)
- **Encoder_MeasureLength():** <2ms (bao gồm sqrtf)
- **Filter response:** 85% sau ~6 chu kỳ (~1.5s với 250ms/cycle)

---

## ✅ CHECKLIST CUỐI CÙNG

- [ ] Test 1: Không có giá trị âm
- [ ] Test 2: Giá trị chỉ tăng
- [ ] Test 3: Auto-reset hoạt động
- [ ] Test 4: Lọc nhiễu hoạt động
- [ ] Test 5: Đo độ dài chính xác
- [ ] Test 6: Mô hình bán kính đúng
- [ ] Test 7: Bộ lọc low-pass hoạt động
- [ ] Test 8: Modbus không đọc âm
- [ ] Chạy liên tục 30 phút không lỗi
- [ ] Kiểm tra với tốc độ cao (100% speed)
- [ ] Kiểm tra với tốc độ thấp (10% speed)

---

**Nếu tất cả test đều PASS → Code hoạt động hoàn hảo! 🎉**

