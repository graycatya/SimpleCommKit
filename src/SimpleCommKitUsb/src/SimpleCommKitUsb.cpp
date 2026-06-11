#include "SimpleCommKitUsb.h"
#include "SimpleCommKitUsb_p.h"

namespace SimpleCommKit {

// ============================================================
// SimpleCommKitUsb Implementation  (thin public facade)
// ============================================================

SimpleCommKitUsb::SimpleCommKitUsb()
    : d_ptr(std::make_unique<SimpleCommKitUsbPrivate>(this))
{
}

SimpleCommKitUsb::~SimpleCommKitUsb() = default;

//
// Static
//
std::vector<SimpleCommKitUsbDeviceInfo>
SimpleCommKitUsb::get_Available_Devices(unsigned short vendor_id,
                                         unsigned short product_id)
{
    return SimpleCommKitUsbPrivate::getAvailableDevices(vendor_id, product_id);
}

//
// Lifecycle
//
bool SimpleCommKitUsb::init()
{
    if (!d_ptr) return false;
    return d_ptr->init();
}

void SimpleCommKitUsb::exit()
{
    if (d_ptr) d_ptr->exit();
}

//
// Open / close
//
bool SimpleCommKitUsb::open(const std::string& path)
{
    if (!d_ptr) return false;
    return d_ptr->open(path);
}

bool SimpleCommKitUsb::open(unsigned short vendor_id, unsigned short product_id,
                             const std::string& serial_number)
{
    if (!d_ptr) return false;
    return d_ptr->open(vendor_id, product_id, serial_number);
}

void SimpleCommKitUsb::close()
{
    if (d_ptr) d_ptr->close();
}

bool SimpleCommKitUsb::is_Open()
{
    if (!d_ptr) return false;
    return d_ptr->isOpen();
}

//
// Claim / release interface
//
bool SimpleCommKitUsb::claim_Interface(int interface_number)
{
    if (!d_ptr) return false;
    return d_ptr->claimInterface(interface_number);
}

bool SimpleCommKitUsb::release_Interface(int interface_number)
{
    if (!d_ptr) return false;
    return d_ptr->releaseInterface(interface_number);
}

//
// Control transfer
//
int SimpleCommKitUsb::control_Transfer(uint8_t bmRequestType, uint8_t bRequest,
                                        uint16_t wValue, uint16_t wIndex,
                                        std::vector<uint8_t>& data, unsigned int timeout)
{
    if (!d_ptr) return -1;
    return d_ptr->controlTransfer(bmRequestType, bRequest, wValue, wIndex, data, timeout);
}

//
// Bulk transfer
//
int SimpleCommKitUsb::bulk_Transfer(uint8_t endpoint, std::vector<uint8_t>& data,
                                     unsigned int timeout)
{
    if (!d_ptr) return -1;
    return d_ptr->bulkTransfer(endpoint, data, timeout);
}

//
// Interrupt transfer
//
int SimpleCommKitUsb::interrupt_Transfer(uint8_t endpoint, std::vector<uint8_t>& data,
                                          unsigned int timeout)
{
    if (!d_ptr) return -1;
    return d_ptr->interruptTransfer(endpoint, data, timeout);
}

//
// Isochronous transfer
//
int SimpleCommKitUsb::isochronous_Transfer(
    uint8_t endpoint, std::vector<uint8_t>& data,
    int num_packets, const std::vector<int>& packet_lengths,
    std::vector<SimpleCommKitUsbIsoPacketResult>& results,
    unsigned int timeout)
{
    if (!d_ptr) return -1;
    return d_ptr->isochronousTransfer(endpoint, data, num_packets, packet_lengths, results, timeout);
}

//
// Read polling
//
void SimpleCommKitUsb::start_Read_Poll(uint8_t endpoint)
{
    if (d_ptr) d_ptr->startReadPoll(endpoint);
}

void SimpleCommKitUsb::stop_Read_Poll()
{
    if (d_ptr) d_ptr->stopReadPoll();
}

bool SimpleCommKitUsb::is_Read_Poll_Active()
{
    if (!d_ptr) return false;
    return d_ptr->isReadPollActive();
}

//
// Hotplug
//
void SimpleCommKitUsb::start_Hotplug(unsigned short vendor_id, unsigned short product_id)
{
    if (d_ptr) d_ptr->startHotplug(vendor_id, product_id);
}

void SimpleCommKitUsb::stop_Hotplug()
{
    if (d_ptr) d_ptr->stopHotplug();
}

bool SimpleCommKitUsb::is_Hotplug_Active()
{
    if (!d_ptr) return false;
    return d_ptr->isHotplugActive();
}

void SimpleCommKitUsb::set_Hotplug_Poll_Interval(int ms)
{
    if (d_ptr) d_ptr->setHotplugPollInterval(ms);
}

int SimpleCommKitUsb::get_Hotplug_Poll_Interval()
{
    if (!d_ptr) return 1000;
    return d_ptr->getHotplugPollInterval();
}

//
// Read config
//
void SimpleCommKitUsb::set_Read_Poll_Interval(int ms)
{
    if (d_ptr) d_ptr->setReadPollInterval(ms);
}

int SimpleCommKitUsb::get_Read_Poll_Interval()
{
    if (!d_ptr) return 100;
    return d_ptr->getReadPollInterval();
}

void SimpleCommKitUsb::set_Read_Data_Length(int length)
{
    if (d_ptr) d_ptr->setReadDataLength(length);
}

int SimpleCommKitUsb::get_Read_Data_Length()
{
    if (!d_ptr) return 64;
    return d_ptr->getReadDataLength();
}

//
// Open path
//
std::string SimpleCommKitUsb::get_Open_Path()
{
    if (!d_ptr) return {};
    return d_ptr->getOpenPath();
}

bool SimpleCommKitUsb::is_Open_Device()
{
    return is_Open();
}

//
// Device list
//
std::vector<SimpleCommKitUsbDeviceInfo> SimpleCommKitUsb::get_Device_List()
{
    if (!d_ptr) return {};
    return d_ptr->getDeviceList();
}

//
// Device descriptor and configuration parsing
//

std::vector<SimpleCommKitUsbInterfaceInfo> SimpleCommKitUsb::get_Device_Interfaces()
{
    if (!d_ptr) return {};
    
    libusb_device* dev = libusb_get_device(d_ptr->m_handle);
    if (!dev) {
        d_ptr->triggerError(ErrorCodes::SimpleCommKitUsbNotOpenError);
        return {};
    }
    
    return d_ptr->parseConfigDescriptor(dev);
}

std::vector<SimpleCommKitUsbEndpointInfo> SimpleCommKitUsb::get_Interface_Endpoints(int interface_number)
{
    if (!d_ptr) return {};
    return d_ptr->getInterfaceEndpoints(interface_number);
}

std::vector<SimpleCommKitUsbEndpointInfo> SimpleCommKitUsb::find_Endpoints_By_Type(SimpleCommKitUsbTransferType transfer_type)
{
    if (!d_ptr) return {};
    return d_ptr->findEndpointsByType(transfer_type);
}

bool SimpleCommKitUsb::auto_Discover_Endpoints(SimpleCommKitUsbTransferType transfer_type,
                                                uint8_t& out_endpoint,
                                                uint8_t& in_endpoint)
{
    if (!d_ptr) return false;
    return d_ptr->autoDiscoverEndpoints(transfer_type, out_endpoint, in_endpoint);
}

//
// Callbacks
//
void SimpleCommKitUsb::set_Callback_On_Read(
    std::function<void(const SimpleCommKitUsbDeviceInfo&, const std::vector<uint8_t>&)> on_read)
{
    if (d_ptr) d_ptr->setCallbackOnRead(std::move(on_read));
}

void SimpleCommKitUsb::set_Callback_On_HotPlug(
    std::function<void(const std::vector<SimpleCommKitUsbDeviceInfo>& added,
                       const std::vector<SimpleCommKitUsbDeviceInfo>& removed)> on_hotplug)
{
    if (d_ptr) d_ptr->setCallbackOnHotPlug(std::move(on_hotplug));
}

void SimpleCommKitUsb::set_Callback_Error(
    std::function<void(SimpleCommKit::ErrorCode)> on_error)
{
    if (d_ptr) d_ptr->setCallbackError(std::move(on_error));
}

} // namespace SimpleCommKit
