#include "SimpleCommKitBleCentral.h"
#include "SimpleCommKitBleCentral_p.h"
#include <algorithm>
#include <iostream>


namespace SimpleCommKitBle {

// SimpleCommKitBleCentralPrivate Implementation

SimpleCommKitBleCentralPrivate::SimpleCommKitBleCentralPrivate(SimpleCommKitBleCentral* parent)
    : q_ptr(parent)
{
}

SimpleCommKitBleCentralPrivate::~SimpleCommKitBleCentralPrivate()
{
    //scanStop();
}


std::vector<SimpleBLE::Adapter> SimpleCommKitBleCentralPrivate::getAdapters()
{
    // if (!SimpleBLE::Adapter::bluetooth_enabled()) {
    //     triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleGetAdaptersError);
    //     return {};
    // }
    return SimpleBLE::Adapter::get_adapters();
}

void SimpleCommKitBleCentralPrivate::setCurrentAdapter(const SimpleCommKitBleAdapter& adapter)
{
    if(m_currentAdapter)
    {
        m_currentAdapter.reset();
    }

    try {
        auto simple_adapters = SimpleBLE::Adapter::get_adapters();

        for (auto& simple_adapter : simple_adapters) {
            if (simple_adapter.identifier() == adapter.dev_identifier ||
                simple_adapter.address() == adapter.dev_address) {
                m_currentAdapter = std::make_unique<SimpleBLE::Adapter>(simple_adapter);
                break;
            }
        }
    } catch (const std::exception& e) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleSetAdapterError);
    }

}

std::optional<SimpleBLE::Adapter*> SimpleCommKitBleCentralPrivate::getCurrentAdapter()
{
    if (!m_currentAdapter) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentAdapterNotSet);
        return std::nullopt;
    }
    return m_currentAdapter.get();
}

void SimpleCommKitBleCentralPrivate::powerOn()
{
    if (!m_currentAdapter) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentAdapterNotSet);
        return;
    }
    m_currentAdapter->power_on();
}

void SimpleCommKitBleCentralPrivate::powerOff()
{
    if (!m_currentAdapter) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentAdapterNotSet);
        return;
    }
    m_currentAdapter->power_off();
}

bool SimpleCommKitBleCentralPrivate::isPowered()
{
    if (!m_currentAdapter) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentAdapterNotSet);
        return false;
    }
    return m_currentAdapter->is_powered();
}

void SimpleCommKitBleCentralPrivate::scanStart()
{
    if (!m_currentAdapter) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentAdapterNotSet);
        return;
    }

    try {
        m_currentAdapter->scan_start();
    } catch (const std::exception& e) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleAdapterScanStartError);
    }
}

void SimpleCommKitBleCentralPrivate::scanStop()
{
    if (!m_currentAdapter) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentAdapterNotSet);
        return;
    }

    try {
        m_currentAdapter->scan_stop();
    } catch (const std::exception& e) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleAdapterScanStartError);
    }
}

void SimpleCommKitBleCentralPrivate::scanFor(int timeout_ms)
{
    if (!m_currentAdapter) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentAdapterNotSet);
        return;
    }

    try {
        m_currentAdapter->scan_for(timeout_ms);
    } catch (const std::exception& e) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleAdapterSetScanTimeoutError);
    }
}

bool SimpleCommKitBleCentralPrivate::scanIsActive()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_currentAdapter) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentAdapterNotSet);
        return false;
    }

    return m_currentAdapter->scan_is_active();

}

bool SimpleCommKitBleCentralPrivate::isInitialized()
{
    if (!m_currentAdapter) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentAdapterNotSet);
        return false;
    }

    return m_currentAdapter->initialized();
}

void SimpleCommKitBleCentralPrivate::setCallbackOnPowerOn(std::function<void()> callback)
{
    if (!m_currentAdapter) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentAdapterNotSet);
        return;
    }
    m_currentAdapter->set_callback_on_power_on(callback);
}

void SimpleCommKitBleCentralPrivate::setCallbackOnPowerOff(std::function<void()> callback)
{
    if (!m_currentAdapter) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentAdapterNotSet);
        return;
    }
    m_currentAdapter->set_callback_on_power_off(callback);
}

void SimpleCommKitBleCentralPrivate::setCallbackOnScanStart(std::function<void ()> callback)
{
    if (!m_currentAdapter) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentAdapterNotSet);
        return;
    }
    m_currentAdapter->set_callback_on_scan_start(callback);
}

void SimpleCommKitBleCentralPrivate::setCallbackOnScanStop(std::function<void ()> callback)
{
    if (!m_currentAdapter) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentAdapterNotSet);
        return;
    }
    m_currentAdapter->set_callback_on_scan_stop(callback);
}

void SimpleCommKitBleCentralPrivate::setCallbackOnScanUpdated(std::function<void (SimpleBLE::Peripheral)> callback)
{
    if (!m_currentAdapter) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentAdapterNotSet);
        return;
    }
    m_currentAdapter->set_callback_on_scan_updated(callback);
}

void SimpleCommKitBleCentralPrivate::setCallbackOnScanFound(std::function<void (SimpleBLE::Peripheral)> callback)
{
    if (!m_currentAdapter) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentAdapterNotSet);
        return;
    }
    m_currentAdapter->set_callback_on_scan_found(callback);
}

void SimpleCommKitBleCentralPrivate::setCallbackError(std::function<void(SimpleCommKit::ErrorCode)> callback)
{
    m_onError = std::move(callback);
}

std::vector<SimpleBLE::Peripheral> SimpleCommKitBleCentralPrivate::scanGetResults()
{
    if (!m_currentAdapter) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentAdapterNotSet);
        return {};
    }
    return m_currentAdapter->scan_get_results();
}

std::vector<SimpleBLE::Peripheral> SimpleCommKitBleCentralPrivate::getPairedPeripherals()
{
    if (!m_currentAdapter) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentAdapterNotSet);
        return {};
    }
    return m_currentAdapter->get_paired_peripherals();
}

std::vector<SimpleBLE::Peripheral> SimpleCommKitBleCentralPrivate::getConnectedPeripherals()
{
    if (!m_currentAdapter) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentAdapterNotSet);
        return {};
    }
    return m_currentAdapter->get_connected_peripherals();
}

void SimpleCommKitBleCentralPrivate::setCurrentPeripheral(const SimpleCommKitBlePeripheral &peripheral)
{
    if (!m_currentAdapter) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentAdapterNotSet);
        return;
    }
    if(m_currentPeripheral)
    {
        m_currentPeripheral.reset();
    }
    std::vector<SimpleBLE::Peripheral> peripherals = m_currentAdapter->scan_get_results();

    for (auto& simple_peripheral : peripherals) {
        if (simple_peripheral.identifier() == peripheral.identifier ||
            simple_peripheral.address() == peripheral.address ||
            simple_peripheral.address_type() ==  static_cast<SimpleBLE::BluetoothAddressType>(peripheral.address_type)) {
            m_currentPeripheral = std::make_unique<SimpleBLE::Peripheral>(simple_peripheral);
            return;
        }
    }

    triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentPeripheralSetError);
}

std::optional<SimpleBLE::Peripheral *> SimpleCommKitBleCentralPrivate::getCurrentPeripheral()
{
    if (!m_currentPeripheral) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentPeripheralNotSet);
        return std::nullopt;
    }
    return m_currentPeripheral.get();
}

int16_t SimpleCommKitBleCentralPrivate::peripheralGetTxPower()
{
    if (!m_currentPeripheral) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentPeripheralNotSet);
        return -1;
    }
    return m_currentPeripheral->tx_power();
}

uint16_t SimpleCommKitBleCentralPrivate::peripheralGetMtu()
{
    if (!m_currentPeripheral) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentPeripheralNotSet);
        return -1;
    }
    return m_currentPeripheral->mtu();
}

void SimpleCommKitBleCentralPrivate::peripheralConnect()
{
    if (!m_currentPeripheral) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentPeripheralNotSet);
        return;
    }
    m_currentPeripheral->connect();
}

void SimpleCommKitBleCentralPrivate::peripheralDisconnect()
{
    if (!m_currentPeripheral) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentPeripheralNotSet);
        return;
    }
    m_currentPeripheral->disconnect();
}

bool SimpleCommKitBleCentralPrivate::peripheralIsConnected()
{
    if (!m_currentPeripheral) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentPeripheralNotSet);
        return false;
    }
    return m_currentPeripheral->is_connected();
}

bool SimpleCommKitBleCentralPrivate::peripheralIsConnectable()
{
    if (!m_currentPeripheral) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentPeripheralNotSet);
        return false;
    }
    return m_currentPeripheral->is_connectable();
}

bool SimpleCommKitBleCentralPrivate::peripheralIsPaired()
{
    if (!m_currentPeripheral) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentPeripheralNotSet);
        return false;
    }
    return m_currentPeripheral->is_paired();
}

void SimpleCommKitBleCentralPrivate::peripheralUnpair()
{
    if (!m_currentPeripheral) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentPeripheralNotSet);
        return;
    }
    m_currentPeripheral->unpair();
}

void SimpleCommKitBleCentralPrivate::peripheralSetCallbackOnConnected(std::function<void ()> callback)
{
    if (!m_currentPeripheral) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentPeripheralNotSet);
        return;
    }
    m_currentPeripheral->set_callback_on_connected(callback);
}

void SimpleCommKitBleCentralPrivate::peripheralSetCallbackOnDisconnected(std::function<void ()> callback)
{
    if (!m_currentPeripheral) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentPeripheralNotSet);
        return;
    }
    m_currentPeripheral->set_callback_on_disconnected(callback);
}

std::vector<SimpleBLE::Service> SimpleCommKitBleCentralPrivate::getPeripheralServices()
{
    if (!m_currentPeripheral) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitBleCurrentPeripheralNotSet);
        return {};
    }
    return m_currentPeripheral->services();
}

void SimpleCommKitBleCentralPrivate::triggerError(SimpleCommKit::ErrorCode error_code)
{
    if (m_onError) {
        m_onError(error_code);
    }
}

}
