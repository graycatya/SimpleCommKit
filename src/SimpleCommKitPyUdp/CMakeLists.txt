
project(SimpleCommKitPyUdp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(SIMPLECOMMKIT_BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS  ON)
endif()

set(PRIVATE_SOURCES
    src/main.cpp
)

set(HEADERS
)

pybind11_add_module(_${PROJECT_NAME}
    ${PRIVATE_SOURCES}
    ${HEADERS}
)

target_include_directories(_${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(_${PROJECT_NAME} PUBLIC
    SimpleCommKit::SimpleCommKitUdp
    SimpleCommKitUtil
)

target_compile_features(_${PROJECT_NAME} PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(_${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(_${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

set_target_properties(_${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/_${PROJECT_NAME}"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/_${PROJECT_NAME}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/_${PROJECT_NAME}"
)
