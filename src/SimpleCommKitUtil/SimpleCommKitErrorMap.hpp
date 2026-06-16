#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <system_error>

namespace SimpleCommKit {

// Define the base type for error codes
using ErrorCode = uint32_t;

// Module domain (8 bits)
enum class ModuleID : uint8_t {
    SimpleCommKitBle = 0,         // Bluetooth module
    SimpleCommKitSerialPort = 1,  // Serial Port module
    SimpleCommKitHid = 2,         // HID module
    SimpleCommKitUsb = 3,         // USB module
    SimpleCommKitTcp = 4,         // TCP module
    SimpleCommKitUdp = 5,         // UDP module
    SimpleCommKitWebSocket = 6,   // WebSocket module
    SimpleCommKitMqtt = 7,         // MQTT module
    SimpleCommKitMaxModule = 0xFF
};

// Error type domain (8 bits)
enum class ErrorType : uint8_t {
    SimpleCommKitSuccess = 0,  // Operation succeeded
    SimpleCommKitError = 1,
    SimpleCommKitMaxType       = 0xFF
};

// Macro to construct error code (concatenate each field)
#define MAKE_ERROR_CODE(module, type, sub_type, code) \
    (static_cast<ErrorCode>( \
        (static_cast<uint32_t>(module) << 24) | \
        (static_cast<uint32_t>(type) << 16) | \
        (static_cast<uint32_t>(sub_type) << 8) | \
        static_cast<uint32_t>(code) \
    ))

// Specific error code definitions (classified by module)
namespace ErrorCodes {
    // SimpleCommKitBle module error codes
    constexpr ErrorCode SimpleCommKitBleSuccess = MAKE_ERROR_CODE(ModuleID::SimpleCommKitBle, ErrorType::SimpleCommKitSuccess, 0, 0);
    constexpr ErrorCode SimpleCommKitBleGetAdaptersError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitBle, ErrorType::SimpleCommKitError, 0, 1);
    constexpr ErrorCode SimpleCommKitBleSetAdapterError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitBle, ErrorType::SimpleCommKitError, 0, 2);
    constexpr ErrorCode SimpleCommKitBleCurrentAdapterNotSet = MAKE_ERROR_CODE(ModuleID::SimpleCommKitBle, ErrorType::SimpleCommKitError, 0, 3);
    constexpr ErrorCode SimpleCommKitBleAdapterScanStartError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitBle, ErrorType::SimpleCommKitError, 0, 4);
    constexpr ErrorCode SimpleCommKitBleAdapterScanStopError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitBle, ErrorType::SimpleCommKitError, 0, 5);
    constexpr ErrorCode SimpleCommKitBleAdapterSetScanTimeoutError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitBle, ErrorType::SimpleCommKitError, 0, 6);
    constexpr ErrorCode SimpleCommKitBleAdapterScanGetResultsError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitBle, ErrorType::SimpleCommKitError, 0, 7);
    constexpr ErrorCode SimpleCommKitBleAdapterGetPairedPeripheralsError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitBle, ErrorType::SimpleCommKitError, 0, 8);
    constexpr ErrorCode SimpleCommKitBleAdapterGetConnectedPeripheralsError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitBle, ErrorType::SimpleCommKitError, 0, 9);
    constexpr ErrorCode SimpleCommKitBleCurrentPeripheralSetError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitBle, ErrorType::SimpleCommKitError, 0, 10);
    constexpr ErrorCode SimpleCommKitBleCurrentPeripheralNotSet = MAKE_ERROR_CODE(ModuleID::SimpleCommKitBle, ErrorType::SimpleCommKitError, 0, 11);
    constexpr ErrorCode SimpleCommKitBleCurrentPeripheralSubIndicateError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitBle, ErrorType::SimpleCommKitError, 0, 12);
    constexpr ErrorCode SimpleCommKitBleCurrentPeripheralSubNotifyError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitBle, ErrorType::SimpleCommKitError, 0, 13);
    constexpr ErrorCode SimpleCommKitBleCurrentPeripheralUnSubScribeError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitBle, ErrorType::SimpleCommKitError, 0, 14);

    // SimpleCommKitSerialPort module error codes
    constexpr ErrorCode SimpleCommKitSerialPortSuccess = MAKE_ERROR_CODE(ModuleID::SimpleCommKitSerialPort, ErrorType::SimpleCommKitSuccess, 0, 0);
    constexpr ErrorCode SimpleCommKitSerialPortInitError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitSerialPort, ErrorType::SimpleCommKitError, 0, 1);
    constexpr ErrorCode SimpleCommKitSerialPortOpenError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitSerialPort, ErrorType::SimpleCommKitError, 0, 2);
    constexpr ErrorCode SimpleCommKitSerialPortCloseError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitSerialPort, ErrorType::SimpleCommKitError, 0, 3);
    constexpr ErrorCode SimpleCommKitSerialPortNotOpenError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitSerialPort, ErrorType::SimpleCommKitError, 0, 4);
    constexpr ErrorCode SimpleCommKitSerialPortReadError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitSerialPort, ErrorType::SimpleCommKitError, 0, 5);
    constexpr ErrorCode SimpleCommKitSerialPortWriteError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitSerialPort, ErrorType::SimpleCommKitError, 0, 6);
    constexpr ErrorCode SimpleCommKitSerialPortHotPlugError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitSerialPort, ErrorType::SimpleCommKitError, 0, 7);
    constexpr ErrorCode SimpleCommKitSerialPortFlushError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitSerialPort, ErrorType::SimpleCommKitError, 0, 8);

    // SimpleCommKitHid module error codes
    constexpr ErrorCode SimpleCommKitHidSuccess = MAKE_ERROR_CODE(ModuleID::SimpleCommKitHid, ErrorType::SimpleCommKitSuccess, 0, 0);
    constexpr ErrorCode SimpleCommKitHidInitError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitHid, ErrorType::SimpleCommKitError, 0, 1);
    constexpr ErrorCode SimpleCommKitHidExitError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitHid, ErrorType::SimpleCommKitError, 0, 2);
    constexpr ErrorCode SimpleCommKitHidOpenError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitHid, ErrorType::SimpleCommKitError, 0, 3);
    constexpr ErrorCode SimpleCommKitHidCloseError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitHid, ErrorType::SimpleCommKitError, 0, 4);
    constexpr ErrorCode SimpleCommKitHidNotOpenError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitHid, ErrorType::SimpleCommKitError, 0, 5);
    constexpr ErrorCode SimpleCommKitHidReadError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitHid, ErrorType::SimpleCommKitError, 0, 6);
    constexpr ErrorCode SimpleCommKitHidWriteError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitHid, ErrorType::SimpleCommKitError, 0, 7);
    constexpr ErrorCode SimpleCommKitHidFeatureReportError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitHid, ErrorType::SimpleCommKitError, 0, 8);
    constexpr ErrorCode SimpleCommKitHidHotplugError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitHid, ErrorType::SimpleCommKitError, 0, 9);
    constexpr ErrorCode SimpleCommKitHidEnumerateError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitHid, ErrorType::SimpleCommKitError, 0, 10);

    // SimpleCommKitUsb module error codes
    constexpr ErrorCode SimpleCommKitUsbSuccess = MAKE_ERROR_CODE(ModuleID::SimpleCommKitUsb, ErrorType::SimpleCommKitSuccess, 0, 0);
    constexpr ErrorCode SimpleCommKitUsbInitError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitUsb, ErrorType::SimpleCommKitError, 0, 1);
    constexpr ErrorCode SimpleCommKitUsbExitError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitUsb, ErrorType::SimpleCommKitError, 0, 2);
    constexpr ErrorCode SimpleCommKitUsbOpenError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitUsb, ErrorType::SimpleCommKitError, 0, 3);
    constexpr ErrorCode SimpleCommKitUsbCloseError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitUsb, ErrorType::SimpleCommKitError, 0, 4);
    constexpr ErrorCode SimpleCommKitUsbNotOpenError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitUsb, ErrorType::SimpleCommKitError, 0, 5);
    constexpr ErrorCode SimpleCommKitUsbReadError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitUsb, ErrorType::SimpleCommKitError, 0, 6);
    constexpr ErrorCode SimpleCommKitUsbWriteError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitUsb, ErrorType::SimpleCommKitError, 0, 7);
    constexpr ErrorCode SimpleCommKitUsbControlTransferError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitUsb, ErrorType::SimpleCommKitError, 0, 8);
    constexpr ErrorCode SimpleCommKitUsbBulkTransferError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitUsb, ErrorType::SimpleCommKitError, 0, 9);
    constexpr ErrorCode SimpleCommKitUsbInterruptTransferError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitUsb, ErrorType::SimpleCommKitError, 0, 10);
    constexpr ErrorCode SimpleCommKitUsbClaimInterfaceError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitUsb, ErrorType::SimpleCommKitError, 0, 11);
    constexpr ErrorCode SimpleCommKitUsbReleaseInterfaceError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitUsb, ErrorType::SimpleCommKitError, 0, 12);
    constexpr ErrorCode SimpleCommKitUsbHotplugError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitUsb, ErrorType::SimpleCommKitError, 0, 13);
    constexpr ErrorCode SimpleCommKitUsbEnumerateError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitUsb, ErrorType::SimpleCommKitError, 0, 14);
    constexpr ErrorCode SimpleCommKitUsbIsochronousTransferError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitUsb, ErrorType::SimpleCommKitError, 0, 15);
    constexpr ErrorCode SimpleCommKitUsbGetConfigError = MAKE_ERROR_CODE(ModuleID::SimpleCommKitUsb, ErrorType::SimpleCommKitError, 0, 16);

    // SimpleCommKitTcp module error codes
    constexpr ErrorCode SimpleCommKitTcpSuccess             = MAKE_ERROR_CODE(ModuleID::SimpleCommKitTcp, ErrorType::SimpleCommKitSuccess, 0, 0);
    constexpr ErrorCode SimpleCommKitTcpConnectError        = MAKE_ERROR_CODE(ModuleID::SimpleCommKitTcp, ErrorType::SimpleCommKitError, 0, 1);
    constexpr ErrorCode SimpleCommKitTcpDisconnectError     = MAKE_ERROR_CODE(ModuleID::SimpleCommKitTcp, ErrorType::SimpleCommKitError, 0, 2);
    constexpr ErrorCode SimpleCommKitTcpNotConnectedError   = MAKE_ERROR_CODE(ModuleID::SimpleCommKitTcp, ErrorType::SimpleCommKitError, 0, 3);
    constexpr ErrorCode SimpleCommKitTcpSendError           = MAKE_ERROR_CODE(ModuleID::SimpleCommKitTcp, ErrorType::SimpleCommKitError, 0, 4);
    constexpr ErrorCode SimpleCommKitTcpStartError          = MAKE_ERROR_CODE(ModuleID::SimpleCommKitTcp, ErrorType::SimpleCommKitError, 0, 5);
    constexpr ErrorCode SimpleCommKitTcpStopError           = MAKE_ERROR_CODE(ModuleID::SimpleCommKitTcp, ErrorType::SimpleCommKitError, 0, 6);
    constexpr ErrorCode SimpleCommKitTcpNotRunningError     = MAKE_ERROR_CODE(ModuleID::SimpleCommKitTcp, ErrorType::SimpleCommKitError, 0, 7);
    constexpr ErrorCode SimpleCommKitTcpBroadcastError      = MAKE_ERROR_CODE(ModuleID::SimpleCommKitTcp, ErrorType::SimpleCommKitError, 0, 8);
    constexpr ErrorCode SimpleCommKitTcpTlsError            = MAKE_ERROR_CODE(ModuleID::SimpleCommKitTcp, ErrorType::SimpleCommKitError, 0, 9);

    // SimpleCommKitUdp module error codes
    constexpr ErrorCode SimpleCommKitUdpSuccess            = MAKE_ERROR_CODE(ModuleID::SimpleCommKitUdp, ErrorType::SimpleCommKitSuccess, 0, 0);
    constexpr ErrorCode SimpleCommKitUdpOpenError          = MAKE_ERROR_CODE(ModuleID::SimpleCommKitUdp, ErrorType::SimpleCommKitError, 0, 1);
    constexpr ErrorCode SimpleCommKitUdpCloseError         = MAKE_ERROR_CODE(ModuleID::SimpleCommKitUdp, ErrorType::SimpleCommKitError, 0, 2);
    constexpr ErrorCode SimpleCommKitUdpNotOpenError       = MAKE_ERROR_CODE(ModuleID::SimpleCommKitUdp, ErrorType::SimpleCommKitError, 0, 3);
    constexpr ErrorCode SimpleCommKitUdpSendError          = MAKE_ERROR_CODE(ModuleID::SimpleCommKitUdp, ErrorType::SimpleCommKitError, 0, 4);
    constexpr ErrorCode SimpleCommKitUdpStartError         = MAKE_ERROR_CODE(ModuleID::SimpleCommKitUdp, ErrorType::SimpleCommKitError, 0, 5);
    constexpr ErrorCode SimpleCommKitUdpStopError          = MAKE_ERROR_CODE(ModuleID::SimpleCommKitUdp, ErrorType::SimpleCommKitError, 0, 6);
    constexpr ErrorCode SimpleCommKitUdpNotRunningError    = MAKE_ERROR_CODE(ModuleID::SimpleCommKitUdp, ErrorType::SimpleCommKitError, 0, 7);
    constexpr ErrorCode SimpleCommKitUdpBroadcastError     = MAKE_ERROR_CODE(ModuleID::SimpleCommKitUdp, ErrorType::SimpleCommKitError, 0, 8);

    // SimpleCommKitWebSocket module error codes
    constexpr ErrorCode SimpleCommKitWebSocketSuccess              = MAKE_ERROR_CODE(ModuleID::SimpleCommKitWebSocket, ErrorType::SimpleCommKitSuccess, 0, 0);
    constexpr ErrorCode SimpleCommKitWebSocketConnectError         = MAKE_ERROR_CODE(ModuleID::SimpleCommKitWebSocket, ErrorType::SimpleCommKitError, 0, 1);
    constexpr ErrorCode SimpleCommKitWebSocketDisconnectError      = MAKE_ERROR_CODE(ModuleID::SimpleCommKitWebSocket, ErrorType::SimpleCommKitError, 0, 2);
    constexpr ErrorCode SimpleCommKitWebSocketNotConnectedError    = MAKE_ERROR_CODE(ModuleID::SimpleCommKitWebSocket, ErrorType::SimpleCommKitError, 0, 3);
    constexpr ErrorCode SimpleCommKitWebSocketSendError            = MAKE_ERROR_CODE(ModuleID::SimpleCommKitWebSocket, ErrorType::SimpleCommKitError, 0, 4);
    constexpr ErrorCode SimpleCommKitWebSocketStartError           = MAKE_ERROR_CODE(ModuleID::SimpleCommKitWebSocket, ErrorType::SimpleCommKitError, 0, 5);
    constexpr ErrorCode SimpleCommKitWebSocketStopError            = MAKE_ERROR_CODE(ModuleID::SimpleCommKitWebSocket, ErrorType::SimpleCommKitError, 0, 6);
    constexpr ErrorCode SimpleCommKitWebSocketNotRunningError      = MAKE_ERROR_CODE(ModuleID::SimpleCommKitWebSocket, ErrorType::SimpleCommKitError, 0, 7);
    constexpr ErrorCode SimpleCommKitWebSocketBroadcastError       = MAKE_ERROR_CODE(ModuleID::SimpleCommKitWebSocket, ErrorType::SimpleCommKitError, 0, 8);
    constexpr ErrorCode SimpleCommKitWebSocketTlsError             = MAKE_ERROR_CODE(ModuleID::SimpleCommKitWebSocket, ErrorType::SimpleCommKitError, 0, 9);

    // SimpleCommKitMqtt module error codes
    constexpr ErrorCode SimpleCommKitMqttSuccess            = MAKE_ERROR_CODE(ModuleID::SimpleCommKitMqtt, ErrorType::SimpleCommKitSuccess, 0, 0);
    constexpr ErrorCode SimpleCommKitMqttConnectError       = MAKE_ERROR_CODE(ModuleID::SimpleCommKitMqtt, ErrorType::SimpleCommKitError, 0, 1);
    constexpr ErrorCode SimpleCommKitMqttDisconnectError    = MAKE_ERROR_CODE(ModuleID::SimpleCommKitMqtt, ErrorType::SimpleCommKitError, 0, 2);
    constexpr ErrorCode SimpleCommKitMqttNotConnectedError  = MAKE_ERROR_CODE(ModuleID::SimpleCommKitMqtt, ErrorType::SimpleCommKitError, 0, 3);
    constexpr ErrorCode SimpleCommKitMqttPublishError       = MAKE_ERROR_CODE(ModuleID::SimpleCommKitMqtt, ErrorType::SimpleCommKitError, 0, 4);
    constexpr ErrorCode SimpleCommKitMqttSubscribeError     = MAKE_ERROR_CODE(ModuleID::SimpleCommKitMqtt, ErrorType::SimpleCommKitError, 0, 5);
    constexpr ErrorCode SimpleCommKitMqttUnsubscribeError   = MAKE_ERROR_CODE(ModuleID::SimpleCommKitMqtt, ErrorType::SimpleCommKitError, 0, 6);
    constexpr ErrorCode SimpleCommKitMqttTlsError           = MAKE_ERROR_CODE(ModuleID::SimpleCommKitMqtt, ErrorType::SimpleCommKitError, 0, 7);
}

// Error code resolver (replaces the original ErrorUtil)
class SimpleCommKitErrorMap {
public:
    // Get error code description string
    static std::string GetErrorDescription(ErrorCode error_code) {
        auto it = error_descriptions_.find(error_code);
        if (it != error_descriptions_.end()) {
            return it->second;
        }
        return "Unknown error";
    }

    // Resolve module ID from error code
    static ModuleID ResolveModuleID(ErrorCode error_code) {  // Method name optimized to ResolveXXX
        return static_cast<ModuleID>((error_code >> 24) & 0xFF);
    }

    // Resolve error type from error code
    static ErrorType ResolveErrorType(ErrorCode error_code) {
        return static_cast<ErrorType>((error_code >> 16) & 0xFF);
    }

private:
    // Error code - description mapping table
    static const std::unordered_map<ErrorCode, std::string> error_descriptions_;
};

} // namespace SimpleCommKit

// Move the definition outside the class and make it inline (C++17)
namespace SimpleCommKit {

// Initialize error description mapping table
inline const std::unordered_map<ErrorCode, std::string> SimpleCommKitErrorMap::error_descriptions_ = {
    {ErrorCodes::SimpleCommKitBleSuccess, "SimpleCommKitBle success"},
    {ErrorCodes::SimpleCommKitBleGetAdaptersError, "SimpleCommKitBle get adapters error"},
    {ErrorCodes::SimpleCommKitBleSetAdapterError, "SimpleCommKitBle set adapter error"},
    {ErrorCodes::SimpleCommKitBleCurrentAdapterNotSet, "SimpleCommKitBle adapter not set error"},
    {ErrorCodes::SimpleCommKitBleAdapterScanStartError, "SimpleCommKitBle adapter start scan error"},
    {ErrorCodes::SimpleCommKitBleAdapterScanStopError, "SimpleCommKitBle adapter stop scan error"},
    {ErrorCodes::SimpleCommKitBleAdapterSetScanTimeoutError, "SimpleCommKitBle adapter set scan timeout error"},
    {ErrorCodes::SimpleCommKitBleAdapterScanGetResultsError, "SimpleCommKitBle adapter scan get results error"},
    {ErrorCodes::SimpleCommKitBleAdapterGetPairedPeripheralsError, "SimpleCommKitBle adapter get paired peripherals error"},
    {ErrorCodes::SimpleCommKitBleAdapterGetConnectedPeripheralsError, "SimpleCommKitBle adapter get connected peripherals error"},
    {ErrorCodes::SimpleCommKitBleCurrentPeripheralSetError, "SimpleCommKitBle peripheral set error"},
    {ErrorCodes::SimpleCommKitBleCurrentPeripheralNotSet, "SimpleCommKitBle peripheral not set error"},
    {ErrorCodes::SimpleCommKitBleCurrentPeripheralSubIndicateError, "SimpleCommKitBle peripheral sub indicate error"},
    {ErrorCodes::SimpleCommKitBleCurrentPeripheralSubNotifyError, "SimpleCommKitBle peripheral sub notify error"},
    {ErrorCodes::SimpleCommKitBleCurrentPeripheralUnSubScribeError, "SimpleCommKitBle peripheral unsubscribe error"},

    {ErrorCodes::SimpleCommKitSerialPortSuccess, "SimpleCommKitSerialPort success"},
    {ErrorCodes::SimpleCommKitSerialPortInitError, "SimpleCommKitSerialPort init error"},
    {ErrorCodes::SimpleCommKitSerialPortOpenError, "SimpleCommKitSerialPort open error"},
    {ErrorCodes::SimpleCommKitSerialPortCloseError, "SimpleCommKitSerialPort close error"},
    {ErrorCodes::SimpleCommKitSerialPortNotOpenError, "SimpleCommKitSerialPort port not open error"},
    {ErrorCodes::SimpleCommKitSerialPortReadError, "SimpleCommKitSerialPort read error"},
    {ErrorCodes::SimpleCommKitSerialPortWriteError, "SimpleCommKitSerialPort write error"},
    {ErrorCodes::SimpleCommKitSerialPortHotPlugError, "SimpleCommKitSerialPort hot plug error"},
    {ErrorCodes::SimpleCommKitSerialPortFlushError, "SimpleCommKitSerialPort flush error"},

    {ErrorCodes::SimpleCommKitHidSuccess, "SimpleCommKitHid success"},
    {ErrorCodes::SimpleCommKitHidInitError, "SimpleCommKitHid init error"},
    {ErrorCodes::SimpleCommKitHidExitError, "SimpleCommKitHid exit error"},
    {ErrorCodes::SimpleCommKitHidOpenError, "SimpleCommKitHid open error"},
    {ErrorCodes::SimpleCommKitHidCloseError, "SimpleCommKitHid close error"},
    {ErrorCodes::SimpleCommKitHidNotOpenError, "SimpleCommKitHid device not open error"},
    {ErrorCodes::SimpleCommKitHidReadError, "SimpleCommKitHid read error"},
    {ErrorCodes::SimpleCommKitHidWriteError, "SimpleCommKitHid write error"},
    {ErrorCodes::SimpleCommKitHidFeatureReportError, "SimpleCommKitHid feature report error"},
    {ErrorCodes::SimpleCommKitHidHotplugError, "SimpleCommKitHid hotplug error"},
    {ErrorCodes::SimpleCommKitHidEnumerateError, "SimpleCommKitHid enumerate error"},

    {ErrorCodes::SimpleCommKitUsbSuccess, "SimpleCommKitUsb success"},
    {ErrorCodes::SimpleCommKitUsbInitError, "SimpleCommKitUsb init error"},
    {ErrorCodes::SimpleCommKitUsbExitError, "SimpleCommKitUsb exit error"},
    {ErrorCodes::SimpleCommKitUsbOpenError, "SimpleCommKitUsb open error"},
    {ErrorCodes::SimpleCommKitUsbCloseError, "SimpleCommKitUsb close error"},
    {ErrorCodes::SimpleCommKitUsbNotOpenError, "SimpleCommKitUsb device not open error"},
    {ErrorCodes::SimpleCommKitUsbReadError, "SimpleCommKitUsb read error"},
    {ErrorCodes::SimpleCommKitUsbWriteError, "SimpleCommKitUsb write error"},
    {ErrorCodes::SimpleCommKitUsbControlTransferError, "SimpleCommKitUsb control transfer error"},
    {ErrorCodes::SimpleCommKitUsbBulkTransferError, "SimpleCommKitUsb bulk transfer error"},
    {ErrorCodes::SimpleCommKitUsbInterruptTransferError, "SimpleCommKitUsb interrupt transfer error"},
    {ErrorCodes::SimpleCommKitUsbClaimInterfaceError, "SimpleCommKitUsb claim interface error"},
    {ErrorCodes::SimpleCommKitUsbReleaseInterfaceError, "SimpleCommKitUsb release interface error"},
    {ErrorCodes::SimpleCommKitUsbHotplugError, "SimpleCommKitUsb hotplug error"},
    {ErrorCodes::SimpleCommKitUsbEnumerateError, "SimpleCommKitUsb enumerate error"},
    {ErrorCodes::SimpleCommKitUsbIsochronousTransferError, "SimpleCommKitUsb isochronous transfer error"},
    {ErrorCodes::SimpleCommKitUsbGetConfigError, "SimpleCommKitUsb get config descriptor error"},

    {ErrorCodes::SimpleCommKitTcpSuccess, "SimpleCommKitTcp success"},
    {ErrorCodes::SimpleCommKitTcpConnectError, "SimpleCommKitTcp connect error"},
    {ErrorCodes::SimpleCommKitTcpDisconnectError, "SimpleCommKitTcp disconnect error"},
    {ErrorCodes::SimpleCommKitTcpNotConnectedError, "SimpleCommKitTcp not connected error"},
    {ErrorCodes::SimpleCommKitTcpSendError, "SimpleCommKitTcp send error"},
    {ErrorCodes::SimpleCommKitTcpStartError, "SimpleCommKitTcp start error"},
    {ErrorCodes::SimpleCommKitTcpStopError, "SimpleCommKitTcp stop error"},
    {ErrorCodes::SimpleCommKitTcpNotRunningError, "SimpleCommKitTcp server not running error"},
    {ErrorCodes::SimpleCommKitTcpBroadcastError, "SimpleCommKitTcp broadcast error"},
    {ErrorCodes::SimpleCommKitTcpTlsError, "SimpleCommKitTcp TLS error"},

    {ErrorCodes::SimpleCommKitUdpSuccess, "SimpleCommKitUdp success"},
    {ErrorCodes::SimpleCommKitUdpOpenError, "SimpleCommKitUdp open error"},
    {ErrorCodes::SimpleCommKitUdpCloseError, "SimpleCommKitUdp close error"},
    {ErrorCodes::SimpleCommKitUdpNotOpenError, "SimpleCommKitUdp not open error"},
    {ErrorCodes::SimpleCommKitUdpSendError, "SimpleCommKitUdp send error"},
    {ErrorCodes::SimpleCommKitUdpStartError, "SimpleCommKitUdp start error"},
    {ErrorCodes::SimpleCommKitUdpStopError, "SimpleCommKitUdp stop error"},
    {ErrorCodes::SimpleCommKitUdpNotRunningError, "SimpleCommKitUdp server not running error"},
    {ErrorCodes::SimpleCommKitUdpBroadcastError, "SimpleCommKitUdp broadcast error"},

    {ErrorCodes::SimpleCommKitWebSocketSuccess, "SimpleCommKitWebSocket success"},
    {ErrorCodes::SimpleCommKitWebSocketConnectError, "SimpleCommKitWebSocket connect error"},
    {ErrorCodes::SimpleCommKitWebSocketDisconnectError, "SimpleCommKitWebSocket disconnect error"},
    {ErrorCodes::SimpleCommKitWebSocketNotConnectedError, "SimpleCommKitWebSocket not connected error"},
    {ErrorCodes::SimpleCommKitWebSocketSendError, "SimpleCommKitWebSocket send error"},
    {ErrorCodes::SimpleCommKitWebSocketStartError, "SimpleCommKitWebSocket start error"},
    {ErrorCodes::SimpleCommKitWebSocketStopError, "SimpleCommKitWebSocket stop error"},
    {ErrorCodes::SimpleCommKitWebSocketNotRunningError, "SimpleCommKitWebSocket server not running error"},
    {ErrorCodes::SimpleCommKitWebSocketBroadcastError, "SimpleCommKitWebSocket broadcast error"},
    {ErrorCodes::SimpleCommKitWebSocketTlsError, "SimpleCommKitWebSocket TLS error"},

    {ErrorCodes::SimpleCommKitMqttSuccess, "SimpleCommKitMqtt success"},
    {ErrorCodes::SimpleCommKitMqttConnectError, "SimpleCommKitMqtt connect error"},
    {ErrorCodes::SimpleCommKitMqttDisconnectError, "SimpleCommKitMqtt disconnect error"},
    {ErrorCodes::SimpleCommKitMqttNotConnectedError, "SimpleCommKitMqtt not connected error"},
    {ErrorCodes::SimpleCommKitMqttPublishError, "SimpleCommKitMqtt publish error"},
    {ErrorCodes::SimpleCommKitMqttSubscribeError, "SimpleCommKitMqtt subscribe error"},
    {ErrorCodes::SimpleCommKitMqttUnsubscribeError, "SimpleCommKitMqtt unsubscribe error"},
    {ErrorCodes::SimpleCommKitMqttTlsError, "SimpleCommKitMqtt TLS error"},

};

}
