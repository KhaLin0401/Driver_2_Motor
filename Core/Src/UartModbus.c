#include "UartModbus.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_it.h"
#include "main.h"
#include "ModbusMap.h"
#include "cmsis_os.h"
#include <string.h>

// Khởi tạo mutex (giữ lại để bảo vệ UART TX)
osMutexId_t modbusTxMutex;

// Buffer đơn giản cho việc nhận dữ liệu
uint8_t rxBuffer[RX_BUFFER_SIZE];
uint8_t rxIndex = 0;
uint8_t frameReceived = 0;
uint32_t g_lastUARTActivity = 0;

// Single byte buffer for UART reception
static uint8_t rxByte = 0;

// Global register arrays definition
uint16_t g_holdingRegisters[HOLDING_REG_COUNT];
uint16_t g_inputRegisters[INPUT_REG_COUNT];
uint8_t g_coils[COIL_COUNT];
uint8_t g_discreteInputs[DISCRETE_COUNT];

// Task counters
uint32_t g_taskCounter = 0;
uint32_t g_modbusCounter = 0;

// Diagnostic variables
uint32_t g_totalReceived = 0;
uint32_t g_corruptionCount = 0;
uint32_t g_timeoutCount = 0;
uint32_t g_queueFullCount = 0;
uint32_t g_lastResetTime = 0;
uint8_t g_receivedIndex = 0;

// LED indicator flag
uint8_t g_ledIndicator = 0;

// UART health monitoring variables
uint32_t last_health_check = 0;
static uint8_t uart_error_count = 0;

void initializeModbusRegisters(void) {
    // Initialize all registers to default values
    
    // System Registers (0x00F0-0x00F6)
    g_holdingRegisters[REG_DEVICE_ID] = DEFAULT_DEVICE_ID;  
    g_holdingRegisters[REG_CONFIG_BAUDRATE] = DEFAULT_CONFIG_BAUDRATE;
    g_holdingRegisters[REG_CONFIG_PARITY] = DEFAULT_CONFIG_PARITY;
    g_holdingRegisters[REG_CONFIG_STOP_BIT] = DEFAULT_CONFIG_STOP_BIT;
    g_holdingRegisters[REG_MODULE_TYPE] = DEFAULT_MODULE_TYPE;
    g_holdingRegisters[REG_FIRMWARE_VERSION] = DEFAULT_FIRMWARE_VERSION;
    g_holdingRegisters[REG_HARDWARE_VERSION] = DEFAULT_HARDWARE_VERSION;
    g_holdingRegisters[REG_SYSTEM_STATUS] = DEFAULT_SYSTEM_STATUS;
    g_holdingRegisters[REG_SYSTEM_ERROR] = DEFAULT_SYSTEM_ERROR;
    g_holdingRegisters[REG_RESET_ERROR_COMMAND] = DEFAULT_RESET_ERROR_COMMAND;
    
    // Motor 1 Registers (0x0000-0x000C)
    g_holdingRegisters[REG_M1_CONTROL_MODE] = DEFAULT_CONTROL_MODE;
    g_holdingRegisters[REG_M1_ENABLE] = DEFAULT_ENABLE;
    g_holdingRegisters[REG_M1_COMMAND_SPEED] = DEFAULT_COMMAND_SPEED;
    g_holdingRegisters[REG_M1_ACTUAL_SPEED] = DEFAULT_ACTUAL_SPEED;
    g_holdingRegisters[REG_M1_DIRECTION] = DEFAULT_DIRECTION;
    g_holdingRegisters[REG_M1_PID_KP] = DEFAULT_PID_KP;
    g_holdingRegisters[REG_M1_PID_KI] = DEFAULT_PID_KI;
    g_holdingRegisters[REG_M1_PID_KD] = DEFAULT_PID_KD;
    g_holdingRegisters[REG_M1_STATUS_WORD] = DEFAULT_STATUS_WORD;
    g_holdingRegisters[REG_M1_ERROR_CODE] = DEFAULT_ERROR_CODE;
    g_holdingRegisters[REG_M1_MAX_SPEED] = DEFAULT_MAX_SPEED;
    g_holdingRegisters[REG_M1_MIN_SPEED] = DEFAULT_MIN_SPEED;
    g_holdingRegisters[REG_M1_MAX_ACCELERATION] = DEFAULT_MAX_ACCELERATION;
    g_holdingRegisters[REG_M1_MAX_DECELERATION] = DEFAULT_MAX_DECELERATION;
    
    // Motor 2 Registers (0x0010-0x001C)
    g_holdingRegisters[REG_M2_CONTROL_MODE] = DEFAULT_CONTROL_MODE;
    g_holdingRegisters[REG_M2_ENABLE] = DEFAULT_ENABLE;
    g_holdingRegisters[REG_M2_COMMAND_SPEED] = DEFAULT_COMMAND_SPEED;
    g_holdingRegisters[REG_M2_ACTUAL_SPEED] = DEFAULT_ACTUAL_SPEED;
    g_holdingRegisters[REG_M2_DIRECTION] = DEFAULT_DIRECTION;
    g_holdingRegisters[REG_M2_PID_KP] = DEFAULT_PID_KP;
    g_holdingRegisters[REG_M2_PID_KI] = DEFAULT_PID_KI;
    g_holdingRegisters[REG_M2_PID_KD] = DEFAULT_PID_KD;
    g_holdingRegisters[REG_M2_STATUS_WORD] = DEFAULT_STATUS_WORD;
    g_holdingRegisters[REG_M2_ERROR_CODE] = DEFAULT_ERROR_CODE;
    g_holdingRegisters[REG_M2_MAX_SPEED] = DEFAULT_MAX_SPEED;
    g_holdingRegisters[REG_M2_MIN_SPEED] = DEFAULT_MIN_SPEED;
    g_holdingRegisters[REG_M2_MAX_ACCELERATION] = DEFAULT_MAX_ACCELERATION;
    g_holdingRegisters[REG_M2_MAX_DECELERATION] = DEFAULT_MAX_DECELERATION;
    
    // Input Registers (0x0020-0x0024)
    g_holdingRegisters[REG_DI_STATUS_WORD] = 0;
    g_holdingRegisters[REG_DI1_ASSIGNMENT] = 0;
    g_holdingRegisters[REG_DI2_ASSIGNMENT] = 0;
    g_holdingRegisters[REG_DI3_ASSIGNMENT] = 0;
    g_holdingRegisters[REG_DI4_ASSIGNMENT] = 0;
    g_holdingRegisters[REG_CURRENT] = DEFAULT_CURRENT;
    // Output Registers (0x0040-0x0044)  
    g_holdingRegisters[REG_DO_STATUS_WORD] = 0;
    g_holdingRegisters[REG_DO1_CONTROL] = 0;
    g_holdingRegisters[REG_DO1_ASSIGNMENT] = 0;
    g_holdingRegisters[REG_DO2_CONTROL] = 0;
    g_holdingRegisters[REG_DO2_ASSIGNMENT] = 0;

    // Initialize other arrays
    for (int i = 0; i < INPUT_REG_COUNT; i++) {
        g_inputRegisters[i] = 0;
    }
    
    for (int i = 0; i < COIL_COUNT; i++) {
        g_coils[i] = 0;
    }
    
    for (int i = 0; i < DISCRETE_COUNT; i++) {
        g_discreteInputs[i] = 0;
    }
}

uint16_t calcCRC(uint8_t *buf, int len) {
    uint16_t crc = 0xFFFF;
    for (int pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)buf[pos];
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        g_lastUARTActivity = HAL_GetTick();
        
        if (rxIndex < RX_BUFFER_SIZE - 1) {
            // Lưu byte vừa nhận
            rxBuffer[rxIndex++] = rxByte;
            
            // Kiểm tra xem có đủ frame chưa
            if (rxIndex >= 3) {
                uint8_t funcCode = rxBuffer[1];
                uint8_t expectedLength = 0;
                
                switch(funcCode) {
                    case 3:  // Read holding registers
                    case 4:  // Read input registers
                    case 6:  // Write single register
                        expectedLength = 8;
                        break;
                    case 16: // Write multiple registers
                        if (rxIndex >= 7) {
                            expectedLength = 9 + rxBuffer[6];
                        }
                        break;
                    default:
                        // Function code không hợp lệ - reset
                        rxIndex = 0;
                        frameReceived = 0;
                        break;
                }
                
                // Nếu đã nhận đủ frame theo expectedLength
                if (expectedLength > 0 && rxIndex >= expectedLength) {
                    frameReceived = 1;
                }
            }
        } else {
            // Buffer overflow - reset
            rxIndex = 0;
            frameReceived = 0;
        }
        
        // Tiếp tục nhận byte tiếp theo
        HAL_UART_Receive_IT(&huart2, &rxByte, 1);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        rxIndex = 0;
        frameReceived = 0;
        HAL_UART_Abort(&huart2);
        HAL_UART_Receive_IT(&huart2, &rxByte, 1);
    }
}

void resetUARTCommunication(void) {
    HAL_UART_Abort(&huart2);
    rxIndex = 0;
    frameReceived = 0;
    HAL_UART_Receive_IT(&huart2, &rxByte, 1);
}

void processModbusFrame(void) {
    if (rxIndex < 6) return;
    if (rxBuffer[0] != g_holdingRegisters[REG_DEVICE_ID]) {
        rxIndex = 0;
        frameReceived = 0;
        return;
    }

    uint16_t crc = calcCRC(rxBuffer, rxIndex - 2);
    if (rxBuffer[rxIndex - 2] != (crc & 0xFF) || rxBuffer[rxIndex - 1] != (crc >> 8)) {
        rxIndex = 0;
        frameReceived = 0;
        g_corruptionCount++;
        return;
    }

    uint8_t funcCode = rxBuffer[1];
    uint8_t txBuffer[256];
    uint8_t txIndex = 0;
    txBuffer[0] = g_holdingRegisters[REG_DEVICE_ID];
    txBuffer[1] = funcCode;

    if (funcCode == 3) {
        uint16_t addr = (rxBuffer[2] << 8) | rxBuffer[3];
        uint16_t qty = (rxBuffer[4] << 8) | rxBuffer[5];
        if (addr + qty <= HOLDING_REG_COUNT) {
            txBuffer[2] = qty * 2;
            txIndex = 3;
            for (int i = 0; i < qty; i++) {
                txBuffer[txIndex++] = g_holdingRegisters[addr + i] >> 8;
                txBuffer[txIndex++] = g_holdingRegisters[addr + i] & 0xFF;
            }
        } else {
            txBuffer[1] |= 0x80;
            txBuffer[2] = 0x02;
            txIndex = 3;
        }
    } else if (funcCode == 4) {
        uint16_t addr = (rxBuffer[2] << 8) | rxBuffer[3];
        uint16_t qty = (rxBuffer[4] << 8) | rxBuffer[5];
        if (addr + qty <= INPUT_REG_COUNT) {
            txBuffer[2] = qty * 2;
            txIndex = 3;
            for (int i = 0; i < qty; i++) {
                txBuffer[txIndex++] = g_inputRegisters[addr + i] >> 8;
                txBuffer[txIndex++] = g_inputRegisters[addr + i] & 0xFF;
            }
        } else {
            txBuffer[1] |= 0x80;
            txBuffer[2] = 0x02;
            txIndex = 3;
        }
    } else if (funcCode == 6) {
        uint16_t addr = (rxBuffer[2] << 8) | rxBuffer[3];
        uint16_t value = (rxBuffer[4] << 8) | rxBuffer[5];
        if (addr < HOLDING_REG_COUNT) {
            g_holdingRegisters[addr] = value;
            
            if (addr == REG_RESET_ERROR_COMMAND && value == 1) {
                g_holdingRegisters[REG_SYSTEM_ERROR] = 0;
            }
            
            txBuffer[2] = rxBuffer[2];
            txBuffer[3] = rxBuffer[3];
            txBuffer[4] = rxBuffer[4];
            txBuffer[5] = rxBuffer[5];
            txIndex = 6;
        } else {
            txBuffer[1] |= 0x80;
            txBuffer[2] = 0x02;
            txIndex = 3;
        }
    } else if (funcCode == 16) {
        uint16_t addr = (rxBuffer[2] << 8) | rxBuffer[3];
        uint16_t qty = (rxBuffer[4] << 8) | rxBuffer[5];
        uint8_t byteCount = rxBuffer[6];
        if (addr + qty <= HOLDING_REG_COUNT && byteCount == qty * 2) {
            for (int i = 0; i < qty; i++) {
                g_holdingRegisters[addr + i] = (rxBuffer[7 + i*2] << 8) | rxBuffer[8 + i*2];
            }
            txBuffer[2] = rxBuffer[2];
            txBuffer[3] = rxBuffer[3];
            txBuffer[4] = rxBuffer[4];
            txBuffer[5] = rxBuffer[5];
            txIndex = 6;
        } else {
            txBuffer[1] |= 0x80;
            txBuffer[2] = 0x02;
            txIndex = 3;
        }
    } else {
        txBuffer[1] |= 0x80;
        txBuffer[2] = 0x01;
        txIndex = 3;
    }

    crc = calcCRC(txBuffer, txIndex);
    txBuffer[txIndex++] = crc & 0xFF;
    txBuffer[txIndex++] = crc >> 8;
    
    // Sử dụng mutex để bảo vệ việc truyền dữ liệu
    if (modbusTxMutex != NULL) {
        osMutexAcquire(modbusTxMutex, osWaitForever);
    }
    
    HAL_UART_Transmit(&huart2, txBuffer, txIndex, 100);
    
    if (modbusTxMutex != NULL) {
        osMutexRelease(modbusTxMutex);
    }
    
    // Đánh dấu để LED nháy
    g_ledIndicator = 1;
    
    // Reset buffer sau khi xử lý
    rxIndex = 0;
    frameReceived = 0;
}

void updateBaudrate(void) {
    if(current_baudrate == g_holdingRegisters[REG_CONFIG_BAUDRATE])
        return;
    
    if (modbusTxMutex != NULL) {
        osMutexAcquire(modbusTxMutex, osWaitForever);
    }
    
    switch(g_holdingRegisters[REG_CONFIG_BAUDRATE]) {
        case 1:
            current_baudrate = 1;
            huart2.Init.BaudRate = 9600;
            break;
        case 2:
            current_baudrate = 2;
            huart2.Init.BaudRate = 19200;
            break;
        case 3:
            current_baudrate = 3;
            huart2.Init.BaudRate = 38400;
            break;
        case 4:
            current_baudrate = 4;
            huart2.Init.BaudRate = 57600;
            break;
        case 5:
            current_baudrate = 5;
            huart2.Init.BaudRate = 115200;
            break;
        default:
            current_baudrate = 5;
            huart2.Init.BaudRate = 115200;
            break;
    }
    
    HAL_UART_DeInit(&huart2);
    HAL_UART_Init(&huart2);
    HAL_UART_Receive_IT(&huart2, &rxByte, 1);
    
    if (modbusTxMutex != NULL) {
        osMutexRelease(modbusTxMutex);
    }
}

void checkUARTHealth(void) {
    uint32_t current_time = HAL_GetTick();
    
    // Kiểm tra định kỳ
    if (current_time - last_health_check >= UART_HEALTH_CHECK_INTERVAL) {
        last_health_check = current_time;
        
        // Kiểm tra timeout dài
        if (current_time - g_lastUARTActivity > 30000) {
            resetUARTCommunication();
            g_lastUARTActivity = current_time;
        }
    }
}

void startModbusUARTReception(void) {
    // Bắt đầu nhận UART vào rxByte
    // Gọi hàm này SAU KHI RTOS đã start
    g_lastUARTActivity = HAL_GetTick();
    last_health_check = g_lastUARTActivity;
    HAL_UART_Receive_IT(&huart2, &rxByte, 1);
}