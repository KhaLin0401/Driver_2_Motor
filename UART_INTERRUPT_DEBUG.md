# UART Interrupt Debug Guide

## Vấn đề hiện tại:
- Task counter hoạt động bình thường
- FreeRTOS chạy ổn định  
- LED3 nhấp nháy mỗi 5 giây
- **NHƯNG** LED4 không nhấp nháy khi gửi dữ liệu UART
- **Nghĩa là** UART interrupt không được gọi

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

### 3. Kiểm tra Interrupt Priority Conflict
Vấn đề có thể là **interrupt priority conflict**:

**Trong CubeMX (.ioc file):**
- **USART2_IRQn priority**: 5 (hiện tại)
- **SysTick_IRQn priority**: 15 (FreeRTOS)
- **PendSV_IRQn priority**: 15 (FreeRTOS)

**Vấn đề:** USART2 priority (5) cao hơn FreeRTOS priority (15)
**Giải pháp:** Giảm USART2 priority xuống thấp hơn FreeRTOS

### 4. Sửa Interrupt Priority
Trong CubeMX:
1. Mở file `.ioc`
2. Vào **NVIC Settings**
3. Tìm **USART2 global interrupt**
4. Thay đổi **Preemption Priority** từ 5 thành 10 hoặc cao hơn
5. **Generate Code**

### 5. Kiểm tra Hardware Connections
- **UART2 TX** (PA2) → USB-to-UART RX
- **UART2 RX** (PA3) → USB-to-UART TX  
- **GND** → GND
- **3.3V** → 3.3V (nếu cần)

### 6. Kiểm tra Terminal Settings
- **Baud Rate**: 9600
- **Data Bits**: 8
- **Stop Bits**: 1
- **Parity**: None
- **Flow Control**: None
- **Line Ending**: CR (\r) hoặc CR+LF (\r\n)

### 7. Test đơn giản
Gửi từng ký tự một:
```
H<CR>
E<CR>
L<CR>
P<CR>
```

### 8. Nếu vẫn không hoạt động:

#### Kiểm tra 1: Disable FreeRTOS tạm thời
Thử test UART trong main loop:
```c
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  
  uartSendString("UART Test without FreeRTOS\r\n");
  HAL_UART_Receive_IT(&huart2, &uartRxBuffer[rxIndex], 1);
  
  while(1)
  {
    if (uartDataReceived)
    {
      uartDataReceived = 0;
      HAL_GPIO_TogglePin(LED4_GPIO_Port, LED4_Pin);
      uartSendString("Received!\r\n");
      HAL_UART_Receive_IT(&huart2, &uartRxBuffer[rxIndex], 1);
    }
  }
}
```

#### Kiểm tra 2: Interrupt Priority
Đảm bảo USART2 priority thấp hơn FreeRTOS:
- **USART2**: Priority 10-15
- **FreeRTOS**: Priority 15

#### Kiểm tra 3: NVIC Configuration
Trong CubeMX:
- **NVIC Priority Group**: NVIC_PRIORITYGROUP_4
- **USART2 global interrupt**: Enabled
- **USART2 global interrupt priority**: 10-15

### 9. Common Issues:
1. **Interrupt priority conflict**: USART2 priority cao hơn FreeRTOS
2. **Wrong pins**: Kiểm tra PA2/PA3
3. **Hardware connection**: Kiểm tra kết nối
4. **Terminal settings**: Kiểm tra baud rate
5. **NVIC configuration**: Kiểm tra interrupt enable

### 10. Expected Behavior:
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

## Next Steps:
1. **Kiểm tra interrupt priority** trong CubeMX
2. **Giảm USART2 priority** xuống 10-15
3. **Regenerate code** và test lại
4. **Nếu vẫn không hoạt động**, test không có FreeRTOS

Nếu vẫn có vấn đề, hãy kiểm tra:
1. Hardware connections
2. Terminal settings  
3. CubeMX configuration
4. Interrupt priorities
