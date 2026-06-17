cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(SimpleCommKitAiBleFastmcpp LANGUAGES CXX)

# ============================================================================
# Build executable: simplecommkitaible-fastmcpp
# ============================================================================
add_executable(simplecommkitaible-fastmcpp
    src/main.cpp
    src/ble_state.cpp
    src/ble_state.hpp
    src/ble_tools.cpp
    src/ble_tools.hpp
)

target_include_directories(simplecommkitaible-fastmcpp PRIVATE src)

target_link_libraries(simplecommkitaible-fastmcpp PRIVATE
    SimpleCommKitBle
    SimpleCommKitUtil
    fastmcpp_core
)

# ============================================================================
# Tests (optional)
# ============================================================================
option(SIMPLECOMMKITAIBLEFASTMCPP_BUILD_TESTS "Build tests for SimpleCommKitAiBleFastmcpp" OFF)

if(SIMPLECOMMKITAIBLEFASTMCPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
