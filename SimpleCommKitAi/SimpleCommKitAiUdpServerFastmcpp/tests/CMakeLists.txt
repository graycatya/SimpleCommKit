cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(udp_server_utility_test
    test_utility.cpp
    ../src/udp_server_state.cpp
)

target_include_directories(udp_server_utility_test PRIVATE ../src)

add_test(NAME UdpServerUtilityTest COMMAND udp_server_utility_test)
