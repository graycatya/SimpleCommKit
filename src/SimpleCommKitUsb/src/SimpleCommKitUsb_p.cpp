#include "SimpleCommKitUsb.h"
#include "SimpleCommKitUsb_p.h"

#include <libusb.h>

#include <iostream>
#include <chrono>
#include <algorithm>
#include <cstring>

// Platform-specific headers for sleep
#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

namespace SimpleCommKit {

// ============================================================
// Helpers
// ============================================================

std::string SimpleCommKitUsbPrivate::makeDevicePath(uint8_t bus, uint8_t addr)
{
    return std::to_string(bus) + ":" + std::to_string(addr);
}

// Helper: read string descriptor from device
static std::string readStringDescriptor(libusb_device_handle* handle, uint8_t index)
{
    if (index == 0) return std::string();
    unsigned char buf[256];
    int len = libusb_get_string_descriptor_ascii(handle, index, buf, static_cast<int>(sizeof(buf)));
    if (len > 0) {
        return std::string(reinterpret_cast<char*>(buf), static_cast<size_t>(len));
    }
    return std::string();
}

// ============================================================
// Device enumeration
// ============================================================

std::vector<SimpleCommKitUsbDeviceInfo>
SimpleCommKitUsbPrivate::enumerateDevices(libusb_context* ctx,
                                           unsigned short vendor_id,
                                           unsigned short product_id)
{
    std::vector<SimpleCommKitUsbDeviceInfo> result;
    if (!ctx) return result;

    libusb_device** devs = nullptr;
    ssize_t cnt = libusb_get_device_list(ctx, &devs);
    if (cnt < 0) return result;

    for (ssize_t i = 0; i < cnt; i++) {
        libusb_device* dev = devs[i];

        libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(dev, &desc) != 0) {
            continue;
        }

        // Filter by VID/PID if specified
        if (vendor_id != 0 && desc.idVendor != vendor_id) continue;
        if (product_id != 0 && desc.idProduct != product_id) continue;

        SimpleCommKitUsbDeviceInfo info;
        info.vendor_id      = desc.idVendor;
        info.product_id     = desc.idProduct;
        info.bus_number     = libusb_get_bus_number(dev);
        info.device_address = libusb_get_device_address(dev);
        info.path           = makeDevicePath(info.bus_number, info.device_address);

        // Open temporarily to read string descriptors
        libusb_device_handle* handle = nullptr;
        if (libusb_open(dev, &handle) == 0) {
            info.manufacturer_string = readStringDescriptor(handle, desc.iManufacturer);
            info.product_string      = readStringDescriptor(handle, desc.iProduct);
            info.serial_number       = readStringDescriptor(handle, desc.iSerialNumber);
            libusb_close(handle);
        }

        result.push_back(info);
    }

    libusb_free_device_list(devs, 1);
    return result;
}

// ============================================================
// Device comparison (for hotplug detection)
// ============================================================

void SimpleCommKitUsbPrivate::compareDevices(
    const std::map<std::string, SimpleCommKitUsbDeviceInfo>& current,
    std::vector<SimpleCommKitUsbDeviceInfo>& added,
    std::vector<SimpleCommKitUsbDeviceInfo>& removed)
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
// SimpleCommKitUsbPrivate Implementation
// ============================================================

SimpleCommKitUsbPrivate::SimpleCommKitUsbPrivate(SimpleCommKitUsb* parent)
    : q_ptr(parent)
{
}

SimpleCommKitUsbPrivate::~SimpleCommKitUsbPrivate()
{
    try {
        stopHotplug();
        stopReadPoll();
        close();
        if (m_usbContext) {
            libusb_exit(m_usbContext);
            m_usbContext = nullptr;
        }
    } catch (...) {
        // ignore errors in destructor
    }
}

//
// Static
//
std::vector<SimpleCommKitUsbDeviceInfo>
SimpleCommKitUsbPrivate::getAvailableDevices(unsigned short vendor_id,
                                              unsigned short product_id)
{
    libusb_context* ctx = nullptr;
    int ret = libusb_init(&ctx);
    if (ret != 0) return {};

    auto result = enumerateDevices(ctx, vendor_id, product_id);

    libusb_exit(ctx);
    return result;
}

//
// Lifecycle
//
bool SimpleCommKitUsbPrivate::init()
{
    try {
        if (m_usbContext) {
            return true;  // already initialized
        }

        int ret = libusb_init(&m_usbContext);
        if (ret != 0) {
            triggerError(ErrorCodes::SimpleCommKitUsbInitError);
            return false;
        }

        // Enumerate and cache all devices
        auto devs = enumerateDevices(m_usbContext, 0, 0);
        {
            std::lock_guard<std::mutex> lock(m_devicesMutex);
            m_devices.clear();
            for (const auto& dev : devs) {
                m_devices[dev.path] = dev;
            }
        }
        return true;
    } catch (const std::exception&) {
        triggerError(ErrorCodes::SimpleCommKitUsbInitError);
        return false;
    }
}

void SimpleCommKitUsbPrivate::exit()
{
    try {
        stopHotplug();
        stopReadPoll();
        close();

        if (m_usbContext) {
            libusb_exit(m_usbContext);
            m_usbContext = nullptr;
        }

        {
            std::lock_guard<std::mutex> lock(m_devicesMutex);
            m_devices.clear();
        }
    } catch (const std::exception&) {
        triggerError(ErrorCodes::SimpleCommKitUsbExitError);
    }
}

//
// Open / close – single device
//
bool SimpleCommKitUsbPrivate::open(const std::string& path)
{
    if (!m_usbContext) {
        triggerError(ErrorCodes::SimpleCommKitUsbOpenError);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_handleMutex);
        if (m_handle) {
            triggerError(ErrorCodes::SimpleCommKitUsbOpenError);
            return false;  // already open
        }
    }

    try {
        // Parse bus:address from path
        size_t colon = path.find(':');
        if (colon == std::string::npos) {
            triggerError(ErrorCodes::SimpleCommKitUsbOpenError);
            return false;
        }
        uint8_t bus  = static_cast<uint8_t>(std::stoi(path.substr(0, colon)));
        uint8_t addr = static_cast<uint8_t>(std::stoi(path.substr(colon + 1)));

        // Find device by bus:address
        libusb_device** devs = nullptr;
        ssize_t cnt = libusb_get_device_list(m_usbContext, &devs);
        if (cnt < 0) {
            triggerError(ErrorCodes::SimpleCommKitUsbOpenError);
            return false;
        }

        libusb_device_handle* handle = nullptr;
        for (ssize_t i = 0; i < cnt; i++) {
            if (libusb_get_bus_number(devs[i]) == bus &&
                libusb_get_device_address(devs[i]) == addr) {
                int ret = libusb_open(devs[i], &handle);
                if (ret != 0) {
                    libusb_free_device_list(devs, 1);
                    triggerError(ErrorCodes::SimpleCommKitUsbOpenError);
                    return false;
                }
                break;
            }
        }
        libusb_free_device_list(devs, 1);

        if (!handle) {
            triggerError(ErrorCodes::SimpleCommKitUsbOpenError);
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(m_handleMutex);
            m_handle = handle;
            m_devicePath = path;
        }

        return true;
    } catch (const std::exception&) {
        triggerError(ErrorCodes::SimpleCommKitUsbOpenError);
        return false;
    }
}

bool SimpleCommKitUsbPrivate::open(unsigned short vendor_id,
                                    unsigned short product_id,
                                    const std::string& serial_number)
{
    if (!m_usbContext) {
        triggerError(ErrorCodes::SimpleCommKitUsbOpenError);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_handleMutex);
        if (m_handle) {
            triggerError(ErrorCodes::SimpleCommKitUsbOpenError);
            return false;
        }
    }

    try {
        libusb_device** devs = nullptr;
        ssize_t cnt = libusb_get_device_list(m_usbContext, &devs);
        if (cnt < 0) {
            triggerError(ErrorCodes::SimpleCommKitUsbOpenError);
            return false;
        }

        libusb_device_handle* handle = nullptr;
        std::string devicePath;

        for (ssize_t i = 0; i < cnt; i++) {
            libusb_device* dev = devs[i];
            libusb_device_descriptor desc;
            if (libusb_get_device_descriptor(dev, &desc) != 0) continue;

            if (desc.idVendor != vendor_id || desc.idProduct != product_id) continue;

            // Open temporarily to check serial
            libusb_device_handle* tmpHandle = nullptr;
            if (libusb_open(dev, &tmpHandle) != 0) continue;

            std::string devSerial = readStringDescriptor(tmpHandle, desc.iSerialNumber);

            if (!serial_number.empty() && devSerial != serial_number) {
                libusb_close(tmpHandle);
                continue;
            }

            uint8_t bus  = libusb_get_bus_number(dev);
            uint8_t addr = libusb_get_device_address(dev);
            devicePath = makeDevicePath(bus, addr);

            handle = tmpHandle;  // keep this handle open
            break;
        }
        libusb_free_device_list(devs, 1);

        if (!handle) {
            triggerError(ErrorCodes::SimpleCommKitUsbOpenError);
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(m_handleMutex);
            if (m_handle) {
                libusb_close(handle);
                triggerError(ErrorCodes::SimpleCommKitUsbOpenError);
                return false;
            }
            m_handle = handle;
            m_devicePath = devicePath;
        }

        return true;
    } catch (const std::exception&) {
        triggerError(ErrorCodes::SimpleCommKitUsbOpenError);
        return false;
    }
}

void SimpleCommKitUsbPrivate::close()
{
    stopReadPoll();

    libusb_device_handle* handle = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_handleMutex);
        handle = m_handle;
        m_handle = nullptr;
    }

    if (handle) {
        // Release all claimed interfaces & restore kernel drivers
        for (int iface : m_claimedInterfaces) {
            libusb_release_interface(handle, iface);
            libusb_attach_kernel_driver(handle, iface);
        }
        m_claimedInterfaces.clear();

        libusb_close(handle);
    }

    {
        std::lock_guard<std::mutex> lock(m_handleMutex);
        m_devicePath.clear();
    }
}

bool SimpleCommKitUsbPrivate::isOpen()
{
    std::lock_guard<std::mutex> lock(m_handleMutex);
    return m_handle != nullptr;
}

//
// Claim / release interface
//
bool SimpleCommKitUsbPrivate::claimInterface(int interface_number)
{
    std::lock_guard<std::mutex> lock(m_handleMutex);
    if (!m_handle) {
        triggerError(ErrorCodes::SimpleCommKitUsbNotOpenError);
        return false;
    }

    // First attempt: claim directly
    int ret = libusb_claim_interface(m_handle, interface_number);
    if (ret == 0) {
        m_claimedInterfaces.insert(interface_number);
        return true;
    }

    // Claim failed — try detaching kernel driver first (Linux; no-op on Windows)
    ret = libusb_detach_kernel_driver(m_handle, interface_number);
    if (ret != 0) {
        triggerError(ErrorCodes::SimpleCommKitUsbClaimInterfaceError);
        return false;
    }

    // Retry claim after detach
    ret = libusb_claim_interface(m_handle, interface_number);
    if (ret != 0) {
        // Restore kernel driver since claim still failed
        libusb_attach_kernel_driver(m_handle, interface_number);
        triggerError(ErrorCodes::SimpleCommKitUsbClaimInterfaceError);
        return false;
    }

    m_claimedInterfaces.insert(interface_number);
    return true;
}

bool SimpleCommKitUsbPrivate::releaseInterface(int interface_number)
{
    std::lock_guard<std::mutex> lock(m_handleMutex);
    if (!m_handle) {
        triggerError(ErrorCodes::SimpleCommKitUsbNotOpenError);
        return false;
    }

    int ret = libusb_release_interface(m_handle, interface_number);
    if (ret != 0) {
        triggerError(ErrorCodes::SimpleCommKitUsbReleaseInterfaceError);
        return false;
    }

    // Restore kernel driver if it was previously detached
    auto it = m_claimedInterfaces.find(interface_number);
    if (it != m_claimedInterfaces.end()) {
        libusb_attach_kernel_driver(m_handle, interface_number);
        m_claimedInterfaces.erase(it);
    }

    return true;
}

//
// Control transfer
//
int SimpleCommKitUsbPrivate::controlTransfer(uint8_t bmRequestType, uint8_t bRequest,
                                              uint16_t wValue, uint16_t wIndex,
                                              std::vector<uint8_t>& data, unsigned int timeout)
{
    std::lock_guard<std::mutex> lock(m_handleMutex);
    if (!m_handle) {
        triggerError(ErrorCodes::SimpleCommKitUsbNotOpenError);
        return -1;
    }

    int ret = libusb_control_transfer(m_handle, bmRequestType, bRequest,
                                       wValue, wIndex,
                                       data.data(), static_cast<uint16_t>(data.size()),
                                       timeout);
    if (ret < 0) {
        triggerError(ErrorCodes::SimpleCommKitUsbControlTransferError);
    }
    return ret;
}

//
// Helper: lookup endpoint max packet size from config descriptor
//
static int getEndpointMaxPkt(libusb_device_handle* handle, uint8_t endpoint)
{
    if (!handle) return 64; // default fallback

    libusb_device* dev = libusb_get_device(handle);
    if (!dev) return 64;

    libusb_config_descriptor* config = nullptr;
    if (libusb_get_config_descriptor(dev, 0, &config) != 0) return 64;

    int maxPkt = 64;
    for (int i = 0; i < config->bNumInterfaces; i++) {
        const libusb_interface& iface = config->interface[i];
        for (int j = 0; j < iface.num_altsetting; j++) {
            const libusb_interface_descriptor& iface_desc = iface.altsetting[j];
            for (int k = 0; k < iface_desc.bNumEndpoints; k++) {
                if (iface_desc.endpoint[k].bEndpointAddress == endpoint) {
                    maxPkt = iface_desc.endpoint[k].wMaxPacketSize;
                    break;
                }
            }
        }
    }
    libusb_free_config_descriptor(config);
    return maxPkt;
}

//
// Bulk transfer
//
int SimpleCommKitUsbPrivate::bulkTransfer(uint8_t endpoint, std::vector<uint8_t>& data,
                                           unsigned int timeout)
{
    std::lock_guard<std::mutex> lock(m_handleMutex);
    if (!m_handle) {
        triggerError(ErrorCodes::SimpleCommKitUsbNotOpenError);
        return -1;
    }

    // Pad OUT transfers to max packet size
    bool is_in = (endpoint & LIBUSB_ENDPOINT_IN) != 0;
    if (!is_in) {
        int maxPkt = getEndpointMaxPkt(m_handle, endpoint);
        if (static_cast<int>(data.size()) < maxPkt) {
            data.resize(static_cast<size_t>(maxPkt), 0x00);
        }
    }

    int transferred = 0;
    int ret = libusb_bulk_transfer(m_handle, endpoint,
                                    data.data(), static_cast<int>(data.size()),
                                    &transferred, timeout);
    if (ret < 0) {
        triggerError(ErrorCodes::SimpleCommKitUsbBulkTransferError);
        return ret;
    }
    return transferred;
}

//
// Interrupt transfer
//
int SimpleCommKitUsbPrivate::interruptTransfer(uint8_t endpoint, std::vector<uint8_t>& data,
                                                unsigned int timeout)
{
    std::lock_guard<std::mutex> lock(m_handleMutex);
    if (!m_handle) {
        triggerError(ErrorCodes::SimpleCommKitUsbNotOpenError);
        return -1;
    }

    // Pad OUT transfers to max packet size
    bool is_in = (endpoint & LIBUSB_ENDPOINT_IN) != 0;
    if (!is_in) {
        int maxPkt = getEndpointMaxPkt(m_handle, endpoint);
        if (static_cast<int>(data.size()) < maxPkt) {
            data.resize(static_cast<size_t>(maxPkt), 0x00);
        }
    }

    int transferred = 0;
    int ret = libusb_interrupt_transfer(m_handle, endpoint,
                                         data.data(), static_cast<int>(data.size()),
                                         &transferred, timeout);
    if (ret < 0) {
        triggerError(ErrorCodes::SimpleCommKitUsbInterruptTransferError);
        return ret;
    }
    return transferred;
}

// ============================================================
// Isochronous transfer (synchronous wrapper over async API)
// ============================================================

// Struct passed as user_data to the isochronous transfer callback
struct IsoTransferContext {
    std::atomic<bool> done{false};
};

void LIBUSB_CALL SimpleCommKitUsbPrivate::isoTransferCallback(struct libusb_transfer* transfer)
{
    auto* ctx = static_cast<IsoTransferContext*>(transfer->user_data);
    if (ctx) {
        ctx->done.store(true);
    }
}

int SimpleCommKitUsbPrivate::syncIsoTransfer(
    libusb_device_handle* handle, uint8_t endpoint,
    unsigned char* data, int length,
    int num_packets, const int* packet_lengths,
    std::vector<SimpleCommKitUsbIsoPacketResult>& results,
    unsigned int timeout_ms)
{
    if (!handle) return LIBUSB_ERROR_NO_DEVICE;
    if (num_packets <= 0) return LIBUSB_ERROR_INVALID_PARAM;

    // Allocate transfer
    libusb_transfer* transfer = libusb_alloc_transfer(num_packets);
    if (!transfer) return LIBUSB_ERROR_NO_MEM;

    IsoTransferContext ctx;

    // Fill isochronous transfer (user_data last param)
    libusb_fill_iso_transfer(transfer, handle, endpoint,
                              data, length, num_packets,
                              isoTransferCallback, &ctx, timeout_ms);

    // Set per-packet lengths
    if (packet_lengths) {
        int total = 0;
        for (int i = 0; i < num_packets; i++) {
            transfer->iso_packet_desc[i].length = packet_lengths[i];
            total += packet_lengths[i];
        }
        // For OUT, verify length matches
        if ((endpoint & LIBUSB_ENDPOINT_DIR_MASK) == 0) {
            // Ensure buffer is large enough
            if (length < total) {
                libusb_free_transfer(transfer);
                return LIBUSB_ERROR_INVALID_PARAM;
            }
        }
    }

    // Submit the transfer
    int ret = libusb_submit_transfer(transfer);
    if (ret != 0) {
        libusb_free_transfer(transfer);
        return ret;
    }

    // Wait for completion by handling events with timeout
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    while (!ctx.done.load()) {
        auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            // Timeout — cancel the transfer
            libusb_cancel_transfer(transfer);
            // Handle events so the cancellation callback fires
            struct timeval tv = {0, 50000}; // 50ms
            libusb_handle_events_timeout(m_usbContext, &tv);
            break;
        }

        // Handle events (the callback fires within this call)
        struct timeval tv = {0, 50000}; // 50ms
        libusb_handle_events_timeout(m_usbContext, &tv);
    }

    // Process results
    results.resize(num_packets);
    int total_actual = 0;
    for (int i = 0; i < num_packets; i++) {
        results[i].length        = transfer->iso_packet_desc[i].length;
        results[i].actual_length = transfer->iso_packet_desc[i].actual_length;
        results[i].status        = transfer->iso_packet_desc[i].status;
        if (transfer->iso_packet_desc[i].status == LIBUSB_TRANSFER_COMPLETED) {
            total_actual += transfer->iso_packet_desc[i].actual_length;
        }
    }

    int transfer_status = transfer->status;
    libusb_free_transfer(transfer);

    if (transfer_status == LIBUSB_TRANSFER_COMPLETED) {
        return total_actual;
    } else if (transfer_status == LIBUSB_TRANSFER_TIMED_OUT) {
        return LIBUSB_ERROR_TIMEOUT;
    } else if (transfer_status == LIBUSB_TRANSFER_CANCELLED) {
        return LIBUSB_ERROR_INTERRUPTED;
    } else {
        return LIBUSB_ERROR_OTHER;
    }
}

int SimpleCommKitUsbPrivate::isochronousTransfer(
    uint8_t endpoint, std::vector<uint8_t>& data,
    int num_packets, const std::vector<int>& packet_lengths,
    std::vector<SimpleCommKitUsbIsoPacketResult>& results,
    unsigned int timeout)
{
    std::lock_guard<std::mutex> lock(m_handleMutex);
    if (!m_handle) {
        triggerError(ErrorCodes::SimpleCommKitUsbIsochronousTransferError);
        return -1;
    }

    // Validate packet lengths
    if (static_cast<size_t>(num_packets) != packet_lengths.size()) {
        triggerError(ErrorCodes::SimpleCommKitUsbIsochronousTransferError);
        return -1;
    }

    bool is_in = (endpoint & LIBUSB_ENDPOINT_IN) != 0;
    int buf_size = static_cast<int>(data.size());

    // For IN, auto-resize buffer to total packet capacity
    if (is_in) {
        int total_length = 0;
        for (int len : packet_lengths) total_length += len;
        data.resize(static_cast<size_t>(total_length));
        buf_size = total_length;
    }

    int ret = syncIsoTransfer(m_handle, endpoint,
                               data.data(), buf_size,
                               num_packets, packet_lengths.data(),
                               results, timeout);
    if (ret < 0) {
        triggerError(ErrorCodes::SimpleCommKitUsbIsochronousTransferError);
    }
    return ret;
}

//
// Read polling (single device)
//
void SimpleCommKitUsbPrivate::startReadPoll(uint8_t endpoint)
{
    stopReadPoll();

    libusb_device_handle* handle = nullptr;
    std::string devicePath;
    {
        std::lock_guard<std::mutex> lock(m_handleMutex);
        if (!m_handle) return;
        handle = m_handle;
        devicePath = m_devicePath;
    }

    m_readEndpoint = endpoint;
    m_readStopFlag.store(false);

    // Look up device info for the callback
    SimpleCommKitUsbDeviceInfo devInfo;
    {
        std::lock_guard<std::mutex> devLock(m_devicesMutex);
        auto devIt = m_devices.find(devicePath);
        if (devIt != m_devices.end()) {
            devInfo = devIt->second;
        } else {
            devInfo.path = devicePath;
        }
    }

    // Detect endpoint type & max packet size from descriptor
    bool isBulk = false;
    int maxPkt = 64; // default fallback
    {
        libusb_device* dev = libusb_get_device(handle);
        if (dev) {
            libusb_config_descriptor* config = nullptr;
            if (libusb_get_config_descriptor(dev, 0, &config) == 0) {
                for (int i = 0; i < config->bNumInterfaces; i++) {
                    const libusb_interface& iface = config->interface[i];
                    for (int j = 0; j < iface.num_altsetting; j++) {
                        const libusb_interface_descriptor& iface_desc = iface.altsetting[j];
                        for (int k = 0; k < iface_desc.bNumEndpoints; k++) {
                            const libusb_endpoint_descriptor& ep = iface_desc.endpoint[k];
                            if (ep.bEndpointAddress == endpoint) {
                                isBulk = ((ep.bmAttributes & 0x03) == LIBUSB_TRANSFER_TYPE_BULK);
                                maxPkt = ep.wMaxPacketSize;
                                break;
                            }
                        }
                    }
                }
                libusb_free_config_descriptor(config);
            }
        }
    }

    m_readThread = std::make_unique<std::thread>([this, handle, devicePath, endpoint, devInfo, isBulk, maxPkt]() {
        auto stopFlag   = &m_readStopFlag;
        auto pollMs     = &m_readPollMs;
        auto dataLength = &m_readDataLength;

        while (!stopFlag->load()) {
            if (!handle) {
                break;
            }
            try {
                int dataLen = dataLength->load();
                std::vector<uint8_t> buf(static_cast<size_t>(dataLen));
                int transferred = 0;
                int res;
                if (isBulk) {
                    res = libusb_bulk_transfer(handle, endpoint,
                                                buf.data(), dataLen,
                                                &transferred, pollMs->load());
                } else {
                    res = libusb_interrupt_transfer(handle, endpoint,
                                                     buf.data(), dataLen,
                                                     &transferred, pollMs->load());
                }

                if (res == 0) {
                    std::vector<uint8_t> data(buf.begin(), buf.begin() + transferred);
                    if (m_onRead) {
                        //std::cout << "transferred: " << transferred << std::endl;
                        m_onRead(devInfo, data);
                    }
                } else if (res == LIBUSB_ERROR_TIMEOUT) {
                    // Timeout may still have partial data (libusb populates transferred)
                    // if (transferred > 0) {
                    //     std::vector<uint8_t> data(buf.begin(), buf.begin() + transferred);
                    //     if (m_onRead) {
                    //         m_onRead(devInfo, data);
                    //     }
                    // }
                    continue;
                } else if (res < 0 && res != LIBUSB_ERROR_TIMEOUT) {
                    triggerError(ErrorCodes::SimpleCommKitUsbReadError);
                    break;
                }
            } catch (const std::exception&) {
                triggerError(ErrorCodes::SimpleCommKitUsbReadError);
                break;
            }
        }
    });
}

void SimpleCommKitUsbPrivate::stopReadPoll()
{
    m_readStopFlag.store(true);

    if (m_readThread) {
        if (m_readThread->joinable()) {
            m_readThread->join();
        }
        m_readThread.reset();
    }
}

bool SimpleCommKitUsbPrivate::isReadPollActive()
{
    return m_readThread != nullptr && !m_readStopFlag.load();
}

//
// Hotplug — helper: fill device info from a libusb_device
//
static SimpleCommKitUsbDeviceInfo fillDeviceInfo(libusb_device* dev,
                                                  const libusb_device_descriptor& desc)
{
    SimpleCommKitUsbDeviceInfo info;
    info.vendor_id      = desc.idVendor;
    info.product_id     = desc.idProduct;
    info.bus_number     = libusb_get_bus_number(dev);
    info.device_address = libusb_get_device_address(dev);
    info.path           = SimpleCommKitUsbPrivate::makeDevicePath(info.bus_number, info.device_address);

    libusb_device_handle* handle = nullptr;
    if (libusb_open(dev, &handle) == 0) {
        info.manufacturer_string = readStringDescriptor(handle, desc.iManufacturer);
        info.product_string      = readStringDescriptor(handle, desc.iProduct);
        info.serial_number       = readStringDescriptor(handle, desc.iSerialNumber);
        libusb_close(handle);
    }
    return info;
}

//
// Native libusb hotplug callback (C-linkage, free function)
//
int LIBUSB_CALL usbHotplugCallback(libusb_context* /*ctx*/,
                                    libusb_device* device,
                                    libusb_hotplug_event event,
                                    void* user_data)
{
    auto* self = static_cast<SimpleCommKitUsbPrivate*>(user_data);
    if (!self) return 0;

    libusb_device_descriptor desc;
    if (libusb_get_device_descriptor(device, &desc) != 0) {
        return 0;
    }

    if (self->m_hotplugVendorId != 0 && desc.idVendor != self->m_hotplugVendorId) return 0;
    if (self->m_hotplugProductId != 0 && desc.idProduct != self->m_hotplugProductId) return 0;

    SimpleCommKitUsbDeviceInfo info = fillDeviceInfo(device, desc);

    if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
        {
            std::lock_guard<std::mutex> lock(self->m_devicesMutex);
            self->m_devices[info.path] = info;
        }
        if (self->m_onHotPlug) {
            self->m_onHotPlug({info}, {});
        }
    } else if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT) {
        self->close();
        {
            std::lock_guard<std::mutex> lock(self->m_devicesMutex);
            self->m_devices.erase(info.path);
        }
        if (self->m_onHotPlug) {
            self->m_onHotPlug({}, {info});
        }
    }
    return 0;
}

//
// Hotplug
//
void SimpleCommKitUsbPrivate::startHotplug(unsigned short vendor_id,
                                            unsigned short product_id)
{
    stopHotplug();

    if (!m_usbContext) return;

    m_hotplugVendorId  = vendor_id;
    m_hotplugProductId = product_id;
    m_hotplugStop      = false;
    m_hotplugCapable   = libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG);

    if (m_hotplugCapable) {
        int vid = (vendor_id != 0) ? static_cast<int>(vendor_id) : LIBUSB_HOTPLUG_MATCH_ANY;
        int pid = (product_id != 0) ? static_cast<int>(product_id) : LIBUSB_HOTPLUG_MATCH_ANY;

        int ret = libusb_hotplug_register_callback(
            m_usbContext,
            static_cast<libusb_hotplug_event>(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED
                                              | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
            LIBUSB_HOTPLUG_ENUMERATE,
            vid, pid,
            LIBUSB_HOTPLUG_MATCH_ANY,
            usbHotplugCallback,
            this,
            &m_hotplugCallbackHandle);

        if (ret == LIBUSB_SUCCESS) {
            m_hotplugThread = std::make_unique<std::thread>([this]() {
                struct timeval tv;
                while (!m_hotplugStop) {
                    tv.tv_sec  = 0;
                    tv.tv_usec = 200000;
                    libusb_handle_events_timeout(m_usbContext, &tv);
                }
            });
            return;
        }
        // Fall through to polling on register failure
        triggerError(ErrorCodes::SimpleCommKitUsbHotplugError);
        m_hotplugCapable = false;
    }

    //
    // Polling fallback
    //
    m_hotplugThread = std::make_unique<std::thread>([this]() {
        while (!m_hotplugStop) {
            try {
                if (!m_usbContext) break;

                libusb_device** devs = nullptr;
                ssize_t cnt = libusb_get_device_list(m_usbContext, &devs);
                if (cnt < 0) continue;

                std::map<std::string, SimpleCommKitUsbDeviceInfo> currentDevices;
                for (ssize_t i = 0; i < cnt; i++) {
                    libusb_device* dev = devs[i];
                    libusb_device_descriptor desc;
                    if (libusb_get_device_descriptor(dev, &desc) != 0) continue;
                    if (m_hotplugVendorId != 0 && desc.idVendor != m_hotplugVendorId) continue;
                    if (m_hotplugProductId != 0 && desc.idProduct != m_hotplugProductId) continue;
                    auto info = fillDeviceInfo(dev, desc);
                    currentDevices[info.path] = std::move(info);
                }
                libusb_free_device_list(devs, 1);

                std::vector<SimpleCommKitUsbDeviceInfo> added;
                std::vector<SimpleCommKitUsbDeviceInfo> removed;
                compareDevices(currentDevices, added, removed);

                if (!removed.empty()) {
                    // Close if the open device was removed
                    close();
                }

                if (m_onHotPlug && (!added.empty() || !removed.empty())) {
                    m_onHotPlug(added, removed);
                }
            } catch (const std::exception&) {
                triggerError(ErrorCodes::SimpleCommKitUsbHotplugError);
            }
            std::this_thread::sleep_for(
                std::chrono::milliseconds(m_hotplugPollMs.load()));
        }
    });
}

void SimpleCommKitUsbPrivate::stopHotplug()
{
    m_hotplugStop = true;

    if (m_hotplugThread) {
        if (m_hotplugThread->joinable()) {
            m_hotplugThread->join();
        }
        m_hotplugThread.reset();
    }

    if (m_hotplugCallbackHandle && m_usbContext) {
        libusb_hotplug_deregister_callback(m_usbContext, m_hotplugCallbackHandle);
        m_hotplugCallbackHandle = {};
    }
}

bool SimpleCommKitUsbPrivate::isHotplugActive()
{
    return m_hotplugThread != nullptr && !m_hotplugStop;
}

void SimpleCommKitUsbPrivate::setHotplugPollInterval(int ms)
{
    m_hotplugPollMs = (ms > 0) ? ms : 1000;
}

int SimpleCommKitUsbPrivate::getHotplugPollInterval()
{
    return m_hotplugPollMs;
}

//
// Read polling config
//
void SimpleCommKitUsbPrivate::setReadPollInterval(int ms)
{
    m_readPollMs = (ms > 0) ? ms : 1;
}

int SimpleCommKitUsbPrivate::getReadPollInterval()
{
    return m_readPollMs;
}

void SimpleCommKitUsbPrivate::setReadDataLength(int length)
{
    m_readDataLength = (length > 0) ? length : 64;
}

int SimpleCommKitUsbPrivate::getReadDataLength()
{
    return m_readDataLength;
}

//
// Open path
//
std::string SimpleCommKitUsbPrivate::getOpenPath()
{
    std::lock_guard<std::mutex> lock(m_handleMutex);
    return m_devicePath;
}

//
// Device list
//
std::vector<SimpleCommKitUsbDeviceInfo> SimpleCommKitUsbPrivate::getDeviceList()
{
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    std::vector<SimpleCommKitUsbDeviceInfo> result;
    result.reserve(m_devices.size());
    for (const auto& [path, info] : m_devices) {
        result.push_back(info);
    }
    return result;
}

//
// Callbacks
//
void SimpleCommKitUsbPrivate::setCallbackOnRead(
    std::function<void(const SimpleCommKitUsbDeviceInfo&,
                       const std::vector<uint8_t>&)> callback)
{
    m_onRead = std::move(callback);
}

void SimpleCommKitUsbPrivate::setCallbackOnHotPlug(
    std::function<void(const std::vector<SimpleCommKitUsbDeviceInfo>&,
                       const std::vector<SimpleCommKitUsbDeviceInfo>&)> callback)
{
    m_onHotPlug = std::move(callback);
}

void SimpleCommKitUsbPrivate::setCallbackError(
    std::function<void(SimpleCommKit::ErrorCode)> callback)
{
    m_onError = std::move(callback);
}

void SimpleCommKitUsbPrivate::triggerError(SimpleCommKit::ErrorCode error_code)
{
    if (m_onError) {
        m_onError(error_code);
    }
}

// ============================================================
// Configuration descriptor parsing
// ============================================================

std::vector<SimpleCommKitUsbInterfaceInfo> SimpleCommKitUsbPrivate::parseConfigDescriptor(libusb_device* dev)
{
    std::vector<SimpleCommKitUsbInterfaceInfo> interfaces;
    
    if (!dev || !m_usbContext) {
        triggerError(ErrorCodes::SimpleCommKitUsbNotOpenError);
        return interfaces;
    }

    libusb_config_descriptor* config = nullptr;
    int ret = libusb_get_config_descriptor(dev, 0, &config);
    if (ret != 0) {
        triggerError(ErrorCodes::SimpleCommKitUsbGetConfigError);
        return interfaces;
    }

    // Iterate through all interfaces
    for (int i = 0; i < config->bNumInterfaces; i++) {
        const libusb_interface& iface = config->interface[i];
        
        // Iterate through alternate settings
        for (int j = 0; j < iface.num_altsetting; j++) {
            const libusb_interface_descriptor& iface_desc = iface.altsetting[j];
            
            SimpleCommKitUsbInterfaceInfo iface_info;
            iface_info.interface_number  = iface_desc.bInterfaceNumber;
            iface_info.alternate_setting = iface_desc.bAlternateSetting;
            iface_info.num_endpoints     = iface_desc.bNumEndpoints;
            iface_info.interface_class   = iface_desc.bInterfaceClass;
            iface_info.interface_subclass = iface_desc.bInterfaceSubClass;
            iface_info.interface_protocol = iface_desc.bInterfaceProtocol;
            
            // Read interface string descriptor
            if (iface_desc.iInterface > 0 && m_handle) {
                unsigned char buf[256];
                int len = libusb_get_string_descriptor_ascii(
                    m_handle, iface_desc.iInterface, buf, sizeof(buf));
                if (len > 0) {
                    iface_info.interface_string = std::string(reinterpret_cast<char*>(buf), len);
                }
            }
            
            // Iterate through endpoints
            for (int k = 0; k < iface_desc.bNumEndpoints; k++) {
                const libusb_endpoint_descriptor& ep_desc = iface_desc.endpoint[k];
                
                SimpleCommKitUsbEndpointInfo ep_info;
                ep_info.endpoint_address = ep_desc.bEndpointAddress;
                ep_info.attributes       = ep_desc.bmAttributes;
                ep_info.max_packet_size = ep_desc.wMaxPacketSize;
                ep_info.interval        = ep_desc.bInterval;
                ep_info.is_in           = (ep_desc.bEndpointAddress & LIBUSB_ENDPOINT_IN) != 0;
                
                iface_info.endpoints.push_back(ep_info);
            }
            
            interfaces.push_back(iface_info);
        }
    }
    
    libusb_free_config_descriptor(config);
    return interfaces;
}

std::vector<SimpleCommKitUsbEndpointInfo> SimpleCommKitUsbPrivate::getInterfaceEndpoints(int interface_number)
{
    std::vector<SimpleCommKitUsbEndpointInfo> endpoints;
    
    if (!m_handle) {
        triggerError(ErrorCodes::SimpleCommKitUsbNotOpenError);
        return endpoints;
    }
    
    libusb_device* dev = libusb_get_device(m_handle);
    if (!dev) {
        triggerError(ErrorCodes::SimpleCommKitUsbGetConfigError);
        return endpoints;
    }
    
    libusb_config_descriptor* config = nullptr;
    int ret = libusb_get_config_descriptor(dev, 0, &config);
    if (ret != 0) {
        triggerError(ErrorCodes::SimpleCommKitUsbGetConfigError);
        return endpoints;
    }
    
    // Find the specified interface
    for (int i = 0; i < config->bNumInterfaces; i++) {
        const libusb_interface& iface = config->interface[i];
        
        for (int j = 0; j < iface.num_altsetting; j++) {
            const libusb_interface_descriptor& iface_desc = iface.altsetting[j];
            
            if (iface_desc.bInterfaceNumber == interface_number) {
                // Found the interface, extract endpoints
                for (int k = 0; k < iface_desc.bNumEndpoints; k++) {
                    const libusb_endpoint_descriptor& ep_desc = iface_desc.endpoint[k];
                    
                    SimpleCommKitUsbEndpointInfo ep_info;
                    ep_info.endpoint_address = ep_desc.bEndpointAddress;
                    ep_info.attributes       = ep_desc.bmAttributes;
                    ep_info.max_packet_size = ep_desc.wMaxPacketSize;
                    ep_info.interval        = ep_desc.bInterval;
                    ep_info.is_in           = (ep_desc.bEndpointAddress & LIBUSB_ENDPOINT_IN) != 0;
                    
                    endpoints.push_back(ep_info);
                }
                libusb_free_config_descriptor(config);
                return endpoints;
            }
        }
    }
    
    libusb_free_config_descriptor(config);
    return endpoints;
}

std::vector<SimpleCommKitUsbEndpointInfo> SimpleCommKitUsbPrivate::findEndpointsByType(SimpleCommKitUsbTransferType transfer_type)
{
    std::vector<SimpleCommKitUsbEndpointInfo> endpoints;
    
    if (!m_handle) {
        triggerError(ErrorCodes::SimpleCommKitUsbNotOpenError);
        return endpoints;
    }
    
    libusb_device* dev = libusb_get_device(m_handle);
    if (!dev) {
        triggerError(ErrorCodes::SimpleCommKitUsbGetConfigError);
        return endpoints;
    }
    
    libusb_config_descriptor* config = nullptr;
    int ret = libusb_get_config_descriptor(dev, 0, &config);
    if (ret != 0) {
        triggerError(ErrorCodes::SimpleCommKitUsbGetConfigError);
        return endpoints;
    }
    
    uint8_t target_type = static_cast<uint8_t>(transfer_type);
    
    // Iterate through all interfaces and endpoints
    for (int i = 0; i < config->bNumInterfaces; i++) {
        const libusb_interface& iface = config->interface[i];
        
        for (int j = 0; j < iface.num_altsetting; j++) {
            const libusb_interface_descriptor& iface_desc = iface.altsetting[j];
            
            for (int k = 0; k < iface_desc.bNumEndpoints; k++) {
                const libusb_endpoint_descriptor& ep_desc = iface_desc.endpoint[k];
                
                // Check if endpoint matches the requested transfer type
                if ((ep_desc.bmAttributes & 0x03) == target_type) {
                    SimpleCommKitUsbEndpointInfo ep_info;
                    ep_info.endpoint_address = ep_desc.bEndpointAddress;
                    ep_info.attributes       = ep_desc.bmAttributes;
                    ep_info.max_packet_size = ep_desc.wMaxPacketSize;
                    ep_info.interval        = ep_desc.bInterval;
                    ep_info.is_in           = (ep_desc.bEndpointAddress & LIBUSB_ENDPOINT_IN) != 0;
                    
                    endpoints.push_back(ep_info);
                }
            }
        }
    }
    
    libusb_free_config_descriptor(config);
    return endpoints;
}

bool SimpleCommKitUsbPrivate::autoDiscoverEndpoints(SimpleCommKitUsbTransferType transfer_type,
                                                     uint8_t& out_endpoint,
                                                     uint8_t& in_endpoint)
{
    out_endpoint = 0;
    in_endpoint  = 0;
    
    if (!m_handle) {
        triggerError(ErrorCodes::SimpleCommKitUsbNotOpenError);
        return false;
    }
    
    libusb_device* dev = libusb_get_device(m_handle);
    if (!dev) {
        triggerError(ErrorCodes::SimpleCommKitUsbGetConfigError);
        return false;
    }
    
    libusb_config_descriptor* config = nullptr;
    int ret = libusb_get_config_descriptor(dev, 0, &config);
    if (ret != 0) {
        triggerError(ErrorCodes::SimpleCommKitUsbGetConfigError);
        return false;
    }
    
    uint8_t target_type = static_cast<uint8_t>(transfer_type);
    
    // Find first IN and OUT endpoints of the specified transfer type
    for (int i = 0; i < config->bNumInterfaces; i++) {
        const libusb_interface& iface = config->interface[i];
        
        for (int j = 0; j < iface.num_altsetting; j++) {
            const libusb_interface_descriptor& iface_desc = iface.altsetting[j];
            
            for (int k = 0; k < iface_desc.bNumEndpoints; k++) {
                const libusb_endpoint_descriptor& ep_desc = iface_desc.endpoint[k];
                
                // Check if endpoint matches the requested transfer type
                if ((ep_desc.bmAttributes & 0x03) == target_type) {
                    uint8_t ep_addr = ep_desc.bEndpointAddress;
                    
                    if ((ep_addr & LIBUSB_ENDPOINT_IN) != 0) {
                        // IN endpoint
                        if (in_endpoint == 0) {
                            in_endpoint = ep_addr;
                        }
                    } else {
                        // OUT endpoint
                        if (out_endpoint == 0) {
                            out_endpoint = ep_addr;
                        }
                    }
                }
            }
        }
    }
    
    libusb_free_config_descriptor(config);
    
    // Return true if at least one endpoint was found
    return (out_endpoint != 0 || in_endpoint != 0);
}

} // namespace SimpleCommKit
