cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(SimpleCommKitAiUsbFastmcpp LANGUAGES CXX)

# ============================================================================
# Build executable: simplecommkitaiusb-fastmcpp
# ============================================================================
add_executable(simplecommkitaiusb-fastmcpp
    src/main.cpp
    src/usb_state.cpp
    src/usb_state.hpp
    src/usb_tools.cpp
    src/usb_tools.hpp
)

target_include_directories(simplecommkitaiusb-fastmcpp PRIVATE src)

target_link_libraries(simplecommkitaiusb-fastmcpp PRIVATE
    SimpleCommKitUsb
    SimpleCommKitUtil
    fastmcpp_core
)

# ============================================================================
# Tests (optional)
# ============================================================================
option(SIMPLECOMMKITAIUSBFASTMCPP_BUILD_TESTS "Build tests for SimpleCommKitAiUsbFastmcpp" OFF)

if(SIMPLECOMMKITAIUSBFASTMCPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
