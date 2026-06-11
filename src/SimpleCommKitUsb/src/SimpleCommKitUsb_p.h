#pragma once

#include <libusb.h>
#include "SimpleCommKitUsb.h"
#include "SimpleCommKitExport.h"

#include <functional>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include <map>
#include <set>

namespace SimpleCommKit {

class SimpleCommKitUsbPrivate
{
    SIMPLECOMMKIT_DECLARE_PUBLIC(SimpleCommKitUsb)
public:
    explicit SimpleCommKitUsbPrivate(SimpleCommKitUsb* parent);
    ~SimpleCommKitUsbPrivate();

    //
    // Static
    //
    static std::vector<SimpleCommKitUsbDeviceInfo>
        getAvailableDevices(unsigned short vendor_id,
                            unsigned short product_id);

    //
    // Lifecycle
    //
    bool init();
    void exit();

    //
    // Open / close – single device
    //
    bool open(const std::string& path);
    bool open(unsigned short vendor_id,
              unsigned short product_id,
              const std::string& serial_number = "");
    void close();
    bool isOpen();

    //
    // Claim / release interface
    //
    bool claimInterface(int interface_number);
    bool releaseInterface(int interface_number);

    //
    // Control transfer
    //
    int controlTransfer(uint8_t bmRequestType, uint8_t bRequest,
                        uint16_t wValue, uint16_t wIndex,
                        std::vector<uint8_t>& data, unsigned int timeout = 1000);

    //
    // Bulk transfer
    //
    int bulkTransfer(uint8_t endpoint, std::vector<uint8_t>& data,
                     unsigned int timeout = 1000);

    //
    // Interrupt transfer
    //
    int interruptTransfer(uint8_t endpoint, std::vector<uint8_t>& data,
                         unsigned int timeout = 1000);

    //
    // Isochronous transfer
    //
    int isochronousTransfer(uint8_t endpoint, std::vector<uint8_t>& data,
                            int num_packets, const std::vector<int>& packet_lengths,
                            std::vector<SimpleCommKitUsbIsoPacketResult>& results,
                            unsigned int timeout);

    //
    // Read polling
    //
    void startReadPoll(uint8_t endpoint);
    void stopReadPoll();
    bool isReadPollActive();

    //
    // Hotplug
    //
    void startHotplug(unsigned short vendor_id = 0, unsigned short product_id = 0);
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
    // Device info
    //
    std::string getOpenPath();
    std::vector<SimpleCommKitUsbDeviceInfo> getDeviceList();

    //
    // Callbacks
    //
    void setCallbackOnRead(std::function<void(const SimpleCommKitUsbDeviceInfo&,
                                              const std::vector<uint8_t>&)> callback);
    void setCallbackOnHotPlug(std::function<void(const std::vector<SimpleCommKitUsbDeviceInfo>&,
                                                  const std::vector<SimpleCommKitUsbDeviceInfo>&)> callback);
    void setCallbackError(std::function<void(SimpleCommKit::ErrorCode)> callback);

    void triggerError(SimpleCommKit::ErrorCode error_code);

    //
    // Internal helpers (public for use by native hotplug callback helpers)
    //
    static std::string makeDevicePath(uint8_t bus, uint8_t addr);

    static std::vector<SimpleCommKitUsbDeviceInfo>
        enumerateDevices(libusb_context* ctx, unsigned short vendor_id, unsigned short product_id);

    void compareDevices(const std::map<std::string, SimpleCommKitUsbDeviceInfo>& current,
                        std::vector<SimpleCommKitUsbDeviceInfo>& added,
                        std::vector<SimpleCommKitUsbDeviceInfo>& removed);

    //
    // Configuration descriptor parsing
    //

    // Parse config descriptor and extract interface info
    std::vector<SimpleCommKitUsbInterfaceInfo> parseConfigDescriptor(libusb_device* dev);

    // Get endpoints for a specific interface
    std::vector<SimpleCommKitUsbEndpointInfo> getInterfaceEndpoints(int interface_number);

    // Find endpoints by transfer type
    std::vector<SimpleCommKitUsbEndpointInfo> findEndpointsByType(SimpleCommKitUsbTransferType transfer_type);

    // Auto-discover IN and OUT endpoints
    bool autoDiscoverEndpoints(SimpleCommKitUsbTransferType transfer_type,
                               uint8_t& out_endpoint,
                               uint8_t& in_endpoint);

private:
    // Native hotplug callback (friend)
    friend int LIBUSB_CALL usbHotplugCallback(libusb_context*, libusb_device*,
                                               libusb_hotplug_event, void*);

    // Isochronous transfer callback (static)
    static void LIBUSB_CALL isoTransferCallback(struct libusb_transfer* transfer);

    // Internal synchronous isochronous helper
    int syncIsoTransfer(libusb_device_handle* handle, uint8_t endpoint,
                        unsigned char* data, int length,
                        int num_packets, const int* packet_lengths,
                        std::vector<SimpleCommKitUsbIsoPacketResult>& results,
                        unsigned int timeout_ms);

    //
    // Members
    //
    SimpleCommKitUsb* q_ptr;

    // libusb context
    libusb_context* m_usbContext = nullptr;

    // Single device handle & path
    libusb_device_handle* m_handle = nullptr;
    std::string m_devicePath;
    mutable std::mutex m_handleMutex;

    // cached device info map (key = path)
    std::map<std::string, SimpleCommKitUsbDeviceInfo> m_devices;
    mutable std::mutex m_devicesMutex;

    // hotplug
    bool m_hotplugCapable = false;
    libusb_hotplug_callback_handle m_hotplugCallbackHandle = {};
    std::unique_ptr<std::thread> m_hotplugThread;
    std::atomic<bool> m_hotplugStop{true};
    std::atomic<int>  m_hotplugPollMs{1000};
    unsigned short    m_hotplugVendorId  = 0;
    unsigned short    m_hotplugProductId = 0;

    std::function<void(const std::vector<SimpleCommKitUsbDeviceInfo>&,
                       const std::vector<SimpleCommKitUsbDeviceInfo>&)> m_onHotPlug;

    // Read polling (single device)
    std::unique_ptr<std::thread> m_readThread;
    std::atomic<bool> m_readStopFlag{true};
    std::atomic<int>  m_readPollMs{500};
    std::atomic<int>  m_readDataLength{64};
    uint8_t m_readEndpoint = 0;

    // Track interfaces that had kernel driver detached (for re-attach on release)
    std::set<int> m_claimedInterfaces;

    // Callbacks
    std::function<void(const SimpleCommKitUsbDeviceInfo&,
                       const std::vector<uint8_t>&)> m_onRead;
    std::function<void(SimpleCommKit::ErrorCode)> m_onError;
};

} // namespace SimpleCommKit
