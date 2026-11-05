#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <roapi.h>
#include "../BleBackend.h"
#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h>
#include <iomanip>
#include <sstream>

namespace SimpleCommKit::Ble::WindowsDetail {
inline ErrorCode bluetoothError(winrt::Windows::Devices::Bluetooth::BluetoothError error) {
    using E = winrt::Windows::Devices::Bluetooth::BluetoothError;
    switch (error) {
    case E::Success: return ErrorCodes::SimpleCommKitBleSuccess;
    case E::DisabledByPolicy: case E::DisabledByUser: case E::ConsentRequired:
        return ErrorCodes::SimpleCommKitBlePermissionDenied;
    case E::NotSupported: case E::TransportNotSupported:
        return ErrorCodes::SimpleCommKitBleNotSupported;
    default: return ErrorCodes::SimpleCommKitBleBackendUnavailable;
    }
}
inline ErrorCode hresultError(int32_t code) {
    if (code == E_ACCESSDENIED) return ErrorCodes::SimpleCommKitBlePermissionDenied;
    if (code == E_INVALIDARG) return ErrorCodes::SimpleCommKitBleInvalidArgument;
    if (code == E_NOTIMPL || code == REGDB_E_CLASSNOTREG) return ErrorCodes::SimpleCommKitBleNotSupported;
    if (code == HRESULT_FROM_WIN32(ERROR_CANCELLED)) return ErrorCodes::SimpleCommKitBleCancelled;
    if (code == HRESULT_FROM_WIN32(ERROR_TIMEOUT)) return ErrorCodes::SimpleCommKitBleTimeout;
    return ErrorCodes::SimpleCommKitBleBackendUnavailable;
}
inline ErrorCode gattError(winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus status) {
    using S = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus;
    switch (status) {
    case S::Success: return ErrorCodes::SimpleCommKitBleSuccess;
    case S::AccessDenied: return ErrorCodes::SimpleCommKitBlePermissionDenied;
    case S::Unreachable: return ErrorCodes::SimpleCommKitBleBackendUnavailable;
    default: return ErrorCodes::SimpleCommKitBleGattError;
    }
}
inline void checkGatt(winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus status) {
    auto error = gattError(status);
    if (error != ErrorCodes::SimpleCommKitBleSuccess) throw Failure(error);
}
inline std::string uuidString(const winrt::guid& value) {
    auto text = winrt::to_string(winrt::to_hstring(value));
    if (text.size() == 38 && text.front() == '{') text = text.substr(1, 36);
    return canonicalUuid(text);
}
// Refuse duplicate UUIDs instead of silently operating on the first handle.
template<class T> T uniqueUuid(const std::vector<T>& values, const std::string& uuid) {
    const auto key = canonicalUuid(uuid);
    T found{nullptr};
    for (const auto& value : values) if (uuidString(value.Uuid()) == key) {
        if (found) throw Failure(ErrorCodes::SimpleCommKitBleAmbiguousIdentity);
        found = value;
    }
    if (!found) throw Failure(ErrorCodes::SimpleCommKitBleInvalidArgument, "GATT UUID not found");
    return found;
}
inline std::string address(uint64_t value) {
    std::ostringstream out;
    out << std::uppercase << std::hex << std::setfill('0');
    for (int i = 5; i >= 0; --i) {
        if (i != 5) out << ':';
        out << std::setw(2) << ((value >> (i * 8)) & 0xff);
    }
    return out.str();
}
inline Peripheral peripheral(uint64_t value, winrt::Windows::Devices::Bluetooth::BluetoothAddressType type) {
    using T = winrt::Windows::Devices::Bluetooth::BluetoothAddressType;
    Peripheral result;
    result.value.address = address(value);
    result.value.address_type = type == T::Public ? PUBLIC : type == T::Random ? RANDOM : UNSPECIFIED;
    result.id = "winrt:" + std::to_string(static_cast<int>(type)) + ":" + result.value.address;
    // Public identifier is the advertised name; id remains the native identity.
    return result;
}
}
