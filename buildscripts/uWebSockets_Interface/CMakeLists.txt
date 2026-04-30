cmake_minimum_required(VERSION 3.10)

set(LIB_PROJECT_VERSION "20.74.0")
set(LIB_PROJECT_NAME "uWebSockets")
project(${LIB_PROJECT_NAME}-${LIB_PROJECT_VERSION} LANGUAGES C)

file(GLOB_RECURSE PROJECT_HEADERS ${3RDPARTY_PATH}/${PROJECT_NAME}/src/*.h ${3RDPARTY_PATH}/${PROJECT_NAME}/src/*.hpp)

add_library(${LIB_PROJECT_NAME} INTERFACE ${PROJECT_HEADERS})
target_compile_definitions(${LIB_PROJECT_NAME} INTERFACE
    UWS_NO_ZLIB
    UWS_USE_LIBDEFLATE
    USE_OPENSSL
)
message("uWebSockets: " ${3RDPARTY_PATH}/${PROJECT_NAME})
target_include_directories(${LIB_PROJECT_NAME} INTERFACE ${3RDPARTY_PATH}/${PROJECT_NAME}/src)
target_link_libraries(${LIB_PROJECT_NAME} INTERFACE libdeflate_shared uSockets uv ssl crypto)
