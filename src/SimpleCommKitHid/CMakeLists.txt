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

# hidapi is built as 3rdparty, targets are available from add_subdirectory
# Platform-specific linking:
#   - Linux: hidapi::hidraw (and optionally hidapi::libusb)
#   - Windows: hidapi::hidapi + hid (system HID library)
#   - macOS/iOS: hidapi::hidapi (already links IOKit/CoreFoundation)
if(NOT WIN32 AND NOT APPLE AND CMAKE_SYSTEM_NAME MATCHES "Linux")
    if(TARGET hidapi::hidraw)
        target_link_libraries(${PROJECT_NAME} PUBLIC hidapi::hidraw)
    endif()
    if(TARGET hidapi::libusb)
        target_link_libraries(${PROJECT_NAME} PUBLIC hidapi::libusb)
    endif()
elseif(WIN32)
    target_link_libraries(${PROJECT_NAME} PUBLIC hidapi::hidapi hid)
else()
    target_link_libraries(${PROJECT_NAME} PUBLIC hidapi::hidapi)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC
    SimpleCommKitUtil
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
