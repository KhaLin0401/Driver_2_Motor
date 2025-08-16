# FreeRTOS Test Guide

## Vấn đề đã sửa:
1. **ModbusTask syntax error** - Đã sửa lỗi cú pháp trong StartModbusTask
2. **HAL_Delay in FreeRTOS** - Thay thế HAL_Delay bằng osDelay trong BLINK command
3. **Interrupt callback logic** - Sửa logic xử lý interrupt để tránh race condition
4. **Heartbeat frequency** - Giảm từ 5 giây xuống 1 giây để dễ test

## Cách test FreeRTOS:

### 1. Kiểm tra LED Heartbeat
- **LED1 (PC15)** sẽ nhấp nháy mỗi 5 giây nếu FreeRTOS chạy
- Nếu LED không nhấp nháy → FreeRTOS không chạy

### 2. Kiểm tra UART Output
- Kết nối UART2 với USB-to-UART converter
- Mở terminal (PuTTY, Tera Term) với cấu hình: 9600 baud, 8N1
- Bạn sẽ thấy:
  ```
  System Starting...
  FreeRTOS UART2 System Ready!
  Type HELP for commands
  ```

### 3. Test Commands
Gửi các lệnh sau qua UART:
```
HELP<CR>
STATUS<CR>
LED1ON<CR>
LED1OFF<CR>
LED2ON<CR>
LED2OFF<CR>
LED3ON<CR>
LED3OFF<CR>
ALLON<CR>
ALLOFF<CR>
BLINK<CR>
```

## Nếu FreeRTOS vẫn không chạy:

### Kiểm tra 1: Compile Errors
- Build project và kiểm tra lỗi compile
- Đảm bảo tất cả FreeRTOS files được include

### Kiểm tra 2: Clock Configuration
- Kiểm tra SystemClock_Config() có đúng không
- Đảm bảo SysTick được cấu hình đúng

### Kiểm tra 3: FreeRTOS Config
- Kiểm tra FreeRTOSConfig.h có đúng cấu hình không
- Đảm bảo configUSE_PREEMPTION = 1

### Kiểm tra 4: Stack Size
- Tăng stack size nếu cần:
  ```c
  const osThreadAttr_t defaultTask_attributes = {
    .name = "defaultTask",
    .stack_size = 256 * 4,  // Tăng từ 128*4 lên 256*4
    .priority = (osPriority_t) osPriorityNormal,
  };
  ```

## Debug Steps:
1. **Thêm debug LED** trong main() trước osKernelStart():
   ```c
   HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET);
   osKernelStart();
   ```

2. **Kiểm tra interrupt priority**:
   - Đảm bảo UART interrupt priority không conflict với FreeRTOS

3. **Test minimal FreeRTOS**:
   - Tạo task đơn giản chỉ toggle LED
   - Loại bỏ UART code tạm thời

## Expected Behavior:
- **LED1**: Nhấp nháy mỗi 1 giây (heartbeat)
- **UART**: Gửi heartbeat message mỗi 1 giây
- **Commands**: Phản hồi ngay lập tức khi gửi lệnh
- **System**: Ổn định, không crash

Nếu vẫn có vấn đề, hãy kiểm tra:
1. Hardware connections
2. Power supply
3. Crystal oscillator
4. Debug output qua UART
