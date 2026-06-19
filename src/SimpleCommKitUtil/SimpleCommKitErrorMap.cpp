#include "SimpleCommKitErrorMap.hpp"

namespace SimpleCommKit {

// ============================================================================
// 静态错误码描述表 — 定义于 .cpp，符号可导出为 DLL/LIB
// ============================================================================
const std::unordered_map<ErrorCode, std::string> SimpleCommKitErrorMap::error_descriptions_ = {
    // ---- SimpleCommKitBle ----
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

    // ---- SimpleCommKitSerialPort ----
    {ErrorCodes::SimpleCommKitSerialPortSuccess, "SimpleCommKitSerialPort success"},
    {ErrorCodes::SimpleCommKitSerialPortInitError, "SimpleCommKitSerialPort init error"},
    {ErrorCodes::SimpleCommKitSerialPortOpenError, "SimpleCommKitSerialPort open error"},
    {ErrorCodes::SimpleCommKitSerialPortCloseError, "SimpleCommKitSerialPort close error"},
    {ErrorCodes::SimpleCommKitSerialPortNotOpenError, "SimpleCommKitSerialPort port not open error"},
    {ErrorCodes::SimpleCommKitSerialPortReadError, "SimpleCommKitSerialPort read error"},
    {ErrorCodes::SimpleCommKitSerialPortWriteError, "SimpleCommKitSerialPort write error"},
    {ErrorCodes::SimpleCommKitSerialPortHotPlugError, "SimpleCommKitSerialPort hot plug error"},
    {ErrorCodes::SimpleCommKitSerialPortFlushError, "SimpleCommKitSerialPort flush error"},

    // ---- SimpleCommKitHid ----
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

    // ---- SimpleCommKitUsb ----
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

    // ---- SimpleCommKitTcp ----
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

    // ---- SimpleCommKitUdp ----
    {ErrorCodes::SimpleCommKitUdpSuccess, "SimpleCommKitUdp success"},
    {ErrorCodes::SimpleCommKitUdpOpenError, "SimpleCommKitUdp open error"},
    {ErrorCodes::SimpleCommKitUdpCloseError, "SimpleCommKitUdp close error"},
    {ErrorCodes::SimpleCommKitUdpNotOpenError, "SimpleCommKitUdp not open error"},
    {ErrorCodes::SimpleCommKitUdpSendError, "SimpleCommKitUdp send error"},
    {ErrorCodes::SimpleCommKitUdpStartError, "SimpleCommKitUdp start error"},
    {ErrorCodes::SimpleCommKitUdpStopError, "SimpleCommKitUdp stop error"},
    {ErrorCodes::SimpleCommKitUdpNotRunningError, "SimpleCommKitUdp server not running error"},
    {ErrorCodes::SimpleCommKitUdpBroadcastError, "SimpleCommKitUdp broadcast error"},

    // ---- SimpleCommKitWebSocket ----
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

    // ---- SimpleCommKitMqtt ----
    {ErrorCodes::SimpleCommKitMqttSuccess, "SimpleCommKitMqtt success"},
    {ErrorCodes::SimpleCommKitMqttConnectError, "SimpleCommKitMqtt connect error"},
    {ErrorCodes::SimpleCommKitMqttDisconnectError, "SimpleCommKitMqtt disconnect error"},
    {ErrorCodes::SimpleCommKitMqttNotConnectedError, "SimpleCommKitMqtt not connected error"},
    {ErrorCodes::SimpleCommKitMqttPublishError, "SimpleCommKitMqtt publish error"},
    {ErrorCodes::SimpleCommKitMqttSubscribeError, "SimpleCommKitMqtt subscribe error"},
    {ErrorCodes::SimpleCommKitMqttUnsubscribeError, "SimpleCommKitMqtt unsubscribe error"},
    {ErrorCodes::SimpleCommKitMqttTlsError, "SimpleCommKitMqtt TLS error"},
};

std::string SimpleCommKitErrorMap::GetErrorDescription(ErrorCode error_code) {
    auto it = error_descriptions_.find(error_code);
    if (it != error_descriptions_.end()) {
        return it->second;
    }
    return "Unknown error";
}

ModuleID SimpleCommKitErrorMap::ResolveModuleID(ErrorCode error_code) {
    return static_cast<ModuleID>((error_code >> 24) & 0xFF);
}

ErrorType SimpleCommKitErrorMap::ResolveErrorType(ErrorCode error_code) {
    return static_cast<ErrorType>((error_code >> 16) & 0xFF);
}

} // namespace SimpleCommKit
