cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(SimpleCommKitAiWebSocketClientFastmcpp LANGUAGES CXX)

# ============================================================================
# Build executable: simplecommkitaiwebsocketclient-fastmcpp
# ============================================================================
add_executable(simplecommkitaiwebsocketclient-fastmcpp
    src/main.cpp
    src/ws_state.cpp
    src/ws_state.hpp
    src/ws_tools.cpp
    src/ws_tools.hpp
)

target_include_directories(simplecommkitaiwebsocketclient-fastmcpp PRIVATE src)

target_link_libraries(simplecommkitaiwebsocketclient-fastmcpp PRIVATE
    SimpleCommKitWebSocket
    SimpleCommKitUtil
    fastmcpp_core
)

# ============================================================================
# Tests (optional)
# ============================================================================
option(SIMPLECOMMKITAIWEBSOCKETCLIENTFASTMCPP_BUILD_TESTS "Build tests for SimpleCommKitAiWebSocketClientFastmcpp" OFF)

if(SIMPLECOMMKITAIWEBSOCKETCLIENTFASTMCPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
