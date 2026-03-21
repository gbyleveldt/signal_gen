add_library(encoder_lib STATIC
    encoder.c
)

target_include_directories(encoder_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(encoder_lib
    pico_stdlib
    hardware_gpio
    hardware_sync
)
