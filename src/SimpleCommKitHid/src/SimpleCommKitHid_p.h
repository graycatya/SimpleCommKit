#pragma once

#include <hidapi.h>
#include "SimpleCommKitHid.h"
#include "SimpleCommKitExport.h"

#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include <map>

namespace SimpleCommKit {

class SimpleCommKitHidPrivate
{
    SIMPLECOMMKIT_DECLARE_PUBLIC(SimpleCommKitHid)
public:
    explicit SimpleCommKitHidPrivate(SimpleCommKitHid* parent);
    ~SimpleCommKitHidPrivate();

    //
    // Static
    //
    static std::vector<SimpleCommKitHidDeviceInfo>
        getAvailableDevices(unsigned short vendor_id,
                            unsigned short product_id);

    //
    // Lifecycle
    //
    bool init(unsigned short vendor_id, unsigned short product_id);
    void exit();

    bool open(const std::string& path);
    bool open(unsigned short vendor_id,
              unsigned short product_id,
              const std::string& serial_number);
    void close();
    bool isOpen();

    //
    // I/O
    //
    int write(const std::vector<uint8_t>& data);
    int sendFeatureReport(const std::vector<uint8_t>& data);

    //
    // Hotplug
    //
    void startHotplug(unsigned short vendor_id, unsigned short product_id);
    void stopHotplug();
    bool isHotplugActive();

    void setHotplugPollInterval(int ms);
    int  getHotplugPollInterval();

    //
    // Read polling config
    //
    void setReadPollInterval(int ms);
    int  getReadPollInterval();

    void setReadDataLength(int length);
    int  getReadDataLength();

    //
    // Device list
    //
    std::vector<SimpleCommKitHidDeviceInfo> getDeviceList();

    //
    // Callbacks
    //
    void setCallbackOnRead(std::function<void(const std::vector<uint8_t>&)> callback);
    void setCallbackOnHotPlug(std::function<void(const std::vector<SimpleCommKitHidDeviceInfo>&,
                                                  const std::vector<SimpleCommKitHidDeviceInfo>&)> callback);
    void setCallbackError(std::function<void(SimpleCommKit::ErrorCode)> callback);

    void triggerError(SimpleCommKit::ErrorCode error_code);

private:
    //
    // Internal helpers
    //
    static std::vector<SimpleCommKitHidDeviceInfo>
        enumerateDevices(unsigned short vendor_id, unsigned short product_id);

    static void copyDeviceInfo(hid_device_info* hid_info,
                               std::map<std::string, SimpleCommKitHidDeviceInfo>& devices);

    void compareDevices(const std::map<std::string, SimpleCommKitHidDeviceInfo>& current,
                        std::vector<SimpleCommKitHidDeviceInfo>& added,
                        std::vector<SimpleCommKitHidDeviceInfo>& removed);

    void startReadThread();
    void stopReadThread();

    static std::wstring utf8ToWstring(const std::string& str);
    static std::string  wstringToUtf8(const std::wstring& wstr);

    //
    // Members
    //
    SimpleCommKitHid* q_ptr;

    // hidapi device handle
    hid_device* m_device = nullptr;

    // cached device map (key = path)
    std::map<std::string, SimpleCommKitHidDeviceInfo> m_devices;
    mutable std::mutex m_devicesMutex;

    // hotplug thread
    std::unique_ptr<std::thread> m_hotplugThread;
    std::atomic<bool> m_hotplugStop{true};
    std::atomic<int>  m_hotplugPollMs{1000};
    unsigned short    m_hotplugVendorId  = 0;
    unsigned short    m_hotplugProductId = 0;

    std::function<void(const std::vector<SimpleCommKitHidDeviceInfo>&,
                       const std::vector<SimpleCommKitHidDeviceInfo>&)> m_onHotPlug;

    // read thread
    std::unique_ptr<std::thread> m_readThread;
    std::atomic<bool> m_readThreadStop{true};
    std::atomic<int>  m_readPollMs{1};
    std::atomic<int>  m_readDataLength{64};

    std::function<void(const std::vector<uint8_t>&)> m_onRead;

    // error callback
    std::function<void(SimpleCommKit::ErrorCode)> m_onError;
};

} // namespace SimpleCommKit
