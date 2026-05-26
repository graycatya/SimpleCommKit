#pragma once

#include <SimpleCommKitErrorMap.hpp>
#include "SimpleCommKitExport.h"

#include <vector>
#include <functional>
#include <memory>
#include <string>
#include <cstdint>

namespace SimpleCommKit {

SIMPLECOMMKIT_DECLARE_PRIVATE_CLASS(SimpleCommKitSerialPort)

enum BaudRate : int {
    BaudRate110 = 110,
    BaudRate300 = 300,
    BaudRate600 = 600,
    BaudRate1200 = 1200,
    BaudRate2400 = 2400,
    BaudRate4800 = 4800,
    BaudRate9600 = 9600,
    BaudRate14400 = 14400,
    BaudRate19200 = 19200,
    BaudRate38400 = 38400,
    BaudRate56000 = 56000,
    BaudRate57600 = 57600,
    BaudRate115200 = 115200,
    BaudRate921600 = 921600,
};

enum Parity : int {
    ParityNone = 0,
    ParityOdd = 1,
    ParityEven = 2,
    ParityMark = 3,
    ParitySpace = 4,
};

enum DataBits : int {
    DataBits5 = 5,
    DataBits6 = 6,
    DataBits7 = 7,
    DataBits8 = 8,
};

enum StopBits : int {
    StopOne = 0,
    StopOneAndHalf = 1,
    StopTwo = 2,
};

enum FlowControl : int {
    FlowNone = 0,
    FlowHardware = 1,
    FlowSoftware = 2,
};

struct SimpleCommKitSerialPortInfo {
    std::string portName;
    std::string description;
    std::string hardwareId;
};

class SIMPLECOMMKIT_API SimpleCommKitSerialPort
{
    SIMPLECOMMKIT_DECLARE_PRIVATE(SimpleCommKitSerialPort)
public:
    SimpleCommKitSerialPort();
    ~SimpleCommKitSerialPort();

    //
    // Static: enumerate available serial ports
    //
    static std::vector<SimpleCommKitSerialPortInfo> get_Available_Ports();

    //
    // Lifecycle: init / open / close
    //
    void init(const std::string& portName,
              int baudRate = BaudRate9600,
              Parity parity = ParityNone,
              DataBits dataBits = DataBits8,
              StopBits stopbits = StopOne,
              FlowControl flowControl = FlowNone,
              unsigned int readBufferSize = 4096);

    bool open();
    void close();
    bool is_Open();

    //
    // I/O
    //
    std::vector<uint8_t> read(int size);
    std::vector<uint8_t> read_All();
    int write(const std::vector<uint8_t>& data);

    //
    // Flush
    //
    bool flush_Buffers();
    bool flush_Read_Buffers();
    bool flush_Write_Buffers();

    //
    // Error
    //
    int get_Last_Error();
    std::string get_Last_Error_Msg();
    void clear_Error();

    //
    // Configuration getters / setters
    //
    void set_Port_Name(const std::string& portName);
    std::string get_Port_Name();

    void set_Baud_Rate(int baudRate);
    int get_Baud_Rate();

    void set_Parity(Parity parity);
    Parity get_Parity();

    void set_Data_Bits(DataBits dataBits);
    DataBits get_Data_Bits();

    void set_Stop_Bits(StopBits stopbits);
    StopBits get_Stop_Bits();

    void set_Flow_Control(FlowControl flowControl);
    FlowControl get_Flow_Control();

    void set_Read_Buffer_Size(unsigned int size);
    unsigned int get_Read_Buffer_Size();

    void set_Dtr(bool set = true);
    void set_Rts(bool set = true);

    void set_Read_Interval_Timeout(unsigned int msecs);
    unsigned int get_Read_Interval_Timeout();

    //
    // Callbacks
    //
    void set_Callback_On_Read(std::function<void(const std::vector<uint8_t>& data)> on_read);
    void set_Callback_On_HotPlug(std::function<void(const std::string& portName, bool isAdd)> on_hotplug);
    void set_Callback_Error(std::function<void(SimpleCommKit::ErrorCode)> on_error);

private:
    std::unique_ptr<SimpleCommKitSerialPortPrivate> d_ptr;
};

} // namespace SimpleCommKit
