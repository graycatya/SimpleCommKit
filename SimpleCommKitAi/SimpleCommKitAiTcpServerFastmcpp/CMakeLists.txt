cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(SimpleCommKitAiTcpServerFastmcpp LANGUAGES CXX)

# ============================================================================
# Build executable: simplecommkitaitcpserver-fastmcpp
# ============================================================================
add_executable(simplecommkitaitcpserver-fastmcpp
    src/main.cpp
    src/tcp_server_state.cpp
    src/tcp_server_state.hpp
    src/tcp_server_tools.cpp
    src/tcp_server_tools.hpp
)

target_include_directories(simplecommkitaitcpserver-fastmcpp PRIVATE src)

target_link_libraries(simplecommkitaitcpserver-fastmcpp PRIVATE
    SimpleCommKitTcp
    SimpleCommKitUtil
    fastmcpp_core
)

# ============================================================================
# Tests (optional)
# ============================================================================
option(SIMPLECOMMKITAITCPSERVERFASTMCPP_BUILD_TESTS "Build tests for SimpleCommKitAiTcpServerFastmcpp" OFF)

if(SIMPLECOMMKITAITCPSERVERFASTMCPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
