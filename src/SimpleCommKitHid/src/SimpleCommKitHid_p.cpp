#include "SimpleCommKitHid.h"
#include "SimpleCommKitHid_p.h"

#include <iostream>
#include <chrono>
#include <algorithm>

// Platform-specific headers for sleep
#ifdef _WIN32
    #include <windows.h>
    #include <hidsdi.h>
#else
    #include <unistd.h>
#endif

// Platform-specific hidapi extensions
#ifndef HID_API_MAKE_VERSION
#define HID_API_MAKE_VERSION(mj, mn, p) (((mj) << 24) | ((mn) << 8) | (p))
#endif
#ifndef HID_API_VERSION
#define HID_API_VERSION \
    HID_API_MAKE_VERSION(HID_API_VERSION_MAJOR, HID_API_VERSION_MINOR, HID_API_VERSION_PATCH)
#endif

#if defined(__APPLE__) && HID_API_VERSION >= HID_API_MAKE_VERSION(0, 12, 0)
#include <hidapi_darwin.h>
#endif

#if defined(_WIN32) && HID_API_VERSION >= HID_API_MAKE_VERSION(0, 12, 0)
#include <hidapi_winapi.h>
#endif

namespace SimpleCommKit {

// ============================================================
// String conversion helpers
// ============================================================

std::wstring SimpleCommKitHidPrivate::utf8ToWstring(const std::string& str)
{
    if (str.empty()) return std::wstring();
#ifdef _WIN32
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (len <= 0) return std::wstring();
    std::wstring result(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], len);
    if (!result.empty() && result.back() == L'\0')
        result.pop_back();
    return result;
#else
    std::wstring result;
    for (auto c : str) {
        result += static_cast<wchar_t>(static_cast<unsigned char>(c));
    }
    return result;
#endif
}

std::string SimpleCommKitHidPrivate::wstringToUtf8(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();
#ifdef _WIN32
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return std::string();
    std::string result(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], len, nullptr, nullptr);
    if (!result.empty() && result.back() == '\0')
        result.pop_back();
    return result;
#else
    std::string result;
    for (auto c : wstr) {
        if (c <= 0x7F) {
            result += static_cast<char>(c);
        } else {
            // Simple UTF-8 encoding for BMP characters
            if (c <= 0x7FF) {
                result += static_cast<char>(0xC0 | (c >> 6));
                result += static_cast<char>(0x80 | (c & 0x3F));
            } else {
                result += static_cast<char>(0xE0 | (c >> 12));
                result += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (c & 0x3F));
            }
        }
    }
    return result;
#endif
}

// ============================================================
// Device enumeration
// ============================================================

void SimpleCommKitHidPrivate::copyDeviceInfo(
    hid_device_info* hid_info,
    std::map<std::string, SimpleCommKitHidDeviceInfo>& devices)
{
    for (; hid_info != nullptr; hid_info = hid_info->next) {
        SimpleCommKitHidDeviceInfo info;
        info.interface_number    = hid_info->interface_number;
        info.manufacturer_string = wstringToUtf8(hid_info->manufacturer_string
                                                     ? hid_info->manufacturer_string
                                                     : L"");
        info.product_string      = wstringToUtf8(hid_info->product_string
                                                     ? hid_info->product_string
                                                     : L"");
        info.release_number      = hid_info->release_number;
        info.bus_type            = static_cast<int>(hid_info->bus_type);
        info.serial_number       = wstringToUtf8(hid_info->serial_number
                                                     ? hid_info->serial_number
                                                     : L"");
        info.path                = hid_info->path ? hid_info->path : "";

        devices[info.path] = info;
    }
}

std::vector<SimpleCommKitHidDeviceInfo>
SimpleCommKitHidPrivate::enumerateDevices(unsigned short vendor_id,
                                           unsigned short product_id)
{
    std::vector<SimpleCommKitHidDeviceInfo> result;
    hid_device_info* hid_info = hid_enumerate(vendor_id, product_id);
    if (!hid_info) {
        return result;
    }

    // Deduplicate by (path, interface_number) — hidapi may return duplicates
    std::map<std::string, SimpleCommKitHidDeviceInfo> dedup;

    for (hid_device_info* cur = hid_info; cur != nullptr; cur = cur->next) {
        std::string key = std::string(cur->path ? cur->path : "")
                        + "#" + std::to_string(cur->interface_number);

        if (dedup.find(key) != dedup.end()) {
            continue;  // already seen this (path, interface) pair
        }

        SimpleCommKitHidDeviceInfo info;
        info.interface_number    = cur->interface_number;
        info.manufacturer_string = wstringToUtf8(cur->manufacturer_string
                                                     ? cur->manufacturer_string
                                                     : L"");
        info.product_string      = wstringToUtf8(cur->product_string
                                                     ? cur->product_string
                                                     : L"");
        info.release_number      = cur->release_number;
        info.bus_type            = static_cast<int>(cur->bus_type);
        info.serial_number       = wstringToUtf8(cur->serial_number
                                                     ? cur->serial_number
                                                     : L"");
        info.path                = cur->path ? cur->path : "";

        dedup[key] = info;
        result.push_back(info);
    }

    hid_free_enumeration(hid_info);
    return result;
}

// ============================================================
// Device comparison (for hotplug detection)
// ============================================================

void SimpleCommKitHidPrivate::compareDevices(
    const std::map<std::string, SimpleCommKitHidDeviceInfo>& current,
    std::vector<SimpleCommKitHidDeviceInfo>& added,
    std::vector<SimpleCommKitHidDeviceInfo>& removed)
{
    added.clear();
    removed.clear();

    std::lock_guard<std::mutex> lock(m_devicesMutex);

    // Find removed devices (in m_devices but not in current)
    for (const auto& [path, info] : m_devices) {
        if (current.find(path) == current.end()) {
            removed.push_back(info);
        }
    }

    // Find added devices (in current but not in m_devices)
    for (const auto& [path, info] : current) {
        if (m_devices.find(path) == m_devices.end()) {
            added.push_back(info);
        }
    }

    // Update cached map
    m_devices = current;
}

// ============================================================
// SimpleCommKitHidPrivate Implementation
// ============================================================

SimpleCommKitHidPrivate::SimpleCommKitHidPrivate(SimpleCommKitHid* parent)
    : q_ptr(parent)
{
}

SimpleCommKitHidPrivate::~SimpleCommKitHidPrivate()
{
    try {
        stopHotplug();
        stopAllReadThreads();
        {
            std::lock_guard<std::mutex> lock(m_deviceMapMutex);
            for (auto& [path, entry] : m_deviceMap) {
                if (entry.handle) {
                    hid_close(entry.handle);
                }
            }
            m_deviceMap.clear();
        }
    } catch (...) {
        // ignore errors in destructor
    }
}

//
// Static
//
std::vector<SimpleCommKitHidDeviceInfo>
SimpleCommKitHidPrivate::getAvailableDevices(unsigned short vendor_id,
                                              unsigned short product_id)
{
    return enumerateDevices(vendor_id, product_id);
}

//
// Lifecycle — multi-device
//
bool SimpleCommKitHidPrivate::init(unsigned short vendor_id,
                                    unsigned short product_id)
{
    try {
        int ret = hid_init();
        if (ret != 0) {
            triggerError(ErrorCodes::SimpleCommKitHidInitError);
            return false;
        }

        // Enumerate and cache matching devices
        auto devs = enumerateDevices(vendor_id, product_id);
        {
            std::lock_guard<std::mutex> lock(m_devicesMutex);
            m_devices.clear();
            for (const auto& dev : devs) {
                m_devices[dev.path] = dev;
            }
        }
        return true;
    } catch (const std::exception&) {
        triggerError(ErrorCodes::SimpleCommKitHidInitError);
        return false;
    }
}

void SimpleCommKitHidPrivate::exit()
{
    try {
        stopHotplug();
        stopAllReadThreads();

        {
            std::lock_guard<std::mutex> lock(m_deviceMapMutex);
            for (auto& [path, entry] : m_deviceMap) {
                if (entry.handle) {
                    hid_close(entry.handle);
                }
            }
            m_deviceMap.clear();
        }

        hid_exit();

        {
            std::lock_guard<std::mutex> lock(m_devicesMutex);
            m_devices.clear();
        }
    } catch (const std::exception&) {
        triggerError(ErrorCodes::SimpleCommKitHidExitError);
    }
}

bool SimpleCommKitHidPrivate::open(const std::string& path, bool readable)
{
    {
        std::lock_guard<std::mutex> lock(m_deviceMapMutex);
        if (m_deviceMap.find(path) != m_deviceMap.end()) {
            triggerError(ErrorCodes::SimpleCommKitHidOpenError);
            return false;  // already open
        }
    }

    try {
        hid_device* handle = hid_open_path(path.c_str());
        if (!handle) {
            triggerError(ErrorCodes::SimpleCommKitHidOpenError);
            return false;
        }

#ifdef _WIN32
        HidD_SetNumInputBuffers(*static_cast<HANDLE*>(handle), 9);
#endif

        // Use non-blocking mode — hid_read_timeout handles the wait
        hid_set_nonblocking(handle, 1);

        HidDeviceEntry entry;
        entry.handle = handle;
        entry.readStopFlag   = std::make_shared<std::atomic<bool>>(true);
        entry.readPollMs     = std::make_shared<std::atomic<int>>(m_defaultReadPollMs.load());
        entry.readDataLength = std::make_shared<std::atomic<int>>(m_defaultReadDataLength.load());

        {
            std::lock_guard<std::mutex> lock(m_deviceMapMutex);
            m_deviceMap[path] = std::move(entry);
        }

        if (readable) {
            startReadThread(path);
        }

        return true;
    } catch (const std::exception&) {
        triggerError(ErrorCodes::SimpleCommKitHidOpenError);
        return false;
    }
}

bool SimpleCommKitHidPrivate::open(unsigned short vendor_id,
                                    unsigned short product_id,
                                    const std::string& serial_number,
                                    bool readable)
{
    try {
        const wchar_t* wsn = nullptr;
        std::wstring wserial;
        if (!serial_number.empty()) {
            wserial = utf8ToWstring(serial_number);
            wsn = wserial.c_str();
        }

        hid_device* handle = hid_open(vendor_id, product_id, wsn);
        if (!handle) {
            triggerError(ErrorCodes::SimpleCommKitHidOpenError);
            return false;
        }

        // Use VID:PID:SERIAL as key (hid_open doesn't return path)
        std::string deviceKey = std::to_string(vendor_id) + ":" +
                                std::to_string(product_id) + ":" +
                                (serial_number.empty() ? "*" : serial_number);

        {
            std::lock_guard<std::mutex> lock(m_deviceMapMutex);
            if (m_deviceMap.find(deviceKey) != m_deviceMap.end()) {
                hid_close(handle);
                triggerError(ErrorCodes::SimpleCommKitHidOpenError);
                return false;  // already open
            }
        }

#ifdef _WIN32
        HidD_SetNumInputBuffers(*static_cast<HANDLE*>(handle), 9);
#endif

        hid_set_nonblocking(handle, 1);

        HidDeviceEntry entry;
        entry.handle = handle;
        entry.readStopFlag   = std::make_shared<std::atomic<bool>>(true);
        entry.readPollMs     = std::make_shared<std::atomic<int>>(m_defaultReadPollMs.load());
        entry.readDataLength = std::make_shared<std::atomic<int>>(m_defaultReadDataLength.load());

        {
            std::lock_guard<std::mutex> lock(m_deviceMapMutex);
            m_deviceMap[deviceKey] = std::move(entry);
        }

        if (readable) {
            startReadThread(deviceKey);
        }

        return true;
    } catch (const std::exception&) {
        triggerError(ErrorCodes::SimpleCommKitHidOpenError);
        return false;
    }
}

void SimpleCommKitHidPrivate::close()
{
    // Close all devices — stopReadThread closes handles for readable ones
    stopAllReadThreads();

    std::lock_guard<std::mutex> lock(m_deviceMapMutex);
    // Close remaining handles (non-readable devices)
    for (auto& [path, entry] : m_deviceMap) {
        if (entry.handle) {
            hid_close(entry.handle);
            entry.handle = nullptr;
        }
    }
    m_deviceMap.clear();
}

void SimpleCommKitHidPrivate::close(const std::string& path)
{
    // stopReadThread sets stopFlag, closes handle, joins thread
    stopReadThread(path);

    std::lock_guard<std::mutex> lock(m_deviceMapMutex);
    auto it = m_deviceMap.find(path);
    if (it != m_deviceMap.end()) {
        // handle already closed by stopReadThread (if readable),
        // still close for non-readable devices
        if (it->second.handle) {
            hid_close(it->second.handle);
        }
        m_deviceMap.erase(it);
    }
}

bool SimpleCommKitHidPrivate::isOpen()
{
    std::lock_guard<std::mutex> lock(m_deviceMapMutex);
    return !m_deviceMap.empty();
}

bool SimpleCommKitHidPrivate::isOpen(const std::string& path)
{
    std::lock_guard<std::mutex> lock(m_deviceMapMutex);
    return m_deviceMap.find(path) != m_deviceMap.end();
}

//
// I/O  (path-less versions operate on first open device)
//
static hid_device* getFirstDevice(std::map<std::string, HidDeviceEntry>& deviceMap)
{
    if (deviceMap.empty()) return nullptr;
    return deviceMap.begin()->second.handle;
}

int SimpleCommKitHidPrivate::write(const std::vector<uint8_t>& data)
{
    if (data.empty()) return 0;

    std::lock_guard<std::mutex> lock(m_deviceMapMutex);
    hid_device* handle = getFirstDevice(m_deviceMap);
    if (!handle) {
        triggerError(ErrorCodes::SimpleCommKitHidNotOpenError);
        return -1;
    }

    try {
        int ret = hid_write(handle, data.data(), data.size());
        if (ret < 0) {
            triggerError(ErrorCodes::SimpleCommKitHidWriteError);
        }
        return ret;
    } catch (const std::exception&) {
        triggerError(ErrorCodes::SimpleCommKitHidWriteError);
        return -1;
    }
}

int SimpleCommKitHidPrivate::write(const std::string& path, const std::vector<uint8_t>& data)
{
    if (data.empty()) return 0;

    std::lock_guard<std::mutex> lock(m_deviceMapMutex);
    auto it = m_deviceMap.find(path);
    if (it == m_deviceMap.end() || !it->second.handle) {
        triggerError(ErrorCodes::SimpleCommKitHidNotOpenError);
        return -1;
    }

    try {
        int ret = hid_write(it->second.handle, data.data(), data.size());
        if (ret < 0) {
            triggerError(ErrorCodes::SimpleCommKitHidWriteError);
        }
        return ret;
    } catch (const std::exception&) {
        triggerError(ErrorCodes::SimpleCommKitHidWriteError);
        return -1;
    }
}

int SimpleCommKitHidPrivate::sendFeatureReport(const std::vector<uint8_t>& data)
{
    if (data.empty()) return 0;

    std::lock_guard<std::mutex> lock(m_deviceMapMutex);
    hid_device* handle = getFirstDevice(m_deviceMap);
    if (!handle) {
        triggerError(ErrorCodes::SimpleCommKitHidNotOpenError);
        return -1;
    }

    try {
        int ret = hid_send_feature_report(handle, data.data(), data.size());
        if (ret < 0) {
            triggerError(ErrorCodes::SimpleCommKitHidFeatureReportError);
        }
        return ret;
    } catch (const std::exception&) {
        triggerError(ErrorCodes::SimpleCommKitHidFeatureReportError);
        return -1;
    }
}

int SimpleCommKitHidPrivate::sendFeatureReport(const std::string& path, const std::vector<uint8_t>& data)
{
    if (data.empty()) return 0;

    std::lock_guard<std::mutex> lock(m_deviceMapMutex);
    auto it = m_deviceMap.find(path);
    if (it == m_deviceMap.end() || !it->second.handle) {
        triggerError(ErrorCodes::SimpleCommKitHidNotOpenError);
        return -1;
    }

    try {
        int ret = hid_send_feature_report(it->second.handle, data.data(), data.size());
        if (ret < 0) {
            triggerError(ErrorCodes::SimpleCommKitHidFeatureReportError);
        }
        return ret;
    } catch (const std::exception&) {
        triggerError(ErrorCodes::SimpleCommKitHidFeatureReportError);
        return -1;
    }
}

//
// Hotplug
//
void SimpleCommKitHidPrivate::startHotplug(unsigned short vendor_id,
                                            unsigned short product_id)
{
    stopHotplug();  // ensure previous thread is stopped

    m_hotplugVendorId  = vendor_id;
    m_hotplugProductId = product_id;
    m_hotplugStop      = false;

    m_hotplugThread = std::make_unique<std::thread>([this]() {
        while (!m_hotplugStop) {
            try {
                hid_device_info* hid_info = hid_enumerate(m_hotplugVendorId,
                                                           m_hotplugProductId);
                std::map<std::string, SimpleCommKitHidDeviceInfo> currentDevices;
                copyDeviceInfo(hid_info, currentDevices);
                hid_free_enumeration(hid_info);

                std::vector<SimpleCommKitHidDeviceInfo> added;
                std::vector<SimpleCommKitHidDeviceInfo> removed;
                compareDevices(currentDevices, added, removed);

                // Auto-close open handles for removed devices
                if (!removed.empty()) {
                    for (const auto& dev : removed) {
                        close(dev.path);
                    }
                }

                if (m_onHotPlug && (!added.empty() || !removed.empty())) {
                    m_onHotPlug(added, removed);
                }
            } catch (const std::exception&) {
                triggerError(ErrorCodes::SimpleCommKitHidHotplugError);
            }

            std::this_thread::sleep_for(
                std::chrono::milliseconds(m_hotplugPollMs.load()));
        }
    });
}

void SimpleCommKitHidPrivate::stopHotplug()
{
    if (m_hotplugThread) {
        m_hotplugStop = true;
        if (m_hotplugThread->joinable()) {
            m_hotplugThread->join();
        }
        m_hotplugThread.reset();
    }
}

bool SimpleCommKitHidPrivate::isHotplugActive()
{
    return m_hotplugThread != nullptr && !m_hotplugStop;
}

void SimpleCommKitHidPrivate::setHotplugPollInterval(int ms)
{
    m_hotplugPollMs = (ms > 0) ? ms : 1000;
}

int SimpleCommKitHidPrivate::getHotplugPollInterval()
{
    return m_hotplugPollMs;
}

//
// Read polling config (global defaults)
//
void SimpleCommKitHidPrivate::setReadPollInterval(int ms)
{
    m_defaultReadPollMs = (ms > 0) ? ms : 1;
}

int SimpleCommKitHidPrivate::getReadPollInterval()
{
    return m_defaultReadPollMs;
}

void SimpleCommKitHidPrivate::setReadDataLength(int length)
{
    m_defaultReadDataLength = (length > 0) ? length : 64;
}

int SimpleCommKitHidPrivate::getReadDataLength()
{
    return m_defaultReadDataLength;
}

//
// Per-device read config
//
void SimpleCommKitHidPrivate::setReadPollInterval(const std::string& path, int ms)
{
    std::lock_guard<std::mutex> lock(m_deviceMapMutex);
    auto it = m_deviceMap.find(path);
    if (it != m_deviceMap.end() && it->second.readPollMs) {
        it->second.readPollMs->store((ms > 0) ? ms : 1);
    }
}

int SimpleCommKitHidPrivate::getReadPollInterval(const std::string& path)
{
    std::lock_guard<std::mutex> lock(m_deviceMapMutex);
    auto it = m_deviceMap.find(path);
    if (it != m_deviceMap.end() && it->second.readPollMs) {
        return it->second.readPollMs->load();
    }
    return m_defaultReadPollMs;
}

void SimpleCommKitHidPrivate::setReadDataLength(const std::string& path, int length)
{
    std::lock_guard<std::mutex> lock(m_deviceMapMutex);
    auto it = m_deviceMap.find(path);
    if (it != m_deviceMap.end() && it->second.readDataLength) {
        it->second.readDataLength->store((length > 0) ? length : 64);
    }
}

int SimpleCommKitHidPrivate::getReadDataLength(const std::string& path)
{
    std::lock_guard<std::mutex> lock(m_deviceMapMutex);
    auto it = m_deviceMap.find(path);
    if (it != m_deviceMap.end() && it->second.readDataLength) {
        return it->second.readDataLength->load();
    }
    return m_defaultReadDataLength;
}

//
// Open paths
//
std::vector<std::string> SimpleCommKitHidPrivate::getOpenPaths()
{
    std::lock_guard<std::mutex> lock(m_deviceMapMutex);
    std::vector<std::string> paths;
    paths.reserve(m_deviceMap.size());
    for (const auto& [path, entry] : m_deviceMap) {
        paths.push_back(path);
    }
    return paths;
}

//
// Device list
//
std::vector<SimpleCommKitHidDeviceInfo> SimpleCommKitHidPrivate::getDeviceList()
{
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    std::vector<SimpleCommKitHidDeviceInfo> result;
    result.reserve(m_devices.size());
    for (const auto& [path, info] : m_devices) {
        result.push_back(info);
    }
    return result;
}

//
// Read thread (per-device)
//
void SimpleCommKitHidPrivate::startReadThread(const std::string& path)
{
    stopReadThread(path);  // ensure no duplicate threads

    // Get the entry under lock
    std::lock_guard<std::mutex> lock(m_deviceMapMutex);
    auto it = m_deviceMap.find(path);
    if (it == m_deviceMap.end()) return;

    auto& entry = it->second;
    hid_device* handle = entry.handle;
    auto stopFlag   = entry.readStopFlag;
    auto pollMs     = entry.readPollMs;
    auto dataLength = entry.readDataLength;

    stopFlag->store(false);

    entry.readThread = std::make_unique<std::thread>([this, path, handle, stopFlag, pollMs, dataLength]() {
        while (!stopFlag->load()) {
            if (!handle) {
                break;
            }
            try {
                int dataLen = dataLength->load();
                // +1 for possible Report ID byte
                std::vector<uint8_t> buf(static_cast<size_t>(dataLen) + 1);
                int res = hid_read_timeout(handle, buf.data(), buf.size(), pollMs->load());
                if (res > 0) {
                    int copyLen = (res >= dataLen) ? dataLen : res;
                    std::vector<uint8_t> data(buf.begin() + 1,
                                               buf.begin() + 1 + copyLen);
                    if (m_onRead) {
                        m_onRead(data);
                    }
                } else if (res < 0) {
                    // Read error or handle closed — just exit loop
                    break;
                }
                // res == 0: timeout expired, no data — loop immediately retries
            } catch (const std::exception&) {
                triggerError(ErrorCodes::SimpleCommKitHidReadError);
                break;
            }
        }
    });
}

void SimpleCommKitHidPrivate::stopReadThread(const std::string& path)
{
    std::unique_lock<std::mutex> lock(m_deviceMapMutex);
    auto it = m_deviceMap.find(path);
    if (it == m_deviceMap.end()) return;

    auto& entry = it->second;
    if (entry.readStopFlag) {
        entry.readStopFlag->store(true);
    }

    // Only close handle if a read thread is actually running.
    // Otherwise we risk closing a freshly-opened handle from open()→startReadThread().
    if (entry.readThread && entry.handle) {
        hid_close(entry.handle);
        entry.handle = nullptr;
    }

    // Move thread out so we can join without holding the lock
    auto thread = std::move(entry.readThread);
    lock.unlock();
    if (thread && thread->joinable()) {
        thread->join();
    }
}

void SimpleCommKitHidPrivate::stopAllReadThreads()
{
    // Copy paths to avoid issues with erasing while iterating
    std::vector<std::string> paths;
    {
        std::lock_guard<std::mutex> lock(m_deviceMapMutex);
        for (const auto& [path, entry] : m_deviceMap) {
            paths.push_back(path);
        }
    }

    for (const auto& path : paths) {
        stopReadThread(path);
    }
}

//
// Callbacks
//
void SimpleCommKitHidPrivate::setCallbackOnRead(
    std::function<void(const std::vector<uint8_t>&)> callback)
{
    m_onRead = std::move(callback);
}

void SimpleCommKitHidPrivate::setCallbackOnHotPlug(
    std::function<void(const std::vector<SimpleCommKitHidDeviceInfo>&,
                       const std::vector<SimpleCommKitHidDeviceInfo>&)> callback)
{
    m_onHotPlug = std::move(callback);
}

void SimpleCommKitHidPrivate::setCallbackError(
    std::function<void(SimpleCommKit::ErrorCode)> callback)
{
    m_onError = std::move(callback);
}

void SimpleCommKitHidPrivate::triggerError(SimpleCommKit::ErrorCode error_code)
{
    if (m_onError) {
        m_onError(error_code);
    }
}

} // namespace SimpleCommKit
