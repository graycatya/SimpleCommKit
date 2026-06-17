cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(SimpleCommKitAiSerialPortFastmcpp LANGUAGES CXX)

# ============================================================================
# Build executable: simplecommkitaiserialport-fastmcpp
# ============================================================================
add_executable(simplecommkitaiserialport-fastmcpp
    src/main.cpp
    src/serial_state.cpp
    src/serial_state.hpp
    src/serial_tools.cpp
    src/serial_tools.hpp
)

target_include_directories(simplecommkitaiserialport-fastmcpp PRIVATE src)

target_link_libraries(simplecommkitaiserialport-fastmcpp PRIVATE
    SimpleCommKitSerialPort
    SimpleCommKitUtil
    fastmcpp_core
)

# ============================================================================
# Tests (optional)
# ============================================================================
option(SIMPLECOMMKITAISERIALPORTFASTMCPP_BUILD_TESTS "Build tests for SimpleCommKitAiSerialPortFastmcpp" OFF)

if(SIMPLECOMMKITAISERIALPORTFASTMCPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
