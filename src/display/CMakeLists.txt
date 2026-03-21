add_library(display_lib STATIC
    st7789.c
)

target_include_directories(display_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}           # lv_conf.h
    ${CMAKE_SOURCE_DIR}/lib/lvgl  # lvgl.h
)

target_link_libraries(display_lib
    pico_stdlib
    hardware_spi
    hardware_gpio
    hardware_pwm
    lvgl
)
