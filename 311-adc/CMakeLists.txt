cmake_minimum_required(VERSION 3.15)
set(PICO_BOARD pico)
set(PICO_PLATFORM rp2040)
include(pico_sdk_import.cmake)
project(311_adc)
pico_sdk_init()
set(CMAKE_C_STANDARD 11)
option(PROFILING_ENABLED "Enable stopwatches" ON)
add_compile_options(-Wall -Wextra)
add_subdirectory(../libs ${CMAKE_BINARY_DIR}/libs)
target_compile_definitions(profiling PUBLIC PROFILING_ENABLED=$<BOOL:${PROFILING_ENABLED}>)
add_library(platform platform/platform.c)
target_include_directories(platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(platform PRIVATE pico_stdlib hardware_adc hardware_dma hardware_pwm hardware_irq hardware_watchdog hardware_clocks)
add_library(logic
    logic/api/api-callbacks.c
    logic/api/api.c
    logic/button-task/button-task.c
    logic/led-task/led-task.c
    logic/pi-task/pi-task.c
    logic/telemetry-task/telemetry-task.c
)
target_include_directories(logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/logic)
target_link_libraries(logic PUBLIC stdio_text_protocol PRIVATE platform systime device profiling logging m)
add_executable(${PROJECT_NAME} main.c)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE logic platform systime profiling logging pico_stdlib)
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_set_linker_script(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/memmap_rp2040.ld)
pico_add_extra_outputs(${PROJECT_NAME})
