#include "Encoder.h"
#include "ModbusMap.h"
#include "main.h"
#include "MotorControl.h"
#include <math.h>

#define ENCODER_PPR 4       // 4 pulses per revolution (quadrature encoding)
#define WIRE_LENGTH_MM 3000 // total wire length in mm
#define R_MAX 35.0f         // Maximum radius (full spool) in mm
#define R_MIN 10.0f         // Minimum radius (empty spool/core) in mm

// Static variables for wire length calculation
static float wire_unrolled = 0.0f;  // Current wire length unrolled in mm
static float current_radius = R_MAX; // Current effective radius in mm
static uint16_t last_encoder_count = 0; // Last encoder count for delta calculation

Encoder_t encoder1;
//Encoder_t encoder2;

void Encoder_Init(void){
    encoder1.Calib_Origin_Status = false;
    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_1 | TIM_CHANNEL_2);
    __HAL_TIM_SET_COUNTER(&htim2, 0);   // Reset counter về 0
    encoder1.Encoder_Count = 0; // Initialize encoder count
    encoder1.Encoder_Config = 100;
    encoder1.Encoder_Reset = 0;
    encoder1.Encoder_Calib_Sensor_Status = 0;
    encoder1.Encoder_Calib_Distance_MM = 100;
    encoder1.Encoder_Calib_Start = 0;
    encoder1.Encoder_Calib_Status = 0;
    encoder1.Encoder_Value = 0;
    
    // Initialize wire length tracking
    wire_unrolled = 0.0f;
    current_radius = R_MAX;
    last_encoder_count = 0;
}

void Encoder_Read(Encoder_t* encoder){
    if(encoder->Encoder_Reset == true){
        __HAL_TIM_SET_COUNTER(&htim2, 0);
        encoder->Encoder_Count = 0;
    }
    encoder->Encoder_Count = __HAL_TIM_GET_COUNTER(&htim2);
}

void Encoder_Write(Encoder_t* encoder, uint16_t value){
    encoder->Encoder_Value = value;
}

void Encoder_Reset(Encoder_t* encoder){
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    encoder->Encoder_Value = 0;
}
 void Encoder_Load(Encoder_t* encoder){
    encoder->Encoder_Count = g_holdingRegisters[REG_M1_ENCODER_COUNT];
    encoder->Encoder_Config = g_holdingRegisters[REG_M1_ENCODER_CONFIG];
    encoder->Encoder_Reset = g_holdingRegisters[REG_M1_ENCODER_RESET];
    encoder->Encoder_Calib_Sensor_Status = g_holdingRegisters[REG_M1_CALIB_SENSOR_STATUS];
    encoder->Encoder_Calib_Distance_MM = g_holdingRegisters[REG_M1_CALIB_DISTANCE_MM];
 }
void Encoder_Save(Encoder_t* encoder){
    g_holdingRegisters[REG_M1_ENCODER_COUNT] = encoder->Encoder_Count;
    g_holdingRegisters[REG_M1_ENCODER_CONFIG] = encoder->Encoder_Config;
    g_holdingRegisters[REG_M1_ENCODER_RESET] = encoder->Encoder_Reset;
    g_holdingRegisters[REG_M1_CALIB_SENSOR_STATUS] = encoder->Encoder_Calib_Sensor_Status;
    g_holdingRegisters[REG_M1_CALIB_DISTANCE_MM] = encoder->Encoder_Calib_Distance_MM;
}
void Encoder_Process(Encoder_t* encoder){
    
    Encoder_Check_Calib_Origin(encoder);

    Encoder_Read(encoder);  // Collect encoder count
    
    Encoder_MeasureLength(encoder);


    // Calculate distance in mm based on encoder count and configuration
    // Encoder_Config is mm per pulse × 1000 (e.g., 500 = 0.5mm/pulse)
    // Distance (mm) = (Encoder_Count × Encoder_Config) / 1000
    uint32_t distance_mm = ((uint32_t)encoder->Encoder_Count * encoder->Encoder_Config) / 1000;
    
    // Read calibration sensor status (assuming it's connected to a GPIO pin)
    // Example: encoder->Encoder_Calib_Sensor_Status = HAL_GPIO_ReadPin(CALIB_SENSOR_GPIO_Port, CALIB_SENSOR_Pin);
    
    // Auto-calibration logic
    static uint8_t calib_state = 0;  // 0=idle, 1=running, 2=done, 3=error
    static uint16_t calib_start_count = 0;
    static uint16_t calib_end_count = 0;
    static uint8_t first_sensor_detected = 0;
    
    // Handle calibration start command
    if(encoder->Encoder_Calib_Start == 1){
        calib_state = 1;  // Start calibration
        calib_start_count = 0;
        calib_end_count = 0;
        first_sensor_detected = 0;
        encoder->Encoder_Calib_Start = 0;  // Clear start flag
    }
    
    // Calibration process
    if(calib_state == 1){  // Running
        if(encoder->Encoder_Calib_Sensor_Status == 1){  // Magnet detected
            if(first_sensor_detected == 0){
                // First sensor detected - record start position
                calib_start_count = encoder->Encoder_Count;
                first_sensor_detected = 1;
            } else {
                // Second sensor detected - record end position
                calib_end_count = encoder->Encoder_Count;
                
                // Calculate pulses between sensors
                uint16_t pulses = calib_end_count - calib_start_count;
                
                if(pulses > 0 && encoder->Encoder_Calib_Distance_MM > 0){
                    // Calculate mm per pulse × 1000
                    // Encoder_Config = (Distance_MM × 10 × 1000) / pulses
                    encoder->Encoder_Config = (encoder->Encoder_Calib_Distance_MM * 1000) / pulses;
                    calib_state = 2;  // Done
                } else {
                    calib_state = 3;  // Error - invalid measurement
                }
            }
        }
    }
    
    // Update calibration status register
    encoder->Encoder_Calib_Status = calib_state;
}

/**
 * @brief Tính toán độ dài dây đã xả ra dựa trên encoder với bán kính biến thiên
 * 
 * Phương pháp:
 * 1. Đọc số pulse từ encoder để tính góc quay
 * 2. Tính độ dài dây xả = chu vi hiện tại × số vòng quay
 * 3. Cập nhật bán kính mới dựa trên mô hình tuyến tính
 * 
 * Mô hình bán kính: R(L) = R_max - (R_max - R_min) × (L / L_total)
 * - Khi dây đầy (L=0): R = R_max (35mm)
 * - Khi dây hết (L=L_total): R = R_min (10mm)
 * 
 * @param encoder: Con trỏ đến cấu trúc encoder
 * @return Độ dài dây đã xả ra tính bằng mm
 */
uint16_t Encoder_MeasureLength(Encoder_t* encoder){
    // Tính delta count (số pulse thay đổi từ lần đọc trước)
    // Sử dụng int16_t để xử lý trường hợp counter overflow
    int16_t delta_count = (int16_t)(encoder->Encoder_Count - last_encoder_count);
    last_encoder_count = encoder->Encoder_Count;
    
    // Chỉ tính toán khi có thay đổi
    if(delta_count != 0){
        // Bước 1: Tính số vòng quay từ số pulse
        // Với encoder quadrature: 4 pulse = 1 vòng quay
        float delta_revolutions = (float)delta_count / (float)ENCODER_PPR;
        
        // Bước 2: Tính chu vi hiện tại dựa trên bán kính hiện tại
        float current_circumference = 2.0f * 3.1415 * current_radius;
        
        // Bước 3: Tính độ dài dây thay đổi
        // delta_L = chu_vi × số_vòng_quay
        // Giá trị dương: dây xả ra (unroll)
        // Giá trị âm: dây cuộn lại (roll)
        float delta_length = current_circumference * delta_revolutions;
        
        // Bước 4: Cập nhật tổng độ dài dây đã xả
        wire_unrolled += delta_length;
        
        // Giới hạn trong khoảng hợp lệ [0, WIRE_LENGTH_MM]
        if(wire_unrolled < 0.0f) {
            wire_unrolled = 0.0f;
        }
        if(wire_unrolled > encoder->Encoder_Config) {
            wire_unrolled = encoder->Encoder_Config;
        }
        
        // Bước 5: Cập nhật bán kính dựa trên mô hình tuyến tính
        // R(L) = R_max - (R_max - R_min) × (L / L_total)
        // Khi xả dây: L tăng → R giảm (từ R_max xuống R_min)
        // Khi cuộn dây: L giảm → R tăng (từ R_min lên R_max)
        current_radius = R_MAX - (R_MAX - R_MIN) * (wire_unrolled / WIRE_LENGTH_MM);
        
        // Đảm bảo bán kính nằm trong giới hạn
        if(current_radius < R_MIN) {
            current_radius = R_MIN;
        }
        if(current_radius > R_MAX) {
            current_radius = R_MAX;
        }
    }
    
    // Trả về độ dài dây đã xả (mm)
    return (uint16_t)wire_unrolled;
}

/**
 * @brief Reset lại tính toán độ dài dây về trạng thái ban đầu (dây đầy)
 * 
 * @param encoder: Con trỏ đến cấu trúc encoder
 */
void Encoder_ResetWireLength(Encoder_t* encoder){
    wire_unrolled = 0.0f;
    current_radius = R_MAX;
    last_encoder_count = encoder->Encoder_Count;
}

/**
 * @brief Đặt độ dài dây hiện tại (dùng cho calibration)
 * 
 * @param encoder: Con trỏ đến cấu trúc encoder
 * @param length_mm: Độ dài dây đã xả (mm)
 */
void Encoder_SetWireLength(Encoder_t* encoder, float length_mm){
    // Giới hạn giá trị
    if(length_mm < 0.0f) length_mm = 0.0f;
    if(length_mm > WIRE_LENGTH_MM) length_mm = WIRE_LENGTH_MM;
    
    wire_unrolled = length_mm;
    
    // Cập nhật bán kính tương ứng
    current_radius = R_MAX - (R_MAX - R_MIN) * (wire_unrolled / WIRE_LENGTH_MM);
    
    // Cập nhật last_encoder_count để tránh nhảy số
    last_encoder_count = encoder->Encoder_Count;
}

/**
 * @brief Lấy bán kính hiện tại của vòng dây
 * 
 * @return Bán kính hiện tại (mm)
 */
float Encoder_GetCurrentRadius(void){
    return current_radius;
}

void Encoder_Check_Calib_Origin(Encoder_t* encoder){
    HAL_GPIO_ReadPin(GPIOA, IN1_Pin);
    if(HAL_GPIO_ReadPin(GPIOA, IN1_Pin) == true){
        encoder->Calib_Origin_Status = true;
        Encoder_Reset(encoder);
    }
    else{
        encoder->Calib_Origin_Status = false;
    }
}