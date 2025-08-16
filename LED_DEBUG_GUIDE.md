# LED DEBUG GUIDE - TIMEOUT ERROR DIAGNOSIS
## DC Driver 2 - Modbus RTU Communication

### **Hi Boss Vũ!**

## **Hệ thống Debug LED:**

### **LED Assignment:**
- **LED1 (PA4)** - UART Communication Status
- **LED2 (PC13)** - Timer/Timeout Status  
- **LED3 (PC14)** - Port/Stack Status
- **LED4 (PC15)** - Overall System Status

## **LED Patterns và Ý nghĩa:**

### **1. Khởi động hệ thống:**
```
LED1: OFF → ON (UART ready)
LED2: OFF → ON (Timer ready)
LED3: OFF → ON (Port ready)
LED4: OFF → ON (System ready)
```

### **2. Normal Operation (MB_ENOERR):**
```
LED1: ON (UART OK)
LED2: ON (Timer OK)
LED3: ON (Port OK)
LED4: ON (System OK)
```

### **3. Error Patterns:**

#### **Port Error (MB_EPORTERR):**
```
LED1: BLINKING (Port error detected)
LED2: ON (Timer OK)
LED3: ON (Port OK)
LED4: ON (System OK)
```
**Nguyên nhân:** Timer configuration sai, interrupt priority conflict

#### **I/O Error (MB_EIO):**
```
LED1: ON (UART OK)
LED2: BLINKING (I/O error detected)
LED3: ON (Port OK)
LED4: ON (System OK)
```
**Nguyên nhân:** UART communication issue, baudrate mismatch, cable problem

#### **Register Error (MB_ENOREG):**
```
LED1: ON (UART OK)
LED2: ON (Timer OK)
LED3: BLINKING (Register error detected)
LED4: ON (System OK)
```
**Nguyên nhân:** Invalid register address, master error

#### **Unknown Error:**
```
LED1: ON (UART OK)
LED2: ON (Timer OK)
LED3: ON (Port OK)
LED4: BLINKING (Unknown error detected)
```
**Nguyên nhân:** Unexpected error, system instability

#### **Multiple Errors:**
```
LED1: BLINKING (UART errors)
LED2: BLINKING (Timer errors)
LED3: BLINKING (Port errors)
LED4: BLINKING (System errors)
```
**Nguyên nhân:** Multiple system failures, hardware issue

## **Troubleshooting Steps:**

### **Step 1: Kiểm tra khởi động**
1. **LED test sequence** = Hardware verification
   - All OFF → All ON → Individual blink test
2. **Tất cả LED sáng** = System ready
3. **LED nào không sáng** = Hardware issue với LED đó
4. **LED nào nhấp nháy** = Error trong quá trình khởi động

### **Step 1.5: LED1 sáng, 3 LED còn lại nhấp nháy**
**Nguyên nhân:** Timer configuration issue + Interrupt priority conflict
**Giải pháp:** 
- Timer priority đã được tăng lên 0 (cao nhất, cao hơn UART priority 3)
- Timer calculation đã được sửa cho chính xác hơn
- Prescaler calculation: `(pclk1_freq / 50000) - 1` (corrected)
- Timer deinitialize trước khi initialize để clean state
- Force timer reset sau khi enable Modbus stack
- Enhanced error handling với timer reset trong Modbus task

### **Step 1.6: Tất cả LED sáng nhưng Modbus vẫn timeout**
**Nguyên nhân:** Timer callback không được gọi hoặc period calculation sai
**Giải pháp:**
- Prescaler corrected: `(pclk1_freq / 50000) - 1`
- Added debug output trong timer callback (LED3 toggle)
- Added timer test function để verify functionality
- LED2 toggle khi timer init, LED4 toggle khi timer start

### **Step 1.7: LED3 không sáng sau khi đổi slaveAddr**
**Triệu chứng:** Sau khi đổi `slaveAddr` từ 1 thành 32, LED3 không sáng nhưng Modbus vẫn timeout.

**Nguyên nhân có thể:**
1. **Function code không được hỗ trợ** - Master gửi function code bị disable
2. **Register address không hợp lệ** - Master yêu cầu register ngoài phạm vi 0x0000-0x0026
3. **Slave address mismatch** - Master vẫn gửi đến address cũ

**Cách khắc phục:**
1. **Kiểm tra function code bằng LED pattern:**
   - Function 0x01 (Read Coils): LED1 + LED2 nhấp nháy
   - Function 0x02 (Read Discrete): LED2 + LED3 nhấp nháy  
   - Function 0x05 (Write Coil): LED3 + LED4 nhấp nháy
   - Function 0x0F (Write Multiple Coils): Tất cả LED nhấp nháy
   
2. **Kiểm tra register range:**
   - Valid registers: 0x0000 đến 0x0026 (39 registers)
   - Motor 1: 0x0000-0x000D (14 registers)
   - Motor 2: 0x0010-0x001D (14 registers)  
   - System: 0x0020-0x0026 (7 registers)
   
3. **Cập nhật Modbus master:**
   - Đổi slave address trong master thành 32
   - Kiểm tra function code được sử dụng
   - Đảm bảo register address trong phạm vi hợp lệ

### **Step 2: Kiểm tra Modbus communication**
1. **Kết nối Modbus master**
2. **Gửi test command**
3. **Quan sát LED pattern**

### **Step 3: Phân tích LED pattern**

#### **Nếu LED1 nhấp nháy (Port Error):**
- Kiểm tra timer configuration
- Verify interrupt priorities
- Check FreeModbus stack initialization

#### **Nếu LED2 nhấp nháy (I/O Error):**
- Kiểm tra UART connections (TX/RX)
- Verify baudrate settings (9600)
- Check cable quality
- Test với different Modbus master

#### **Nếu LED3 nhấp nháy (Register Error):**
- Kiểm tra slave address (1)
- Verify register addresses
- Check ModbusMap configuration

#### **Nếu LED4 nhấp nháy (System Error):**
- Kiểm tra power supply
- Verify clock configuration
- Check FreeRTOS task priorities

### **Step 4: Error Recovery**
1. **Disconnect Modbus master**
2. **Reset system**
3. **Quan sát LED pattern khi khởi động**
4. **Reconnect và test lại**

## **Expected Results:**

### **Normal Operation:**
```
LED1: ON (UART communication stable)
LED2: ON (Timer working correctly)
LED3: ON (Port/Stack functioning)
LED4: ON (System healthy)
```

### **Timeout Error Scenarios:**

#### **Scenario 1: Timer Configuration Issue**
```
LED1: ON
LED2: BLINKING (Timer timeout)
LED3: ON
LED4: ON
```
**Solution:** Fix timer prescaler/period calculation

#### **Scenario 2: UART Communication Issue**
```
LED1: BLINKING (UART errors)
LED2: ON
LED3: ON
LED4: ON
```
**Solution:** Check UART connections, baudrate

#### **Scenario 3: Interrupt Priority Conflict**
```
LED1: BLINKING
LED2: BLINKING
LED3: ON
LED4: ON
```
**Solution:** Adjust interrupt priorities

#### **Scenario 4: System Instability**
```
LED1: BLINKING
LED2: BLINKING
LED3: BLINKING
LED4: BLINKING
```
**Solution:** Check power supply, clock, hardware

## **Debug Commands:**

### **Test LED System:**
```c
// Test all LEDs
debugShowErrorCode(MB_ENOERR);  // All ON
debugShowErrorCode(MB_EPORTERR); // LED1 blink
debugShowErrorCode(MB_EIO);      // LED2 blink
debugShowErrorCode(MB_ENOREG);   // LED3 blink
```

### **Monitor Statistics:**
```c
// Check error counts
g_modbusDebugStats.timeoutErrors
g_modbusDebugStats.portErrors
g_modbusDebugStats.ioErrors
g_modbusDebugStats.consecutiveErrors
```

## **Maintenance Notes:**

### **Regular Checks:**
- Monitor LED patterns during operation
- Log error patterns for analysis
- Check LED brightness (hardware issue indicator)
- Verify LED connections

### **Performance Optimization:**
- Adjust polling frequency based on LED patterns
- Optimize error handling based on most common errors
- Fine-tune system parameters based on LED feedback

---

**Date:** 2025-01-03  
**Author:** AI Agent  
**Version:** 1.0  
**Status:** Implemented and Ready for Testing
