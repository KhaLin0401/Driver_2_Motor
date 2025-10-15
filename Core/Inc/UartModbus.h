#ifndef UART_MODBUS_H
#define UART_MODBUS_H

#include "main.h"
#include <stdint.h>
#include <string.h>
#include "cmsis_os.h"

#define MODBUS_SLAVE_ADDRESS    3
#define MODBUS_BAUDRATE         115200
#define HOLDING_REG_START       0x0000
#define HOLDING_REG_COUNT       300  // Increased to cover all register addresses
#define INPUT_REG_START         0x0000
#define INPUT_REG_COUNT         5
#define COIL_START              0x0000
#define COIL_COUNT              8
#define DISCRETE_START          0x0000
#define DISCRETE_COUNT          4
#define RX_BUFFER_SIZE          256
extern osMutexId_t modbusTxMutex;
// Global register arrays
extern uint16_t g_holdingRegisters[HOLDING_REG_COUNT];
extern uint16_t g_inputRegisters[INPUT_REG_COUNT];
extern uint8_t g_coils[COIL_COUNT];
extern uint8_t g_discreteInputs[DISCRETE_COUNT];
extern uint8_t current_baudrate;

// Task counters
extern uint32_t g_taskCounter;
extern uint32_t g_modbusCounter;

// UART buffer variables
extern uint8_t rxBuffer[RX_BUFFER_SIZE];
extern uint8_t rxIndex;
extern uint8_t frameReceived;
extern uint32_t g_lastUARTActivity;

// Single byte buffer for UART reception (declared in UartModbus.c)
// extern uint8_t rxByte;  // Static variable, không export

// Diagnostic variables
extern uint32_t g_totalReceived;
extern uint32_t g_corruptionCount;
extern uint32_t g_timeoutCount;
extern uint32_t g_queueFullCount;
extern uint32_t g_lastResetTime;
extern uint8_t g_receivedIndex;

#define UART_HEALTH_CHECK_INTERVAL 1000  // Check every 1 second
#define UART_MAX_TIMEOUT_COUNT 3         // Reset after 3 timeouts
#define UART_QUEUE_TIMEOUT 10            // 10ms timeout for queue operations

// UART health monitoring variables
extern uint32_t last_health_check;

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
void updateBaudrate(void);
void checkUARTHealth(void);
void startModbusUARTReception(void);

#endif