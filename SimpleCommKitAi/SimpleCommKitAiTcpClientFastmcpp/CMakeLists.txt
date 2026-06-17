cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(SimpleCommKitAiTcpClientFastmcpp LANGUAGES CXX)

# ============================================================================
# Build executable: simplecommkitaitcpclient-fastmcpp
# ============================================================================
add_executable(simplecommkitaitcpclient-fastmcpp
    src/main.cpp
    src/tcp_state.cpp
    src/tcp_state.hpp
    src/tcp_tools.cpp
    src/tcp_tools.hpp
)

target_include_directories(simplecommkitaitcpclient-fastmcpp PRIVATE src)

target_link_libraries(simplecommkitaitcpclient-fastmcpp PRIVATE
    SimpleCommKitTcp
    SimpleCommKitUtil
    fastmcpp_core
)

# ============================================================================
# Tests (optional)
# ============================================================================
option(SIMPLECOMMKITAITCPCLIENTFASTMCPP_BUILD_TESTS "Build tests for SimpleCommKitAiTcpClientFastmcpp" OFF)

if(SIMPLECOMMKITAITCPCLIENTFASTMCPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
