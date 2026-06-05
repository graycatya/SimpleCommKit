#pragma once

#include <SimpleCommKitErrorMap.hpp>
#include "SimpleCommKitExport.h"

#include <vector>
#include <functional>
#include <memory>
#include <string>
#include <cstdint>

namespace SimpleCommKit {

SIMPLECOMMKIT_DECLARE_PRIVATE_CLASS(SimpleCommKitHid)

//
// HID bus type enumeration
//
enum SimpleCommKitHidBusType : int {
    HID_BUS_TYPE_UNKNOWN  = 0x00,
    HID_BUS_TYPE_USB      = 0x01,
    HID_BUS_TYPE_BLUETOOTH = 0x02,
    HID_BUS_TYPE_I2C      = 0x03,
    HID_BUS_TYPE_SPI      = 0x04,
};

//
// HID device information structure
//
struct SimpleCommKitHidDeviceInfo {
    int             interface_number;
    std::string     manufacturer_string;
    std::string     product_string;
    unsigned short  release_number;
    int             bus_type;          // SimpleCommKitHidBusType
    std::string     serial_number;
    std::string     path;              // platform-specific device path (used for open)
};

class SIMPLECOMMKIT_API SimpleCommKitHid
{
    SIMPLECOMMKIT_DECLARE_PRIVATE(SimpleCommKitHid)
public:
    SimpleCommKitHid();
    ~SimpleCommKitHid();

    //
    // Static: enumerate available HID devices
    //
    static std::vector<SimpleCommKitHidDeviceInfo> get_Available_Devices(
        unsigned short vendor_id  = 0x0,
        unsigned short product_id = 0x0);

    //
    // Lifecycle: init / exit / open / close  (multi-device)
    //
    bool init(unsigned short vendor_id  = 0x0,
              unsigned short product_id = 0x0);
    void exit();

    // Open by path — can be called multiple times for different paths
    bool open(const std::string& path, bool readable = true);
    bool open(unsigned short vendor_id,
              unsigned short product_id,
              const std::string& serial_number = "",
              bool readable = true);

    // Close all devices
    void close();
    // Close a specific device by path
    void close(const std::string& path);

    // At least one device open
    bool is_Open();
    // Check if a specific device is open
    bool is_Open(const std::string& path);

    //
    // I/O  (path-less versions operate on the first opened device)
    //
    int write(const std::vector<uint8_t>& data);
    int write(const std::string& path, const std::vector<uint8_t>& data);
    int send_Feature_Report(const std::vector<uint8_t>& data);
    int send_Feature_Report(const std::string& path, const std::vector<uint8_t>& data);

    //
    // Hotplug (polling-based detection)
    //
    void start_Hotplug(unsigned short vendor_id, unsigned short product_id);
    void stop_Hotplug();
    bool is_Hotplug_Active();

    void set_Hotplug_Poll_Interval(int ms);
    int  get_Hotplug_Poll_Interval();

    //
    // Read polling configuration (global defaults for new devices)
    //
    void set_Read_Poll_Interval(int ms);
    int  get_Read_Poll_Interval();

    //
    // Read data length (number of bytes to read per poll; default 64)
    //
    void set_Read_Data_Length(int length);
    int  get_Read_Data_Length();

    //
    // Per-device read config
    //
    void set_Read_Poll_Interval(const std::string& path, int ms);
    int  get_Read_Poll_Interval(const std::string& path);
    void set_Read_Data_Length(const std::string& path, int length);
    int  get_Read_Data_Length(const std::string& path);

    //
    // Get list of currently open device paths
    //
    std::vector<std::string> get_Open_Paths();

    //
    // Cached device list (populated by init / start_Hotplug)
    //
    std::vector<SimpleCommKitHidDeviceInfo> get_Device_List();

    //
    // Callbacks
    //
    void set_Callback_On_Read(
        std::function<void(const std::vector<uint8_t>& data)> on_read);
    void set_Callback_On_HotPlug(
        std::function<void(const std::vector<SimpleCommKitHidDeviceInfo>& added,
                           const std::vector<SimpleCommKitHidDeviceInfo>& removed)> on_hotplug);
    void set_Callback_Error(
        std::function<void(SimpleCommKit::ErrorCode)> on_error);

private:
    std::unique_ptr<SimpleCommKitHidPrivate> d_ptr;
};

} // namespace SimpleCommKit
