if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    message(FATAL_ERROR "SimpleCommKitBleIosExample requires an iOS toolchain")
endif()
if(SIMPLECOMMKIT_BUILD_SHARED_LIBS)
    message(FATAL_ERROR "The iOS test App uses static libraries; set SIMPLECOMMKIT_BUILD_SHARED_LIBS=OFF")
endif()
set_target_properties(SimpleCommKitBle SimpleCommKitUtil PROPERTIES XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO")
enable_language(OBJCXX)
find_library(BLE_UIKIT UIKit REQUIRED)
find_library(BLE_COREGRAPHICS CoreGraphics REQUIRED)
add_executable(SimpleCommKitBleIosExample MACOSX_BUNDLE main.mm)
set_target_properties(SimpleCommKitBleIosExample PROPERTIES
    OBJCXX_STANDARD 17 OBJCXX_STANDARD_REQUIRED ON
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.catgray.SimpleCommKit.BleProbe"
    MACOSX_BUNDLE_BUNDLE_NAME "SimpleCommKit BLE"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0" MACOSX_BUNDLE_BUNDLE_VERSION "1"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.catgray.SimpleCommKit.BleProbe"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
    XCODE_ATTRIBUTE_SUPPORTS_MACCATALYST "NO"
    XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
    XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES")
target_compile_options(SimpleCommKitBleIosExample PRIVATE -fobjc-arc -Wall -Wextra -Werror)
target_include_directories(SimpleCommKitBleIosExample PRIVATE ../SimpleCommKitBleCentralExample)
target_link_libraries(SimpleCommKitBleIosExample PRIVATE SimpleCommKitBle ${BLE_UIKIT} ${BLE_COREGRAPHICS})

set(SIMPLECOMMKIT_IOS_TEAM "" CACHE STRING "Development team for the iOS probe App")
set(SIMPLECOMMKIT_IOS_PROFILE "" CACHE STRING "Optional existing provisioning profile UUID/name")
if(SIMPLECOMMKIT_IOS_TEAM)
    set_target_properties(SimpleCommKitBleIosExample PROPERTIES
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${SIMPLECOMMKIT_IOS_TEAM}")
endif()
if(SIMPLECOMMKIT_IOS_PROFILE)
    if(NOT SIMPLECOMMKIT_IOS_TEAM)
        message(FATAL_ERROR "An iOS provisioning profile requires SIMPLECOMMKIT_IOS_TEAM")
    endif()
    set_target_properties(SimpleCommKitBleIosExample PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development"
        XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "${SIMPLECOMMKIT_IOS_PROFILE}")
endif()
