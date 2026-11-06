cmake_minimum_required(VERSION 3.21)
project(SimpleCommKitBleContractTests LANGUAGES CXX)
enable_testing()
add_executable(ble_android_scan_tests test_android_scan.cpp)
target_include_directories(ble_android_scan_tests PRIVATE
    ../../src/SimpleCommKitBle/src/backends ../../src/SimpleCommKitBle ../../src/SimpleCommKitUtil)
target_compile_features(ble_android_scan_tests PRIVATE cxx_std_17)
add_test(NAME ble_android_scan_tests COMMAND ble_android_scan_tests)
find_package(Threads REQUIRED)
set(BLE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../src/SimpleCommKitBle")
set(UTIL_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../src/SimpleCommKitUtil")
add_executable(ble_contract_tests test_native.cpp
    "${BLE_ROOT}/src/backends/BleCentralNative.cpp"
    "${BLE_ROOT}/src/backends/BleRuntime.cpp"
    "${UTIL_ROOT}/SimpleCommKitErrorMap.cpp")
target_include_directories(ble_contract_tests PRIVATE "${BLE_ROOT}" "${BLE_ROOT}/src/backends" "${UTIL_ROOT}")
target_compile_features(ble_contract_tests PRIVATE cxx_std_17)
target_link_libraries(ble_contract_tests PRIVATE Threads::Threads)
if(MSVC)
    target_compile_options(ble_contract_tests PRIVATE /W4)
else()
    target_compile_options(ble_contract_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME ble_contract_tests COMMAND ble_contract_tests)
set_tests_properties(ble_contract_tests PROPERTIES TIMEOUT 20)

# Optional Linux integration: a private bus and Python service exercise real
# libdbus serialization/signals, without touching the system Bluetooth service.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(TEST_DBUS QUIET IMPORTED_TARGET dbus-1>=1.12)
    endif()
    find_program(DBUS_RUN_SESSION dbus-run-session)
    find_program(DBUS_TEST_PYTHON NAMES python3 PATHS /usr/bin NO_DEFAULT_PATH)
    if(DBUS_TEST_PYTHON)
        execute_process(COMMAND "${DBUS_TEST_PYTHON}" -c "import dbus; from gi.repository import GLib"
            RESULT_VARIABLE HAVE_DBUS_PYTHON OUTPUT_QUIET ERROR_QUIET)
    endif()
    if(TEST_DBUS_FOUND AND DBUS_RUN_SESSION AND DBUS_TEST_PYTHON AND HAVE_DBUS_PYTHON EQUAL 0)
        add_executable(ble_bluez_tests test_bluez.cpp
            "${BLE_ROOT}/src/backends/BleCentralNative.cpp"
            "${BLE_ROOT}/src/backends/BleRuntime.cpp"
            "${BLE_ROOT}/src/backends/linux/BluezBackend.cpp"
            "${UTIL_ROOT}/SimpleCommKitErrorMap.cpp")
        target_include_directories(ble_bluez_tests PRIVATE "${BLE_ROOT}" "${UTIL_ROOT}")
        target_compile_features(ble_bluez_tests PRIVATE cxx_std_17)
        target_link_libraries(ble_bluez_tests PRIVATE PkgConfig::TEST_DBUS Threads::Threads)
        add_test(NAME ble_bluez_tests COMMAND "${DBUS_RUN_SESSION}" -- "${DBUS_TEST_PYTHON}"
            "${CMAKE_CURRENT_LIST_DIR}/mock_bluez.py" "$<TARGET_FILE:ble_bluez_tests>")
        set_tests_properties(ble_bluez_tests PROPERTIES TIMEOUT 20)
        add_test(NAME ble_bluez_transient_device_tests COMMAND "${DBUS_RUN_SESSION}" -- "${DBUS_TEST_PYTHON}"
            "${CMAKE_CURRENT_LIST_DIR}/mock_bluez.py" "$<TARGET_FILE:ble_bluez_tests>")
        set_tests_properties(ble_bluez_transient_device_tests PROPERTIES TIMEOUT 20
            ENVIRONMENT "BLE_MOCK_TRANSIENT_DEVICE=1")
        add_test(NAME ble_bluez_unavailable_tests COMMAND ble_bluez_tests --unavailable)
        set_tests_properties(ble_bluez_unavailable_tests PROPERTIES TIMEOUT 5
            ENVIRONMENT "DBUS_SYSTEM_BUS_ADDRESS=unix:path=${CMAKE_CURRENT_BINARY_DIR}/nonexistent-bus")
    else()
        message(STATUS "Skipping private BlueZ integration tests: need libdbus, dbus-run-session, python dbus and gi")
    endif()
endif()

if(TARGET ble_bluez_tests AND TARGET _SimpleCommKitPyBle)
    add_test(NAME ble_python_tests COMMAND "${DBUS_RUN_SESSION}" -- "${DBUS_TEST_PYTHON}"
        "${CMAKE_CURRENT_LIST_DIR}/mock_bluez.py" "${DBUS_TEST_PYTHON}"
        "${CMAKE_CURRENT_LIST_DIR}/test_python.py" "$<TARGET_FILE_DIR:_SimpleCommKitPyBle>")
    set_tests_properties(ble_python_tests PROPERTIES TIMEOUT 20)
endif()

if(WIN32 AND MSVC)
    add_executable(ble_winrt_tests test_winrt.cpp
        "${BLE_ROOT}/src/backends/BleRuntime.cpp"
        "${BLE_ROOT}/src/backends/windows/WinrtBackend.cpp")
    target_include_directories(ble_winrt_tests PRIVATE "${BLE_ROOT}" "${BLE_ROOT}/src/backends" "${UTIL_ROOT}")
    target_compile_features(ble_winrt_tests PRIVATE cxx_std_17)
    target_compile_options(ble_winrt_tests PRIVATE /W4 /permissive- /EHsc /utf-8)
    target_link_libraries(ble_winrt_tests PRIVATE windowsapp)
    add_test(NAME ble_winrt_tests COMMAND ble_winrt_tests)
    set_tests_properties(ble_winrt_tests PROPERTIES TIMEOUT 10)
    # Hardware probes: --scan or --connect <MAC> (no business-data writes/unpair).
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    enable_language(OBJCXX)
    add_executable(ble_corebluetooth_tests test_corebluetooth.mm
        "${BLE_ROOT}/src/backends/BleRuntime.cpp")
    set_target_properties(ble_corebluetooth_tests PROPERTIES OBJCXX_STANDARD 17 OBJCXX_STANDARD_REQUIRED ON)
    target_include_directories(ble_corebluetooth_tests PRIVATE "${BLE_ROOT}" "${UTIL_ROOT}")
    target_compile_features(ble_corebluetooth_tests PRIVATE cxx_std_17)
    target_compile_options(ble_corebluetooth_tests PRIVATE -Wall -Wextra -Werror "$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>")
    target_link_libraries(ble_corebluetooth_tests PRIVATE "-framework CoreBluetooth" "-framework Foundation" Threads::Threads)
    target_link_options(ble_corebluetooth_tests PRIVATE "LINKER:-sectcreate,__TEXT,__info_plist,${CMAKE_CURRENT_LIST_DIR}/macos-Info.plist")
    set_property(TARGET ble_corebluetooth_tests APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/macos-Info.plist")
    add_test(NAME ble_corebluetooth_tests COMMAND ble_corebluetooth_tests)
    set_tests_properties(ble_corebluetooth_tests PROPERTIES TIMEOUT 15)
    # Opt-in hardware scan: ble_corebluetooth_tests --scan
endif()
