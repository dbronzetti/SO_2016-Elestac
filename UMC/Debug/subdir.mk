################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../UMC.c 

OBJS += \
./UMC.o 

C_DEPS += \
./UMC.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/Documentos/Projects/SO_2016/Github/YoNoFui/libraries/commons-YoNoFui" -include"/home/utnso/Documentos/Projects/SO_2016/Github/YoNoFui/libraries/commons-YoNoFui/sockets.c" -O0 -g3 -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


