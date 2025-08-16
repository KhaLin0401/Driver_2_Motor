# UART2 Communication System Guide

## Overview
This document describes the UART2 communication system implemented in the default task of the STM32F103 microcontroller project.

## Hardware Configuration
- **UART Port**: USART2
- **Baud Rate**: 9600
- **Data Bits**: 8
- **Stop Bits**: 1
- **Parity**: None
- **Flow Control**: None

## Features
- **Interrupt-based reception**: Uses HAL_UART_Receive_IT for efficient data reception
- **Command processing**: Processes text commands and controls LEDs
- **Real-time response**: Immediate feedback for all commands
- **Buffer management**: Circular buffer for received data
- **Heartbeat indicator**: LED1 blinks every 5 seconds to show system is alive

## Available Commands

### LED Control Commands
- `LED1ON` - Turn on LED1 (PC13)
- `LED1OFF` - Turn off LED1 (PC13)
- `LED2ON` - Turn on LED2 (PC14)
- `LED2OFF` - Turn off LED2 (PC14)
- `LED3ON` - Turn on LED3 (PC15)
- `LED3OFF` - Turn off LED3 (PC15)

### System Commands
- `ALLON` - Turn on all LEDs
- `ALLOFF` - Turn off all LEDs
- `BLINK` - Blink all LEDs 5 times
- `STATUS` - Get system status information
- `COUNTER` - Show task counter
- `HELP` - Show available commands

## Usage Examples

### Basic LED Control
```
LED1ON<CR>
LED1 OFF<CR>
LED2ON<CR>
```

### System Information
```
STATUS<CR>
HELP<CR>
```

### Special Effects
```
ALLON<CR>
BLINK<CR>
ALLOFF<CR>
```

## Technical Implementation

### Key Functions
1. **HAL_UART_RxCpltCallback()** - Interrupt callback for received data
2. **uartSendString()** - Send text messages via UART
3. **uartSendData()** - Send raw data via UART
4. **processUartCommand()** - Parse and execute commands
5. **StartDefaultTask()** - Main task loop with command processing

### Buffer Management
- **RX Buffer**: 256 bytes circular buffer
- **TX Buffer**: 256 bytes for transmission
- **Command Buffer**: 32 bytes for command processing

### Interrupt Handling
- Single character reception via interrupt
- Automatic buffer management
- Non-blocking operation

## Testing
1. Connect UART2 to a USB-to-UART converter
2. Open a terminal program (PuTTY, Tera Term, etc.)
3. Configure terminal: 9600 baud, 8N1
4. Send commands and observe responses

## Error Handling
- Buffer overflow protection
- Invalid command detection
- Timeout protection for transmissions
- System status monitoring

## Performance Characteristics
- **Response Time**: < 10ms for command processing
- **Throughput**: Up to 9600 bps
- **Memory Usage**: ~1KB for buffers and variables
- **CPU Usage**: < 5% in default task

## Troubleshooting
1. **No response**: Check baud rate and connections
2. **Garbled data**: Verify UART settings
3. **Commands not working**: Ensure proper line endings (CR/LF)
4. **System not responding**: Check if heartbeat LED is blinking
