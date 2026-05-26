#include "SimpleCommKitSerialPort.h"
#include "SimpleCommKitSerialPort_p.h"
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

namespace SimpleCommKit {

#ifdef _WIN32
/// Convert a string from the system's ANSI code page (e.g. GBK on Chinese Windows)
/// to UTF-8, so pybind11 can safely decode it as a Python str.
static std::string toUtf8(const std::string& ansiString)
{
    if (ansiString.empty())
        return ansiString;

    // ANSI → wide
    int wlen = MultiByteToWideChar(CP_ACP, 0, ansiString.c_str(), -1, nullptr, 0);
    if (wlen <= 0)
        return ansiString;  // fallback: return as-is
    std::wstring wide(wlen, L'\0');
    MultiByteToWideChar(CP_ACP, 0, ansiString.c_str(), -1, &wide[0], wlen);

    // wide → UTF-8
    int utf8len = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8len <= 0)
        return ansiString;
    std::string utf8(utf8len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &utf8[0], utf8len, nullptr, nullptr);

    // Strip the trailing null terminator
    if (!utf8.empty() && utf8.back() == '\0')
        utf8.pop_back();
    return utf8;
}
#endif

// ============================================================
// Static Callbacks for C API
// ============================================================

SimpleCommKitSerialPortPrivate* SimpleCommKitSerialPortPrivate::s_currentInstance = nullptr;

void SimpleCommKitSerialPortPrivate::onReadEventCallback(i_handle_t handle, const char* portName, unsigned int readBufferLen)
{
    (void)portName;
    if (s_currentInstance && s_currentInstance->m_listenerBridge && s_currentInstance->m_listenerBridge->m_onRead && readBufferLen > 0) {
        std::vector<uint8_t> buffer(readBufferLen);
        int bytesRead = CSerialPortReadData(handle, buffer.data(), static_cast<int>(readBufferLen));
        if (bytesRead > 0) {
            buffer.resize(static_cast<size_t>(bytesRead));
            s_currentInstance->m_listenerBridge->m_onRead(buffer);
        }
    }
}

void SimpleCommKitSerialPortPrivate::onHotPlugEventCallback(i_handle_t handle, const char* portName, int isAdd)
{
    (void)handle;
    if (s_currentInstance && s_currentInstance->m_listenerBridge && s_currentInstance->m_listenerBridge->m_onHotPlug) {
        s_currentInstance->m_listenerBridge->m_onHotPlug(std::string(portName ? portName : ""), isAdd != 0);
    }
}

// ============================================================
// Type Conversion Helpers
// ============================================================

itas109::Parity SimpleCommKitSerialPortPrivate::toCSerialPortParity(Parity parity)
{
    switch (parity) {
    case ParityNone:  return itas109::ParityNone;
    case ParityOdd:   return itas109::ParityOdd;
    case ParityEven:  return itas109::ParityEven;
    case ParityMark:  return itas109::ParityMark;
    case ParitySpace: return itas109::ParitySpace;
    default:          return itas109::ParityNone;
    }
}

itas109::DataBits SimpleCommKitSerialPortPrivate::toCSerialPortDataBits(DataBits dataBits)
{
    switch (dataBits) {
    case DataBits5: return itas109::DataBits5;
    case DataBits6: return itas109::DataBits6;
    case DataBits7: return itas109::DataBits7;
    case DataBits8: return itas109::DataBits8;
    default:        return itas109::DataBits8;
    }
}

itas109::StopBits SimpleCommKitSerialPortPrivate::toCSerialPortStopBits(StopBits stopbits)
{
    switch (stopbits) {
    case StopOne:        return itas109::StopOne;
    case StopOneAndHalf: return itas109::StopOneAndHalf;
    case StopTwo:        return itas109::StopTwo;
    default:             return itas109::StopOne;
    }
}

itas109::FlowControl SimpleCommKitSerialPortPrivate::toCSerialPortFlowControl(FlowControl flowControl)
{
    switch (flowControl) {
    case FlowNone:     return itas109::FlowNone;
    case FlowHardware: return itas109::FlowHardware;
    case FlowSoftware: return itas109::FlowSoftware;
    default:           return itas109::FlowNone;
    }
}

Parity SimpleCommKitSerialPortPrivate::fromCSerialPortParity(itas109::Parity parity)
{
    switch (parity) {
    case itas109::ParityNone:  return ParityNone;
    case itas109::ParityOdd:   return ParityOdd;
    case itas109::ParityEven:  return ParityEven;
    case itas109::ParityMark:  return ParityMark;
    case itas109::ParitySpace: return ParitySpace;
    default:                   return ParityNone;
    }
}

DataBits SimpleCommKitSerialPortPrivate::fromCSerialPortDataBits(itas109::DataBits dataBits)
{
    switch (dataBits) {
    case itas109::DataBits5: return DataBits5;
    case itas109::DataBits6: return DataBits6;
    case itas109::DataBits7: return DataBits7;
    case itas109::DataBits8: return DataBits8;
    default:                 return DataBits8;
    }
}

StopBits SimpleCommKitSerialPortPrivate::fromCSerialPortStopBits(itas109::StopBits stopbits)
{
    switch (stopbits) {
    case itas109::StopOne:        return StopOne;
    case itas109::StopOneAndHalf: return StopOneAndHalf;
    case itas109::StopTwo:        return StopTwo;
    default:                      return StopOne;
    }
}

FlowControl SimpleCommKitSerialPortPrivate::fromCSerialPortFlowControl(itas109::FlowControl flowControl)
{
    switch (flowControl) {
    case itas109::FlowNone:     return FlowNone;
    case itas109::FlowHardware: return FlowHardware;
    case itas109::FlowSoftware: return FlowSoftware;
    default:                    return FlowNone;
    }
}

// ============================================================
// SimpleCommKitSerialPortPrivate Implementation
// ============================================================

SimpleCommKitSerialPortPrivate::SimpleCommKitSerialPortPrivate(SimpleCommKitSerialPort* parent)
    : q_ptr(parent)
    , m_serialPort(std::make_unique<itas109::CSerialPort>())
    , m_listenerBridge(std::make_unique<SerialPortListenerBridge>())
{
    m_handle = reinterpret_cast<i_handle_t>(m_serialPort.get());
}

SimpleCommKitSerialPortPrivate::~SimpleCommKitSerialPortPrivate()
{
    try {
        if (m_serialPort && m_serialPort->isOpen()) {
            CSerialPortDisconnectReadEvent(m_handle);
            CSerialPortDisconnectHotPlugEvent(m_handle);
            m_serialPort->close();
        }
        s_currentInstance = nullptr;
    } catch (...) {
        // ignore errors in destructor
    }
}

//
// Static
//
std::vector<SimpleCommKitSerialPortInfo> SimpleCommKitSerialPortPrivate::getAvailablePorts()
{
    std::vector<SimpleCommKitSerialPortInfo> result;
    try {
        auto infos = itas109::CSerialPortInfo::availablePortInfos();
        for (const auto& info : infos) {
            SimpleCommKitSerialPortInfo portInfo;
            portInfo.portName    = info.portName;
#ifdef _WIN32
            // On Windows, descriptions come in the system ANSI code page (e.g. GBK).
            // Convert to UTF-8 so pybind11 and downstream consumers can decode them.
            portInfo.description = toUtf8(info.description);
            portInfo.hardwareId  = toUtf8(info.hardwareId);
#else
            portInfo.description = info.description;
            portInfo.hardwareId  = info.hardwareId;
#endif
            result.push_back(std::move(portInfo));
        }
    } catch (const std::exception&) {
        // silently return empty vector
    }
    return result;
}

//
// Lifecycle
//
void SimpleCommKitSerialPortPrivate::init(const std::string& portName,
                                          int baudRate,
                                          Parity parity,
                                          DataBits dataBits,
                                          StopBits stopbits,
                                          FlowControl flowControl,
                                          unsigned int readBufferSize)
{
    m_portName       = portName;
    m_baudRate       = baudRate;
    m_parity         = parity;
    m_dataBits       = dataBits;
    m_stopbits       = stopbits;
    m_flowControl    = flowControl;
    m_readBufferSize = readBufferSize;

    try {
        // Disconnect and close any previous port
        CSerialPortDisconnectReadEvent(m_handle);
        CSerialPortDisconnectHotPlugEvent(m_handle);
        m_serialPort->close();

        // Create a new serial port instance
        m_serialPort = std::make_unique<itas109::CSerialPort>();
        m_handle = reinterpret_cast<i_handle_t>(m_serialPort.get());

        m_serialPort->init(portName.c_str(),
                           baudRate,
                           toCSerialPortParity(parity),
                           toCSerialPortDataBits(dataBits),
                           toCSerialPortStopBits(stopbits),
                           toCSerialPortFlowControl(flowControl),
                           readBufferSize);
    } catch (const std::exception&) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitSerialPortInitError);
    }
}

bool SimpleCommKitSerialPortPrivate::open()
{
    try {
        if (m_serialPort->open()) {
            s_currentInstance = this;
            // connect for read
            CSerialPortConnectReadEvent(m_handle, onReadEventCallback);
            // connect for hot plug
            CSerialPortConnectHotPlugEvent(m_handle, onHotPlugEventCallback);
            return true;
        } else {
            triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitSerialPortOpenError);
            return false;
        }
    } catch (const std::exception&) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitSerialPortOpenError);
        return false;
    }
}

void SimpleCommKitSerialPortPrivate::close()
{
    try {
        CSerialPortDisconnectReadEvent(m_handle);
        CSerialPortDisconnectHotPlugEvent(m_handle);
        m_serialPort->close();
    } catch (const std::exception&) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitSerialPortCloseError);
    }
}

bool SimpleCommKitSerialPortPrivate::isOpen()
{
    return m_serialPort->isOpen();
}

//
// I/O
//
std::vector<uint8_t> SimpleCommKitSerialPortPrivate::read(int size)
{
    if (!m_serialPort->isOpen()) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitSerialPortNotOpenError);
        return {};
    }

    std::vector<uint8_t> buffer(size);
    try {
        int bytesRead = m_serialPort->readData(buffer.data(), size);
        if (bytesRead < 0) {
            triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitSerialPortReadError);
            return {};
        }
        buffer.resize(static_cast<size_t>(bytesRead));
        return buffer;
    } catch (const std::exception&) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitSerialPortReadError);
        return {};
    }
}

std::vector<uint8_t> SimpleCommKitSerialPortPrivate::readAll()
{
    if (!m_serialPort->isOpen()) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitSerialPortNotOpenError);
        return {};
    }

    unsigned int bufLen = m_serialPort->getReadBufferUsedLen();
    if (bufLen == 0) {
        return {};
    }

    std::vector<uint8_t> buffer(bufLen);
    try {
        int bytesRead = m_serialPort->readAllData(buffer.data());
        if (bytesRead < 0) {
            triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitSerialPortReadError);
            return {};
        }
        buffer.resize(static_cast<size_t>(bytesRead));
        return buffer;
    } catch (const std::exception&) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitSerialPortReadError);
        return {};
    }
}

int SimpleCommKitSerialPortPrivate::write(const std::vector<uint8_t>& data)
{
    if (!m_serialPort->isOpen()) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitSerialPortNotOpenError);
        return -1;
    }

    if (data.empty()) {
        return 0;
    }

    try {
        int bytesWritten = m_serialPort->writeData(data.data(), static_cast<int>(data.size()));
        if (bytesWritten < 0) {
            triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitSerialPortWriteError);
            return -1;
        }
        return bytesWritten;
    } catch (const std::exception&) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitSerialPortWriteError);
        return -1;
    }
}

//
// Flush
//
bool SimpleCommKitSerialPortPrivate::flushBuffers()
{
    try {
        return m_serialPort->flushBuffers();
    } catch (const std::exception&) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitSerialPortFlushError);
        return false;
    }
}

bool SimpleCommKitSerialPortPrivate::flushReadBuffers()
{
    try {
        return m_serialPort->flushReadBuffers();
    } catch (const std::exception&) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitSerialPortFlushError);
        return false;
    }
}

bool SimpleCommKitSerialPortPrivate::flushWriteBuffers()
{
    try {
        return m_serialPort->flushWriteBuffers();
    } catch (const std::exception&) {
        triggerError(SimpleCommKit::ErrorCodes::SimpleCommKitSerialPortFlushError);
        return false;
    }
}

//
// Error
//
int SimpleCommKitSerialPortPrivate::getLastError()
{
    return m_serialPort->getLastError();
}

std::string SimpleCommKitSerialPortPrivate::getLastErrorMsg()
{
    const char* msg = m_serialPort->getLastErrorMsg();
    return msg ? std::string(msg) : std::string();
}

void SimpleCommKitSerialPortPrivate::clearError()
{
    m_serialPort->clearError();
}

//
// Config
//
void SimpleCommKitSerialPortPrivate::setPortName(const std::string& portName)
{
    m_portName = portName;
    m_serialPort->setPortName(portName.c_str());
}

std::string SimpleCommKitSerialPortPrivate::getPortName()
{
    const char* name = m_serialPort->getPortName();
    return name ? std::string(name) : m_portName;
}

void SimpleCommKitSerialPortPrivate::setBaudRate(int baudRate)
{
    m_baudRate = baudRate;
    m_serialPort->setBaudRate(baudRate);
}

int SimpleCommKitSerialPortPrivate::getBaudRate()
{
    return m_serialPort->getBaudRate();
}

void SimpleCommKitSerialPortPrivate::setParity(Parity parity)
{
    m_parity = parity;
    m_serialPort->setParity(toCSerialPortParity(parity));
}

Parity SimpleCommKitSerialPortPrivate::getParity()
{
    return fromCSerialPortParity(m_serialPort->getParity());
}

void SimpleCommKitSerialPortPrivate::setDataBits(DataBits dataBits)
{
    m_dataBits = dataBits;
    m_serialPort->setDataBits(toCSerialPortDataBits(dataBits));
}

DataBits SimpleCommKitSerialPortPrivate::getDataBits()
{
    return fromCSerialPortDataBits(m_serialPort->getDataBits());
}

void SimpleCommKitSerialPortPrivate::setStopBits(StopBits stopbits)
{
    m_stopbits = stopbits;
    m_serialPort->setStopBits(toCSerialPortStopBits(stopbits));
}

StopBits SimpleCommKitSerialPortPrivate::getStopBits()
{
    return fromCSerialPortStopBits(m_serialPort->getStopBits());
}

void SimpleCommKitSerialPortPrivate::setFlowControl(FlowControl flowControl)
{
    m_flowControl = flowControl;
    m_serialPort->setFlowControl(toCSerialPortFlowControl(flowControl));
}

FlowControl SimpleCommKitSerialPortPrivate::getFlowControl()
{
    return fromCSerialPortFlowControl(m_serialPort->getFlowControl());
}

void SimpleCommKitSerialPortPrivate::setReadBufferSize(unsigned int size)
{
    m_readBufferSize = size;
    m_serialPort->setReadBufferSize(size);
}

unsigned int SimpleCommKitSerialPortPrivate::getReadBufferSize()
{
    return m_serialPort->getReadBufferSize();
}

void SimpleCommKitSerialPortPrivate::setDtr(bool set)
{
    m_serialPort->setDtr(set);
}

void SimpleCommKitSerialPortPrivate::setRts(bool set)
{
    m_serialPort->setRts(set);
}

void SimpleCommKitSerialPortPrivate::setReadIntervalTimeout(unsigned int msecs)
{
    m_serialPort->setReadIntervalTimeout(msecs);
}

unsigned int SimpleCommKitSerialPortPrivate::getReadIntervalTimeout()
{
    return m_serialPort->getReadIntervalTimeout();
}

//
// Callbacks
//
void SimpleCommKitSerialPortPrivate::setCallbackOnRead(std::function<void(const std::vector<uint8_t>&)> callback)
{
    m_listenerBridge->m_onRead = std::move(callback);
}

void SimpleCommKitSerialPortPrivate::setCallbackOnHotPlug(std::function<void(const std::string&, bool)> callback)
{
    m_listenerBridge->m_onHotPlug = std::move(callback);
}

void SimpleCommKitSerialPortPrivate::setCallbackError(std::function<void(SimpleCommKit::ErrorCode)> callback)
{
    m_onError = std::move(callback);
}

void SimpleCommKitSerialPortPrivate::triggerError(SimpleCommKit::ErrorCode error_code)
{
    if (m_onError) {
        m_onError(error_code);
    }
}

} // namespace SimpleCommKit
