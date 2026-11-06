cmake_minimum_required(VERSION 3.21)


project(SimpleCommKitBleCentralExample)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)




# 添加测试可执行文件
add_executable(${PROJECT_NAME} main.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    SimpleCommKitBle
    SimpleCommKitUtil
)

if(SIMPLECOMMKIT_BLE_NATIVE AND CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_link_options(${PROJECT_NAME} PRIVATE
        "LINKER:-sectcreate,__TEXT,__info_plist,${CMAKE_CURRENT_SOURCE_DIR}/Info.plist")
    set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist")
endif()
