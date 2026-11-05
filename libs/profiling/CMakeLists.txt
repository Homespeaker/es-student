add_library(profiling src/profiling.c)
target_include_directories(profiling PUBLIC include)
target_link_libraries(profiling PRIVATE pico_stdlib)
