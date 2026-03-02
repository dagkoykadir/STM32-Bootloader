cmake_minimum_required(VERSION 3.15)
project(USER_BOOTLOADER C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# MCU ve toolchain ayarları
set(CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/STM32F407VGTx_FLASH.ld") # Linker script yolunu güncelleyin

set(CMAKE_C_FLAGS "${CPU_FLAGS} -Wall -fdata-sections -ffunction-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CPU_FLAGS} -T${LINKER_SCRIPT} -Wl,--gc-sections")

# Kaynak dosyalarını ekleyin
file(GLOB SRC
    "Core/Src/*.c"
    "Core/Startup/*.s"
    "Core/Inc/*.h"
    "Bootloader/*.c"
    "Bootloader/*.h"
)

add_executable(${PROJECT_NAME}.elf ${SRC})

# STM32 HAL ve CMSIS dosyalarını ekleyin (yolunu kendi projenize göre ayarlayın)
include_directories(
    ${CMAKE_SOURCE_DIR}/Core/Inc
    ${CMAKE_SOURCE_DIR}/Bootloader
    ${CMAKE_SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include
)

# OpenOCD ile yükleme için hedef ekleyin (isteğe bağlı)
add_custom_target(flash
    COMMAND openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program ${PROJECT_NAME}.elf verify reset exit"
    DEPENDS ${PROJECT_NAME}.elf
)
