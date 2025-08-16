# Debug Guide - System Not Running

## Vấn đề hiện tại:
Chương trình không chạy sau khi thêm UART communication

## Các bước debug:

### 1. Kiểm tra cơ bản
- **Compile errors**: Build project và kiểm tra lỗi
- **Hardware connections**: Kiểm tra nguồn điện, crystal
- **LED indicators**: Xem có LED nào sáng không

### 2. Test phiên bản đơn giản
Đã tạo phiên bản minimal để test:
- Loại bỏ UART interrupt
- Chỉ có FreeRTOS task đơn giản
- LED toggle và UART output cơ bản

### 3. Expected Output
Nếu hệ thống chạy, bạn sẽ thấy:
```
System Starting...
FreeRTOS Task Started!
FreeRTOS Running - Count: 1
FreeRTOS Running - Count: 2
FreeRTOS Running - Count: 3
...
```

### 4. Nếu vẫn không chạy:

#### Kiểm tra 1: Clock Configuration
```c
// Trong SystemClock_Config()
// Đảm bảo HSE được enable
RCC_OscInitStruct.HSEState = RCC_HSE_ON;
```

#### Kiểm tra 2: FreeRTOS Config
- Kiểm tra FreeRTOSConfig.h
- Đảm bảo configUSE_PREEMPTION = 1
- Đảm bảo configUSE_TICKLESS_IDLE = 0

#### Kiểm tra 3: Stack Overflow
- Tăng stack size:
```c
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 512 * 4,  // Tăng lên 512*4
  .priority = (osPriority_t) osPriorityNormal,
};
```

#### Kiểm tra 4: Interrupt Priority
- Đảm bảo UART interrupt priority không conflict
- Kiểm tra NVIC configuration

### 5. Debug Steps:

#### Step 1: Test without UART
```c
void StartDefaultTask(void *argument)
{
  for(;;)
  {
    HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
    osDelay(1000);
  }
}
```

#### Step 2: Test UART only
```c
void StartDefaultTask(void *argument)
{
  uartSendString("Test UART\r\n");
  for(;;)
  {
    osDelay(1000);
  }
}
```

#### Step 3: Test FreeRTOS only
```c
void StartDefaultTask(void *argument)
{
  uint32_t counter = 0;
  for(;;)
  {
    counter++;
    HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
    osDelay(1000);
  }
}
```

### 6. Common Issues:

1. **Stack overflow**: Tăng stack size
2. **Clock not configured**: Kiểm tra SystemClock_Config()
3. **Interrupt conflict**: Kiểm tra interrupt priorities
4. **Memory issues**: Kiểm tra heap size
5. **UART configuration**: Kiểm tra baud rate, pins

### 7. Recovery Steps:

1. **Reset to basic**: Chỉ có LED toggle
2. **Add UART gradually**: Thêm UART sau khi LED hoạt động
3. **Test each component**: Test từng phần riêng biệt
4. **Check hardware**: Kiểm tra kết nối phần cứng

### 8. Expected Behavior:
- **LED1 (PC15)**: Nhấp nháy mỗi 1 giây
- **UART**: Gửi message mỗi 1 giây
- **System**: Ổn định, không crash

Nếu vẫn có vấn đề, hãy kiểm tra:
1. Power supply voltage
2. Crystal oscillator frequency
3. Reset pin state
4. Boot pin configuration
