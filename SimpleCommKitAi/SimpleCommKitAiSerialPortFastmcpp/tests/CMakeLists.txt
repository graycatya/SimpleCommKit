cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# Test: serial_utility_test (pure utility functions, no hardware)
# ============================================================================
add_executable(serial_utility_test
    test_utility.cpp
    ../src/serial_state.cpp
)

target_include_directories(serial_utility_test PRIVATE ../src)

add_test(NAME SerialUtilityTest COMMAND serial_utility_test)
