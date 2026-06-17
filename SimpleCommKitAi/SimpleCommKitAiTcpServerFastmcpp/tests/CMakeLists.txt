cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(tcp_server_utility_test
    test_utility.cpp
    ../src/tcp_server_state.cpp
)

target_include_directories(tcp_server_utility_test PRIVATE ../src)

add_test(NAME TcpServerUtilityTest COMMAND tcp_server_utility_test)
