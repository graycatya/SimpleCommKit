cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(SimpleCommKitAiUdpServerFastmcpp LANGUAGES CXX)

# ============================================================================
# Build executable: simplecommkitaiupdserver-fastmcpp
# ============================================================================
add_executable(simplecommkitaiupdserver-fastmcpp
    src/main.cpp
    src/udp_server_state.cpp
    src/udp_server_state.hpp
    src/udp_server_tools.cpp
    src/udp_server_tools.hpp
)

target_include_directories(simplecommkitaiupdserver-fastmcpp PRIVATE src)

target_link_libraries(simplecommkitaiupdserver-fastmcpp PRIVATE
    SimpleCommKitUdp
    SimpleCommKitUtil
    fastmcpp_core
)

# ============================================================================
# Tests (optional)
# ============================================================================
option(SIMPLECOMMKITAIUDPSERVERFASTMCPP_BUILD_TESTS "Build tests for SimpleCommKitAiUdpServerFastmcpp" OFF)

if(SIMPLECOMMKITAIUDPSERVERFASTMCPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
