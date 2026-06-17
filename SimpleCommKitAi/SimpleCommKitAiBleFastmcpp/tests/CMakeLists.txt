cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# Test: ble_utility_test (pure utility functions, no hardware)
# ============================================================================
add_executable(ble_utility_test
    test_utility.cpp
    ../src/ble_state.cpp
)

target_include_directories(ble_utility_test PRIVATE ../src)

add_test(NAME BleUtilityTest COMMAND ble_utility_test)
