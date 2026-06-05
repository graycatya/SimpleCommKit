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

// Per-device entry stored in m_deviceMap
struct HidDeviceEntry {
    hid_device*                       handle = nullptr;
    std::unique_ptr<std::thread>      readThread;
    std::shared_ptr<std::atomic<bool>> readStopFlag;    // shared so lambda sees updates
    std::shared_ptr<std::atomic<int>>  readPollMs;
    std::shared_ptr<std::atomic<int>>  readDataLength;
};

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
    // Lifecycle – multi-device
    //
    bool init(unsigned short vendor_id, unsigned short product_id);
    void exit();

    bool open(const std::string& path, bool readable = true);
    bool open(unsigned short vendor_id,
              unsigned short product_id,
              const std::string& serial_number = "",
              bool readable = true);
    void close();                                    // close all
    void close(const std::string& path);             // close one
    bool isOpen();                                   // any open?
    bool isOpen(const std::string& path);            // specific open?

    //
    // I/O  (path-less versions operate on first open device)
    //
    int write(const std::vector<uint8_t>& data);
    int write(const std::string& path, const std::vector<uint8_t>& data);
    int sendFeatureReport(const std::vector<uint8_t>& data);
    int sendFeatureReport(const std::string& path, const std::vector<uint8_t>& data);

    //
    // Hotplug
    //
    void startHotplug(unsigned short vendor_id, unsigned short product_id);
    void stopHotplug();
    bool isHotplugActive();

    void setHotplugPollInterval(int ms);
    int  getHotplugPollInterval();

    //
    // Read polling config (global defaults)
    //
    void setReadPollInterval(int ms);
    int  getReadPollInterval();

    void setReadDataLength(int length);
    int  getReadDataLength();

    //
    // Per-device read config
    //
    void setReadPollInterval(const std::string& path, int ms);
    int  getReadPollInterval(const std::string& path);
    void setReadDataLength(const std::string& path, int length);
    int  getReadDataLength(const std::string& path);

    //
    // Device list
    //
    std::vector<std::string> getOpenPaths();
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

    void startReadThread(const std::string& path);
    void stopReadThread(const std::string& path);
    void stopAllReadThreads();

    static std::wstring utf8ToWstring(const std::string& str);
    static std::string  wstringToUtf8(const std::wstring& wstr);

    //
    // Members
    //
    SimpleCommKitHid* q_ptr;

    // Multi-device map: key = device path
    std::map<std::string, HidDeviceEntry> m_deviceMap;
    mutable std::mutex m_deviceMapMutex;

    // cached device info map (key = path)
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

    // Read config defaults (used for new devices)
    std::atomic<int>  m_defaultReadPollMs{100};
    std::atomic<int>  m_defaultReadDataLength{64};

    // Callbacks
    std::function<void(const std::vector<uint8_t>&)> m_onRead;
    std::function<void(SimpleCommKit::ErrorCode)> m_onError;
};

} // namespace SimpleCommKit
