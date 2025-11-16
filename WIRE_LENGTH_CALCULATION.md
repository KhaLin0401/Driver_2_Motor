# Hướng dẫn tính toán độ dài dây đai từ Encoder với bán kính biến thiên

## 1. Mô tả bài toán

### Cấu tạo cơ khí:
- **Trục quay**: Được điều khiển bởi motor
- **Đĩa encoder**: Gắn cố định trên trục, bán kính không đổi
- **Vòng cuộn dây**: Gắn đồng trục với đĩa encoder
- **Dây đai**: Được cuộn/xả tùy theo chiều quay của motor

### Vấn đề:
- Encoder chỉ đo được góc quay của trục
- Bán kính đĩa encoder cố định (không đổi)
- **Bán kính vòng dây thay đổi** khi dây được cuộn/xả
  - Khi dây đầy: bán kính lớn (R_max = 35mm)
  - Khi dây hết: bán kính nhỏ (R_min = 10mm - chỉ còn lõi)

### Mục tiêu:
Tính toán độ dài dây đã xả ra dựa trên số pulse từ encoder

---

## 2. Phương pháp giải quyết

### 2.1. Mô hình toán học

#### Công thức cơ bản:
```
Độ dài dây = Chu vi × Số vòng quay
L = 2πR × N
```

Nhưng vấn đề là **R thay đổi** theo thời gian!

#### Mô hình bán kính biến thiên (Linear Model):
```
R(L) = R_max - (R_max - R_min) × (L / L_total)
```

Trong đó:
- `R(L)`: Bán kính tại độ dài dây đã xả là L
- `R_max`: Bán kính khi dây đầy (35mm)
- `R_min`: Bán kính lõi khi dây hết (10mm)
- `L`: Độ dài dây đã xả ra
- `L_total`: Tổng độ dài dây (3000mm)

**Giải thích:**
- Khi L = 0 (dây đầy): R = R_max = 35mm
- Khi L = L_total (dây hết): R = R_min = 10mm
- Bán kính giảm tuyến tính theo độ dài dây xả ra

### 2.2. Thuật toán tính toán từng bước

```
Bước 1: Đọc số pulse từ encoder
    delta_pulse = pulse_hiện_tại - pulse_lần_trước

Bước 2: Tính số vòng quay
    delta_vòng = delta_pulse / PPR
    (PPR = 4 với encoder quadrature)

Bước 3: Tính độ dài dây thay đổi
    chu_vi_hiện_tại = 2π × R_hiện_tại
    delta_L = chu_vi_hiện_tại × delta_vòng

Bước 4: Cập nhật tổng độ dài
    L_total = L_total + delta_L

Bước 5: Cập nhật bán kính mới
    R_mới = R_max - (R_max - R_min) × (L_total / L_max)
```

### 2.3. Ví dụ minh họa

**Giả sử:**
- R_max = 35mm
- R_min = 10mm
- L_total = 3000mm
- PPR = 4 pulse/vòng

**Tình huống 1: Dây đầy (L = 0mm)**
- R = 35mm
- Chu vi = 2π × 35 = 219.9mm
- Motor quay 1 vòng (4 pulse) → Dây xả 219.9mm
- L_mới = 0 + 219.9 = 219.9mm
- R_mới = 35 - (35-10) × (219.9/3000) = 33.17mm

**Tình huống 2: Dây đã xả 1500mm (L = 1500mm)**
- R = 35 - (35-10) × (1500/3000) = 22.5mm
- Chu vi = 2π × 22.5 = 141.4mm
- Motor quay 1 vòng (4 pulse) → Dây xả 141.4mm
- L_mới = 1500 + 141.4 = 1641.4mm
- R_mới = 35 - (35-10) × (1641.4/3000) = 21.32mm

**Tình huống 3: Dây gần hết (L = 2900mm)**
- R = 35 - (35-10) × (2900/3000) = 10.83mm
- Chu vi = 2π × 10.83 = 68.0mm
- Motor quay 1 vòng (4 pulse) → Dây xả 68.0mm
- L_mới = 2900 + 68.0 = 2968mm
- R_mới = 35 - (35-10) × (2968/3000) = 10.27mm

---

## 3. Implementation trong code

### 3.1. Các tham số cấu hình

```c
#define ENCODER_PPR 4       // 4 pulses per revolution (quadrature)
#define WIRE_LENGTH_MM 3000 // Tổng độ dài dây (mm)
#define R_MAX 35.0f         // Bán kính khi dây đầy (mm)
#define R_MIN 10.0f         // Bán kính lõi (mm)
```

**Lưu ý:** Bạn cần đo đạc thực tế để có các giá trị chính xác:
- `R_MAX`: Đo bán kính khi dây cuộn đầy
- `R_MIN`: Đo bán kính lõi trống
- `WIRE_LENGTH_MM`: Đo tổng độ dài dây
- `ENCODER_PPR`: Kiểm tra datasheet của encoder

### 3.2. Hàm chính: `Encoder_MeasureLength()`

```c
uint16_t Encoder_MeasureLength(Encoder_t* encoder)
```

**Chức năng:** Tính toán và trả về độ dài dây đã xả ra (mm)

**Cách hoạt động:**
1. Tính số pulse thay đổi từ lần đọc trước
2. Chuyển đổi pulse → số vòng quay
3. Tính độ dài dây thay đổi = chu vi × số vòng
4. Cập nhật tổng độ dài dây
5. Cập nhật bán kính mới theo mô hình tuyến tính

**Sử dụng:**
```c
// Trong vòng lặp chính
uint16_t wire_length = Encoder_MeasureLength(&encoder1);
// wire_length chứa độ dài dây đã xả (mm)
```

### 3.3. Các hàm hỗ trợ

#### `Encoder_ResetWireLength()`
Reset về trạng thái ban đầu (dây đầy)
```c
Encoder_ResetWireLength(&encoder1);
```

#### `Encoder_SetWireLength()`
Đặt độ dài dây hiện tại (dùng cho calibration)
```c
Encoder_SetWireLength(&encoder1, 1500.0f); // Đặt là 1500mm
```

#### `Encoder_GetCurrentRadius()`
Lấy bán kính hiện tại của vòng dây
```c
float radius = Encoder_GetCurrentRadius();
```

---

## 4. Calibration và Fine-tuning

### 4.1. Kiểm tra độ chính xác

**Phương pháp:**
1. Đánh dấu vị trí ban đầu trên dây
2. Xả dây một khoảng cố định (ví dụ: 1000mm)
3. So sánh giá trị đo được từ `Encoder_MeasureLength()` với độ dài thực tế
4. Điều chỉnh các tham số nếu cần

### 4.2. Các tham số cần điều chỉnh

**Nếu đo được ít hơn thực tế:**
- Tăng `R_MAX` hoặc `R_MIN`
- Kiểm tra lại `ENCODER_PPR`

**Nếu đo được nhiều hơn thực tế:**
- Giảm `R_MAX` hoặc `R_MIN`
- Kiểm tra encoder có bị trượt pulse không

### 4.3. Cải thiện độ chính xác

**Mô hình bán kính chính xác hơn:**

Nếu mô hình tuyến tính không đủ chính xác, có thể dùng công thức chính xác hơn:

```c
// Mô hình dựa trên diện tích vòng tròn
// A = π(R² - r²) = L × t (với t là độ dày dây)
// R² = r² + (L × t) / π
// R = sqrt(r² + (L × t) / π)

float wire_thickness = 0.5f; // mm (đo thực tế)
float area_per_length = wire_length * wire_thickness;
current_radius = sqrtf(R_MIN * R_MIN + area_per_length / M_PI);
```

Tuy nhiên, mô hình tuyến tính đơn giản thường đủ chính xác cho hầu hết ứng dụng.

---

## 5. Lưu ý quan trọng

### 5.1. Xử lý overflow
- Encoder counter có thể overflow (vượt quá 65535)
- Code đã xử lý bằng cách dùng `int16_t` cho delta_count

### 5.2. Hướng quay
- Giá trị dương: dây xả ra (unroll)
- Giá trị âm: dây cuộn lại (roll)
- Code tự động xử lý cả 2 chiều

### 5.3. Giới hạn
- Độ dài dây luôn được giới hạn trong [0, WIRE_LENGTH_MM]
- Bán kính luôn được giới hạn trong [R_MIN, R_MAX]

### 5.4. Khởi tạo
- Mặc định giả sử dây đầy (L=0, R=R_MAX)
- Nếu khởi động ở trạng thái khác, dùng `Encoder_SetWireLength()`

---

## 6. Ví dụ sử dụng trong main.c

```c
#include "Encoder.h"

int main(void) {
    // Khởi tạo
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM2_Init();
    
    Encoder_Init();
    
    // Nếu dây không ở trạng thái đầy, đặt giá trị ban đầu
    // Encoder_SetWireLength(&encoder1, 500.0f); // Dây đã xả 500mm
    
    while (1) {
        // Đọc và xử lý encoder
        Encoder_Process(&encoder1);
        
        // Tính độ dài dây
        uint16_t wire_length = Encoder_MeasureLength(&encoder1);
        
        // Lấy bán kính hiện tại
        float current_radius = Encoder_GetCurrentRadius();
        
        // Hiển thị hoặc gửi qua Modbus
        printf("Wire length: %u mm, Radius: %.2f mm\n", 
               wire_length, current_radius);
        
        // Reset nếu cần
        if(/* điều kiện reset */) {
            Encoder_ResetWireLength(&encoder1);
        }
        
        HAL_Delay(10);
    }
}
```

---

## 7. Troubleshooting

### Vấn đề 1: Giá trị nhảy lung tung
**Nguyên nhân:** Nhiễu điện từ, encoder bị lỏng
**Giải pháp:** 
- Kiểm tra kết nối encoder
- Thêm tụ lọc nhiễu
- Kiểm tra nguồn cấp cho encoder

### Vấn đề 2: Độ dài không chính xác
**Nguyên nhân:** Tham số cấu hình sai
**Giải pháp:**
- Đo lại R_MAX, R_MIN thực tế
- Kiểm tra ENCODER_PPR
- Thực hiện calibration

### Vấn đề 3: Giá trị không tăng/giảm
**Nguyên nhân:** Encoder không hoạt động
**Giải pháp:**
- Kiểm tra timer encoder đã được khởi tạo chưa
- Kiểm tra kết nối A, B channel
- Kiểm tra nguồn encoder

---

## 8. Tóm tắt

**Ưu điểm của phương pháp:**
- ✅ Tính toán chính xác với bán kính biến thiên
- ✅ Xử lý được cả 2 chiều quay
- ✅ Tự động cập nhật bán kính theo thời gian
- ✅ Có các hàm hỗ trợ calibration

**Hạn chế:**
- ⚠️ Cần đo đạc chính xác các tham số ban đầu
- ⚠️ Mô hình tuyến tính có thể chưa hoàn hảo với một số loại dây
- ⚠️ Cần reset khi thay dây hoặc khởi động lại

**Kết luận:**
Phương pháp này cho phép tính toán độ dài dây đai chính xác dựa trên encoder, 
kể cả khi bán kính vòng dây thay đổi theo thời gian. Độ chính xác phụ thuộc 
vào việc đo đạc và cấu hình các tham số ban đầu.





