#include "SimpleCommKitSerialPort.h"
#include "SimpleCommKitSerialPort_p.h"

namespace SimpleCommKit {

// ============================================================
// SimpleCommKitSerialPort Implementation
// ============================================================

SimpleCommKitSerialPort::SimpleCommKitSerialPort()
    : d_ptr(std::make_unique<SimpleCommKitSerialPortPrivate>(this))
{
}

SimpleCommKitSerialPort::~SimpleCommKitSerialPort() = default;

//
// Static
//
std::vector<SimpleCommKitSerialPortInfo> SimpleCommKitSerialPort::get_Available_Ports()
{
    return SimpleCommKitSerialPortPrivate::getAvailablePorts();
}

//
// Lifecycle
//
void SimpleCommKitSerialPort::init(const std::string& portName,
                                   int baudRate,
                                   Parity parity,
                                   DataBits dataBits,
                                   StopBits stopbits,
                                   FlowControl flowControl,
                                   unsigned int readBufferSize)
{
    if (d_ptr) {
        d_ptr->init(portName, baudRate, parity, dataBits, stopbits, flowControl, readBufferSize);
    }
}

bool SimpleCommKitSerialPort::open()
{
    if (!d_ptr) {
        return false;
    }
    return d_ptr->open();
}

void SimpleCommKitSerialPort::close()
{
    if (d_ptr) {
        d_ptr->close();
    }
}

bool SimpleCommKitSerialPort::is_Open()
{
    if (!d_ptr) {
        return false;
    }
    return d_ptr->isOpen();
}

//
// I/O
//
std::vector<uint8_t> SimpleCommKitSerialPort::read(int size)
{
    if (!d_ptr) {
        return {};
    }
    return d_ptr->read(size);
}

std::vector<uint8_t> SimpleCommKitSerialPort::read_All()
{
    if (!d_ptr) {
        return {};
    }
    return d_ptr->readAll();
}

int SimpleCommKitSerialPort::write(const std::vector<uint8_t>& data)
{
    if (!d_ptr) {
        return -1;
    }
    return d_ptr->write(data);
}

//
// Flush
//
bool SimpleCommKitSerialPort::flush_Buffers()
{
    if (!d_ptr) {
        return false;
    }
    return d_ptr->flushBuffers();
}

bool SimpleCommKitSerialPort::flush_Read_Buffers()
{
    if (!d_ptr) {
        return false;
    }
    return d_ptr->flushReadBuffers();
}

bool SimpleCommKitSerialPort::flush_Write_Buffers()
{
    if (!d_ptr) {
        return false;
    }
    return d_ptr->flushWriteBuffers();
}

//
// Error
//
int SimpleCommKitSerialPort::get_Last_Error()
{
    if (!d_ptr) {
        return -1;
    }
    return d_ptr->getLastError();
}

std::string SimpleCommKitSerialPort::get_Last_Error_Msg()
{
    if (!d_ptr) {
        return {};
    }
    return d_ptr->getLastErrorMsg();
}

void SimpleCommKitSerialPort::clear_Error()
{
    if (d_ptr) {
        d_ptr->clearError();
    }
}

//
// Configuration
//
void SimpleCommKitSerialPort::set_Port_Name(const std::string& portName)
{
    if (d_ptr) {
        d_ptr->setPortName(portName);
    }
}

std::string SimpleCommKitSerialPort::get_Port_Name()
{
    if (!d_ptr) {
        return {};
    }
    return d_ptr->getPortName();
}

void SimpleCommKitSerialPort::set_Baud_Rate(int baudRate)
{
    if (d_ptr) {
        d_ptr->setBaudRate(baudRate);
    }
}

int SimpleCommKitSerialPort::get_Baud_Rate()
{
    if (!d_ptr) {
        return BaudRate9600;
    }
    return d_ptr->getBaudRate();
}

void SimpleCommKitSerialPort::set_Parity(Parity parity)
{
    if (d_ptr) {
        d_ptr->setParity(parity);
    }
}

Parity SimpleCommKitSerialPort::get_Parity()
{
    if (!d_ptr) {
        return ParityNone;
    }
    return d_ptr->getParity();
}

void SimpleCommKitSerialPort::set_Data_Bits(DataBits dataBits)
{
    if (d_ptr) {
        d_ptr->setDataBits(dataBits);
    }
}

DataBits SimpleCommKitSerialPort::get_Data_Bits()
{
    if (!d_ptr) {
        return DataBits8;
    }
    return d_ptr->getDataBits();
}

void SimpleCommKitSerialPort::set_Stop_Bits(StopBits stopbits)
{
    if (d_ptr) {
        d_ptr->setStopBits(stopbits);
    }
}

StopBits SimpleCommKitSerialPort::get_Stop_Bits()
{
    if (!d_ptr) {
        return StopOne;
    }
    return d_ptr->getStopBits();
}

void SimpleCommKitSerialPort::set_Flow_Control(FlowControl flowControl)
{
    if (d_ptr) {
        d_ptr->setFlowControl(flowControl);
    }
}

FlowControl SimpleCommKitSerialPort::get_Flow_Control()
{
    if (!d_ptr) {
        return FlowNone;
    }
    return d_ptr->getFlowControl();
}

void SimpleCommKitSerialPort::set_Read_Buffer_Size(unsigned int size)
{
    if (d_ptr) {
        d_ptr->setReadBufferSize(size);
    }
}

unsigned int SimpleCommKitSerialPort::get_Read_Buffer_Size()
{
    if (!d_ptr) {
        return 4096;
    }
    return d_ptr->getReadBufferSize();
}

void SimpleCommKitSerialPort::set_Dtr(bool set)
{
    if (d_ptr) {
        d_ptr->setDtr(set);
    }
}

void SimpleCommKitSerialPort::set_Rts(bool set)
{
    if (d_ptr) {
        d_ptr->setRts(set);
    }
}

void SimpleCommKitSerialPort::set_Read_Interval_Timeout(unsigned int msecs)
{
    if (d_ptr) {
        d_ptr->setReadIntervalTimeout(msecs);
    }
}

unsigned int SimpleCommKitSerialPort::get_Read_Interval_Timeout()
{
    if (!d_ptr) {
        return 0;
    }
    return d_ptr->getReadIntervalTimeout();
}

//
// Callbacks
//
void SimpleCommKitSerialPort::set_Callback_On_Read(std::function<void(const std::vector<uint8_t>& data)> on_read)
{
    if (d_ptr) {
        d_ptr->setCallbackOnRead(std::move(on_read));
    }
}

void SimpleCommKitSerialPort::set_Callback_On_HotPlug(std::function<void(const std::string& portName, bool isAdd)> on_hotplug)
{
    if (d_ptr) {
        d_ptr->setCallbackOnHotPlug(std::move(on_hotplug));
    }
}

void SimpleCommKitSerialPort::set_Callback_Error(std::function<void(SimpleCommKit::ErrorCode)> on_error)
{
    if (d_ptr) {
        d_ptr->setCallbackError(std::move(on_error));
    }
}

} // namespace SimpleCommKit
