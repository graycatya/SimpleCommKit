project(SimpleCommKitHid)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(SIMPLECOMMKIT_BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS ON)
endif()

set(PRIVATE_SOURCES
    src/SimpleCommKitHid.cpp
    src/SimpleCommKitHid_p.cpp
)
set(PRIVATE_HEADER
    src/SimpleCommKitHid_p.h
)

set(HEADERS
    SimpleCommKitHid.h
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

# hidapi include path (assuming hidapi is provided via find_package or 3rdparty)
# Adjust HIDAPI_INCLUDE_DIR or use find_package(hidapi) as appropriate
find_package(hidapi QUIET)
if(hidapi_FOUND)
    target_link_libraries(${PROJECT_NAME} PUBLIC hidapi::hidapi)
else()
    # Fallback: try pkg-config style or assume it's in the system
    find_library(HIDAPI_LIBRARY hidapi)
    find_path(HIDAPI_INCLUDE_DIR hidapi.h)
    if(HIDAPI_LIBRARY AND HIDAPI_INCLUDE_DIR)
        target_include_directories(${PROJECT_NAME} PRIVATE ${HIDAPI_INCLUDE_DIR})
        target_link_libraries(${PROJECT_NAME} PUBLIC ${HIDAPI_LIBRARY})
    else()
        message(WARNING "hidapi not found – please set HIDAPI_INCLUDE_DIR and HIDAPI_LIBRARY")
    endif()
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC
    SimpleCommKitUtil
    hidapi
)



if(SIMPLECOMMKIT_BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SIMPLECOMMKIT_SHARED)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SIMPLECOMMKIT_EXPORTS)
endif()

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

install(FILES ${HEADERS}
    DESTINATION include/${PROJECT_NAME}
)
