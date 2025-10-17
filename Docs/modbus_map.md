# 📘 Modbus Register Map – Dual DC Motor Driver (STM32F103C8T6)

## 🟣 System Registers

| Address | Name                    | Type     | R/W | Description                                  | Default |
|---------|-------------------------|----------|-----|----------------------------------------------|---------|
| 0x0100  | Device_ID               | uint8   | R/W | Modbus slave address                         | 3       |
| 0x0101  | Config_Baudrate        | uint8   | R/W   | 	1=9600, 2=19200, 3=38400, 4=57600, 5=115200       | 5  |
| 0x0102  | Config_Parity           | uint8   | R/W | 0=None, 1=Even, 2=Odd     |    0      |
| 0x0103  | Config_Stop_bit           | uint8   | R/W | 1 or 2  |   1         | 
| 0x0104  | Module Type           | uint8   | R | Type of module                         | 4 = `motor driver`       |
| 0x0105  | Firmware Version           | uint16   | R | Version of firmware                         | 0x001=`v0.01`       |
| 0x0106  | Hardware Version            | uint16   | R | Version of hardware                         | 0x001=`v0.01`       |
| 0x0107  | System_Status           | uint16   | R   | Bitfield: system status                      | 0x0000  |
| 0x0108  | System_Error            | uint16   | R   | Global error code                            | 0       |
| 0x0109  | Reset_Error_Command     | uint16   | W   | Write 1 to reset all error flags             | 0   |


---

## 🔵 Motor 1 Registers (Base Address: 0x0010)

| Address | Name                    | Type     | R/W | Description                                  | Default |    Range   |
|---------|-------------------------|----------|-----|----------------------------------------------|---------|------------|
| 0x0000  | M1_Control_Mode         | uint8   | R/W | 1=ONOFF, 2=PID                               | 1       |             |
| 0x0001  | M1_Enable               | uint8   | R/W | 0=DISABLE, 1=ENABLE                          | 0       |             |
| 0x0002  | M1_Command_Speed        | uint8   | R/W | Speed setpoint                               | 0       |             |
| 0x0003  | M1_Actual_Speed         | uint8   | R   | Measured speed                               | 0       |             |
| 0x0004  | M1_Direction            | uint8   | R/W | 0=Idle, 1=Forward, 2=Reverse                 | 0       |             |
| 0x0005  | M1_Max_Speed            | uint8   | R/W | Maximum speed limit                          | 100     |             |
| 0x0006  | M1_Min_Speed            | uint8   | R/W | Minimum speed limit                          | 0       |             |
| 0x0007  | M1_PID_Kp               | uint8   | R/W | PID Kp gain (×100)                           | 100     |             |
| 0x0008  | M1_PID_Ki               | uint8   | R/W | PID Ki gain (×100)                           | 10      |             |
| 0x0009  | M1_PID_Kd               | uint8   | R/W | PID Kd gain (×100)                           | 5       |             |
| 0x000A  | M1_Max_Acceleration     | uint8   | R/W | Maximum acceleration rate                    | 5       |             |
| 0x000B  | M1_Max_Deceleration     | uint8   | R/W | Maximum deceleration rate                    | 4       |             |
| 0x000C  | M1_Status_Word          | uint8   | R   | Motor status flags                           | 0x0000  |             |
| 0x000D  | M1_Error_Code           | uint8   | R   | Error code if any                            | 0       |             |

---

## 🟢 Motor 2 Registers (Base Address: 0x0020)

| Address | Name                    | Type     | R/W | Description                                  | Default |    Range   |
|---------|-------------------------|----------|-----|----------------------------------------------|---------|------------|
| 0x0010  | M2_Control_Mode         | uint8   | R/W | 1=ONOFF, 2=PID                               | 1       |             |
| 0x0011  | M2_Enable               | uint8   | R/W | 0=OFF, 1=ON                                  | 0       |             |
| 0x0012  | M2_Command_Speed        | uint8   | R/W | Speed setpoint                               | 0       |             |
| 0x0013  | M2_Actual_Speed         | uint8   | R   | Measured speed                               | 0       |             |
| 0x0014  | M2_Direction            | uint8   | R/W | 0=Idle, 1=Forward, 2=Reverse                 | 0       |             |
| 0x0015  | M2_Max_Speed            | uint8   | R/W | Maximum speed limit                          | 100     |             |
| 0x0016  | M2_Min_Speed            | uint8   | R/W | Minimum speed limit                          | 0       |             |
| 0x0017  | M2_PID_Kp               | uint8   | R/W | PID Kp gain (×100)                           | 100     |             |
| 0x0018  | M2_PID_Ki               | uint8   | R/W | PID Ki gain (×100)                           | 10      |             |
| 0x0019  | M2_PID_Kd               | uint8   | R/W | PID Kd gain (×100)                           | 5       |             |
| 0x001A  | M2_Max_Acceleration     | uint8   | R/W | Maximum acceleration rate                    | 5       |             |
| 0x001B  | M2_Max_Deceleration     | uint8   | R/W | Maximum deceleration rate                    | 4       |             |
| 0x001C  | M2_Status_Word          | uint8   | R   | Motor status flags                           | 0x0000  |             |
| 0x001D  | M2_Error_Code           | uint8   | R   | Error code if any                            | 0       |             |

---

## 🟢 Digital Input Registers (Base Address: 0x0030)

| Address | Name            | Type   | R/W | Description                                                                                                          | Default | Range    |
|---------|----------------|--------|-----|----------------------------------------------------------------------------------------------------------------------|---------|----------|
| 0x0020  | DI_Status_Word | uint16 | R   | Bitfield for 4 digital inputs (bit 0: DI1, bit 1: DI2, bit 2: DI3, bit 3: DI4; 1=active)                           | 0x0000  | 0–0x000F |
| 0x0021  | DI1_Assignment | uint8  | R/W | Function assignment for DI1 (0=none, 1=start M1, 2=stop M1, 3=reverse M1, 4=fault reset, 5=mode switch)             | 0       | 0–10     |
| 0x0022  | DI2_Assignment | uint8  | R/W | Function assignment for DI2 (same options as DI1)                                                                     | 0       | 0–10     |
| 0x0023  | DI3_Assignment | uint8  | R/W | Function assignment for DI3 (same options as DI1)                                                                     | 0       | 0–10     |
| 0x0024  | DI4_Assignment | uint8  | R/W | Function assignment for DI4 (same options as DI1)                                                                     | 0       | 0–10     |
| 0x0025  | CURRENT   | uint8 | R | Current of module (scale 100) | 0 | |
---

## 🟢 Digital Output Registers (Base Address: 0x0040)

| Address | Name            | Type   | R/W | Description                                                                                | Default | Range    |
|---------|----------------|--------|-----|--------------------------------------------------------------------------------------------|---------|----------|
| 0x0030  | DO_Status_Word | uint16 | R   | Bitfield for 2 digital outputs (bit 0: DO1, bit 1: DO2; 1=active)                         | 0x0000  | 0–0x0003 |
| 0x0031  | DO1_Control    | uint8  | R/W | Control DO1 (0=off, 1=on)                                                                  | 0       | 0–1      |
| 0x0032  | DO1_Assignment | uint8  | R/W | Function assignment for DO1 (0=none, 1=running M1, 2=fault M1, 3=speed reached M1, 4=ready)| 0       | 0–10     |
| 0x0033  | DO2_Control    | uint8  | R/W | Control DO2 (0=off, 1=on)                                                                  | 0       | 0–1      |
| 0x0034  | DO2_Assignment | uint8  | R/W | Function assignment for DO2 (same options as DO1)                                          | 0       | 0–10     |

---