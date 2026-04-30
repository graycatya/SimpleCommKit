#include "SimpleCommKitBleCentral.h"
#include "SimpleCommKitBleCentral_p.h"
#include <algorithm>
#include <iostream>

namespace SimpleCommKitBle {


SimpleCommKitBlePeripheral convertToSimpleCommKitPeripheral(SimpleBLE::Peripheral peripheral)
{
    SimpleCommKitBlePeripheral result;

    result.identifier = peripheral.identifier();
    result.address = peripheral.address();
    result.address_type = static_cast<SimpleCommKitBlePeripheralAddressType>(peripheral.address_type());
    result.rssi = peripheral.rssi();
    std::map<uint16_t, SimpleBLE::ByteArray> manufacturer_data = peripheral.manufacturer_data();
    for (const auto& [company_id, data] : manufacturer_data) {
        result.manufacturer[company_id] = data;
    }
    return result;
}



// SimpleCommKitBleCentral Implementation

SimpleCommKitBleCentral::SimpleCommKitBleCentral()
    : d_ptr(std::make_unique<SimpleCommKitBleCentralPrivate>(this))
{
}

bool SimpleCommKitBleCentral::Bluetooth_Enabled()
{
    return SimpleCommKitBle::bluetooth_Enabled();
}

SimpleCommKitBleCentral::~SimpleCommKitBleCentral() = default;

std::vector<SimpleCommKitBleAdapter> SimpleCommKitBleCentral::get_Adapters()
{
    if (!d_ptr) {
        return {};
    }
    std::vector<SimpleCommKitBleAdapter> adapters;
    for (auto& adapter : d_ptr->getAdapters()) {
        SimpleCommKitBleAdapter padapter;
        padapter.dev_identifier = adapter.identifier();
        padapter.dev_address = adapter.address();
        adapters.push_back(padapter);
    }
    return adapters;
}

std::optional<SimpleCommKitBleAdapter> SimpleCommKitBleCentral::get_CurrentAdapter()
{
    if (!d_ptr) {
        return std::nullopt;
    }
    SimpleCommKitBleAdapter adapter;
    if(auto adapter_opt = d_ptr->getCurrentAdapter())
    {
        adapter.dev_identifier = adapter_opt.value()->identifier();
        adapter.dev_address = adapter_opt.value()->address();
    } else {
        return std::nullopt;
    }
    return adapter;
}

void SimpleCommKitBleCentral::set_CurrentAdapter(const SimpleCommKitBleAdapter& adapter)
{
    if (d_ptr) {
        d_ptr->setCurrentAdapter(adapter);
    }
}

void SimpleCommKitBleCentral::adapter_Power_On()
{
    if (d_ptr) {
        d_ptr->powerOn();
    }
}

void SimpleCommKitBleCentral::adapter_Power_Off()
{
    if (d_ptr) {
        d_ptr->powerOff();
    }
}

bool SimpleCommKitBleCentral::adapter_Is_Powered()
{
    if (!d_ptr) {
        return false;
    }
    return d_ptr->isPowered();
}

void SimpleCommKitBleCentral::adapter_Set_Callback_On_Power_On(std::function<void()> on_power_on)
{
    if (d_ptr) {
        d_ptr->setCallbackOnPowerOn(std::move(on_power_on));
    }
}

void SimpleCommKitBleCentral::adapter_Set_Callback_On_Power_Off(std::function<void()> on_power_off)
{
    if (d_ptr) {
        d_ptr->setCallbackOnPowerOff(std::move(on_power_off));
    }
}

void SimpleCommKitBleCentral::adapter_Set_Callback_On_Scan_Start(std::function<void ()> on_scan_start)
{
    if (d_ptr) {
        d_ptr->m_peripherals.clear();
        d_ptr->setCallbackOnScanStart(std::move(on_scan_start));
    }
}

void SimpleCommKitBleCentral::adapter_Set_Callback_On_Scan_Stop(std::function<void ()> on_scan_stop)
{
    if (d_ptr) {
        d_ptr->setCallbackOnScanStop(std::move(on_scan_stop));
    }
}

void SimpleCommKitBleCentral::adapter_Set_Callback_On_Scan_Updated(std::function<void (SimpleCommKitBlePeripheral)> on_scan_updated)
{
    if (d_ptr) {
        if(on_scan_updated)
        {
            d_ptr->setCallbackOnScanUpdated([=](SimpleBLE::Peripheral p_peripheral){
                if(d_ptr->m_peripherals.count(p_peripheral.address()))
                {
                    SimpleCommKitBlePeripheral t_peripheral = convertToSimpleCommKitPeripheral(p_peripheral);
                    d_ptr->m_peripherals[p_peripheral.address()].identifier = t_peripheral.identifier;
                    d_ptr->m_peripherals[p_peripheral.address()].rssi = t_peripheral.rssi;
                    d_ptr->m_peripherals[p_peripheral.address()].manufacturer.merge(t_peripheral.manufacturer);
                } else {
                    d_ptr->m_peripherals.insert(std::make_pair(p_peripheral.address(), convertToSimpleCommKitPeripheral(p_peripheral)));
                }

                on_scan_updated(d_ptr->m_peripherals.at(p_peripheral.address()));
            });
        } else {
            d_ptr->setCallbackOnScanUpdated(nullptr);
        }
    }
}

void SimpleCommKitBleCentral::adapter_Set_Callback_On_Scan_Found(std::function<void (SimpleCommKitBlePeripheral)> on_scan_found)
{
    if (d_ptr) {
        if(on_scan_found)
        {
            d_ptr->setCallbackOnScanFound([=](SimpleBLE::Peripheral p_peripheral){
                if(d_ptr->m_peripherals.count(p_peripheral.address()))
                {
                    SimpleCommKitBlePeripheral t_peripheral = convertToSimpleCommKitPeripheral(p_peripheral);
                    d_ptr->m_peripherals[p_peripheral.address()].identifier = t_peripheral.identifier;
                    d_ptr->m_peripherals[p_peripheral.address()].rssi = t_peripheral.rssi;
                    d_ptr->m_peripherals[p_peripheral.address()].manufacturer.merge(t_peripheral.manufacturer);
                } else {
                    d_ptr->m_peripherals.insert(std::make_pair(p_peripheral.address(), convertToSimpleCommKitPeripheral(p_peripheral)));
                }
                on_scan_found(d_ptr->m_peripherals.at(p_peripheral.address()));
            });
        } else {
            d_ptr->setCallbackOnScanFound(nullptr);
        }
    }
}

std::vector<SimpleCommKitBlePeripheral> SimpleCommKitBleCentral::adapter_Get_Scan_Results()
{
    if (!d_ptr) {
        return {};
    }

    try {
        auto simple_peripherals = d_ptr->scanGetResults();
        std::vector<SimpleCommKitBlePeripheral> peripherals;
        for (const auto& peripheral : simple_peripherals) {
            if(d_ptr->m_peripherals.count(static_cast<SimpleBLE::Peripheral>(peripheral).address()))
            {
                peripherals.push_back(d_ptr->m_peripherals.at(static_cast<SimpleBLE::Peripheral>(peripheral).address()));
            }
        }
        return peripherals;
    } catch (const std::exception& e) {
        d_ptr->triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleAdapterScanGetResultsError);
        return {};
    }
}

std::vector<SimpleCommKitBlePeripheral> SimpleCommKitBleCentral::adapter_Get_Paired_Peripherals()
{
    if (!d_ptr) {
        return {};
    }

    try {
        auto simple_peripherals = d_ptr->getPairedPeripherals();
        std::vector<SimpleCommKitBlePeripheral> peripherals;
        for (const auto& peripheral : simple_peripherals) {
            SimpleCommKitBlePeripheral t_peripheral = convertToSimpleCommKitPeripheral(peripheral);
            if(d_ptr->m_peripherals.count(static_cast<SimpleBLE::Peripheral>(peripheral).address()))
            {
                t_peripheral.manufacturer = d_ptr->m_peripherals[static_cast<SimpleBLE::Peripheral>(peripheral).address()].manufacturer;
            }
            peripherals.push_back(t_peripheral);
        }
        return peripherals;
    } catch (const std::exception& e) {
        d_ptr->triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleAdapterGetPairedPeripheralsError);
        return {};
    }
}

std::vector<SimpleCommKitBlePeripheral> SimpleCommKitBleCentral::adapter_Get_Connected_Peripherals()
{
    if (!d_ptr) {
        return {};
    }

    try {
        auto simple_peripherals = d_ptr->getConnectedPeripherals();
        std::vector<SimpleCommKitBlePeripheral> peripherals;
        for (const auto& peripheral : simple_peripherals) {
            SimpleCommKitBlePeripheral t_peripheral = convertToSimpleCommKitPeripheral(peripheral);
            if(d_ptr->m_peripherals.count(static_cast<SimpleBLE::Peripheral>(peripheral).address()))
            {
                t_peripheral.manufacturer = d_ptr->m_peripherals[static_cast<SimpleBLE::Peripheral>(peripheral).address()].manufacturer;
            }
            peripherals.push_back(t_peripheral);
        }
        return peripherals;
    } catch (const std::exception& e) {
        d_ptr->triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleAdapterGetConnectedPeripheralsError);
        return {};
    }
}

void SimpleCommKitBleCentral::adapter_Scan_Start()
{
    if (d_ptr) {
        d_ptr->scanStart();
    }
}

void SimpleCommKitBleCentral::adapter_Scan_Stop()
{
    if (d_ptr) {
        d_ptr->scanStop();
    }
}

void SimpleCommKitBleCentral::adapter_Scan_For(int timeout_ms)
{
    if (d_ptr) {
        d_ptr->scanFor(timeout_ms);
    }
}

bool SimpleCommKitBleCentral::adapter_Scan_Is_Active()
{
    if (!d_ptr) {
        return false;
    }
    return d_ptr->scanIsActive();
}

bool SimpleCommKitBleCentral::adapter_Initialized()
{
    if (!d_ptr) {
        return false;
    }
    return d_ptr->isInitialized();
}

void SimpleCommKitBleCentral::set_CurrentPeripheral(const SimpleCommKitBlePeripheral &peripheral)
{
    if (!d_ptr) {
        return;
    }
    d_ptr->setCurrentPeripheral(peripheral);
}

int16_t SimpleCommKitBleCentral::peripheral_Get_Tx_Power()
{
    if (!d_ptr) {
        return -1;
    }
    d_ptr->peripheralGetTxPower();
}

uint16_t SimpleCommKitBleCentral::peripheral_Get_Mtu()
{
    if (!d_ptr) {
        return -1;
    }
    d_ptr->peripheralGetMtu();
}

void SimpleCommKitBleCentral::peripheral_Connect()
{
    if (!d_ptr) {
        return;
    }
    d_ptr->peripheralConnect();
}

void SimpleCommKitBleCentral::peripheral_Disconnect()
{
    if (!d_ptr) {
        return;
    }
    d_ptr->peripheralDisconnect();
}

bool SimpleCommKitBleCentral::peripheral_Is_Connected()
{
    if (!d_ptr) {
        return false;
    }
    return d_ptr->peripheralIsConnected();
}

bool SimpleCommKitBleCentral::peripheral_Is_Connectable()
{
    if (!d_ptr) {
        return false;
    }
    return d_ptr->peripheralIsConnected();
}

bool SimpleCommKitBleCentral::peripheral_Is_Paired()
{
    if (!d_ptr) {
        return false;
    }
    return d_ptr->peripheralIsPaired();
}

void SimpleCommKitBleCentral::peripheral_Unpair()
{
    if (!d_ptr) {
        return;
    }
    d_ptr->peripheralUnpair();
}

void SimpleCommKitBleCentral::peripheral_Set_Callback_On_Connected(std::function<void ()> on_connected)
{
    if (!d_ptr) {
        return;
    }
    d_ptr->peripheralSetCallbackOnConnected(on_connected);
}

void SimpleCommKitBleCentral::peripheral_Set_Callback_On_Disconnected(std::function<void ()> on_disconnected)
{
    if (!d_ptr) {
        return;
    }
    d_ptr->peripheralSetCallbackOnConnected(on_disconnected);
}

std::vector<SimpleCommKitBleService> SimpleCommKitBleCentral::peripheral_Services()
{
    if (!d_ptr) {
        return {};
    }
    std::vector<SimpleCommKitBleService> r_services;
    std::vector<SimpleBLE::Service> services = d_ptr->getPeripheralServices();
    for(auto& service : services)
    {
        SimpleCommKitBleService r_service;
        r_service.uuid = service.uuid();
        r_service.data = service.data();
        std::vector<SimpleBLE::Characteristic> characteristics = service.characteristics();
        for(auto& characteristic : characteristics)
        {
            SimpleCommKitBleCharacteristic r_characteristic;
            r_characteristic.uuid = characteristic.uuid();
            
            std::vector<SimpleBLE::Descriptor> descriptors = characteristic.descriptors();
            for(auto& descriptor : descriptors)
            {
                r_characteristic.descriptors_uuid.push_back(descriptor.uuid());
            }
            
            r_characteristic.capabilities = characteristic.capabilities();
            r_characteristic.can_read = characteristic.can_read();
            r_characteristic.can_write_request = characteristic.can_write_request();
            r_characteristic.can_write_command = characteristic.can_write_command();
            r_characteristic.can_notify = characteristic.can_notify();
            r_characteristic.can_indicate = characteristic.can_indicate();
            
            r_service.characteristics.push_back(r_characteristic);
        }
        r_services.push_back(r_service);
    }
    return r_services;
}

std::vector<uint8_t> SimpleCommKitBleCentral::peripheral_Read(const std::string &service, const std::string &characteristic)
{
    if (!d_ptr) {
        return {};
    }
    if(!d_ptr->getCurrentPeripheral().has_value())
    {
        return {};
    }
    return d_ptr->getCurrentPeripheral().value()->read(service, characteristic);
}

void SimpleCommKitBleCentral::peripheral_Write_Request(const std::string &service, const std::string &characteristic, const std::vector<uint8_t> &data)
{
    if (!d_ptr) {
        return;
    }
    if(!d_ptr->getCurrentPeripheral().has_value())
    {
        return;
    }
    return d_ptr->getCurrentPeripheral().value()->write_request(service, characteristic, data);
}

void SimpleCommKitBleCentral::peripheral_Write_Command(const std::string &service, const std::string &characteristic, const std::vector<uint8_t> &data)
{
    if (!d_ptr) {
        return;
    }
    if(!d_ptr->getCurrentPeripheral().has_value())
    {
        return;
    }
    return d_ptr->getCurrentPeripheral().value()->write_command(service, characteristic, data);
}

void SimpleCommKitBleCentral::peripheral_Notify(const std::string &service, const std::string &characteristic, std::function<void (std::string)> callback)
{
    if (!d_ptr) {
        return;
    }
    if(!d_ptr->getCurrentPeripheral().has_value())
    {
        return;
    }
    try {
        d_ptr->getCurrentPeripheral().value()->notify(service, characteristic, callback);
    } catch (...) {
        d_ptr->triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentPeripheralSubNotifyError);
    }
}

void SimpleCommKitBleCentral::peripheral_Indicate(const std::string &service, const std::string &characteristic, std::function<void (std::string)> callback)
{
    if (!d_ptr) {
        return;
    }
    if(!d_ptr->getCurrentPeripheral().has_value())
    {
        return;
    }
    try {
        d_ptr->getCurrentPeripheral().value()->indicate(service, characteristic, callback);
    } catch (...) {
        d_ptr->triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentPeripheralSubIndicateError);
    }
}

void SimpleCommKitBleCentral::peripheral_Unsubscribe(const std::string &service, const std::string &characteristic)
{
    if (!d_ptr) {
        return;
    }
    if(!d_ptr->getCurrentPeripheral().has_value())
    {
        return;
    }
    try {
        d_ptr->getCurrentPeripheral().value()->unsubscribe(service, characteristic);
    } catch (...) {
        d_ptr->triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentPeripheralUnSubScribeError);
    }
}

std::vector<uint8_t> SimpleCommKitBleCentral::peripheral_Read(const std::string &service, const std::string &characteristic, const std::string &descriptor)
{
    if (!d_ptr) {
        return {};
    }
    if(!d_ptr->getCurrentPeripheral().has_value())
    {
        return {};
    }
    return d_ptr->getCurrentPeripheral().value()->read(service, characteristic, descriptor);
}

void SimpleCommKitBleCentral::peripheral_Write(const std::string &service, const std::string &characteristic, const std::string &descriptor, const std::vector<uint8_t> &data)
{
    if (!d_ptr) {
        return;
    }
    if(!d_ptr->getCurrentPeripheral().has_value())
    {
        return;
    }
    d_ptr->getCurrentPeripheral().value()->write(service, characteristic, descriptor, data);
}

void SimpleCommKitBleCentral::set_Callback_Error(std::function<void(SimpleCommKit::ErrorCode)> on_error)
{
    if (d_ptr) {
        d_ptr->setCallbackError(std::move(on_error));
    }
}

} // namespace SimpleCommKitBle
