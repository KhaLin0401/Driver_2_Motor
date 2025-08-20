#include "UartModbus.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_it.h"
#include "main.h"
#include "ModbusMap.h"

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

// Global register arrays definition
uint16_t g_holdingRegisters[HOLDING_REG_COUNT];
uint16_t g_inputRegisters[INPUT_REG_COUNT];
uint8_t g_coils[COIL_COUNT];
uint8_t g_discreteInputs[DISCRETE_COUNT];

// Task counters
uint32_t g_taskCounter = 0;
uint32_t g_modbusCounter = 0;

// UART buffer variables
uint8_t rxBuffer[RX_BUFFER_SIZE];
uint8_t rxIndex = 0;
uint8_t frameReceived = 0;
uint32_t g_lastUARTActivity = 0;

// Diagnostic variables
uint32_t g_totalReceived = 0;
uint32_t g_corruptionCount = 0;
uint8_t g_receivedIndex = 0;

// MODIFICATION LOG
// Date: 2025-01-26
// Changed by: AI Agent
// Description: Added complete modbus register implementation with all missing registers
// Reason: To implement complete modbus map as specified in modbus_map.md
// Impact: All modbus functionality now available
// Testing: Test all register read/write operations

void initializeModbusRegisters(void) {
    // Initialize all registers to default values
    
    // System Registers (0x00F0-0x00F6)
    g_holdingRegisters[REG_DEVICE_ID] = DEFAULT_DEVICE_ID;
    g_holdingRegisters[REG_FIRMWARE_VERSION] = DEFAULT_FIRMWARE_VERSION;
    g_holdingRegisters[REG_SYSTEM_STATUS] = DEFAULT_SYSTEM_STATUS;
    g_holdingRegisters[REG_SYSTEM_ERROR] = DEFAULT_SYSTEM_ERROR;
    g_holdingRegisters[REG_RESET_ERROR_COMMAND] = 0;
    g_holdingRegisters[REG_CONFIG_BAUDRATE] = DEFAULT_CONFIG_BAUDRATE;
    g_holdingRegisters[REG_CONFIG_PARITY] = DEFAULT_CONFIG_PARITY;
    
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

void updateSystemStatus(void) {
    // Update system status based on current state
    uint16_t systemStatus = 0;
    
    // Check if motors are running
    if (g_holdingRegisters[REG_M1_ENABLE] && g_holdingRegisters[REG_M1_ACTUAL_SPEED] > 0) {
        systemStatus |= 0x0001; // Motor 1 running
    }
    if (g_holdingRegisters[REG_M2_ENABLE] && g_holdingRegisters[REG_M2_ACTUAL_SPEED] > 0) {
        systemStatus |= 0x0002; // Motor 2 running
    }
    
    // Check for errors
    if (g_holdingRegisters[REG_M1_ERROR_CODE] > 0 || g_holdingRegisters[REG_M2_ERROR_CODE] > 0) {
        systemStatus |= 0x0004; // Error present
    }
    
    // Check if system is ready
    if (g_holdingRegisters[REG_M1_ENABLE] == 0 && g_holdingRegisters[REG_M2_ENABLE] == 0) {
        systemStatus |= 0x0008; // System ready
    }
    
    g_holdingRegisters[REG_SYSTEM_STATUS] = systemStatus;
}

void updateMotorStatus(void) {
    // Update Motor 1 status word
    uint16_t m1Status = 0;
    if (g_holdingRegisters[REG_M1_ENABLE]) {
        m1Status |= 0x0001; // Enabled
        if (g_holdingRegisters[REG_M1_ACTUAL_SPEED] > 0) {
            m1Status |= 0x0002; // Running
        }
        if (g_holdingRegisters[REG_M1_ACTUAL_SPEED] >= g_holdingRegisters[REG_M1_COMMAND_SPEED]) {
            m1Status |= 0x0004; // Speed reached
        }
    }
    if (g_holdingRegisters[REG_M1_ERROR_CODE] > 0) {
        m1Status |= 0x0008; // Error
    }
    g_holdingRegisters[REG_M1_STATUS_WORD] = m1Status;
    
    // Update Motor 2 status word
    uint16_t m2Status = 0;
    if (g_holdingRegisters[REG_M2_ENABLE]) {
        m2Status |= 0x0001; // Enabled
        if (g_holdingRegisters[REG_M2_ACTUAL_SPEED] > 0) {
            m2Status |= 0x0002; // Running
        }
        if (g_holdingRegisters[REG_M2_ACTUAL_SPEED] >= g_holdingRegisters[REG_M2_COMMAND_SPEED]) {
            m2Status |= 0x0004; // Speed reached
        }
    }
    if (g_holdingRegisters[REG_M2_ERROR_CODE] > 0) {
        m2Status |= 0x0008; // Error
    }
    g_holdingRegisters[REG_M2_STATUS_WORD] = m2Status;
}

void updateDigitalIOStatus(void) {
    // Update digital input status word based on discrete inputs
    uint16_t diStatus = 0;
    for (int i = 0; i < DISCRETE_COUNT; i++) {
        if (g_discreteInputs[i]) {
            diStatus |= (1 << i);
        }
    }
    g_holdingRegisters[REG_DI_STATUS_WORD] = diStatus;
    
    // Update digital output status word based on coils
    uint16_t doStatus = 0;
    for (int i = 0; i < 2; i++) { // Only 2 digital outputs
        if (g_coils[i]) {
            doStatus |= (1 << i);
        }
    }
    g_holdingRegisters[REG_DO_STATUS_WORD] = doStatus;
}

static void MX_USART2_UART_Init(void) {
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) {
        Error_Handler();
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
            rxBuffer[rxIndex++] = huart->Instance->DR;
            frameReceived = 1;
            
            if (rxIndex >= 6) {
                uint8_t expectedLength = 0;
                if (rxBuffer[1] == 3 || rxBuffer[1] == 6) {
                    expectedLength = 8;
                } else if (rxBuffer[1] == 4) {
                    expectedLength = 8;
                } else if (rxBuffer[1] == 16) {
                    if (rxIndex >= 7) {
                        expectedLength = 9 + rxBuffer[6];
                    }
                }
                
                if (rxIndex >= expectedLength) {
                    processModbusFrame();
                }
            }
        } else {
            rxIndex = 0;
            frameReceived = 0;
        }
        HAL_UART_Receive_IT(&huart2, &rxBuffer[rxIndex], 1);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        HAL_GPIO_TogglePin(LED4_GPIO_Port, LED4_Pin);
        rxIndex = 0;
        frameReceived = 0;
        HAL_UART_Abort(&huart2);
        HAL_UART_Receive_IT(&huart2, &rxBuffer[rxIndex], 1);
    }
}

void resetUARTCommunication(void) {
    HAL_UART_Abort(&huart2);
    rxIndex = 0;
    frameReceived = 0;
    HAL_UART_Receive_IT(&huart2, &rxBuffer[0], 1);
}

void processModbusFrame(void) {
    if (rxIndex < 6) return;
    if (rxBuffer[0] != MODBUS_SLAVE_ADDRESS) return;

    uint16_t crc = calcCRC(rxBuffer, rxIndex - 2);
    if (rxBuffer[rxIndex - 2] != (crc & 0xFF) || rxBuffer[rxIndex - 1] != (crc >> 8)) {
        HAL_GPIO_TogglePin(LED4_GPIO_Port, LED4_Pin);
        return;
    }

    uint8_t funcCode = rxBuffer[1];
    uint8_t txBuffer[256];
    uint8_t txIndex = 0;
    txBuffer[0] = MODBUS_SLAVE_ADDRESS;
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
            
            // Handle special register writes
            if (addr == REG_RESET_ERROR_COMMAND && value == 1) {
                g_holdingRegisters[REG_M1_ERROR_CODE] = 0;
                g_holdingRegisters[REG_M2_ERROR_CODE] = 0;
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
    
    if (HAL_UART_Transmit(&huart2, txBuffer, txIndex, 100) != HAL_OK) {
        HAL_GPIO_TogglePin(LED4_GPIO_Port, LED4_Pin);
        HAL_UART_Abort(&huart2);
    } else {
        HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
    }
    
    rxIndex = 0;
    frameReceived = 0;
}