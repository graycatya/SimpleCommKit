#include "SimpleCommKitHid.h"
#include "SimpleCommKitHid_p.h"

namespace SimpleCommKit {

// ============================================================
// SimpleCommKitHid Implementation  (thin public façade)
// ============================================================

SimpleCommKitHid::SimpleCommKitHid()
    : d_ptr(std::make_unique<SimpleCommKitHidPrivate>(this))
{
}

SimpleCommKitHid::~SimpleCommKitHid() = default;

//
// Static
//
std::vector<SimpleCommKitHidDeviceInfo>
SimpleCommKitHid::get_Available_Devices(unsigned short vendor_id,
                                        unsigned short product_id)
{
    return SimpleCommKitHidPrivate::getAvailableDevices(vendor_id, product_id);
}

//
// Lifecycle
//
bool SimpleCommKitHid::init(unsigned short vendor_id,
                             unsigned short product_id)
{
    if (!d_ptr) {
        return false;
    }
    return d_ptr->init(vendor_id, product_id);
}

void SimpleCommKitHid::exit()
{
    if (d_ptr) {
        d_ptr->exit();
    }
}

bool SimpleCommKitHid::open(const std::string& path)
{
    if (!d_ptr) {
        return false;
    }
    return d_ptr->open(path);
}

bool SimpleCommKitHid::open(unsigned short vendor_id,
                             unsigned short product_id,
                             const std::string& serial_number)
{
    if (!d_ptr) {
        return false;
    }
    return d_ptr->open(vendor_id, product_id, serial_number);
}

void SimpleCommKitHid::close()
{
    if (d_ptr) {
        d_ptr->close();
    }
}

bool SimpleCommKitHid::is_Open()
{
    if (!d_ptr) {
        return false;
    }
    return d_ptr->isOpen();
}

//
// I/O
//
int SimpleCommKitHid::write(const std::vector<uint8_t>& data)
{
    if (!d_ptr) {
        return -1;
    }
    return d_ptr->write(data);
}

int SimpleCommKitHid::send_Feature_Report(const std::vector<uint8_t>& data)
{
    if (!d_ptr) {
        return -1;
    }
    return d_ptr->sendFeatureReport(data);
}

//
// Hotplug
//
void SimpleCommKitHid::start_Hotplug(unsigned short vendor_id,
                                      unsigned short product_id)
{
    if (d_ptr) {
        d_ptr->startHotplug(vendor_id, product_id);
    }
}

void SimpleCommKitHid::stop_Hotplug()
{
    if (d_ptr) {
        d_ptr->stopHotplug();
    }
}

bool SimpleCommKitHid::is_Hotplug_Active()
{
    if (!d_ptr) {
        return false;
    }
    return d_ptr->isHotplugActive();
}

void SimpleCommKitHid::set_Hotplug_Poll_Interval(int ms)
{
    if (d_ptr) {
        d_ptr->setHotplugPollInterval(ms);
    }
}

int SimpleCommKitHid::get_Hotplug_Poll_Interval()
{
    if (!d_ptr) {
        return 1000;
    }
    return d_ptr->getHotplugPollInterval();
}

//
// Read polling config
//
void SimpleCommKitHid::set_Read_Poll_Interval(int ms)
{
    if (d_ptr) {
        d_ptr->setReadPollInterval(ms);
    }
}

int SimpleCommKitHid::get_Read_Poll_Interval()
{
    if (!d_ptr) {
        return 1;
    }
    return d_ptr->getReadPollInterval();
}

void SimpleCommKitHid::set_Read_Data_Length(int length)
{
    if (d_ptr) {
        d_ptr->setReadDataLength(length);
    }
}

int SimpleCommKitHid::get_Read_Data_Length()
{
    if (!d_ptr) {
        return 64;
    }
    return d_ptr->getReadDataLength();
}

//
// Device list
//
std::vector<SimpleCommKitHidDeviceInfo> SimpleCommKitHid::get_Device_List()
{
    if (!d_ptr) {
        return {};
    }
    return d_ptr->getDeviceList();
}

//
// Callbacks
//
void SimpleCommKitHid::set_Callback_On_Read(
    std::function<void(const std::vector<uint8_t>& data)> on_read)
{
    if (d_ptr) {
        d_ptr->setCallbackOnRead(std::move(on_read));
    }
}

void SimpleCommKitHid::set_Callback_On_HotPlug(
    std::function<void(const std::vector<SimpleCommKitHidDeviceInfo>& added,
                       const std::vector<SimpleCommKitHidDeviceInfo>& removed)> on_hotplug)
{
    if (d_ptr) {
        d_ptr->setCallbackOnHotPlug(std::move(on_hotplug));
    }
}

void SimpleCommKitHid::set_Callback_Error(
    std::function<void(SimpleCommKit::ErrorCode)> on_error)
{
    if (d_ptr) {
        d_ptr->setCallbackError(std::move(on_error));
    }
}

} // namespace SimpleCommKit
