#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include <memory>
#include <string>
#include <vector>

#include <SimpleCommKitUsb.h>
#include <SimpleCommKitErrorMap.hpp>
#include <SimpleCommKitVersion.h>

namespace py = pybind11;
using namespace SimpleCommKit;

// ============================================================================
// UsbDeviceInfo struct
// ============================================================================
void wrap_device_info(py::module& m) {
    py::class_<SimpleCommKitUsbDeviceInfo>(m, "UsbDeviceInfo",
        "Represents a USB device's identification information.")
        .def(py::init<>())
        .def_readwrite("vendor_id",          &SimpleCommKitUsbDeviceInfo::vendor_id,
            "USB vendor ID")
        .def_readwrite("product_id",         &SimpleCommKitUsbDeviceInfo::product_id,
            "USB product ID")
        .def_readwrite("manufacturer_string",&SimpleCommKitUsbDeviceInfo::manufacturer_string,
            "Manufacturer string")
        .def_readwrite("product_string",     &SimpleCommKitUsbDeviceInfo::product_string,
            "Product string")
        .def_readwrite("serial_number",      &SimpleCommKitUsbDeviceInfo::serial_number,
            "Serial number string")
        .def_readwrite("bus_number",         &SimpleCommKitUsbDeviceInfo::bus_number,
            "USB bus number")
        .def_readwrite("device_address",     &SimpleCommKitUsbDeviceInfo::device_address,
            "USB device address")
        .def_readwrite("path",               &SimpleCommKitUsbDeviceInfo::path,
            "Unique device path (format: 'bus:address')")
        .def("__repr__", [](const SimpleCommKitUsbDeviceInfo& d) {
            return "<UsbDeviceInfo path='" + d.path +
                   "' vid=0x" + std::to_string(d.vendor_id) +
                   " pid=0x" + std::to_string(d.product_id) +
                   " product='" + d.product_string + "'>";
        });
}

// ============================================================================
// IsochronousPacketResult struct
// ============================================================================
void wrap_iso_packet_result(py::module& m) {
    py::class_<SimpleCommKitUsbIsoPacketResult>(m, "IsochronousPacketResult",
        "Per-packet result for an isochronous transfer.")
        .def(py::init<>())
        .def_readwrite("length",         &SimpleCommKitUsbIsoPacketResult::length,
            "Requested length for this packet")
        .def_readwrite("actual_length",  &SimpleCommKitUsbIsoPacketResult::actual_length,
            "Actual transferred bytes")
        .def_readwrite("status",         &SimpleCommKitUsbIsoPacketResult::status,
            "0=completed, or LIBUSB_TRANSFER_* error code")
        .def("__repr__", [](const SimpleCommKitUsbIsoPacketResult& p) {
            return "<IsochronousPacketResult req=" + std::to_string(p.length) +
                   " actual=" + std::to_string(p.actual_length) +
                   " status=" + std::to_string(p.status) + ">";
        });
}

// ============================================================================
// SimpleCommKitUsb class wrapping (single-device mode)
// ============================================================================
void wrap_usb(py::module& m) {
    py::class_<SimpleCommKitUsb>(m, "SimpleCommKitUsb",
        "Cross-platform USB manager (single-device mode).")
        .def(py::init<>(), "Create a new SimpleCommKitUsb instance.")

        // --- Static: enumerate ---
        .def_static("get_available_devices", &SimpleCommKitUsb::get_Available_Devices,
            py::arg("vendor_id")  = 0x0,
            py::arg("product_id") = 0x0,
            "Enumerate available USB devices, optionally filtered by VID/PID.")

        // --- Lifecycle ---
        .def("init", &SimpleCommKitUsb::init,
            "Initialize the USB subsystem (libusb).")
        .def("exit", &SimpleCommKitUsb::exit,
            "Shutdown the USB subsystem.")

        // Open by path
        .def("open",
            [](SimpleCommKitUsb& self, const std::string& path) -> bool {
                return self.open(path);
            },
            py::arg("path"),
            "Open a device by path (format: 'bus:address').")

        // Open by VID/PID/serial
        .def("open",
            [](SimpleCommKitUsb& self, unsigned short vendor_id,
               unsigned short product_id, const std::string& serial_number) -> bool {
                return self.open(vendor_id, product_id, serial_number);
            },
            py::arg("vendor_id"), py::arg("product_id"),
            py::arg("serial_number") = "",
            "Open a device by VID, PID and optional serial number.")

        // Close
        .def("close",
            [](SimpleCommKitUsb& self) { self.close(); },
            "Close the open device.")

        .def("is_open",
            [](SimpleCommKitUsb& self) -> bool { return self.is_Open(); },
            "Check if a device is open.")

        // --- Claim / release interface ---
        .def("claim_interface",
            [](SimpleCommKitUsb& self, int iface) -> bool {
                return self.claim_Interface(iface);
            },
            py::arg("interface_number"),
            "Claim a USB interface.")

        .def("release_interface",
            [](SimpleCommKitUsb& self, int iface) -> bool {
                return self.release_Interface(iface);
            },
            py::arg("interface_number"),
            "Release a USB interface.")

        // --- Control transfer ---
        .def("control_transfer",
            [](SimpleCommKitUsb& self,
               uint8_t bmReqType, uint8_t bReq,
               uint16_t wVal, uint16_t wIdx,
               py::bytes data, unsigned int timeout) -> py::bytes {
                std::string str = data;
                std::vector<uint8_t> vec(str.begin(), str.end());
                int ret = self.control_Transfer(bmReqType, bReq, wVal, wIdx, vec, timeout);
                if (ret < 0) return py::bytes();
                return py::bytes(reinterpret_cast<const char*>(vec.data()),
                                 static_cast<size_t>(std::max(ret, 0)));
            },
            py::arg("bm_request_type"), py::arg("b_request"),
            py::arg("w_value"), py::arg("w_index"),
            py::arg("data"), py::arg("timeout") = 1000,
            "Perform a USB control transfer. For IN transfers, set bm_request_type bit 7. "
            "Returns received data (empty on error).")

        // --- Bulk transfer ---
        .def("bulk_transfer",
            [](SimpleCommKitUsb& self,
               uint8_t endpoint, py::object data_or_length, unsigned int timeout) -> py::object {
                bool is_in = (endpoint & 0x80) != 0;
                std::vector<uint8_t> vec;
                if (is_in) {
                    int length = py::cast<int>(data_or_length);
                    vec.resize(static_cast<size_t>(length));
                } else {
                    py::bytes py_data = py::cast<py::bytes>(data_or_length);
                    std::string str = py_data;
                    vec.assign(str.begin(), str.end());
                }
                int ret = self.bulk_Transfer(endpoint, vec, timeout);
                if (ret < 0) {
                    return is_in ? py::bytes() : py::int_(ret);
                }
                if (is_in) {
                    return py::bytes(reinterpret_cast<const char*>(vec.data()),
                                     static_cast<size_t>(ret));
                }
                return py::int_(ret);
            },
            py::arg("endpoint"),
            py::arg("data_or_length"),
            py::arg("timeout") = 1000,
            "Bulk transfer. Direction auto-detected from endpoint. "
            "For OUT (bit7=0): pass data as bytes, returns bytes transferred. "
            "For IN (bit7=1): pass length as int, returns received bytes.")

        // --- Interrupt transfer ---
        .def("interrupt_transfer",
            [](SimpleCommKitUsb& self,
               uint8_t endpoint, py::object data_or_length, unsigned int timeout) -> py::object {
                bool is_in = (endpoint & 0x80) != 0;
                std::vector<uint8_t> vec;
                if (is_in) {
                    int length = py::cast<int>(data_or_length);
                    vec.resize(static_cast<size_t>(length));
                } else {
                    py::bytes py_data = py::cast<py::bytes>(data_or_length);
                    std::string str = py_data;
                    vec.assign(str.begin(), str.end());
                }
                int ret = self.interrupt_Transfer(endpoint, vec, timeout);
                if (ret < 0) {
                    return is_in ? py::bytes() : py::int_(ret);
                }
                if (is_in) {
                    return py::bytes(reinterpret_cast<const char*>(vec.data()),
                                     static_cast<size_t>(ret));
                }
                return py::int_(ret);
            },
            py::arg("endpoint"),
            py::arg("data_or_length"),
            py::arg("timeout") = 1000,
            "Interrupt transfer. Direction auto-detected from endpoint. "
            "For OUT (bit7=0): pass data as bytes. "
            "For IN (bit7=1): pass length as int, returns received bytes.")

        // --- Isochronous transfer ---
        .def("isochronous_transfer",
            [](SimpleCommKitUsb& self,
               uint8_t endpoint, py::object data_or_length,
               int num_packets, const std::vector<int>& packet_lengths,
               unsigned int timeout) -> py::object {
                bool is_in = (endpoint & 0x80) != 0;
                std::vector<uint8_t> vec;
                std::vector<SimpleCommKitUsbIsoPacketResult> results;

                if (is_in) {
                    int total = py::cast<int>(data_or_length);
                    vec.resize(static_cast<size_t>(total));
                } else {
                    py::bytes py_data = py::cast<py::bytes>(data_or_length);
                    std::string str = py_data;
                    vec.assign(str.begin(), str.end());
                }

                int ret = self.isochronous_Transfer(endpoint, vec, num_packets,
                                                      packet_lengths, results, timeout);
                if (is_in && ret > 0) {
                    py::bytes data = py::bytes(
                        reinterpret_cast<const char*>(vec.data()),
                        static_cast<size_t>(ret));
                    return py::make_tuple(ret, data, results);
                }
                return py::make_tuple(ret, py::bytes(), results);
            },
            py::arg("endpoint"),
            py::arg("data_or_length"),
            py::arg("num_packets"), py::arg("packet_lengths"),
            py::arg("timeout") = 1000,
            "Isochronous transfer. Direction auto-detected from endpoint. "
            "For OUT: pass data as bytes. For IN: pass buffer size as int. "
            "Returns (total_bytes, received_data, list_of_packet_results).")

        // --- Read polling (single device) ---
        .def("start_read_poll",
            [](SimpleCommKitUsb& self, uint8_t endpoint) {
                self.start_Read_Poll(endpoint);
            },
            py::arg("endpoint"),
            "Start continuous read polling on an endpoint. Data arrives via callback.")

        .def("stop_read_poll",
            [](SimpleCommKitUsb& self) { self.stop_Read_Poll(); },
            "Stop continuous read polling.")

        .def("is_read_poll_active",
            [](SimpleCommKitUsb& self) { return self.is_Read_Poll_Active(); },
            "Check if continuous read poll is active.")

        // --- Hotplug ---
        .def("start_hotplug", &SimpleCommKitUsb::start_Hotplug,
            py::arg("vendor_id")  = 0,
            py::arg("product_id") = 0,
            "Start USB hotplug detection. Uses native libusb callbacks when available, "
            "falls back to polling otherwise.")

        .def("stop_hotplug", &SimpleCommKitUsb::stop_Hotplug,
            "Stop hotplug detection.")

        .def("is_hotplug_active", &SimpleCommKitUsb::is_Hotplug_Active,
            "Check if hotplug detection is active.")

        .def("set_hotplug_poll_interval", &SimpleCommKitUsb::set_Hotplug_Poll_Interval,
            py::arg("ms"),
            "Set hotplug poll interval in milliseconds (for polling fallback mode).")

        .def("get_hotplug_poll_interval", &SimpleCommKitUsb::get_Hotplug_Poll_Interval,
            "Get hotplug poll interval in milliseconds.")

        // --- Read poll configuration ---
        .def("set_read_poll_interval", &SimpleCommKitUsb::set_Read_Poll_Interval,
            py::arg("ms"),
            "Set global read poll interval in milliseconds.")

        .def("get_read_poll_interval", &SimpleCommKitUsb::get_Read_Poll_Interval,
            "Get global read poll interval in milliseconds.")

        .def("set_read_data_length", &SimpleCommKitUsb::set_Read_Data_Length,
            py::arg("length"),
            "Set global read data length (bytes per poll, default 64).")

        .def("get_read_data_length", &SimpleCommKitUsb::get_Read_Data_Length,
            "Get global read data length.")

        // --- Device info ---
        .def("get_open_path", &SimpleCommKitUsb::get_Open_Path,
            "Get the path of the currently open device.")

        .def("is_open_device", &SimpleCommKitUsb::is_Open_Device,
            "Check if a device is currently open.")

        .def("get_device_list", &SimpleCommKitUsb::get_Device_List,
            "Get cached device list (populated by init / start_hotplug).")

        // --- Callbacks ---
        .def("set_callback_on_read",
            [](SimpleCommKitUsb& self, py::function callback) {
                auto cb = std::shared_ptr<py::object>(
                    new py::object(std::move(callback)),
                    [](py::object* p) {
                        py::gil_scoped_acquire gil;
                        delete p;
                    });
                self.set_Callback_On_Read(
                    [cb](const SimpleCommKitUsbDeviceInfo& device_info,
                          const std::vector<uint8_t>& data) {
                        py::gil_scoped_acquire gil;
                        (*cb)(device_info,
                              py::bytes(reinterpret_cast<const char*>(data.data()), data.size()));
                    });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback for incoming read data. Callback receives (UsbDeviceInfo, bytes).")

        .def("set_callback_on_hotplug",
            [](SimpleCommKitUsb& self, py::function callback) {
                auto cb = std::shared_ptr<py::object>(
                    new py::object(std::move(callback)),
                    [](py::object* p) {
                        py::gil_scoped_acquire gil;
                        delete p;
                    });
                self.set_Callback_On_HotPlug(
                    [cb](const std::vector<SimpleCommKitUsbDeviceInfo>& added,
                         const std::vector<SimpleCommKitUsbDeviceInfo>& removed) {
                        py::gil_scoped_acquire gil;
                        (*cb)(added, removed);
                    });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback for hotplug events. Callback receives (added_devices, removed_devices).")

        .def("set_callback_error",
            [](SimpleCommKitUsb& self, py::function callback) {
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
PYBIND11_MODULE(_SimpleCommKitPyUsb, m) {
    m.doc() = R"pbdoc(
        SimpleCommKitPyUsb - Python bindings for SimpleCommKit USB

        Provides a cross-platform USB API for Python using libusb (single-device mode).
        Supports control, bulk, interrupt, and isochronous transfers.
    )pbdoc";

    m.def("get_error_description", &SimpleCommKitErrorMap::GetErrorDescription,
          py::arg("error_code"),
          "Get a human-readable description for an error code.");

    m.def("get_version", []() {
        return SIMPLECOMMKIT_VERSION;
    }, "Get the SimpleCommKit library version string.");

    wrap_device_info(m);
    wrap_iso_packet_result(m);
    wrap_usb(m);
}
