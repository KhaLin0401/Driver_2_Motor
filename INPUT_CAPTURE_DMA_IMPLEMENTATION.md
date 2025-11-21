# Input Capture với DMA - Triển khai đếm xung Encoder

## Tổng quan

Dự án đã được chuyển đổi từ **Encoder Mode** sang **Input Capture Mode với DMA** để đếm xung từ encoder quang học 1 kênh.

## Lý do thay đổi

### Phương pháp cũ (Encoder Mode)
- ❌ Encoder Mode yêu cầu 2 kênh (A và B) để xác định hướng
- ❌ Với encoder 1 kênh, kênh B nhận noise → counter nhảy lung tung
- ❌ Cần polling liên tục để đọc counter
- ❌ Có thể mất xung khi CPU bận xử lý task khác

### Phương pháp mới (Input Capture + DMA)
- ✅ Chỉ cần 1 kênh encoder (phù hợp với hardware)
- ✅ DMA tự động đếm xung, không cần CPU
- ✅ Không bị mất xung khi CPU bận
- ✅ Chính xác cao (hardware capture timestamp)
- ✅ Giảm tải CPU đáng kể

## Cấu hình Hardware

### TIM2 - Input Capture Configuration
```
Timer: TIM2
Channel: CH1 (PA0)
Mode: Input Capture
Polarity: Falling Edge
Prescaler: 0 (không chia tần)
Counter Period: 65535 (16-bit counter)
Filter: 0 (có thể tăng nếu cần lọc noise)
```

### DMA Configuration
```
DMA: DMA1 Channel 5
Direction: Peripheral to Memory (TIM2_CCR1 → Buffer)
Mode: Circular (tự động quay vòng)
Buffer Size: 100 phần tử
Data Width: 32-bit (WORD)
Priority: Low
```

## Nguyên lý hoạt động

### 1. Hardware Capture
```
Encoder Pulse (Falling Edge)
    ↓
TIM2_CH1 Input Capture
    ↓
CCR1 Register lưu giá trị counter
    ↓
DMA Request được kích hoạt
    ↓
DMA tự động copy CCR1 → Buffer[index]
    ↓
DMA Counter (CNDTR) giảm: 100 → 99 → 98 → ...
```

### 2. Software Counting
```c
// Đọc DMA counter (số phần tử còn lại trong buffer)
uint32_t current_dma_counter = __HAL_DMA_GET_COUNTER(htim2.hdma[TIM_DMA_ID_CC1]);

// Tính số xung mới
if(current_dma_counter <= last_dma_counter) {
    // Trường hợp bình thường: DMA counter giảm
    new_pulses = last_dma_counter - current_dma_counter;
}
else {
    // DMA wrapped: 0 → 100
    new_pulses = last_dma_counter + (DMA_BUFFER_SIZE - current_dma_counter);
}

// Cập nhật tổng số xung
pulse_count += new_pulses;
```

### 3. Direction Handling
Vì encoder chỉ có 1 kênh, hướng quay được xác định từ motor:
```c
extern MotorRegisterMap_t motor1;

if (motor1.Direction == FORWARD) {
    direction_sign = +1;  // Dây đang được thả ra
}
else if (motor1.Direction == REVERSE) {
    direction_sign = -1;  // Dây đang được cuốn lại
}
```

## Thay đổi trong Code

### 1. Encoder.c - Cấu trúc dữ liệu

**Cũ:**
```c
typedef struct {
    uint16_t last_hardware_count;
    uint16_t stable_count;
    // ...
} EncoderState_t;
```

**Mới:**
```c
#define DMA_BUFFER_SIZE 100
static uint32_t dma_capture_buffer[DMA_BUFFER_SIZE];

typedef struct {
    uint32_t pulse_count;           // Tổng số xung từ DMA
    uint32_t last_dma_counter;      // DMA counter lần trước
    uint32_t dma_half_complete;     // Diagnostic
    uint32_t dma_full_complete;     // Diagnostic
    // ...
} EncoderState_t;
```

### 2. Encoder_Init() - Khởi tạo

**Cũ:**
```c
void Encoder_Init(void) {
    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_1);
    __HAL_TIM_SET_COUNTER(&htim2, 0);
}
```

**Mới:**
```c
void Encoder_Init(void) {
    // Initialize DMA buffer
    for(int i = 0; i < DMA_BUFFER_SIZE; i++){
        dma_capture_buffer[i] = 0;
    }
    
    // Start Input Capture with DMA (Circular mode)
    HAL_TIM_IC_Start_DMA(&htim2, TIM_CHANNEL_1, 
                         dma_capture_buffer, DMA_BUFFER_SIZE);
    
    encoder_state.last_dma_counter = DMA_BUFFER_SIZE;
}
```

### 3. Encoder_Read() - Đọc xung

**Cũ:**
```c
void Encoder_Read(Encoder_t* encoder) {
    uint16_t raw_count = __HAL_TIM_GET_COUNTER(&htim2);
    // Xử lý delta, noise rejection...
    encoder->Encoder_Count = raw_count;
}
```

**Mới:**
```c
void Encoder_Read(Encoder_t* encoder) {
    // Đọc DMA counter
    uint32_t current_dma_counter = 
        __HAL_DMA_GET_COUNTER(htim2.hdma[TIM_DMA_ID_CC1]);
    
    // Tính số xung mới
    uint32_t new_pulses = 0;
    if(current_dma_counter <= encoder_state.last_dma_counter) {
        new_pulses = encoder_state.last_dma_counter - current_dma_counter;
    }
    else {
        // DMA wrapped
        new_pulses = encoder_state.last_dma_counter + 
                    (DMA_BUFFER_SIZE - current_dma_counter);
    }
    
    // Cập nhật
    encoder_state.pulse_count += new_pulses;
    encoder_state.last_dma_counter = current_dma_counter;
    encoder->Encoder_Count = (uint16_t)(encoder_state.pulse_count % AUTO_RESET_THRESHOLD);
}
```

### 4. DMA Callbacks (Optional)

```c
void HAL_TIM_IC_CaptureHalfCpltCallback(TIM_HandleTypeDef *htim) {
    if(htim->Instance == TIM2) {
        encoder_state.dma_half_complete++;
        // Buffer nửa đầu đã đầy, có thể xử lý nếu cần
    }
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
    if(htim->Instance == TIM2) {
        encoder_state.dma_full_complete++;
        // Buffer đã đầy, DMA tự động quay về đầu
    }
}
```

### 5. stm32f1xx_hal_msp.c - DMA Configuration

**Thay đổi quan trọng:**
```c
hdma_tim2_ch1.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;  // 32-bit
hdma_tim2_ch1.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;      // 32-bit
hdma_tim2_ch1.Init.Mode = DMA_CIRCULAR;  // ✅ CRITICAL: Circular mode
```

## Diagnostic Functions

Các hàm mới để debug và giám sát:

```c
uint32_t Encoder_GetPulseCount(void);        // Tổng số xung hiện tại
uint32_t Encoder_GetDMAHalfComplete(void);   // Số lần DMA half-complete
uint32_t Encoder_GetDMAFullComplete(void);   // Số lần DMA full-complete
uint32_t Encoder_GetNoiseRejectCount(void);  // Số lần reject noise
uint32_t Encoder_GetOverflowCount(void);     // Số lần DMA wrap
void Encoder_ResetDiagnostics(void);         // Reset counters
```

## Ưu điểm của giải pháp

### 1. Không mất xung
- DMA hoạt động độc lập với CPU
- Ngay cả khi CPU bận với RTOS tasks, DMA vẫn capture được mọi xung

### 2. Giảm tải CPU
- Không cần polling liên tục
- CPU chỉ đọc khi cần (10ms/lần trong EncoderTask)

### 3. Chính xác cao
- Hardware capture timestamp chính xác đến từng clock cycle
- Không bị ảnh hưởng bởi interrupt latency

### 4. Dễ debug
- Có thể xem buffer DMA để phân tích timing
- Các counter diagnostic giúp phát hiện vấn đề

## Lưu ý khi sử dụng

### 1. Buffer Size
```c
#define DMA_BUFFER_SIZE 100
```
- Phải đủ lớn để chứa số xung giữa 2 lần đọc
- Với EncoderTask chạy 10ms/lần:
  - Tốc độ tối đa: 10,000 xung/giây = 100 xung/10ms
  - Buffer 100 phần tử là đủ

### 2. Noise Filtering
```c
#define NOISE_THRESHOLD_TICKS   2
#define MAX_DELTA_PER_CYCLE     200
```
- Vẫn giữ noise filtering trong software
- Reject các thay đổi quá nhỏ hoặc quá lớn

### 3. Direction từ Motor
- Encoder 1 kênh không biết hướng quay
- Phải dựa vào `motor1.Direction` để xác định
- Đảm bảo motor direction được cập nhật đúng

### 4. DMA Circular Mode
- **QUAN TRỌNG**: DMA phải ở mode CIRCULAR
- Nếu để NORMAL, DMA sẽ dừng sau khi đầy buffer

## Testing & Validation

### 1. Kiểm tra DMA hoạt động
```c
// Trong debug, kiểm tra:
uint32_t dma_counter = __HAL_DMA_GET_COUNTER(htim2.hdma[TIM_DMA_ID_CC1]);
// Counter phải giảm khi có xung: 100 → 99 → 98 → ...
```

### 2. Kiểm tra pulse count
```c
uint32_t pulses = Encoder_GetPulseCount();
// Phải tăng khi motor quay
```

### 3. Kiểm tra callbacks
```c
uint32_t half = Encoder_GetDMAHalfComplete();
uint32_t full = Encoder_GetDMAFullComplete();
// Tăng khi buffer đầy (ở tốc độ cao)
```

## Kết luận

Việc chuyển đổi từ Encoder Mode sang Input Capture + DMA mang lại:
- ✅ Độ chính xác cao hơn
- ✅ Không mất xung
- ✅ Giảm tải CPU
- ✅ Phù hợp với encoder 1 kênh

Cấu trúc dữ liệu và API không thay đổi, chỉ thay đổi cách đếm xung bên trong.

