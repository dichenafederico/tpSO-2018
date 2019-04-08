################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/algoritmos.c \
../src/consola.c \
../src/globales.c \
../src/planificador.c 

OBJS += \
./src/algoritmos.o \
./src/consola.o \
./src/globales.o \
./src/planificador.o 

C_DEPS += \
./src/algoritmos.d \
./src/consola.d \
./src/globales.d \
./src/planificador.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/tp-2018-1c--0/biblioteca-propia" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


