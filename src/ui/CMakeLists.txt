add_library(ui_lib STATIC
    ui.c
)

target_include_directories(ui_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/lib/lvgl
)

target_link_libraries(ui_lib
    pico_stdlib
    lvgl
    display_lib
    audio_lib
    encoder_lib
)
