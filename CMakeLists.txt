cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 禁用编译器特定的扩展（可选，推荐开启以保证跨平台兼容性）
set(CMAKE_CXX_EXTENSIONS OFF)
project(SimpleCommKit LANGUAGES C CXX)

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

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
option(SIMPLECOMMKIT_BUILD_SHARED_LIBS "SimpleCommKit Build shared libraries" OFF)
option(SIMPLECOMMKIT_EXAMPLES "SimpleCommKit Build all modules examples" OFF)
option(SIMPLECOMMKITBLE_EXAMPLES "SimpleCommKitBle Examples" OFF)
option(SIMPLECOMMKITUTIL_EXAMPLES "SimpleCommKitUtil Examples" OFF)
option(SIMPLECOMMKITSERIALPORT_EXAMPLES "SimpleCommKitSerialPort Examples" OFF)
option(ENABLE_SIMPLECOMMKITPYBIND "Enable SimpleCommKitPyBind" OFF)


option(ENABLE_PROXY "Enable proxy for network requests" OFF)
set(DEFAULT_HTTP_PROXY "" CACHE STRING "Default HTTP proxy URL")
set(DEFAULT_HTTPS_PROXY "" CACHE STRING "Default HTTPS proxy URL")
set(WITH_APPS OFF CACHE INTERNAL "openssl-cmake Not Build applications")
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

include(DownloadUnzipProject)
include(ExecuteDownloadProjects)


set(3RDPARTY_ZIPS
    simpleble
    libusb-cmake
    CSerialPort
    hidapi

    openssl-cmake
    libuv
    uSockets
    mongoose

)

if(ENABLE_SIMPLECOMMKITPYBIND)
    list(APPEND 3RDPARTY_ZIPS pybind11)
endif()

set(IS_CUSTOM_BUILDS_PROJECT
    uSockets
    mongoose
)

set(SRC_MODULES
    SimpleCommKitUtil
    SimpleCommKitBle
    SimpleCommKitSerialPort
)

if(ENABLE_SIMPLECOMMKITPYBIND)
    list(APPEND SRC_MODULES SimpleCommKitPyBle SimpleCommKitPySerialPort)
endif()




set(MODULE_EXAMPLES_MAPPING
    SimpleCommKitBle:SimpleCommKitBleCentralExample
    SimpleCommKitSerialPort:SimpleCommKitSerialPortExample
)


if(CMAKE_SYSTEM_NAME MATCHES "IOS")
    list(REMOVE_ITEM 3RDPARTY_ZIPS
        libusb-cmake
        CSerialPort
        hidapi
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

        # 构建对应的option变量名
        set(EXAMPLE_OPTION "${MODULE_NAME_UPPER}_EXAMPLES")

        # 检查examples目录是否存在
        if(${EXAMPLE_OPTION})
            set(EXAMPLE_PATH "${SIMPLECOMMKIT_EXAMPLES_PATH}/${MODULE_NAME}/${EXAMPLES_DIR}")
            if(EXISTS "${EXAMPLE_PATH}" AND IS_DIRECTORY "${EXAMPLE_PATH}")
                message(STATUS " Building ${EXAMPLES_DIR} example...")
                add_subdirectory(${EXAMPLE_PATH})
            else()
                message(STATUS " Skipping ${MODULE_NAME} examples (no examples directory)")
            endif()
        else()
            message(STATUS " Skipping ${MODULE_NAME} examples (${EXAMPLE_OPTION}=OFF)")
        endif()
    endforeach()
    message(STATUS "============================================================")
endif()


