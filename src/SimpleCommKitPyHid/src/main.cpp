#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include <memory>
#include <string>
#include <vector>

#include <SimpleCommKitHid.h>
#include <SimpleCommKitErrorMap.hpp>
#include <SimpleCommKitVersion.h>

namespace py = pybind11;
using namespace SimpleCommKit;

// ============================================================================
// HidBusType enum
// ============================================================================
void wrap_enums(py::module& m) {
    py::enum_<SimpleCommKitHidBusType>(m, "HidBusType",
        "HID bus type enumeration.")
        .value("UNKNOWN",    SimpleCommKitHidBusType::HID_BUS_TYPE_UNKNOWN)
        .value("USB",        SimpleCommKitHidBusType::HID_BUS_TYPE_USB)
        .value("BLUETOOTH",  SimpleCommKitHidBusType::HID_BUS_TYPE_BLUETOOTH)
        .value("I2C",        SimpleCommKitHidBusType::HID_BUS_TYPE_I2C)
        .value("SPI",        SimpleCommKitHidBusType::HID_BUS_TYPE_SPI)
        .export_values();
}

// ============================================================================
// HidDeviceInfo struct
// ============================================================================
void wrap_device_info(py::module& m) {
    py::class_<SimpleCommKitHidDeviceInfo>(m, "HidDeviceInfo",
        "Represents a HID device's identification information.")
        .def(py::init<>())
        .def_readwrite("interface_number",    &SimpleCommKitHidDeviceInfo::interface_number,
            "Interface number of the device")
        .def_readwrite("manufacturer_string", &SimpleCommKitHidDeviceInfo::manufacturer_string,
            "Manufacturer string")
        .def_readwrite("product_string",      &SimpleCommKitHidDeviceInfo::product_string,
            "Product string")
        .def_readwrite("release_number",      &SimpleCommKitHidDeviceInfo::release_number,
            "Release/bcdDevice number")
        .def_readwrite("bus_type",            &SimpleCommKitHidDeviceInfo::bus_type,
            "Bus type (see HidBusType enum)")
        .def_readwrite("serial_number",       &SimpleCommKitHidDeviceInfo::serial_number,
            "Serial number string")
        .def_readwrite("path",                &SimpleCommKitHidDeviceInfo::path,
            "Platform-specific device path (used for open)")
        .def("__repr__", [](const SimpleCommKitHidDeviceInfo& d) {
            return "<HidDeviceInfo path='" + d.path +
                   "' product='" + d.product_string +
                   "' serial='" + d.serial_number + "'>";
        });
}

// ============================================================================
// SimpleCommKitHid class wrapping
// ============================================================================
void wrap_hid(py::module& m) {
    py::class_<SimpleCommKitHid>(m, "SimpleCommKitHid",
        "Cross-platform HID manager for enumerating, opening, and interacting with HID devices.")

        .def(py::init<>(), "Create a new SimpleCommKitHid instance.")

        // --- Static: enumerate ---
        .def_static("get_available_devices", &SimpleCommKitHid::get_Available_Devices,
            py::arg("vendor_id")  = 0x0,
            py::arg("product_id") = 0x0,
            "Enumerate available HID devices, optionally filtered by VID/PID.")

        // --- Lifecycle ---
        .def("init", &SimpleCommKitHid::init,
            py::arg("vendor_id")  = 0x0,
            py::arg("product_id") = 0x0,
            "Initialize the HID subsystem.")

        .def("exit", &SimpleCommKitHid::exit,
            "Shutdown the HID subsystem.")

        // Open by path
        .def("open",
            [](SimpleCommKitHid& self, const std::string& path, bool readable) -> bool {
                return self.open(path, readable);
            },
            py::arg("path"), py::arg("readable") = true,
            "Open a device by path. Set readable=False for write-only mode.")

        // Open by VID/PID/serial
        .def("open",
            [](SimpleCommKitHid& self, unsigned short vendor_id,
               unsigned short product_id, const std::string& serial_number,
               bool readable) -> bool {
                return self.open(vendor_id, product_id, serial_number, readable);
            },
            py::arg("vendor_id"), py::arg("product_id"),
            py::arg("serial_number") = "", py::arg("readable") = true,
            "Open a device by VID, PID and optional serial number.")

        // Close all or by path
        .def("close",
            [](SimpleCommKitHid& self) { self.close(); },
            "Close all open devices.")

        .def("close",
            [](SimpleCommKitHid& self, const std::string& path) { self.close(path); },
            py::arg("path"),
            "Close a specific device by path.")

        // is_Open (all or specific)
        .def("is_open",
            [](SimpleCommKitHid& self) -> bool { return self.is_Open(); },
            "Check if at least one device is open.")

        .def("is_open",
            [](SimpleCommKitHid& self, const std::string& path) -> bool {
                return self.is_Open(path);
            },
            py::arg("path"),
            "Check if a specific device is open.")

        // --- I/O ---
        .def("write",
            [](SimpleCommKitHid& self, const std::string& path, py::bytes data) -> int {
                std::string str = data;
                std::vector<uint8_t> vec(str.begin(), str.end());
                return self.write(path, vec);
            },
            py::arg("path"), py::arg("data"),
            "Write a report to a device by path. Data is bytes. Returns bytes written or -1 on error.")

        .def("write",
            [](SimpleCommKitHid& self, py::bytes data) -> int {
                std::string str = data;
                std::vector<uint8_t> vec(str.begin(), str.end());
                return self.write(vec);
            },
            py::arg("data"),
            "Write a report to the first opened device. Data is bytes. Returns bytes written or -1 on error.")

        .def("send_feature_report",
            [](SimpleCommKitHid& self, const std::string& path, py::bytes data) -> int {
                std::string str = data;
                std::vector<uint8_t> vec(str.begin(), str.end());
                return self.send_Feature_Report(path, vec);
            },
            py::arg("path"), py::arg("data"),
            "Send a feature report to a device by path. Returns bytes sent or -1 on error.")

        .def("send_feature_report",
            [](SimpleCommKitHid& self, py::bytes data) -> int {
                std::string str = data;
                std::vector<uint8_t> vec(str.begin(), str.end());
                return self.send_Feature_Report(vec);
            },
            py::arg("data"),
            "Send a feature report to the first opened device. Returns bytes sent or -1 on error.")

        // --- Hotplug ---
        .def("start_hotplug", &SimpleCommKitHid::start_Hotplug,
            py::arg("vendor_id"), py::arg("product_id"),
            "Start hotplug detection (polling-based).")

        .def("stop_hotplug", &SimpleCommKitHid::stop_Hotplug,
            "Stop hotplug detection.")

        .def("is_hotplug_active", &SimpleCommKitHid::is_Hotplug_Active,
            "Check if hotplug detection is active.")

        .def("set_hotplug_poll_interval", &SimpleCommKitHid::set_Hotplug_Poll_Interval,
            py::arg("ms"),
            "Set hotplug poll interval in milliseconds.")

        .def("get_hotplug_poll_interval", &SimpleCommKitHid::get_Hotplug_Poll_Interval,
            "Get hotplug poll interval in milliseconds.")

        // --- Read poll configuration (global) ---
        .def("set_read_poll_interval",
            [](SimpleCommKitHid& self, int ms) { self.set_Read_Poll_Interval(ms); },
            py::arg("ms"),
            "Set global read poll interval in milliseconds.")

        .def("set_read_poll_interval",
            [](SimpleCommKitHid& self, const std::string& path, int ms) {
                self.set_Read_Poll_Interval(path, ms);
            },
            py::arg("path"), py::arg("ms"),
            "Set per-device read poll interval in milliseconds.")

        .def("get_read_poll_interval",
            [](SimpleCommKitHid& self) -> int { return self.get_Read_Poll_Interval(); },
            "Get global read poll interval in milliseconds.")

        .def("get_read_poll_interval",
            [](SimpleCommKitHid& self, const std::string& path) -> int {
                return self.get_Read_Poll_Interval(path);
            },
            py::arg("path"),
            "Get per-device read poll interval in milliseconds.")

        .def("set_read_data_length",
            [](SimpleCommKitHid& self, int length) { self.set_Read_Data_Length(length); },
            py::arg("length"),
            "Set global read data length (bytes per poll, default 64).")

        .def("set_read_data_length",
            [](SimpleCommKitHid& self, const std::string& path, int length) {
                self.set_Read_Data_Length(path, length);
            },
            py::arg("path"), py::arg("length"),
            "Set per-device read data length (bytes per poll).")

        .def("get_read_data_length",
            [](SimpleCommKitHid& self) -> int { return self.get_Read_Data_Length(); },
            "Get global read data length.")

        .def("get_read_data_length",
            [](SimpleCommKitHid& self, const std::string& path) -> int {
                return self.get_Read_Data_Length(path);
            },
            py::arg("path"),
            "Get per-device read data length.")

        // --- Device list ---
        .def("get_open_paths", &SimpleCommKitHid::get_Open_Paths,
            "Get list of currently open device paths.")

        .def("get_device_list", &SimpleCommKitHid::get_Device_List,
            "Get cached device list (populated by init / start_hotplug).")

        // --- Callbacks ---
        .def("set_callback_on_read",
            [](SimpleCommKitHid& self, py::function callback) {
                auto cb = std::shared_ptr<py::object>(
                    new py::object(std::move(callback)),
                    [](py::object* p) {
                        py::gil_scoped_acquire gil;
                        delete p;
                    });
                self.set_Callback_On_Read(
                    [cb](const SimpleCommKitHidDeviceInfo& device_info,
                          const std::vector<uint8_t>& data) {
                        py::gil_scoped_acquire gil;
                        (*cb)(py::bytes(reinterpret_cast<const char*>(data.data()), data.size()),
                              device_info.path);
                    });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback for incoming read data. Callback receives (bytes, path).")

        .def("set_callback_on_hotplug",
            [](SimpleCommKitHid& self, py::function callback) {
                auto cb = std::shared_ptr<py::object>(
                    new py::object(std::move(callback)),
                    [](py::object* p) {
                        py::gil_scoped_acquire gil;
                        delete p;
                    });
                self.set_Callback_On_HotPlug(
                    [cb](const std::vector<SimpleCommKitHidDeviceInfo>& added,
                         const std::vector<SimpleCommKitHidDeviceInfo>& removed) {
                        py::gil_scoped_acquire gil;
                        (*cb)(added, removed);
                    });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback for hotplug events. Callback receives (added_devices, removed_devices).")

        .def("set_callback_error",
            [](SimpleCommKitHid& self, py::function callback) {
                self.set_Callback_Error([callback](SimpleCommKit::ErrorCode error_code) {
                    py::gil_scoped_acquire gil;
                    callback(static_cast<int>(error_code));
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
PYBIND11_MODULE(_SimpleCommKitPyHid, m) {
    m.doc() = R"pbdoc(
        SimpleCommKitPyHid - Python bindings for SimpleCommKit HID

        Provides a cross-platform HID API for Python.
    )pbdoc";

    // Version / utility
    m.def("get_error_description", &SimpleCommKitErrorMap::GetErrorDescription,
          py::arg("error_code"),
          "Get a human-readable description for an error code.");

    m.def("get_version", &SimpleCommKit::getSimpleCommKitVersion,
        "Get the SimpleCommKit library version string.");

    // Wrap types
    wrap_enums(m);
    wrap_device_info(m);

    // Wrap main class
    wrap_hid(m);
}
