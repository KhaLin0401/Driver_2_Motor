# UART Debug Guide - MCU không nhận được dữ liệu

## Vấn đề hiện tại:
- Task counter hoạt động bình thường
- FreeRTOS chạy ổn định
- LED3 nhấp nháy mỗi 5 giây
- **NHƯNG** MCU không nhận được dữ liệu UART

## Debug Steps:

### 1. Kiểm tra UART Transmission
Gửi command `TEST` để kiểm tra UART transmission:
```
TEST<CR>
```
**Expected Output:**
```
UART Test - Sending test message
If you see this, UART transmission works
If LED4 blinks when you send data, interrupt works
```

### 2. Kiểm tra UART Reception
Gửi bất kỳ ký tự nào và quan sát:
- **LED4** có nhấp nháy không?
- Có thấy `Received: [ký tự]` không?

### 3. Kiểm tra Hardware Connections
- **UART2 TX** (PA2) → USB-to-UART RX
- **UART2 RX** (PA3) → USB-to-UART TX
- **GND** → GND
- **3.3V** → 3.3V (nếu cần)

### 4. Kiểm tra Terminal Settings
- **Baud Rate**: 9600
- **Data Bits**: 8
- **Stop Bits**: 1
- **Parity**: None
- **Flow Control**: None
- **Line Ending**: CR (\r) hoặc CR+LF (\r\n)

### 5. Kiểm tra CubeMX Configuration
Mở file `.ioc` và kiểm tra:
- **USART2** → **Mode**: Asynchronous
- **USART2** → **NVIC Settings** → **USART2 global interrupt**: Enabled
- **USART2** → **GPIO Settings**:
  - PA2: USART2_TX
  - PA3: USART2_RX

### 6. Test đơn giản
Gửi từng ký tự một:
```
H<CR>
E<CR>
L<CR>
P<CR>
```

### 7. Nếu LED4 không nhấp nháy:
**Vấn đề có thể là:**
1. **UART interrupt không được enable**
2. **NVIC priority conflict**
3. **UART pins không đúng**
4. **Hardware connection sai**

### 8. Kiểm tra Interrupt Priority
Trong CubeMX:
- **NVIC Settings** → **USART2 global interrupt priority**: Thấp hơn FreeRTOS
- **FreeRTOS** → **Config parameters** → **NVIC_SYSTICK_PREEMPTION_PRIORITY**: Cao nhất

### 9. Test với minimal code
Nếu vẫn không hoạt động, thử:
1. **Disable FreeRTOS** tạm thời
2. **Test UART trong main loop**
3. **Kiểm tra interrupt priority**

### 10. Common Issues:
1. **Wrong baud rate**: Đảm bảo 9600 baud
2. **Wrong pins**: Kiểm tra PA2/PA3
3. **Interrupt conflict**: Kiểm tra priority
4. **Hardware issue**: Kiểm tra kết nối
5. **Terminal settings**: Kiểm tra cấu hình

## Expected Behavior:
- **LED2**: Sáng 100ms khi khởi động
- **LED3**: Nhấp nháy mỗi 5 giây (heartbeat)
- **LED4**: Nhấp nháy mỗi khi có dữ liệu UART nhận được
- **UART**: Phản hồi ngay lập tức cho mọi command

## Debug Commands:
```
HELP<CR>          - Xem danh sách lệnh
TEST<CR>          - Test UART transmission
COUNTER<CR>       - Hiển thị task counter
STATUS<CR>        - Trạng thái hệ thống
```

Nếu vẫn có vấn đề, hãy kiểm tra:
1. Hardware connections
2. Terminal settings
3. CubeMX configuration
4. Interrupt priorities
