cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 禁用编译器特定的扩展（可选，推荐开启以保证跨平台兼容性）
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# 统一版本号管理 — 读取根目录 VERSION 文件作为唯一版本来源
# ============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" SIMPLECOMMKIT_VERSION)
    string(STRIP "${SIMPLECOMMKIT_VERSION}" SIMPLECOMMKIT_VERSION)
    message(STATUS "SimpleCommKit version: ${SIMPLECOMMKIT_VERSION}")
else()
    set(SIMPLECOMMKIT_VERSION "0.0.0")
    message(WARNING "VERSION file not found, using default: ${SIMPLECOMMKIT_VERSION}")
endif()

project(SimpleCommKit VERSION ${SIMPLECOMMKIT_VERSION} LANGUAGES C CXX)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
elseif (UNIX)
add_compile_options(-Wall -fPIC -Wno-stringop-overflow)
endif()

message(STATUS "------------------------------------------------------------" )
message(STATUS "[${PROJECT_NAME}] Configuration summary."                     )
message(STATUS "------------------------------------------------------------ ")
message(STATUS " System configuration:"                                       )
message(STATUS " .. Processor type .............. = ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS " .. CMake executable ............ = ${CMAKE_COMMAND}"         )
message(STATUS " .. CMake version ............... = ${CMAKE_VERSION}"         )
message(STATUS " .. System name ................. = ${CMAKE_SYSTEM}"          )
message(STATUS " .. C++ compiler ................ = ${CMAKE_CXX_COMPILER}"    )
message(STATUS " .. C compiler .................. = ${CMAKE_C_COMPILER}"      )
message(STATUS " .. size(void*) ................. = ${CMAKE_SIZEOF_VOID_P}"   )
message(STATUS " .. cmake build type ................. = ${CMAKE_BUILD_TYPE}")
message(STATUS " .. cmake current source ................. = ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS " .. cmake system name ................. = ${CMAKE_SYSTEM_NAME}")
message(STATUS "------------------------------------------------------------ ")

# Prepend so custom FindOpenSSL.cmake is found before system's
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
option(SIMPLECOMMKIT_BUILD_SHARED_LIBS "SimpleCommKit Build shared libraries" OFF)
option(SIMPLECOMMKIT_EXAMPLES "SimpleCommKit Build all modules examples" OFF)
option(ENABLE_SIMPLECOMMKITPYBIND "Enable SimpleCommKitPyBind" OFF)
option(ENABLE_SIMPLECOMMKITAI_FASTMCPP "Enable SimpleCommKitAiFastmcpp" OFF)

# Enable MQTT in libhv (off by default in libhv's CMakeLists.txt)
set(WITH_MQTT ON CACHE BOOL "Enable MQTT protocol in libhv" FORCE)


option(ENABLE_PROXY "Enable proxy for network requests" OFF)
set(DEFAULT_HTTP_PROXY "" CACHE STRING "Default HTTP proxy URL")
set(DEFAULT_HTTPS_PROXY "" CACHE STRING "Default HTTPS proxy URL")
set(BUILD_SHARED_LIBS  OFF CACHE INTERNAL "3rdparty libraries build type")


if(ENABLE_PROXY)
    if(DEFINED HTTP_PROXY OR DEFINED HTTPS_PROXY)
        if(DEFINED HTTP_PROXY)
            set(ENV{HTTP_PROXY} ${HTTP_PROXY})
            message(STATUS "Using HTTP proxy from CMake variable: ${HTTP_PROXY}")
        endif()
        if(DEFINED HTTPS_PROXY)
            set(ENV{HTTPS_PROXY} ${HTTPS_PROXY})
            message(STATUS "Using HTTPS proxy from CMake variable: ${HTTPS_PROXY}")
        endif()
    elseif(DEFINED ENV{HTTP_PROXY} OR DEFINED ENV{HTTPS_PROXY})
        if(DEFINED ENV{HTTP_PROXY})
            set(ENV{HTTP_PROXY} $ENV{HTTP_PROXY})
            message(STATUS "Using HTTP proxy from environment: $ENV{HTTP_PROXY}")
        endif()
        if(DEFINED ENV{HTTPS_PROXY})
            set(ENV{HTTPS_PROXY} $ENV{HTTPS_PROXY})
            message(STATUS "Using HTTPS proxy from environment: $ENV{HTTPS_PROXY}")
        endif()
    else()
        set(ENV{HTTP_PROXY} ${DEFAULT_HTTP_PROXY})
        set(ENV{HTTPS_PROXY} ${DEFAULT_HTTPS_PROXY})
        message(STATUS "Using default proxies: HTTP=${DEFAULT_HTTP_PROXY}, HTTPS=${DEFAULT_HTTPS_PROXY}")
    endif()
else()
    message(STATUS "Proxy disabled")
endif()

set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "Installation prefix")

set(3RDPARTY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)
set(SRC_MODULES_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(3RDPARTY_ZIP_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty-download)
set(BUILDSCRIPTS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/buildscripts)
set(SIMPLECOMMKIT_INSTALL_DIR ${CMAKE_BINARY_DIR}/install) 
set(SIMPLECOMMKIT_EXAMPLES_PATH ${CMAKE_CURRENT_SOURCE_DIR}/examples)
set(SIMPLECOMMKITAI_FASTMCPP_PATH ${CMAKE_CURRENT_SOURCE_DIR}/SimpleCommKitAi)

include(DownloadUnzipProject)
include(ExecuteDownloadProjects)



set(3RDPARTY_ZIPS
    simpleble
    libusb-cmake
    CSerialPort
    hidapi
    libhv
    fastmcpp
)

if(ENABLE_SIMPLECOMMKITPYBIND)
    list(APPEND 3RDPARTY_ZIPS pybind11)
endif()

set(IS_CUSTOM_BUILDS_PROJECT
  
)

# ============================================================================
# 从 VERSION 文件生成 C++ 版本头文件
# ============================================================================
configure_file(
    "${SRC_MODULES_PATH}/SimpleCommKitUtil/SimpleCommKitVersion.h.in"
    "${CMAKE_BINARY_DIR}/generated/SimpleCommKitVersion.h"
    @ONLY
)
# 将生成目录添加到 include 路径，所有模块均可 #include <SimpleCommKitVersion.h>
include_directories("${CMAKE_BINARY_DIR}/generated")

set(SRC_MODULES
    SimpleCommKitUtil
    SimpleCommKitBle
    SimpleCommKitSerialPort
    SimpleCommKitHid
    SimpleCommKitUsb
    SimpleCommKitTcp
    SimpleCommKitUdp
    SimpleCommKitWebSocket
    SimpleCommKitMqttClient
)

if(ENABLE_SIMPLECOMMKITPYBIND)
    list(APPEND SRC_MODULES SimpleCommKitPyBle SimpleCommKitPySerialPort SimpleCommKitPyHid SimpleCommKitPyUsb SimpleCommKitPyTcp SimpleCommKitPyUdp SimpleCommKitPyWebSocket SimpleCommKitPyMqttClient)
endif()


set(SIMPLECOMMKITAI_FASTMCPP_MAPPING
    SimpleCommKitHid:SimpleCommKitAiHidFastmcpp
    SimpleCommKitBle:SimpleCommKitAiBleFastmcpp
    SimpleCommKitMqttClient:SimpleCommKitAiMqttClientFastmcpp
    SimpleCommKitSerialPort:SimpleCommKitAiSerialPortFastmcpp
    SimpleCommKitTcp:SimpleCommKitAiTcpClientFastmcpp
    SimpleCommKitTcp:SimpleCommKitAiTcpServerFastmcpp
    SimpleCommKitUdp:SimpleCommKitAiUdpClientFastmcpp
    SimpleCommKitUdp:SimpleCommKitAiUdpServerFastmcpp
    SimpleCommKitUsb:SimpleCommKitAiUsbFastmcpp
    SimpleCommKitWebSocket:SimpleCommKitAiWebSocketClientFastmcpp
    SimpleCommKitWebSocket:SimpleCommKitAiWebSocketServerFastmcpp
)

set(MODULE_EXAMPLES_MAPPING
    SimpleCommKitBle:SimpleCommKitBleCentralExample
    SimpleCommKitSerialPort:SimpleCommKitSerialPortExample
    SimpleCommKitHid:SimpleCommKitHidExample
    SimpleCommKitUsb:SimpleCommKitUsbExample
    SimpleCommKitTcp:SimpleCommKitTcpClientExample
    SimpleCommKitTcp:SimpleCommKitTcpServerExample
    SimpleCommKitUdp:SimpleCommKitUdpClientExample
    SimpleCommKitUdp:SimpleCommKitUdpServerExample
    SimpleCommKitWebSocket:SimpleCommKitWebSocketClientExample
    SimpleCommKitWebSocket:SimpleCommKitWebSocketServerExample
    SimpleCommKitMqttClient:SimpleCommKitMqttClientExample
)


if(CMAKE_SYSTEM_NAME MATCHES "IOS")
    list(REMOVE_ITEM 3RDPARTY_ZIPS
        libusb-cmake
        CSerialPort
        hidapi
    )
    list(REMOVE_ITEM ENABLE_SIMPLECOMMKITPYBIND
        SimpleCommKitPySerialPort
        SimpleCommKitPyHid
    )
endif()


foreach(3RDPARTY ${3RDPARTY_ZIPS})
    if("${3RDPARTY}" IN_LIST IS_CUSTOM_BUILDS_PROJECT)
        add_subdirectory(${BUILDSCRIPTS_PATH}/${3RDPARTY})
    elseif("${3RDPARTY}" STREQUAL "simpleble")
        include(${3RDPARTY_PATH}/simpleble/cmake/prelude.cmake)
        include(${3RDPARTY_PATH}/simpleble/cmake/epilogue.cmake)
        add_subdirectory(${3RDPARTY_PATH}/simpleble/simpleble)
    else()
        add_subdirectory(${3RDPARTY_PATH}/${3RDPARTY})
    endif()
endforeach(3RDPARTY)

foreach(SRC_MODULE ${SRC_MODULES})
    add_subdirectory(${SRC_MODULES_PATH}/${SRC_MODULE})
endforeach(SRC_MODULE ${SRC_MODULES})


# ============================================================
# Examples Building
# ============================================================
if(SIMPLECOMMKIT_EXAMPLES)
    message(STATUS "============================================================")
    message(STATUS "Building all module examples (SIMPLECOMMKIT_EXAMPLES=ON)")

    foreach(MAPPING ${MODULE_EXAMPLES_MAPPING})
      # 解析模块名和examples目录名
      string(REPLACE ":" ";" MAPPING_LIST ${MAPPING})
      list(GET MAPPING_LIST 0 MODULE_NAME)
      list(GET MAPPING_LIST 1 EXAMPLES_DIR)
      string(TOUPPER ${MODULE_NAME} MODULE_NAME_UPPER)


      set(EXAMPLE_PATH "${SIMPLECOMMKIT_EXAMPLES_PATH}/${MODULE_NAME}/${EXAMPLES_DIR}")
      if(EXISTS "${EXAMPLE_PATH}" AND IS_DIRECTORY "${EXAMPLE_PATH}")
          message(STATUS " Building ${EXAMPLES_DIR} example...")
          add_subdirectory(${EXAMPLE_PATH})
      else()
          message(STATUS " Skipping ${MODULE_NAME} examples (no examples directory)")
      endif()

    endforeach()
    message(STATUS "============================================================")
endif()


if(ENABLE_SIMPLECOMMKITAI_FASTMCPP)
    message(STATUS "============================================================")
    message(STATUS "Building all simplecommkitai fastmcpp(ENABLE_SIMPLECOMMKITAI_FASTMCPP=ON)")
        foreach(MAPPING ${SIMPLECOMMKITAI_FASTMCPP_MAPPING})

        string(REPLACE ":" ";" MAPPING_LIST ${MAPPING})
        list(GET MAPPING_LIST 0 MODULE_NAME)
        list(GET MAPPING_LIST 1 MODULE_DIR)
        string(TOUPPER ${MODULE_NAME} MODULE_NAME_UPPER)

        set(MODULE_PATH "${SIMPLECOMMKITAI_FASTMCPP_PATH}/${MODULE_DIR}")
        if(EXISTS "${MODULE_PATH}" AND IS_DIRECTORY "${MODULE_PATH}")
            message(STATUS " Building ${MODULE_DIR} fastmcpp...")
            add_subdirectory(${MODULE_PATH})
        else()
            message(STATUS " Skipping ${MODULE_DIR} fastmcpp (no fastmcpp directory)")
        endif()

    endforeach()
    message(STATUS "============================================================")
endif()

