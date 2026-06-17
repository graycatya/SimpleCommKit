cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# Test: hid_state_test
# ============================================================================
add_executable(hid_state_test
    test_hid_state.cpp
    ../src/hid_state.cpp
)

target_include_directories(hid_state_test PRIVATE ../src)
target_link_libraries(hid_state_test PRIVATE SimpleCommKitHid SimpleCommKitUtil)

add_test(NAME HidStateTest COMMAND hid_state_test)

# ============================================================================
# Test: hid_utility_test (pure utility functions, no hardware)
# ============================================================================
add_executable(hid_utility_test
    test_utility.cpp
    ../src/hid_state.cpp
)

target_include_directories(hid_utility_test PRIVATE ../src)

add_test(NAME HidUtilityTest COMMAND hid_utility_test)
