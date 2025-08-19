#ifndef UART_MODBUS_H
#define UART_MODBUS_H

#include "ModbusRegisters.h"

#define MODBUS_SLAVE_ADDRESS    3
#define MODBUS_BAUDRATE         9600
#define HOLDING_REG_START       0x0000
#define HOLDING_REG_COUNT       53  // Updated to match complete modbus map
#define INPUT_REG_START         0x0000
#define INPUT_REG_COUNT         5
#define COIL_START              0x0000
#define COIL_COUNT              8
#define DISCRETE_START          0x0000
#define DISCRETE_COUNT          4
#define RX_BUFFER_SIZE          256

// Global register arrays
extern uint16_t g_holdingRegisters[HOLDING_REG_COUNT];
extern uint16_t g_inputRegisters[INPUT_REG_COUNT];
extern uint8_t g_coils[COIL_COUNT];
extern uint8_t g_discreteInputs[DISCRETE_COUNT];

// Task counters
extern uint32_t g_taskCounter;
extern uint32_t g_modbusCounter;

// UART buffer variables
extern uint8_t rxBuffer[RX_BUFFER_SIZE];
extern uint8_t rxIndex;
extern uint8_t frameReceived;
extern uint32_t g_lastUARTActivity;

// Diagnostic variables
extern uint32_t g_totalReceived;
extern uint32_t g_corruptionCount;
extern uint8_t g_receivedIndex;

// Function declarations
static void MX_USART2_UART_Init(void);
uint16_t calcCRC(uint8_t *buf, int len);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);
void resetUARTCommunication(void);
void processModbusFrame(void);
void initializeModbusRegisters(void);
void updateSystemStatus(void);
void updateMotorStatus(void);
void updateDigitalIOStatus(void);

#endif