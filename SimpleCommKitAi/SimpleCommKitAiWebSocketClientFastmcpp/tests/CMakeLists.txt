cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# Test: ws_utility_test (pure utility functions, no hardware)
# ============================================================================
add_executable(ws_utility_test
    test_utility.cpp
    ../src/ws_state.cpp
)

target_include_directories(ws_utility_test PRIVATE ../src)

add_test(NAME WsUtilityTest COMMAND ws_utility_test)
