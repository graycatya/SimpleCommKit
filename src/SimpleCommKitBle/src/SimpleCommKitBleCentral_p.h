#pragma once

#include <simpleble/SimpleBLE.h>
#include <simpleble/Adapter.h>
#include <simpleble/Peripheral.h>
#include "SimpleCommKitBleCentral.h"
#include "SimpleCommKitExport.h"
#include <map>
#include <mutex>
#include <functional>

namespace SimpleCommKitBle {

inline bool bluetooth_Enabled()
{
    return SimpleBLE::Adapter::bluetooth_enabled();
}

class SimpleCommKitBleCentralPrivate
{
    SIMPLECOMMKIT_DECLARE_PUBLIC(SimpleCommKitBleCentral)
public:
    SimpleCommKitBleCentralPrivate(SimpleCommKitBleCentral* parent);
    ~SimpleCommKitBleCentralPrivate();



    std::vector<SimpleBLE::Adapter> getAdapters();
    void setCurrentAdapter(const SimpleCommKitBleAdapter& adapter);
    std::optional<SimpleBLE::Adapter*> getCurrentAdapter();

    void powerOn();
    void powerOff();
    bool isPowered();

    void scanStart();
    void scanStop();
    void scanFor(int timeout_ms);
    bool scanIsActive();

    bool isInitialized();

    void setCallbackOnPowerOn(std::function<void()> callback);
    void setCallbackOnPowerOff(std::function<void()> callback);
    void setCallbackOnScanStart(std::function<void()> callback);
    void setCallbackOnScanStop(std::function<void()> callback);
    void setCallbackOnScanUpdated(std::function<void(SimpleBLE::Peripheral)> callback);
    void setCallbackOnScanFound(std::function<void(SimpleBLE::Peripheral)> callback);
    void setCallbackError(std::function<void(SimpleCommKit::ErrorCode)> callback);

    std::vector<SimpleBLE::Peripheral> scanGetResults();
    std::vector<SimpleBLE::Peripheral> getPairedPeripherals();
    std::vector<SimpleBLE::Peripheral> getConnectedPeripherals();

    void setCurrentPeripheral(const SimpleCommKitBlePeripheral& peripheral);
    std::optional<SimpleBLE::Peripheral*> getCurrentPeripheral();

    int16_t peripheralGetTxPower();
    uint16_t peripheralGetMtu();

    void peripheralConnect();
    void peripheralDisconnect();
    bool peripheralIsConnected();
    bool peripheralIsConnectable();
    bool peripheralIsPaired();
    void peripheralUnpair();

    void peripheralSetCallbackOnConnected(std::function<void()> callback);
    void peripheralSetCallbackOnDisconnected(std::function<void()> callback);

    std::vector<SimpleBLE::Service> getPeripheralServices();


    void triggerError(SimpleCommKit::ErrorCode error_code);

private:
    SimpleCommKitBleCentral* q_ptr;
    std::unique_ptr<SimpleBLE::Adapter> m_currentAdapter;
    std::unique_ptr<SimpleBLE::Peripheral> m_currentPeripheral;

    std::map<std::string, SimpleCommKitBle::SimpleCommKitBlePeripheral> m_peripherals;
    std::function<void(SimpleCommKit::ErrorCode)> m_onError;

    bool m_initialized = false;
    mutable std::mutex m_mutex;
};

} // namespace SimpleCommKitBle


