# ✅ CẢI THIỆN CẤU HÌNH ENCODER - HOÀN THÀNH

## 📋 Tóm tắt thay đổi

Đã sửa cấu hình Timer2 (Encoder Mode) để giảm nhiễu và cải thiện độ ổn định của encoder 1 kênh.

---

## 🔧 Thay đổi trong `Core/Src/main.c`

### **Trước khi sửa:**
```c
sConfig.EncoderMode = TIM_ENCODERMODE_TI1;
sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
sConfig.IC1Filter = 5;
sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
sConfig.IC2Filter = 0;  // ❌ KHÔNG CÓ FILTER → NHIỄU!
```

### **Sau khi sửa:**
```c
sConfig.EncoderMode = TIM_ENCODERMODE_TI1;

// Channel 1: Encoder signal input
sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
sConfig.IC1Filter = 10;  // ✅ Tăng từ 5→10 (chống nhiễu tốt hơn)

// Channel 2: Direction detection (NOISE SOURCE!)
sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
sConfig.IC2Filter = 15;  // ✅ MAX filter để chặn noise trên CH2!
```

---

## 🎯 Lý do thay đổi

### ⚠️ **Vấn đề gốc:**

1. **Encoder chỉ có 1 kênh** (chỉ có Channel A)
2. **TIM_ENCODERMODE_TI1** vẫn sử dụng CH2 để xác định chiều đếm
3. **CH2 không có tín hiệu thật** → đọc noise từ môi trường
4. **IC2Filter = 0** → không lọc nhiễu → counter nhảy lung tung (↑↓↑↓)

### ✅ **Giải pháp áp dụng:**

#### **1. Hardware Level (Timer Configuration):**
- **IC1Filter = 10**: Tăng filter cho kênh encoder thực (từ 5→10)
- **IC2Filter = 15**: MAX filter để chặn hoàn toàn noise trên CH2

#### **2. Software Level (Code đã có sẵn):**
- Treat counter as ABSOLUTE position (luôn tăng)
- Sử dụng `Motor_Direction` để xác định chiều thực tế
- Noise rejection trong `Encoder_Read()` (dòng 202-216)
- Low-pass filter trong `Encoder_MeasureLength()` (dòng 514-515)

---

## 📊 Bảng giá trị IC Filter

| IC Filter | Sampling Frequency | Latency | Use Case |
|-----------|-------------------|---------|----------|
| 0 | No filter | 0 | ❌ Không khuyến nghị (nhiễu) |
| 1-5 | fSAMPLING=fCK_INT, N=2-8 | Thấp | Encoder tốc độ cao |
| 6-10 | fSAMPLING=fCK_INT/4, N=6-8 | Trung bình | ✅ Encoder trung bình (KHUYẾN NGHỊ) |
| 11-15 | fSAMPLING=fCK_INT/8, N=6-8 | Cao | ✅ Chặn noise cực mạnh |

**Lưu ý:** 
- Filter càng cao → độ trễ càng lớn → tốc độ encoder tối đa càng thấp
- Với encoder 8 PPR tốc độ thấp → IC Filter cao (10-15) là hợp lý

---

## 🧪 Kết quả mong đợi

### **Trước khi sửa:**
```
Counter: 100 → 105 → 103 → 108 → 106 → 110  (nhảy lung tung)
Direction: ↑ → ↓ → ↑ → ↓ → ↑  (đảo chiều liên tục)
```

### **Sau khi sửa:**
```
Counter: 100 → 105 → 110 → 115 → 120 → 125  (tăng đều)
Direction: ↑ → ↑ → ↑ → ↑ → ↑  (ổn định)
```

---

## 📝 Các file đã chỉnh sửa

1. **`Core/Src/main.c`** (dòng 409-440)
   - Tăng IC1Filter: 5 → 10
   - Tăng IC2Filter: 0 → 15
   - Thêm comment giải thích chi tiết

2. **`Core/Src/Encoder.c`** (dòng 10-19)
   - Cập nhật comment phản ánh giải pháp đã áp dụng

---

## ✅ Checklist

- [x] Tăng IC1Filter lên 10 (chống nhiễu kênh encoder)
- [x] Tăng IC2Filter lên 15 (chặn noise kênh direction)
- [x] Thống nhất polarity (cả 2 kênh dùng RISING)
- [x] Thêm comment giải thích chi tiết
- [x] Cập nhật documentation

---

## 🔍 Cách kiểm tra

### **1. Kiểm tra counter ổn định:**
```c
// Trong hàm Encoder_Read()
uint16_t raw_count = __HAL_TIM_GET_COUNTER(&htim2);
// → Giá trị phải tăng đều, không nhảy lung tung
```

### **2. Kiểm tra noise rejection:**
```c
uint32_t noise_count = Encoder_GetNoiseRejectCount();
// → Giá trị phải GIẢM đáng kể so với trước
```

### **3. Kiểm tra độ dài đo được:**
```c
uint16_t length_mm = Encoder_MeasureLength(&encoder1);
// → Giá trị phải mượt mà, không nhảy cóc
```

---

## 🎓 Kiến thức bổ sung

### **Input Capture Filter hoạt động như thế nào?**

IC Filter sử dụng bộ lọc số (digital filter) để loại bỏ nhiễu:

```
Filter = N → Cần N mẫu liên tiếp giống nhau mới chấp nhận thay đổi
```

**Ví dụ với IC2Filter = 15:**
```
Tín hiệu CH2: ___↑↓↑↓↑___  (noise spikes)
Sau filter:   ____________  (bị loại bỏ hoàn toàn)
```

**Công thức:**
```
t_filter = N × (1 / f_sampling)
```

Với STM32F1 @ 72MHz, TIM2 prescaler = 0:
- IC2Filter = 15 → t_filter ≈ 1-2 µs
- Encoder 8 PPR @ 100 RPM → T_pulse ≈ 75 ms
- → Filter delay không đáng kể!

---

## 📚 Tài liệu tham khảo

- **STM32F1 Reference Manual**: Section 15.3.12 (Input Capture)
- **HAL Driver**: `stm32f1xx_hal_tim.c` - HAL_TIM_Encoder_Init()
- **Application Note AN4013**: STM32 Timer Cookbook

---

## 💡 Lưu ý quan trọng

1. **Không cần thay đổi code logic** - chỉ sửa cấu hình hardware
2. **Giữ nguyên code xử lý noise** trong `Encoder_Read()` - vẫn cần làm defense layer
3. **Nếu vẫn còn nhiễu**: Kiểm tra hardware (dây nối, nguồn, заземление)
4. **Nếu encoder quá chậm**: Giảm IC1Filter xuống 5-7

---

**Tác giả:** AI Assistant  
**Ngày:** 2025-11-21  
**Phiên bản:** 1.0

