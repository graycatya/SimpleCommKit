cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(SimpleCommKitAiMqttClientFastmcpp LANGUAGES CXX)

# ============================================================================
# Build executable: simplecommkitaimqttclient-fastmcpp
# ============================================================================
add_executable(simplecommkitaimqttclient-fastmcpp
    src/main.cpp
    src/mqtt_state.cpp
    src/mqtt_state.hpp
    src/mqtt_tools.cpp
    src/mqtt_tools.hpp
)

target_include_directories(simplecommkitaimqttclient-fastmcpp PRIVATE src)

target_link_libraries(simplecommkitaimqttclient-fastmcpp PRIVATE
    SimpleCommKitMqttClient
    SimpleCommKitUtil
    fastmcpp_core
)

# ============================================================================
# Tests (optional)
# ============================================================================
option(SIMPLECOMMKITAIMQTTCLIENTFASTMCPP_BUILD_TESTS "Build tests for SimpleCommKitAiMqttClientFastmcpp" OFF)

if(SIMPLECOMMKITAIMQTTCLIENTFASTMCPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
