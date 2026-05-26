#pragma once

#include <CSerialPort/SerialPort.h>
#include <CSerialPort/SerialPortInfo.h>
#include "cserialport.h"
#include "SimpleCommKitSerialPort.h"
#include "SimpleCommKitExport.h"
#include <functional>
#include <memory>

namespace SimpleCommKit {

//
// Internal listener class that bridges CSerialPort C API callbacks
// to std::function callbacks used by SimpleCommKitSerialPort
//
class SerialPortListenerBridge
{
public:
    SerialPortListenerBridge() = default;

    std::function<void(const std::vector<uint8_t>&)> m_onRead;
    std::function<void(const std::string&, bool)> m_onHotPlug;
};

class SimpleCommKitSerialPortPrivate
{
    SIMPLECOMMKIT_DECLARE_PUBLIC(SimpleCommKitSerialPort)
public:
    SimpleCommKitSerialPortPrivate(SimpleCommKitSerialPort* parent);
    ~SimpleCommKitSerialPortPrivate();

    //
    // Static
    //
    static std::vector<SimpleCommKitSerialPortInfo> getAvailablePorts();

    //
    // Lifecycle
    //
    void init(const std::string& portName,
              int baudRate,
              Parity parity,
              DataBits dataBits,
              StopBits stopbits,
              FlowControl flowControl,
              unsigned int readBufferSize);

    bool open();
    void close();
    bool isOpen();

    //
    // I/O
    //
    std::vector<uint8_t> read(int size);
    std::vector<uint8_t> readAll();
    int write(const std::vector<uint8_t>& data);

    //
    // Flush
    //
    bool flushBuffers();
    bool flushReadBuffers();
    bool flushWriteBuffers();

    //
    // Error
    //
    int getLastError();
    std::string getLastErrorMsg();
    void clearError();

    //
    // Config getters / setters
    //
    void setPortName(const std::string& portName);
    std::string getPortName();

    void setBaudRate(int baudRate);
    int getBaudRate();

    void setParity(Parity parity);
    Parity getParity();

    void setDataBits(DataBits dataBits);
    DataBits getDataBits();

    void setStopBits(StopBits stopbits);
    StopBits getStopBits();

    void setFlowControl(FlowControl flowControl);
    FlowControl getFlowControl();

    void setReadBufferSize(unsigned int size);
    unsigned int getReadBufferSize();

    void setDtr(bool set);
    void setRts(bool set);

    void setReadIntervalTimeout(unsigned int msecs);
    unsigned int getReadIntervalTimeout();

    //
    // Callbacks
    //
    void setCallbackOnRead(std::function<void(const std::vector<uint8_t>&)> callback);
    void setCallbackOnHotPlug(std::function<void(const std::string&, bool)> callback);
    void setCallbackError(std::function<void(SimpleCommKit::ErrorCode)> callback);

    void triggerError(SimpleCommKit::ErrorCode error_code);

private:
    //
    // Static callbacks for C API
    //
    static void onReadEventCallback(i_handle_t handle, const char* portName, unsigned int readBufferLen);
    static void onHotPlugEventCallback(i_handle_t handle, const char* portName, int isAdd);
    static SimpleCommKitSerialPortPrivate* s_currentInstance;

    // Convert our enum types to CSerialPort enum types
    static itas109::Parity     toCSerialPortParity(Parity parity);
    static itas109::DataBits   toCSerialPortDataBits(DataBits dataBits);
    static itas109::StopBits   toCSerialPortStopBits(StopBits stopbits);
    static itas109::FlowControl toCSerialPortFlowControl(FlowControl flowControl);

    static Parity     fromCSerialPortParity(itas109::Parity parity);
    static DataBits   fromCSerialPortDataBits(itas109::DataBits dataBits);
    static StopBits   fromCSerialPortStopBits(itas109::StopBits stopbits);
    static FlowControl fromCSerialPortFlowControl(itas109::FlowControl flowControl);

    SimpleCommKitSerialPort* q_ptr;
    std::unique_ptr<itas109::CSerialPort> m_serialPort;
    i_handle_t m_handle = 0;
    std::unique_ptr<SerialPortListenerBridge> m_listenerBridge;

    std::function<void(SimpleCommKit::ErrorCode)> m_onError;

    std::string    m_portName;
    int            m_baudRate      = BaudRate9600;
    Parity         m_parity        = ParityNone;
    DataBits       m_dataBits      = DataBits8;
    StopBits       m_stopbits      = StopOne;
    FlowControl    m_flowControl   = FlowNone;
    unsigned int   m_readBufferSize = 4096;
};

} // namespace SimpleCommKit
