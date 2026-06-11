#pragma once

#include <SimpleCommKitErrorMap.hpp>
#include "SimpleCommKitExport.h"

#include <vector>
#include <functional>
#include <memory>
#include <string>
#include <cstdint>

namespace SimpleCommKit {

SIMPLECOMMKIT_DECLARE_PRIVATE_CLASS(SimpleCommKitUsb)

//
// USB transfer types (mirrors libusb, but keeps public API independent)
//
enum class SimpleCommKitUsbTransferType : uint8_t {
    Control     = 0,   // LIBUSB_TRANSFER_TYPE_CONTROL
    Isochronous = 1,   // LIBUSB_TRANSFER_TYPE_ISOCHRONOUS
    Bulk        = 2,   // LIBUSB_TRANSFER_TYPE_BULK
    Interrupt   = 3    // LIBUSB_TRANSFER_TYPE_INTERRUPT
};

//
// USB device information structure
//
struct SimpleCommKitUsbDeviceInfo {
    unsigned short  vendor_id;
    unsigned short  product_id;
    std::string     manufacturer_string;
    std::string     product_string;
    std::string     serial_number;
    uint8_t         bus_number;
    uint8_t         device_address;
    std::string     path;               // "bus:address" as unique key (e.g. "1:3")
};

//
// Isochronous packet result (per-packet status after transfer)
//
struct SimpleCommKitUsbIsoPacketResult {
    int length        = 0;   // requested length for this packet
    int actual_length = 0;   // actual transferred
    int status        = 0;   // 0 = LIBUSB_TRANSFER_COMPLETED, or LIBUSB_TRANSFER_* error
};

//
// Endpoint information structure
//
struct SimpleCommKitUsbEndpointInfo {
    uint8_t  endpoint_address;   // Endpoint address (includes direction bit)
    uint8_t  attributes;         // Transfer type (0=control, 1=isoch, 2=bulk, 3=interrupt)
    uint16_t max_packet_size;    // Maximum packet size
    uint8_t  interval;           // Polling interval (for interrupt/isochronous)
    bool     is_in;              // true = IN endpoint, false = OUT endpoint
    
    // Helper methods
    bool isBulk() const        { return (attributes & 0x03) == static_cast<uint8_t>(SimpleCommKitUsbTransferType::Bulk); }
    bool isInterrupt() const   { return (attributes & 0x03) == static_cast<uint8_t>(SimpleCommKitUsbTransferType::Interrupt); }
    bool isIsochronous() const { return (attributes & 0x03) == static_cast<uint8_t>(SimpleCommKitUsbTransferType::Isochronous); }
    bool isControl() const     { return (attributes & 0x03) == static_cast<uint8_t>(SimpleCommKitUsbTransferType::Control); }
};

//
// Interface information structure
//
struct SimpleCommKitUsbInterfaceInfo {
    uint8_t  interface_number;       // Interface number
    uint8_t  alternate_setting;      // Alternate setting
    uint8_t  num_endpoints;          // Number of endpoints
    uint8_t  interface_class;        // Interface class (e.g., 0x03 = HID)
    uint8_t  interface_subclass;     // Interface subclass
    uint8_t  interface_protocol;     // Interface protocol
    std::string interface_string;     // Interface string descriptor
    std::vector<SimpleCommKitUsbEndpointInfo> endpoints;  // Endpoints in this interface
};

class SIMPLECOMMKIT_API SimpleCommKitUsb
{
    SIMPLECOMMKIT_DECLARE_PRIVATE(SimpleCommKitUsb)
public:
    SimpleCommKitUsb();
    ~SimpleCommKitUsb();

    //
    // Static: enumerate available USB devices
    //
    static std::vector<SimpleCommKitUsbDeviceInfo> get_Available_Devices(
        unsigned short vendor_id  = 0x0,
        unsigned short product_id = 0x0);

    //
    // Lifecycle: init / exit
    //
    bool init();
    void exit();

    //
    // Open / close (single device)
    //
    bool open(const std::string& path);
    bool open(unsigned short vendor_id,
              unsigned short product_id,
              const std::string& serial_number = "");
    void close();
    bool is_Open();

    //
    // Claim / release interface
    //
    bool claim_Interface(int interface_number);
    bool release_Interface(int interface_number);

    //
    // Control transfer
    //
    int control_Transfer(uint8_t bmRequestType, uint8_t bRequest,
                         uint16_t wValue, uint16_t wIndex,
                         std::vector<uint8_t>& data, unsigned int timeout = 1000);

    //
    // Bulk transfer
    //
    // data: for OUT, contains data to send; for IN, resized to read length and filled
    // Returns bytes transferred, or negative libusb error
    //
    int bulk_Transfer(uint8_t endpoint, std::vector<uint8_t>& data,
                      unsigned int timeout = 1000);

    //
    // Interrupt transfer
    //
    int interrupt_Transfer(uint8_t endpoint, std::vector<uint8_t>& data,
                           unsigned int timeout = 1000);

    //
    // Isochronous transfer (synchronous wrapper over libusb async API)
    //
    // Direction is determined by endpoint address (bit 7).
    // For OUT: data is sent split across num_packets packets, packet_lengths[i] = size of packet i.
    // For IN : data is resized to total capacity and filled.
    // results: filled with per-packet actual_length and status on return.
    // Returns total actual bytes transferred, or negative libusb error.
    //
    int isochronous_Transfer(uint8_t endpoint, std::vector<uint8_t>& data,
                             int num_packets, const std::vector<int>& packet_lengths,
                             std::vector<SimpleCommKitUsbIsoPacketResult>& results,
                             unsigned int timeout = 1000);

    //
    // Read polling (continuous interrupt/bulk IN read on a specific endpoint)
    //
    void start_Read_Poll(uint8_t endpoint);
    void stop_Read_Poll();
    bool is_Read_Poll_Active();

    //
    // Hotplug (polling-based detection)
    //
    void start_Hotplug(unsigned short vendor_id = 0, unsigned short product_id = 0);
    void stop_Hotplug();
    bool is_Hotplug_Active();

    void set_Hotplug_Poll_Interval(int ms);
    int  get_Hotplug_Poll_Interval();

    //
    // Read polling configuration
    //
    void set_Read_Poll_Interval(int ms);
    int  get_Read_Poll_Interval();
    void set_Read_Data_Length(int length);
    int  get_Read_Data_Length();

    //
    // Current device path
    //
    std::string get_Open_Path();
    bool is_Open_Device();

    //
    // Cached device list (populated by init / start_Hotplug)
    //
    std::vector<SimpleCommKitUsbDeviceInfo> get_Device_List();

    //
    // Device descriptor and configuration parsing
    //

    // Get all interfaces of the currently opened device
    std::vector<SimpleCommKitUsbInterfaceInfo> get_Device_Interfaces();

    // Get endpoints for a specific interface of the currently opened device
    std::vector<SimpleCommKitUsbEndpointInfo> get_Interface_Endpoints(int interface_number);

    // Find endpoints by transfer type (bulk, interrupt, isochronous)
    std::vector<SimpleCommKitUsbEndpointInfo> find_Endpoints_By_Type(SimpleCommKitUsbTransferType transfer_type);

    // Auto-discover IN and OUT endpoints for common transfer types
    bool auto_Discover_Endpoints(SimpleCommKitUsbTransferType transfer_type,
                                  uint8_t& out_endpoint,
                                  uint8_t& in_endpoint);

    //
    // Callbacks
    //
    void set_Callback_On_Read(
        std::function<void(const SimpleCommKitUsbDeviceInfo& device_info,
                           const std::vector<uint8_t>& data)> on_read);
    void set_Callback_On_HotPlug(
        std::function<void(const std::vector<SimpleCommKitUsbDeviceInfo>& added,
                           const std::vector<SimpleCommKitUsbDeviceInfo>& removed)> on_hotplug);
    void set_Callback_Error(
        std::function<void(SimpleCommKit::ErrorCode)> on_error);

private:
    std::unique_ptr<SimpleCommKitUsbPrivate> d_ptr;
};

} // namespace SimpleCommKit
