cmake_minimum_required(VERSION 3.20)
project(Zen_gdb VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Core Debugger Executable
add_executable(Zen_gdb
    src/main.cpp
    src/debugger.cpp
    src/debugger.hpp
    src/registers.cpp
    src/registers.hpp
)

target_include_directories(Zen_gdb PRIVATE src)
target_link_libraries(Zen_gdb PRIVATE psapi)

# Synthetic test target for safe analysis & testing
add_executable(dummy_target
    samples/dummy_target.cpp
)

# Output directories
set_target_properties(Zen_gdb dummy_target PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
