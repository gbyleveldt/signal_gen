add_library(audio_lib STATIC
    audio_engine.c
)

target_include_directories(audio_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(audio_lib
    pico_stdlib
    hardware_sync
    i2s_lib
)
