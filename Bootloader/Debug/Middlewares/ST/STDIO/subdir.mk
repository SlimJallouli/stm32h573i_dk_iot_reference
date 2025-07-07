################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middlewares/ST/STDIO/stdout_usart.c 

OBJS += \
./Middlewares/ST/STDIO/stdout_usart.o 

C_DEPS += \
./Middlewares/ST/STDIO/stdout_usart.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/ST/STDIO/%.o Middlewares/ST/STDIO/%.su Middlewares/ST/STDIO/%.cyclo: ../Middlewares/ST/STDIO/%.c Middlewares/ST/STDIO/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H573xx -c -I../Inc -I../Drivers/STM32H5xx_HAL_Driver/Inc -I../Drivers/STM32H5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H5xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middlewares-2f-ST-2f-STDIO

clean-Middlewares-2f-ST-2f-STDIO:
	-$(RM) ./Middlewares/ST/STDIO/stdout_usart.cyclo ./Middlewares/ST/STDIO/stdout_usart.d ./Middlewares/ST/STDIO/stdout_usart.o ./Middlewares/ST/STDIO/stdout_usart.su

.PHONY: clean-Middlewares-2f-ST-2f-STDIO

