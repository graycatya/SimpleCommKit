cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(tcp_utility_test
    test_utility.cpp
    ../src/tcp_state.cpp
)

target_include_directories(tcp_utility_test PRIVATE ../src)

add_test(NAME TcpUtilityTest COMMAND tcp_utility_test)
