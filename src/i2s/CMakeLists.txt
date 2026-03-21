add_library(i2s_lib STATIC
    i2s.c
)

# Generate the PIO header from i2s_tx.pio
pico_generate_pio_header(i2s_lib ${CMAKE_CURRENT_LIST_DIR}/i2s_tx.pio)

target_include_directories(i2s_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src       # for config.h
)

target_link_libraries(i2s_lib
    pico_stdlib
    hardware_pio
    hardware_dma
    hardware_clocks
)
