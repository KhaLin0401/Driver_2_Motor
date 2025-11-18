// #include "DInput.h"
// #include "stdint.h"
// #include "main.h"

// uint8_t current_value;

// uint16_t Read_ADC_Value(void) {
//     HAL_ADC_Start(&hadc1);
//     HAL_ADC_PollForConversion(&hadc1, 10);
//     uint16_t val = HAL_ADC_GetValue(&hadc1);
//     HAL_ADC_Stop(&hadc1);
//     return val;
// }

// // Hàm đọc dòng điện từ ACS712 (đơn vị A, scale 100)
// uint16_t Read_ACS712(void) {
//     float average = 0;
    
//     // Lấy 1000 mẫu để tính trung bình
//     for(int i = 0; i < 100; i++) {
//         uint16_t adcVal = Read_ADC_Value();
        
//         // Công thức từ nhà sản xuất cho ACS712-5A
//         average += (0.0264f * adcVal - 13.51f) / 1000.0f;
        
//         // Delay 1ms giữa các lần đọc
//         osDelay(1);
//     }
    
//     // Scale 100 và chuyển sang uint16
//     uint16_t current_scaled = (uint16_t)(average * 100.0f);
//     g_holdingRegisters[REG_CURRENT] = current_scaled;
    
//     return current_scaled;
// }
