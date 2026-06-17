cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(SimpleCommKitAiHidFastmcpp LANGUAGES CXX)

# ============================================================================
# Build executable: simplecommkitaihid-fastmcpp
# ============================================================================
add_executable(simplecommkitaihid-fastmcpp
    src/main.cpp
    src/hid_state.cpp
    src/hid_state.hpp
    src/hid_tools.cpp
    src/hid_tools.hpp
)

target_include_directories(simplecommkitaihid-fastmcpp PRIVATE src)

target_link_libraries(simplecommkitaihid-fastmcpp PRIVATE
    SimpleCommKitHid
    SimpleCommKitUtil
    fastmcpp_core
)

# Platform-specific linkage (Windows needs hid.lib)
if(WIN32)
    target_link_libraries(simplecommkitaihid-fastmcpp PRIVATE hid)
endif()

# ============================================================================
# Tests (optional)
# ============================================================================
option(SIMPLECOMMKITAIHIDFASTMCPP_BUILD_TESTS "Build tests for SimpleCommKitAiHidFastmcpp" OFF)

if(SIMPLECOMMKITAIHIDFASTMCPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
