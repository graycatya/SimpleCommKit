set(PROJECT_NAMES "CSerialPort" 
                    "simpleble"
                    "hidapi"
                    "mongoose"
                    "libusb-cmake"
                    "libuv"
                    "openssl-cmake"
                    "uSockets")


set(CSerialPort_VERSION_TAG "4.3.3")
set(CSerialPort_URL "https://github.com/itas109/CSerialPort/archive/refs/tags/v${CSerialPort_VERSION_TAG}.zip")

set(simpleble_VERSION_TAG "0.12.1")
set(simpleble_URL "https://github.com/simpleble/simpleble/archive/refs/tags/v${simpleble_VERSION_TAG}.zip")

set(hidapi_VERSION_TAG "hidapi-0.15.0")
set(hidapi_URL "https://github.com/libusb/hidapi/archive/refs/tags/${hidapi_VERSION_TAG}.zip")

set(mongoose_VERSION_TAG "7.20")
set(mongoose_URL "https://github.com/cesanta/mongoose/archive/refs/tags/${mongoose_VERSION_TAG}.zip")

set(libusb-cmake_VERSION_TAG "1.0.29-0")
set(libusb-cmake_URL "https://github.com/libusb/libusb-cmake/archive/refs/tags/v${libusb-cmake_VERSION_TAG}.zip")

set(libuv_VERSION_TAG "1.52.0")
set(libuv_URL "https://github.com/libuv/libuv/archive/refs/tags/v${libuv_VERSION_TAG}.zip")

set(openssl-cmake_VERSION_TAG "1.1.1w-20250419")
set(openssl-cmake_URL "https://github.com/janbar/openssl-cmake/archive/refs/tags/${openssl-cmake_VERSION_TAG}.zip")

set(uSockets_VERSION_TAG "0.8.8")
set(uSockets_URL "https://github.com/uNetworking/uSockets/archive/refs/tags/v${uSockets_VERSION_TAG}.zip")


foreach(PROJ_NAME IN LISTS PROJECT_NAMES)
    set(URL_VAR "${PROJ_NAME}_URL")
    set(VERSION_VAR "${PROJ_NAME}_VERSION_TAG")
    

    if (NOT DEFINED ${URL_VAR} OR NOT DEFINED ${VERSION_VAR})
        message(WARNING "Project ${PROJ_NAME} has no URL or version configured, skipping!")
        continue()
    endif()


    set(PROJ_URL "${${URL_VAR}}")
    set(PROJ_VERSION "${${VERSION_VAR}}")


    message(STATUS "========================")
    message(STATUS "Project Name: ${PROJ_NAME}")
    message(STATUS "Download URL: ${PROJ_URL}")
    message(STATUS "Version: ${PROJ_VERSION}")
    download_and_unzip(
        PROJECT_NAME        "${PROJ_NAME}"
        PROJECT_VERSION     "${PROJ_VERSION}"
        DOWNLOAD_URL        "${PROJ_URL}"
        ZIP_SAVE_PATH      "${3RDPARTY_ZIP_PATH}/${PROJ_NAME}-${PROJ_VERSION}.zip"
        UNZIP_TARGET_DIR   "${3RDPARTY_PATH}"
        RENAME_TO           "${PROJ_NAME}" 
        DOWNLOAD_RETRY      3
        DOWNLOAD_TIMEOUT    600
    )
    message(STATUS "========================")
endforeach()