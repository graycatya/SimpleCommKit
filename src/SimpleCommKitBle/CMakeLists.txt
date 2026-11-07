

project(SimpleCommKitBle)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(SIMPLECOMMKIT_BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS  ON)
endif()

set(HEADERS SimpleCommKitBleCentral.h)

if(ANDROID)
    if(ANDROID_PLATFORM_LEVEL AND ANDROID_PLATFORM_LEVEL LESS 23)
        message(FATAL_ERROR "Native Android BLE requires API 23 or newer")
    endif()
    set(BLE_PLATFORM_SOURCE src/backends/android/AndroidBackend.cpp)
    # Package these sources in the host App; CMake does not compile Kotlin.
    install(DIRECTORY android/ DESTINATION share/SimpleCommKitBle/android)
elseif(WIN32)
    if(NOT MSVC)
        message(FATAL_ERROR "Native Windows BLE requires MSVC and Windows SDK C++/WinRT headers")
    endif()
    set(BLE_PLATFORM_SOURCE src/backends/windows/WinrtBackend.cpp)
    set(BLE_PLATFORM_LIBRARIES windowsapp)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT ANDROID)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(DBUS REQUIRED IMPORTED_TARGET dbus-1>=1.12)
    find_package(Threads REQUIRED)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/BLE_BUILD_DEPENDENCIES.txt"
        "Backend: BlueZ D-Bus (experimental)\nlibdbus pkg-config version: ${DBUS_VERSION}\nLink libraries: ${DBUS_LINK_LIBRARIES}\nLicense review: see BLE_DEPENDENCIES.md; bundled copyright is the 1.16.2 reference package\n")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/BLE_BUILD_DEPENDENCIES.txt"
        DESTINATION share/licenses/SimpleCommKitBle)
    set(BLE_PLATFORM_SOURCE src/backends/linux/BluezBackend.cpp)
    set(BLE_PLATFORM_LIBRARIES PkgConfig::DBUS Threads::Threads)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    if(CMAKE_OSX_DEPLOYMENT_TARGET AND CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS "10.15")
        message(FATAL_ERROR "Native macOS BLE requires a deployment target of macOS 10.15 or newer")
    endif()
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 17)
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
    find_library(BLE_COREBLUETOOTH CoreBluetooth REQUIRED)
    find_library(BLE_FOUNDATION Foundation REQUIRED)
    set(BLE_PLATFORM_SOURCE src/backends/apple/CoreBluetoothBackend.mm)
    set_source_files_properties(${BLE_PLATFORM_SOURCE} PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
    set(BLE_PLATFORM_LIBRARIES ${BLE_COREBLUETOOTH} ${BLE_FOUNDATION})
else()
    message(FATAL_ERROR "Native BLE supports Linux, Windows, macOS and Android; iOS is pending")
endif()
set(PRIVATE_SOURCES
    src/backends/BleCentralNative.cpp
    src/backends/BleRuntime.cpp
    ${BLE_PLATFORM_SOURCE})
set(PRIVATE_HEADER src/backends/BleBackend.h src/backends/BleRuntime.h)

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

target_link_libraries(${PROJECT_NAME} PUBLIC SimpleCommKitUtil)
target_link_libraries(${PROJECT_NAME} PRIVATE ${BLE_PLATFORM_LIBRARIES})


target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)


if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
    target_compile_options(${PROJECT_NAME} PRIVATE /permissive- /EHsc /utf-8)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${HEADERS}
    DESTINATION include/${PROJECT_NAME}
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/../../doc/BLE_DEPENDENCIES.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../doc/licenses/dbus-1.16.2-debian-copyright"
        DESTINATION share/licenses/SimpleCommKitBle)
endif()
