################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Application/log.c \
../Application/temperature.c \
../Application/w25q128.c 

OBJS += \
./Application/log.o \
./Application/temperature.o \
./Application/w25q128.o 

C_DEPS += \
./Application/log.d \
./Application/temperature.d \
./Application/w25q128.d 


# Each subdirectory must supply rules for building sources it contributes
Application/%.o Application/%.su Application/%.cyclo: ../Application/%.c Application/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"C:/workspace/Develop/ST/workspace/stm32f407vet6-fw/Application" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Application

clean-Application:
	-$(RM) ./Application/log.cyclo ./Application/log.d ./Application/log.o ./Application/log.su ./Application/temperature.cyclo ./Application/temperature.d ./Application/temperature.o ./Application/temperature.su ./Application/w25q128.cyclo ./Application/w25q128.d ./Application/w25q128.o ./Application/w25q128.su

.PHONY: clean-Application

