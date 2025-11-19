# 📐 HƯỚNG DẪN CALIBRATION ENCODER

## 🎯 MỤC ĐÍCH
Hướng dẫn calibration encoder để đo chính xác độ dài dây xả ra từ cuộn dây.

---

## ⚙️ BƯỚC 1: XÁC ĐỊNH THÔNG SỐ ENCODER

### 1.1. Kiểm tra Datasheet Encoder
Tìm thông số **PPR (Pulses Per Revolution)** của encoder:
- Encoder thông dụng: 100 PPR, 200 PPR, 400 PPR, 600 PPR
- Encoder chính xác cao: 1000 PPR, 2048 PPR, 2500 PPR

### 1.2. Cấu hình trong Code
Với **TIM_ENCODERMODE_TI12**, STM32 đếm **4 lần** mỗi chu kỳ encoder:
```c
// Ví dụ: Encoder 100 PPR
#define ENCODER_PPR 100
#define ENCODER_CPR (ENCODER_PPR * 4)  // = 400 counts/vòng
```

### 1.3. Kiểm tra bằng Test
1. Đặt encoder vào chế độ test
2. Quay tay 1 vòng chính xác (360°)
3. Đọc giá trị `REG_M1_ENCODER_COUNT`
4. Giá trị đọc được = `ENCODER_CPR`

**Ví dụ:**
- Quay 1 vòng → đọc được 400 counts → Encoder là 100 PPR ✅
- Quay 1 vòng → đọc được 800 counts → Encoder là 200 PPR ✅

---

## 📏 BƯỚC 2: ĐO KÍCH THƯỚC CUỘN DÂY

### 2.1. Đo Bán Kính Cuộn Đầy (R_MAX)
1. Cuộn đầy dây vào trục
2. Dùng thước kẹp (caliper) đo **đường kính ngoài** cuộn dây
3. Tính bán kính: `R_MAX = Đường_kính / 2`

**Ví dụ:**
- Đường kính cuộn đầy: 70mm → `R_MAX = 35.0mm` ✅

### 2.2. Đo Bán Kính Lõi Trống (R_MIN)
1. Tháo hết dây ra
2. Đo **đường kính lõi trống** (trục cuộn)
3. Tính bán kính: `R_MIN = Đường_kính / 2`

**Ví dụ:**
- Đường kính lõi: 20mm → `R_MIN = 10.0mm` ✅

### 2.3. Cập nhật vào Code
```c
#define R_MAX 35.0f  // mm - Thay bằng giá trị đo được
#define R_MIN 10.0f  // mm - Thay bằng giá trị đo được
```

---

## 🧪 BƯỚC 3: CALIBRATION THỰC TẾ

### 3.1. Chuẩn bị
- Đánh dấu vị trí bắt đầu trên dây (dùng băng dính màu)
- Chuẩn bị thước dây hoặc thước cuộn đo chiều dài

### 3.2. Quy trình Calibration

#### **Phương pháp 1: Calibration Toàn bộ**
1. Reset encoder về 0:
   ```
   Ghi REG_M1_ENCODER_RESET = 1
   ```

2. Xả toàn bộ dây ra với tốc độ chậm (20-30%)

3. Đo chiều dài dây thực tế bằng thước: `L_actual` (cm)

4. Đọc giá trị encoder đo được: `L_measured` (cm)

5. Tính hệ số hiệu chỉnh:
   ```
   Correction_Factor = L_actual / L_measured
   ```

6. Cập nhật vào code:
   ```c
   #define CORRECTION_FACTOR 1.05f  // Ví dụ: sai số +5%
   
   // Trong hàm Encoder_MeasureLength():
   wire_unrolled = wire_unrolled * CORRECTION_FACTOR;
   ```

#### **Phương pháp 2: Calibration Từng Đoạn**
1. Xả 50cm dây → đo thực tế → so sánh
2. Xả thêm 50cm → đo thực tế → so sánh
3. Lặp lại cho đến hết dây

4. Tạo bảng hiệu chỉnh:
   | Encoder (cm) | Thực tế (cm) | Sai số (%) |
   |--------------|--------------|------------|
   | 50           | 48           | -4%        |
   | 100          | 97           | -3%        |
   | 150          | 147          | -2%        |
   | 200          | 198          | -1%        |

5. Tính hệ số trung bình hoặc dùng interpolation

---

## 🔧 BƯỚC 4: TINH CHỈNH MÔ HÌNH BÁN KÍNH

### 4.1. Vấn đề Mô hình Tuyến tính
Mô hình hiện tại giả định bán kính giảm **tuyến tính**:
```c
R(L) = R_MAX - (R_MAX - R_MIN) × (L / L_total)
```

**Thực tế:** Bán kính thay đổi theo **thể tích dây**, không tuyến tính!

### 4.2. Mô hình Chính xác hơn (Tùy chọn)
Nếu cần độ chính xác cao (< 2%), sử dụng mô hình thể tích:

```c
// Thêm vào Encoder.c
#define WIRE_DIAMETER 0.5f  // mm - Đường kính dây

float Encoder_CalculateRadius(float length_mm) {
    // Diện tích mặt cắt dây
    float wire_cross_section = M_PI * (WIRE_DIAMETER / 2.0f) * (WIRE_DIAMETER / 2.0f);
    
    // Thể tích dây đã xả
    float volume_unrolled = length_mm * wire_cross_section;
    
    // Diện tích vòng tròn còn lại
    float remaining_area = M_PI * (R_MAX * R_MAX - R_MIN * R_MIN) - volume_unrolled / WIRE_DIAMETER;
    
    // Bán kính hiện tại
    float current_R = sqrtf(R_MIN * R_MIN + remaining_area / M_PI);
    
    return current_R;
}
```

---

## 📊 BƯỚC 5: KIỂM TRA ĐỘ CHÍNH XÁC

### 5.1. Test Độ Lặp lại (Repeatability)
1. Xả dây ra 100cm → đọc giá trị encoder
2. Cuộn lại về 0
3. Lặp lại 10 lần
4. Tính độ lệch chuẩn (Standard Deviation)

**Kết quả tốt:** SD < 1% (< 1cm trên 100cm)

### 5.2. Test Độ Chính xác (Accuracy)
1. Xả dây ra 50cm, 100cm, 150cm, 200cm, 250cm, 300cm
2. Đo thực tế bằng thước
3. So sánh với giá trị encoder

**Kết quả tốt:** Sai số < 3% trên toàn dải

### 5.3. Test Tốc độ cao
1. Xả dây với tốc độ 100% PWM
2. Kiểm tra có bị mất xung không
3. So sánh với xả ở tốc độ chậm

**Lưu ý:** Nếu sai số tăng ở tốc độ cao → tăng bộ lọc IC trong TIM2:
```c
sConfig.IC1Filter = 5;  // Tăng từ 0 lên 5
sConfig.IC2Filter = 5;
```

---

## 🐛 BƯỚC 6: XỬ LÝ SAI SỐ

### 6.1. Các Nguồn Sai số Thường gặp

| Nguyên nhân | Sai số | Giải pháp |
|-------------|--------|-----------|
| Encoder PPR sai | ±50-90% | Kiểm tra datasheet, test quay tay |
| R_MAX/R_MIN sai | ±10-20% | Đo lại bằng thước kẹp chính xác |
| Mô hình tuyến tính | ±5-10% | Dùng mô hình thể tích hoặc calibration |
| Nhiễu encoder | ±2-5% | Thêm filter, kiểm tra dây nối |
| Trượt dây | ±1-3% | Tăng lực kéo, kiểm tra ma sát |

### 6.2. Công thức Tổng hợp Sai số
```
Sai_số_tổng = √(ε₁² + ε₂² + ε₃² + ...)

Ví dụ:
- Encoder PPR: ±1%
- Bán kính: ±3%
- Mô hình: ±5%
→ Tổng: √(1² + 3² + 5²) = √35 ≈ ±5.9%
```

---

## ✅ CHECKLIST HOÀN THÀNH

- [ ] Xác định đúng ENCODER_PPR từ datasheet
- [ ] Test quay tay 1 vòng → đếm đúng ENCODER_CPR counts
- [ ] Đo R_MAX và R_MIN bằng thước kẹp
- [ ] Cập nhật giá trị vào code
- [ ] Chạy calibration toàn bộ dây
- [ ] Tính hệ số hiệu chỉnh
- [ ] Test độ lặp lại (10 lần)
- [ ] Test độ chính xác (6 điểm)
- [ ] Kiểm tra ở tốc độ cao
- [ ] Ghi lại kết quả vào tài liệu

---

## 📝 GHI CHÚ KẾT QUẢ CALIBRATION

```
Ngày calibration: _______________
Người thực hiện: _______________

THÔNG SỐ ĐO ĐƯỢC:
- Encoder PPR: _______ PPR
- R_MAX: _______ mm
- R_MIN: _______ mm
- Chiều dài dây thực tế: _______ cm
- Chiều dài encoder đo: _______ cm
- Hệ số hiệu chỉnh: _______

ĐỘ CHÍNH XÁC:
- Sai số trung bình: _______ %
- Độ lệch chuẩn: _______ cm
- Sai số tối đa: _______ %

GHI CHÚ:
_________________________________
_________________________________
_________________________________
```

---

## 🔗 THAM KHẢO

1. **STM32 Encoder Mode:**
   - TIM_ENCODERMODE_TI1: Đếm 1x (chỉ channel A)
   - TIM_ENCODERMODE_TI2: Đếm 1x (chỉ channel B)
   - TIM_ENCODERMODE_TI12: Đếm 4x (cả A và B, cả 2 cạnh) ✅

2. **Công thức Chu vi:**
   ```
   C = 2πR
   L = C × N (N = số vòng quay)
   ```

3. **Độ phân giải:**
   ```
   Resolution = 2πR / ENCODER_CPR
   
   Ví dụ: R=35mm, CPR=400
   → Resolution = 2π×35/400 = 0.55mm/count
   ```

---

**Chúc bạn calibration thành công! 🎉**

