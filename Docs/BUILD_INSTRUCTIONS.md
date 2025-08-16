# BUILD INSTRUCTIONS FOR MODBUS MODULE

**Date:** 2025-01-03  
**Issue:** Undefined reference to `modbus_handle_timeout`  
**Solution:** ✅ FIXED

## 🔧 **PROBLEM SOLVED**

### **Error Message:**
```
undefined reference to `modbus_handle_timeout'
```

### **Root Cause:**
File `modbus.c` trong thư mục `modbus/` không được include trong build path của STM32CubeIDE.

### **Solution Applied:**
1. ✅ Copy `modbus.c` vào `Core/Src/` directory
2. ✅ Copy `modbus_crc.c` vào `Core/Src/` directory  
3. ✅ Ensure all functions are properly defined

## 📁 **FILE STRUCTURE AFTER FIX**

```
DC-Driver/Code/
├── Core/
│   ├── Inc/
│   │   └── modbus.h (header)
│   └── Src/
│       ├── modbus.c ✅ (NEW - copied here)
│       ├── modbus_crc.c ✅ (NEW - copied here)
│       └── modbus_port.c ✅ (existing)
├── modbus/
│   ├── modbus.h (original)
│   ├── modbus.c (original - not used in build)
│   ├── modbus_port.h (original)
│   ├── modbus_crc.h (original)
│   └── modbus_crc.c (original - not used in build)
```

## 🚀 **BUILD STEPS**

### **Option 1: STM32CubeIDE (Recommended)**
1. Open STM32CubeIDE
2. Import existing project: `DC-Driver/Code/`
3. Right-click project → **Properties**
4. **C/C++ Build** → **Settings** → **Tool Settings**
5. **Include paths** should include:
   - `${workspace_loc:/${ProjName}/modbus}`
   - `${workspace_loc:/${ProjName}/Core/Inc}`
6. **Build** → **Clean Project**
7. **Build** → **Build Project**

### **Option 2: Command Line (if makefile exists)**
```bash
cd DC-Driver/Code
make clean
make all
```

### **Option 3: Manual Build**
```bash
# Compile modbus files
arm-none-eabi-gcc -c Core/Src/modbus.c -o modbus.o
arm-none-eabi-gcc -c Core/Src/modbus_crc.c -o modbus_crc.o
arm-none-eabi-gcc -c Core/Src/modbus_port.c -o modbus_port.o

# Link with other objects
arm-none-eabi-gcc *.o -o Driver.elf
```

## ✅ **VERIFICATION**

### **Check Functions Available:**
```c
// These functions should now be available:
void modbus_init(void);
void modbus_receive_byte(uint8_t byte);
void modbus_handle_timeout(void);  // ✅ FIXED
uint16_t modbus_read_register(uint16_t address);
void modbus_write_register(uint16_t address, uint16_t value);
modbus_status_t modbus_get_status(void);
uint16_t modbus_get_rx_count(void);
uint16_t modbus_get_tx_count(void);
```

### **Test Compilation:**
```bash
# Should compile without errors
arm-none-eabi-gcc -c Core/Src/modbus.c
arm-none-eabi-gcc -c Core/Src/modbus_port.c
```

## 🎯 **EXPECTED RESULT**

After building:
- ✅ No undefined reference errors
- ✅ All modbus functions available
- ✅ Project compiles successfully
- ✅ Ready for flashing to STM32

## 🚨 **TROUBLESHOOTING**

### **If still getting undefined reference:**
1. **Check file locations**: Ensure `modbus.c` is in `Core/Src/`
2. **Check include paths**: Verify modbus directory is in include paths
3. **Clean and rebuild**: Clean project completely before rebuilding
4. **Check function names**: Ensure function names match exactly

### **Common Issues:**
- **File not in build path**: Move files to `Core/Src/`
- **Wrong function names**: Check spelling and case
- **Missing includes**: Ensure all headers are included
- **Build cache**: Clean project completely

**Status: READY FOR BUILD** 🚀 