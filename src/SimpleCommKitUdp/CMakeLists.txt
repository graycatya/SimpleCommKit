project(SimpleCommKitUdp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(SIMPLECOMMKIT_BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS ON)
endif()

set(PRIVATE_SOURCES
    src/SimpleCommKitUdpClient.cpp
    src/SimpleCommKitUdpClient_p.cpp
    src/SimpleCommKitUdpServer.cpp
    src/SimpleCommKitUdpServer_p.cpp
)
set(PRIVATE_HEADER
    src/SimpleCommKitUdpClient_p.h
    src/SimpleCommKitUdpServer_p.h
)

set(HEADERS
    SimpleCommKitUdp.h
)

add_library(${PROJECT_NAME}
    ${PRIVATE_HEADER}
    ${PRIVATE_SOURCES}
    ${HEADERS}
)
add_library(SimpleCommKit::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(SIMPLECOMMKIT_BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SIMPLECOMMKIT_SHARED)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SIMPLECOMMKIT_EXPORTS)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC
    hv_static SimpleCommKitUtil
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

install(FILES ${HEADERS}
    DESTINATION include/${PROJECT_NAME}
)
