cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(SimpleCommKitAiWebSocketServerFastmcpp LANGUAGES CXX)

# ============================================================================
# Build executable: simplecommkitaiwebsocketserver-fastmcpp
# ============================================================================
add_executable(simplecommkitaiwebsocketserver-fastmcpp
    src/main.cpp
    src/ws_server_state.cpp
    src/ws_server_state.hpp
    src/ws_server_tools.cpp
    src/ws_server_tools.hpp
)

target_include_directories(simplecommkitaiwebsocketserver-fastmcpp PRIVATE src)

target_link_libraries(simplecommkitaiwebsocketserver-fastmcpp PRIVATE
    SimpleCommKitWebSocket
    SimpleCommKitUtil
    fastmcpp_core
)

# ============================================================================
# Tests (optional)
# ============================================================================
option(SIMPLECOMMKITAIWEBSOCKETSERVERFASTMCPP_BUILD_TESTS "Build tests for SimpleCommKitAiWebSocketServerFastmcpp" OFF)

if(SIMPLECOMMKITAIWEBSOCKETSERVERFASTMCPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
