# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD waveshare_rp2040_zero)
set(PICO_SDK_PATH "${USERHOME}/.pico-sdk/sdk/2.2.0")

include(pico_sdk_import.cmake)

project(signal_gen C CXX ASM)
set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

pico_sdk_init()

set(LV_CONF_PATH ${CMAKE_SOURCE_DIR}/lv_conf.h CACHE STRING "" FORCE)
set(LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LV_BUILD_DEMOS    OFF CACHE BOOL "" FORCE)
add_subdirectory(lib/lvgl)

add_subdirectory(src)

add_executable(signal_gen
    src/main.c
)

target_include_directories(signal_gen PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(signal_gen
    pico_stdlib
    pico_multicore
    hardware_pio
    hardware_dma
    hardware_spi
    hardware_i2c
    hardware_pwm
    hardware_gpio
    hardware_irq
    hardware_clocks
    lvgl
    encoder_lib
    i2s_lib
    audio_lib
    display_lib
    ui_lib
)

pico_add_extra_outputs(signal_gen)
pico_enable_stdio_usb(signal_gen 1)
pico_enable_stdio_uart(signal_gen 0)