#pragma once

#include <SimpleCommKitErrorMap.hpp>
#include "SimpleCommKitExport.h"

#include <vector>
#include <functional>
#include <memory>
#include <optional>
#include <map>



namespace SimpleCommKit {

SIMPLECOMMKIT_DECLARE_PRIVATE_CLASS(SimpleCommKitBleCentral)
struct SimpleCommKitBleAdapter {
    std::string dev_identifier;
    std::string dev_address;
};

enum SimpleCommKitBlePeripheralAddressType : int32_t {
    PUBLIC = 0,
    RANDOM = 1,
    UNSPECIFIED = 2
};

struct SimpleCommKitBlePeripheral {
    std::string identifier;
    std::string address;
    SimpleCommKitBlePeripheralAddressType address_type;
    int16_t rssi;
    std::map<uint16_t, std::vector<uint8_t>> manufacturer;
};

struct SimpleCommKitBleCharacteristic {
    std::string uuid;
    std::vector<std::string> descriptors_uuid;
    std::vector<std::string> capabilities;
    bool can_read;
    bool can_write_request;
    bool can_write_command;
    bool can_notify;
    bool can_indicate;
};

struct SimpleCommKitBleService {
    std::string uuid;
    std::vector<uint8_t> data;
    std::vector<SimpleCommKitBleCharacteristic> characteristics;
};

class SIMPLECOMMKIT_API SimpleCommKitBleCentral
{
    SIMPLECOMMKIT_DECLARE_PRIVATE(SimpleCommKitBleCentral)
public:
    SimpleCommKitBleCentral();
    ~SimpleCommKitBleCentral();

    static bool Bluetooth_Enabled();

    std::vector<SimpleCommKitBleAdapter> get_Adapters();
    std::optional<SimpleCommKitBleAdapter> get_CurrentAdapter();
    void set_CurrentAdapter(const SimpleCommKitBleAdapter& adapter);

    void adapter_Power_On();
    void adapter_Power_Off();
    bool adapter_Is_Powered();
    void adapter_Set_Callback_On_Power_On(std::function<void()> on_power_on);
    void adapter_Set_Callback_On_Power_Off(std::function<void()> on_power_off);
    void adapter_Set_Callback_On_Scan_Start(std::function<void()> on_scan_start);
    void adapter_Set_Callback_On_Scan_Stop(std::function<void()> on_scan_stop);
    void adapter_Set_Callback_On_Scan_Updated(std::function<void(SimpleCommKitBlePeripheral)> on_scan_updated);
    void adapter_Set_Callback_On_Scan_Found(std::function<void(SimpleCommKitBlePeripheral)> on_scan_found);
    std::vector<SimpleCommKitBlePeripheral> adapter_Get_Scan_Results();
    std::vector<SimpleCommKitBlePeripheral> adapter_Get_Paired_Peripherals();
    std::vector<SimpleCommKitBlePeripheral> adapter_Get_Connected_Peripherals();

    void adapter_Scan_Start();
    void adapter_Scan_Stop();
    void adapter_Scan_For(int timeout_ms);
    bool adapter_Scan_Is_Active();
    bool adapter_Initialized();

    void set_CurrentPeripheral(const SimpleCommKitBlePeripheral& peripheral);
    int16_t peripheral_Get_Tx_Power();
    uint16_t peripheral_Get_Mtu();

    void peripheral_Connect();
    void peripheral_Disconnect();
    bool peripheral_Is_Connected();
    bool peripheral_Is_Connectable();
    bool peripheral_Is_Paired();
    void peripheral_Unpair();

    void peripheral_Set_Callback_On_Connected(std::function<void()> on_connected);
    void peripheral_Set_Callback_On_Disconnected(std::function<void()> on_disconnected);

    std::vector<SimpleCommKitBleService> peripheral_Services();

    std::vector<uint8_t> peripheral_Read(std::string const& service, std::string const& characteristic);
    void peripheral_Write_Request(std::string const& service, std::string const& characteristic, std::vector<uint8_t> const& data);
    void peripheral_Write_Command(std::string const& service, std::string const& characteristic, std::vector<uint8_t> const& data);
    void peripheral_Notify(std::string const& service, std::string const& characteristic, std::function<void(std::vector<uint8_t> payload)> callback);
    void peripheral_Indicate(std::string const& service, std::string const& characteristic, std::function<void(std::vector<uint8_t> payload)> callback);
    void peripheral_Unsubscribe(std::string const& service, std::string const& characteristic);

    std::vector<uint8_t> peripheral_Read(std::string const& service, std::string const& characteristic, std::string const& descriptor);
    void peripheral_Write(std::string const& service, std::string const& characteristic, std::string const& descriptor, std::vector<uint8_t> const& data);

    void set_Callback_Error(std::function<void(SimpleCommKit::ErrorCode)> on_error);

private:
    std::unique_ptr<SimpleCommKitBleCentralPrivate> d_ptr;
};

} // namespace SimpleCommKit
