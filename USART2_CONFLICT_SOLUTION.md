# Giải pháp cho lỗi USART2 Pin Conflict

## 🔍 **Vấn đề:**
Trình biên dịch báo lỗi "Partly disabled conflict" cho USART2 vì các chân PA0, PA1, PA4 đã được mapping cho chức năng khác.

## 🔧 **Giải pháp:**

### **Bước 1: Kiểm tra cấu hình hiện tại**
- **PA0** = CUR_SENS (Analog Input)
- **PA1** = LED1 (GPIO Output)  
- **PA2** = USART2_TX (cần giữ nguyên)
- **PA3** = USART2_RX (cần giữ nguyên)
- **PA4** = DIR2 (GPIO Output)

### **Bước 2: Cách giải quyết**

#### **Phương án 1: Sử dụng STM32CubeIDE (Khuyến nghị)**
1. Mở file `DC-Driver_Project.ioc` trong STM32CubeIDE
2. Vào tab "Pinout & Configuration"
3. Kiểm tra USART2 đã được cấu hình đúng:
   - PA2 = USART2_TX
   - PA3 = USART2_RX
4. Nếu có conflict, click vào chân bị conflict và chọn "Reset_State"
5. Generate code lại từ .ioc file

#### **Phương án 2: Sửa code thủ công**
Thêm vào function `MX_GPIO_Init()` trong `main.c`:

```c
/*Configure USART2 pins */
GPIO_InitTypeDef GPIO_InitStruct = {0};

// Configure PA2 (USART2_TX) and PA3 (USART2_RX)
GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
GPIO_InitStruct.Pull = GPIO_NOPULL;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
```

#### **Phương án 3: Thay đổi pin mapping**
Nếu cần thiết, có thể thay đổi USART2 sang chân khác:
- **USART1**: PA9 (TX), PA10 (RX)
- **USART3**: PB10 (TX), PB11 (RX)

### **Bước 3: Kiểm tra sau khi sửa**
1. Compile lại project
2. Kiểm tra không còn lỗi conflict
3. Test USART2 communication

## 📋 **Các file cần kiểm tra:**
- `DC-Driver_Project.ioc` - Cấu hình pin
- `main.c` - GPIO initialization
- `main.h` - Pin definitions

## ⚠️ **Lưu ý quan trọng:**
- **PA2 và PA3** phải được giữ nguyên cho USART2
- **PA0, PA1, PA4** có thể được sử dụng cho chức năng khác
- Sau khi sửa, phải generate code lại từ .ioc file

## 🔄 **Quy trình khuyến nghị:**
1. Mở STM32CubeIDE
2. Mở file .ioc
3. Kiểm tra và sửa pin conflicts
4. Generate code
5. Compile và test
