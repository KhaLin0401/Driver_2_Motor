################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/FreeModbus/mb.c \
../Core/Src/FreeModbus/mbcrc.c \
../Core/Src/FreeModbus/mbfunccoils.c \
../Core/Src/FreeModbus/mbfuncdiag.c \
../Core/Src/FreeModbus/mbfuncdisc.c \
../Core/Src/FreeModbus/mbfuncholding.c \
../Core/Src/FreeModbus/mbfuncinput.c \
../Core/Src/FreeModbus/mbfuncother.c \
../Core/Src/FreeModbus/mbrtu.c \
../Core/Src/FreeModbus/mbutils.c \
../Core/Src/FreeModbus/portevent.c \
../Core/Src/FreeModbus/portserial.c \
../Core/Src/FreeModbus/porttimer.c 

OBJS += \
./Core/Src/FreeModbus/mb.o \
./Core/Src/FreeModbus/mbcrc.o \
./Core/Src/FreeModbus/mbfunccoils.o \
./Core/Src/FreeModbus/mbfuncdiag.o \
./Core/Src/FreeModbus/mbfuncdisc.o \
./Core/Src/FreeModbus/mbfuncholding.o \
./Core/Src/FreeModbus/mbfuncinput.o \
./Core/Src/FreeModbus/mbfuncother.o \
./Core/Src/FreeModbus/mbrtu.o \
./Core/Src/FreeModbus/mbutils.o \
./Core/Src/FreeModbus/portevent.o \
./Core/Src/FreeModbus/portserial.o \
./Core/Src/FreeModbus/porttimer.o 

C_DEPS += \
./Core/Src/FreeModbus/mb.d \
./Core/Src/FreeModbus/mbcrc.d \
./Core/Src/FreeModbus/mbfunccoils.d \
./Core/Src/FreeModbus/mbfuncdiag.d \
./Core/Src/FreeModbus/mbfuncdisc.d \
./Core/Src/FreeModbus/mbfuncholding.d \
./Core/Src/FreeModbus/mbfuncinput.d \
./Core/Src/FreeModbus/mbfuncother.d \
./Core/Src/FreeModbus/mbrtu.d \
./Core/Src/FreeModbus/mbutils.d \
./Core/Src/FreeModbus/portevent.d \
./Core/Src/FreeModbus/portserial.d \
./Core/Src/FreeModbus/porttimer.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/FreeModbus/%.o Core/Src/FreeModbus/%.su Core/Src/FreeModbus/%.cyclo: ../Core/Src/FreeModbus/%.c Core/Src/FreeModbus/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/ASUS/Desktop/RAYBOT/SOURCE/DC_Driver_2/Core/Inc/FreeModbus" -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-FreeModbus

clean-Core-2f-Src-2f-FreeModbus:
	-$(RM) ./Core/Src/FreeModbus/mb.cyclo ./Core/Src/FreeModbus/mb.d ./Core/Src/FreeModbus/mb.o ./Core/Src/FreeModbus/mb.su ./Core/Src/FreeModbus/mbcrc.cyclo ./Core/Src/FreeModbus/mbcrc.d ./Core/Src/FreeModbus/mbcrc.o ./Core/Src/FreeModbus/mbcrc.su ./Core/Src/FreeModbus/mbfunccoils.cyclo ./Core/Src/FreeModbus/mbfunccoils.d ./Core/Src/FreeModbus/mbfunccoils.o ./Core/Src/FreeModbus/mbfunccoils.su ./Core/Src/FreeModbus/mbfuncdiag.cyclo ./Core/Src/FreeModbus/mbfuncdiag.d ./Core/Src/FreeModbus/mbfuncdiag.o ./Core/Src/FreeModbus/mbfuncdiag.su ./Core/Src/FreeModbus/mbfuncdisc.cyclo ./Core/Src/FreeModbus/mbfuncdisc.d ./Core/Src/FreeModbus/mbfuncdisc.o ./Core/Src/FreeModbus/mbfuncdisc.su ./Core/Src/FreeModbus/mbfuncholding.cyclo ./Core/Src/FreeModbus/mbfuncholding.d ./Core/Src/FreeModbus/mbfuncholding.o ./Core/Src/FreeModbus/mbfuncholding.su ./Core/Src/FreeModbus/mbfuncinput.cyclo ./Core/Src/FreeModbus/mbfuncinput.d ./Core/Src/FreeModbus/mbfuncinput.o ./Core/Src/FreeModbus/mbfuncinput.su ./Core/Src/FreeModbus/mbfuncother.cyclo ./Core/Src/FreeModbus/mbfuncother.d ./Core/Src/FreeModbus/mbfuncother.o ./Core/Src/FreeModbus/mbfuncother.su ./Core/Src/FreeModbus/mbrtu.cyclo ./Core/Src/FreeModbus/mbrtu.d ./Core/Src/FreeModbus/mbrtu.o ./Core/Src/FreeModbus/mbrtu.su ./Core/Src/FreeModbus/mbutils.cyclo ./Core/Src/FreeModbus/mbutils.d ./Core/Src/FreeModbus/mbutils.o ./Core/Src/FreeModbus/mbutils.su ./Core/Src/FreeModbus/portevent.cyclo ./Core/Src/FreeModbus/portevent.d ./Core/Src/FreeModbus/portevent.o ./Core/Src/FreeModbus/portevent.su ./Core/Src/FreeModbus/portserial.cyclo ./Core/Src/FreeModbus/portserial.d ./Core/Src/FreeModbus/portserial.o ./Core/Src/FreeModbus/portserial.su ./Core/Src/FreeModbus/porttimer.cyclo ./Core/Src/FreeModbus/porttimer.d ./Core/Src/FreeModbus/porttimer.o ./Core/Src/FreeModbus/porttimer.su

.PHONY: clean-Core-2f-Src-2f-FreeModbus

