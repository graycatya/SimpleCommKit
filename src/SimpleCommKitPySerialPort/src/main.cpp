#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include <memory>
#include <string>
#include <vector>

#include <SimpleCommKitSerialPort.h>
#include <SimpleCommKitErrorMap.hpp>
#include <SimpleCommKitVersion.h>

namespace py = pybind11;
using namespace SimpleCommKit;

// ============================================================================
// Enums
// ============================================================================
void wrap_enums(py::module& m) {
    py::enum_<BaudRate>(m, "BaudRate", "Serial port baud rate enumeration.")
        .value("BAUD_110", BaudRate110)
        .value("BAUD_300", BaudRate300)
        .value("BAUD_600", BaudRate600)
        .value("BAUD_1200", BaudRate1200)
        .value("BAUD_2400", BaudRate2400)
        .value("BAUD_4800", BaudRate4800)
        .value("BAUD_9600", BaudRate9600)
        .value("BAUD_14400", BaudRate14400)
        .value("BAUD_19200", BaudRate19200)
        .value("BAUD_38400", BaudRate38400)
        .value("BAUD_56000", BaudRate56000)
        .value("BAUD_57600", BaudRate57600)
        .value("BAUD_115200", BaudRate115200)
        .value("BAUD_921600", BaudRate921600)
        .export_values();

    py::enum_<Parity>(m, "Parity", "Serial port parity enumeration.")
        .value("NONE", ParityNone)
        .value("ODD", ParityOdd)
        .value("EVEN", ParityEven)
        .value("MARK", ParityMark)
        .value("SPACE", ParitySpace)
        .export_values();

    py::enum_<DataBits>(m, "DataBits", "Serial port data bits enumeration.")
        .value("BITS_5", DataBits5)
        .value("BITS_6", DataBits6)
        .value("BITS_7", DataBits7)
        .value("BITS_8", DataBits8)
        .export_values();

    py::enum_<StopBits>(m, "StopBits", "Serial port stop bits enumeration.")
        .value("ONE", StopOne)
        .value("ONE_AND_HALF", StopOneAndHalf)
        .value("TWO", StopTwo)
        .export_values();

    py::enum_<FlowControl>(m, "FlowControl", "Serial port flow control enumeration.")
        .value("NONE", FlowNone)
        .value("HARDWARE", FlowHardware)
        .value("SOFTWARE", FlowSoftware)
        .export_values();
}

// ============================================================================
// SerialPortInfo struct
// ============================================================================
void wrap_port_info(py::module& m) {
    py::class_<SimpleCommKitSerialPortInfo>(m, "SerialPortInfo",
        "Represents information about an available serial port.")
        .def(py::init<>())
        .def_readwrite("port_name", &SimpleCommKitSerialPortInfo::portName,
            "Port name (e.g. COM3, /dev/ttyUSB0)")
        .def_readwrite("description", &SimpleCommKitSerialPortInfo::description,
            "Human-readable description of the port")
        .def_readwrite("hardware_id", &SimpleCommKitSerialPortInfo::hardwareId,
            "Hardware ID of the device")
        .def("__repr__", [](const SimpleCommKitSerialPortInfo& info) {
            return "<SerialPortInfo port_name='" + info.portName +
                   "' description='" + info.description + "'>";
        });
}

// ============================================================================
// SerialPort class wrapping SimpleCommKitSerialPort
// ============================================================================
void wrap_serial_port(py::module& m) {
    py::class_<SimpleCommKitSerialPort>(m, "SerialPort",
        "Serial port manager for opening, reading, writing, and monitoring serial ports.")

        .def(py::init<>(), "Create a new SerialPort instance.")

        // --- Static: enumerate available ports ---
        .def_static("get_available_ports", &SimpleCommKitSerialPort::get_Available_Ports,
            "Get a list of available serial ports on the system.")

        // --- Lifecycle: init / open / close ---
        .def("init", &SimpleCommKitSerialPort::init,
            py::arg("port_name"),
            py::arg("baud_rate") = BaudRate9600,
            py::arg("parity") = ParityNone,
            py::arg("data_bits") = DataBits8,
            py::arg("stop_bits") = StopOne,
            py::arg("flow_control") = FlowNone,
            py::arg("read_buffer_size") = 4096,
            "Initialize the serial port with the given parameters.")

        .def("open", &SimpleCommKitSerialPort::open,
            "Open the serial port. Returns True on success.")

        .def("close", &SimpleCommKitSerialPort::close,
            "Close the serial port.")

        .def("is_open", &SimpleCommKitSerialPort::is_Open,
            "Check if the serial port is currently open.")

        // --- I/O ---
        .def("read",
            [](SimpleCommKitSerialPort& self, int size) -> py::bytes {
                auto data = self.read(size);
                return py::bytes(reinterpret_cast<const char*>(data.data()), data.size());
            },
            py::arg("size"),
            "Read up to 'size' bytes from the serial port. Returns bytes.")

        .def("read_all",
            [](SimpleCommKitSerialPort& self) -> py::bytes {
                auto data = self.read_All();
                return py::bytes(reinterpret_cast<const char*>(data.data()), data.size());
            },
            "Read all available bytes from the serial port. Returns bytes.")

        .def("write",
            [](SimpleCommKitSerialPort& self, py::bytes data) -> int {
                std::string str = data;
                std::vector<uint8_t> vec(str.begin(), str.end());
                return self.write(vec);
            },
            py::arg("data"),
            "Write data to the serial port. Data is bytes. Returns number of bytes written.")

        // --- Flush ---
        .def("flush_buffers", &SimpleCommKitSerialPort::flush_Buffers,
            "Flush all buffers (read and write). Returns True on success.")

        .def("flush_read_buffers", &SimpleCommKitSerialPort::flush_Read_Buffers,
            "Flush read buffer. Returns True on success.")

        .def("flush_write_buffers", &SimpleCommKitSerialPort::flush_Write_Buffers,
            "Flush write buffer. Returns True on success.")

        // --- Error ---
        .def("get_last_error", &SimpleCommKitSerialPort::get_Last_Error,
            "Get the last error code.")

        .def("get_last_error_msg", &SimpleCommKitSerialPort::get_Last_Error_Msg,
            "Get the last error message string.")

        .def("clear_error", &SimpleCommKitSerialPort::clear_Error,
            "Clear the last error.")

        // --- Configuration getters / setters ---
        .def("set_port_name", &SimpleCommKitSerialPort::set_Port_Name,
            py::arg("port_name"),
            "Set the serial port name (e.g. COM3, /dev/ttyUSB0).")

        .def("get_port_name", &SimpleCommKitSerialPort::get_Port_Name,
            "Get the serial port name.")

        .def("set_baud_rate", &SimpleCommKitSerialPort::set_Baud_Rate,
            py::arg("baud_rate"),
            "Set the baud rate (use BaudRate enum values).")

        .def("get_baud_rate", &SimpleCommKitSerialPort::get_Baud_Rate,
            "Get the current baud rate.")

        .def("set_parity", &SimpleCommKitSerialPort::set_Parity,
            py::arg("parity"),
            "Set the parity mode (use Parity enum values).")

        .def("get_parity", &SimpleCommKitSerialPort::get_Parity,
            "Get the current parity mode.")

        .def("set_data_bits", &SimpleCommKitSerialPort::set_Data_Bits,
            py::arg("data_bits"),
            "Set the data bits (use DataBits enum values).")

        .def("get_data_bits", &SimpleCommKitSerialPort::get_Data_Bits,
            "Get the current data bits.")

        .def("set_stop_bits", &SimpleCommKitSerialPort::set_Stop_Bits,
            py::arg("stop_bits"),
            "Set the stop bits (use StopBits enum values).")

        .def("get_stop_bits", &SimpleCommKitSerialPort::get_Stop_Bits,
            "Get the current stop bits.")

        .def("set_flow_control", &SimpleCommKitSerialPort::set_Flow_Control,
            py::arg("flow_control"),
            "Set the flow control mode (use FlowControl enum values).")

        .def("get_flow_control", &SimpleCommKitSerialPort::get_Flow_Control,
            "Get the current flow control mode.")

        .def("set_read_buffer_size", &SimpleCommKitSerialPort::set_Read_Buffer_Size,
            py::arg("size"),
            "Set the read buffer size in bytes.")

        .def("get_read_buffer_size", &SimpleCommKitSerialPort::get_Read_Buffer_Size,
            "Get the current read buffer size.")

        .def("set_dtr", &SimpleCommKitSerialPort::set_Dtr,
            py::arg("set") = true,
            "Set or clear the DTR (Data Terminal Ready) line.")

        .def("set_rts", &SimpleCommKitSerialPort::set_Rts,
            py::arg("set") = true,
            "Set or clear the RTS (Request To Send) line.")

        .def("set_read_interval_timeout", &SimpleCommKitSerialPort::set_Read_Interval_Timeout,
            py::arg("msecs"),
            "Set the read interval timeout in milliseconds.")

        .def("get_read_interval_timeout", &SimpleCommKitSerialPort::get_Read_Interval_Timeout,
            "Get the current read interval timeout in milliseconds.")

        // --- Callbacks ---
        .def("set_callback_on_read",
            [](SimpleCommKitSerialPort& self, py::function callback) {
                auto cb = std::shared_ptr<py::object>(
                    new py::object(std::move(callback)),
                    [](py::object* p) {
                        py::gil_scoped_acquire gil;
                        delete p;
                    });
                self.set_Callback_On_Read(
                    [cb](const std::vector<uint8_t>& data) {
                        py::gil_scoped_acquire gil;
                        (*cb)(py::bytes(reinterpret_cast<const char*>(data.data()), data.size()));
                    });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback invoked when data is received. Callback receives bytes payload.")

        .def("set_callback_on_hotplug",
            [](SimpleCommKitSerialPort& self, py::function callback) {
                auto cb = std::shared_ptr<py::object>(
                    new py::object(std::move(callback)),
                    [](py::object* p) {
                        py::gil_scoped_acquire gil;
                        delete p;
                    });
                self.set_Callback_On_HotPlug(
                    [cb](const std::string& portName, bool isAdd) {
                        py::gil_scoped_acquire gil;
                        (*cb)(portName, isAdd);
                    });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback invoked on hot-plug events. "
            "Callback receives (port_name: str, is_add: bool).")

        .def("set_callback_error",
            [](SimpleCommKitSerialPort& self, py::function callback) {
                self.set_Callback_Error([callback](ErrorCode error_code) {
                    py::gil_scoped_acquire gil;
                    callback(error_code);
                });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set a callback to receive error notifications. "
            "Callback receives error_code (int); use get_error_description(error_code) for details.");
}

// ============================================================================
// Module entry point
// ============================================================================
PYBIND11_MODULE(_SimpleCommKitPySerialPort, m) {
    m.doc() = R"pbdoc(
        SimpleCommKitPySerialPort - Python bindings for SimpleCommKit SerialPort

        Provides a cross-platform serial port API for Python.
    )pbdoc";

    // Version / utility
    m.def("get_error_description", &SimpleCommKitErrorMap::GetErrorDescription,
          py::arg("error_code"),
          "Get a human-readable description for an error code.");

    m.def("get_version", &SimpleCommKit::getSimpleCommKitVersion,
        "Get the SimpleCommKit library version string.");

    // Wrap types
    wrap_enums(m);
    wrap_port_info(m);

    // Wrap main class
    wrap_serial_port(m);
}
