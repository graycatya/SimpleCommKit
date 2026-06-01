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

    for (hid_device_info* cur = hid_info; cur != nullptr; cur = cur->next) {
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
        stopReadThread();
        if (m_device) {
            hid_close(m_device);
            m_device = nullptr;
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
// Lifecycle
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
        stopReadThread();

        if (m_device) {
            hid_close(m_device);
            m_device = nullptr;
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

bool SimpleCommKitHidPrivate::open(const std::string& path)
{
    if (m_device) {
        triggerError(ErrorCodes::SimpleCommKitHidOpenError);
        return false;
    }

    try {
        m_device = hid_open_path(path.c_str());
        if (!m_device) {
            triggerError(ErrorCodes::SimpleCommKitHidOpenError);
            return false;
        }

#ifdef _WIN32
        // Increase input buffer count on Windows for better throughput
        HidD_SetNumInputBuffers(*static_cast<HANDLE*>(m_device), 9);
#endif

        // Set blocking mode
        hid_set_nonblocking(m_device, 0);

        // Start background read thread
        startReadThread();

        return true;
    } catch (const std::exception&) {
        triggerError(ErrorCodes::SimpleCommKitHidOpenError);
        return false;
    }
}

bool SimpleCommKitHidPrivate::open(unsigned short vendor_id,
                                    unsigned short product_id,
                                    const std::string& serial_number)
{
    if (m_device) {
        triggerError(ErrorCodes::SimpleCommKitHidOpenError);
        return false;
    }

    try {
        const wchar_t* wsn = nullptr;
        std::wstring wserial;
        if (!serial_number.empty()) {
            wserial = utf8ToWstring(serial_number);
            wsn = wserial.c_str();
        }

        m_device = hid_open(vendor_id, product_id, wsn);
        if (!m_device) {
            triggerError(ErrorCodes::SimpleCommKitHidOpenError);
            return false;
        }

#ifdef _WIN32
        HidD_SetNumInputBuffers(*static_cast<HANDLE*>(m_device), 9);
#endif

        hid_set_nonblocking(m_device, 0);

        startReadThread();

        return true;
    } catch (const std::exception&) {
        triggerError(ErrorCodes::SimpleCommKitHidOpenError);
        return false;
    }
}

void SimpleCommKitHidPrivate::close()
{
    try {
        stopReadThread();

        if (m_device) {
            hid_close(m_device);
            m_device = nullptr;
        }
    } catch (const std::exception&) {
        triggerError(ErrorCodes::SimpleCommKitHidCloseError);
    }
}

bool SimpleCommKitHidPrivate::isOpen()
{
    return m_device != nullptr;
}

//
// I/O
//
int SimpleCommKitHidPrivate::write(const std::vector<uint8_t>& data)
{
    if (!m_device) {
        triggerError(ErrorCodes::SimpleCommKitHidNotOpenError);
        return -1;
    }

    if (data.empty()) {
        return 0;
    }

    try {
        int ret = hid_write(m_device, data.data(), data.size());
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
    if (!m_device) {
        triggerError(ErrorCodes::SimpleCommKitHidNotOpenError);
        return -1;
    }

    if (data.empty()) {
        return 0;
    }

    try {
        int ret = hid_send_feature_report(m_device, data.data(), data.size());
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
// Read polling config
//
void SimpleCommKitHidPrivate::setReadPollInterval(int ms)
{
    m_readPollMs = (ms > 0) ? ms : 1;
}

int SimpleCommKitHidPrivate::getReadPollInterval()
{
    return m_readPollMs;
}

void SimpleCommKitHidPrivate::setReadDataLength(int length)
{
    m_readDataLength = (length > 0) ? length : 64;
}

int SimpleCommKitHidPrivate::getReadDataLength()
{
    return m_readDataLength;
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
// Read thread
//
void SimpleCommKitHidPrivate::startReadThread()
{
    stopReadThread();  // ensure no duplicate threads

    m_readThreadStop = false;

    m_readThread = std::make_unique<std::thread>([this]() {
        while (!m_readThreadStop) {
            if (!m_device) {
                break;
            }

            try {
                int dataLen = m_readDataLength.load();
                // +1 for possible Report ID byte
                std::vector<uint8_t> buf(static_cast<size_t>(dataLen) + 1);

                int res = hid_read(m_device, buf.data(), buf.size());

                if (res > 0) {
                    // Return only the requested data length (skip Report ID byte
                    // if present)
                    int copyLen = (res >= dataLen) ? dataLen : res;
                    std::vector<uint8_t> data(buf.begin() + 1,
                                               buf.begin() + 1 + copyLen);
                    if (m_onRead) {
                        m_onRead(data);
                    }
                } else if (res < 0) {
                    // Read error – device may have been unplugged
                    triggerError(ErrorCodes::SimpleCommKitHidReadError);
                    break;
                }
                // res == 0 means no data available (shouldn't happen in
                // blocking mode, but guard anyway)
            } catch (const std::exception&) {
                triggerError(ErrorCodes::SimpleCommKitHidReadError);
                break;
            }

            std::this_thread::sleep_for(
                std::chrono::milliseconds(m_readPollMs.load()));
        }
    });
}

void SimpleCommKitHidPrivate::stopReadThread()
{
    if (m_readThread) {
        m_readThreadStop = true;
        if (m_readThread->joinable()) {
            m_readThread->join();
        }
        m_readThread.reset();
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
