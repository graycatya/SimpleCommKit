cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(SimpleCommKitAiUdpClientFastmcpp LANGUAGES CXX)

# ============================================================================
# Build executable: simplecommkitaiudpclient-fastmcpp
# ============================================================================
add_executable(simplecommkitaiudpclient-fastmcpp
    src/main.cpp
    src/udp_state.cpp
    src/udp_state.hpp
    src/udp_tools.cpp
    src/udp_tools.hpp
)

target_include_directories(simplecommkitaiudpclient-fastmcpp PRIVATE src)

target_link_libraries(simplecommkitaiudpclient-fastmcpp PRIVATE
    SimpleCommKitUdp
    SimpleCommKitUtil
    fastmcpp_core
)

# ============================================================================
# Tests (optional)
# ============================================================================
option(SIMPLECOMMKITAIUDPCLIENTFASTMCPP_BUILD_TESTS "Build tests for SimpleCommKitAiUdpClientFastmcpp" OFF)

if(SIMPLECOMMKITAIUDPCLIENTFASTMCPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
