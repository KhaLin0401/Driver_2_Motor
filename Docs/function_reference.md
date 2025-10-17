# 📘 Function Reference for Dual DC Motor Driver with Modbus RTU (STM32F103C8T6)

# 📘 Tài liệu tham khảo các hàm trong Driver điều khiển 2 động cơ DC (STM32F103C8T6)

Dựa trên code hiện tại trong dự án, dưới đây là các hàm chính được sử dụng:

## 🔧 Khởi tạo và cấu hình hệ thống

### `void SystemClock_Config(void)`
Cấu hình clock hệ thống cho STM32F103C8T6.

### `void MX_GPIO_Init(void)`
Khởi tạo các chân GPIO được sử dụng.

### `void MX_TIM1_Init(void)` 
Khởi tạo Timer 1 cho PWM điều khiển động cơ.

### `void MX_USART1_UART_Init(void)`
Khởi tạo UART1 cho giao tiếp Modbus.

## 🔄 Xử lý động cơ

### `void Motor_ProcessControl(Motor_t* motor)`
Xử lý logic điều khiển cho một động cơ, bao gồm:
- Đọc trạng thái từ thanh ghi Modbus
- Thực hiện điều khiển theo mode đã chọn
- Cập nhật trạng thái trở lại thanh ghi

### `void MotorRegisters_Load(Motor_t* motor, uint16_t baseAddr)`
Đọc dữ liệu từ thanh ghi Modbus vào cấu trúc điều khiển động cơ.

### `void MotorRegisters_Save(Motor_t* motor, uint16_t baseAddr)` 
Lưu trạng thái động cơ ra thanh ghi Modbus.

## 🎮 Điều khiển PID

### `void PID_Init(uint8_t motor_id, float kp, float ki, float kd)`
Khởi tạo bộ điều khiển PID cho động cơ với các thông số:
- DEFAULT_PID_KP: Hệ số tỉ lệ
- DEFAULT_PID_KI: Hệ số tích phân  
- DEFAULT_PID_KD: Hệ số vi phân

## 🧠 Các Task RTOS

### `void StartDefaultTask(void *argument)`
Task mặc định của hệ thống.

### `void StartMotorTask(void *argument)`
Task chính điều khiển động cơ:
- Chu kỳ thực hiện: 10ms
- Đọc dữ liệu từ Modbus
- Xử lý điều khiển cho 2 động cơ
- Cập nhật trạng thái

### `void StartVisibleTask(void *argument)`
Task hiển thị và giám sát hệ thống.

## ⚠️ Xử lý lỗi

### `void Error_Handler(void)`
Xử lý khi có lỗi xảy ra:
- Disable ngắt
- Dừng hệ thống

### `void assert_failed(uint8_t *file, uint32_t line)`
Xử lý khi có lỗi assert trong debug:
- Báo file và dòng code lỗi
- Chỉ hoạt động khi USE_FULL_ASSERT được enable