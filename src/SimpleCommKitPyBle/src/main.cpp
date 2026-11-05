#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include <memory>

#include <SimpleCommKitBleCentral.h>
#include <SimpleCommKitErrorMap.hpp>
#include <SimpleCommKitVersion.h>

namespace py = pybind11;
using namespace SimpleCommKit;

namespace {
// Backend queues may copy/destroy callbacks on any thread. Only the shared
// holder crosses threads; Python reference changes always hold the GIL.
std::shared_ptr<py::object> holdCallback(py::function callback) {
    return {new py::object(std::move(callback)), [](py::object* object) {
        py::gil_scoped_acquire gil;
        delete object;
    }};
}
struct BleCentralDeleter {
    void operator()(SimpleCommKitBleCentral* central) const {
        // A callback may be waiting for the GIL while shutdown joins its thread.
        if (PyGILState_Check()) {
            py::gil_scoped_release release;
            delete central;
        } else {
            delete central;
        }
    }
};
} // namespace


// ============================================================================
// PeripheralAddressType enum
// ============================================================================
void wrap_enums(py::module& m) {
    py::enum_<SimpleCommKitBlePeripheralAddressType>(m, "PeripheralAddressType",
        "Bluetooth address type enumeration.")
        .value("PUBLIC", SimpleCommKitBlePeripheralAddressType::PUBLIC)
        .value("RANDOM", SimpleCommKitBlePeripheralAddressType::RANDOM)
        .value("UNSPECIFIED", SimpleCommKitBlePeripheralAddressType::UNSPECIFIED)
        .export_values();
}

// ============================================================================
// Adapter struct
// ============================================================================
void wrap_adapter_struct(py::module& m) {
    py::class_<SimpleCommKitBleAdapter>(m, "Adapter",
        "Represents a Bluetooth adapter.")
        .def(py::init<>())
        .def_readwrite("identifier", &SimpleCommKitBleAdapter::dev_identifier,
            "Identifier of the adapter")
        .def_readwrite("address", &SimpleCommKitBleAdapter::dev_address,
            "MAC address of the adapter")
        .def("__repr__", [](const SimpleCommKitBleAdapter& a) {
            return "<Adapter identifier='" + a.dev_identifier +
                   "' address='" + a.dev_address + "'>";
        });
}

// ============================================================================
// Peripheral struct
// ============================================================================
void wrap_peripheral_struct(py::module& m) {
    py::class_<SimpleCommKitBlePeripheral>(m, "Peripheral",
        "Represents a BLE peripheral device.")
        .def(py::init<>())
        .def_readwrite("identifier", &SimpleCommKitBlePeripheral::identifier,
            "Identifier of the peripheral")
        .def_readwrite("address", &SimpleCommKitBlePeripheral::address,
            "Address of the peripheral")
        .def_readwrite("address_type", &SimpleCommKitBlePeripheral::address_type,
            "Address type (PUBLIC/RANDOM/UNSPECIFIED)")
        .def_readwrite("rssi", &SimpleCommKitBlePeripheral::rssi,
            "RSSI signal strength in dBm")
        .def_property_readonly("manufacturer_data",
            [](const SimpleCommKitBlePeripheral& p) -> py::dict {
                py::dict result;
                for (const auto& [company_id, data] : p.manufacturer) {
                    result[py::int_(company_id)] = py::bytes(
                        reinterpret_cast<const char*>(data.data()), data.size());
                }
                return result;
            },
            "Manufacturer data as dict of {company_id: bytes}")
        .def("__repr__", [](const SimpleCommKitBlePeripheral& p) {
            return "<Peripheral identifier='" + p.identifier +
                   "' address='" + p.address +
                   "' rssi=" + std::to_string(p.rssi) + ">";
        });
}

// ============================================================================
// Characteristic struct
// ============================================================================
void wrap_characteristic_struct(py::module& m) {
    py::class_<SimpleCommKitBleCharacteristic>(m, "Characteristic",
        "Represents a GATT characteristic.")
        .def(py::init<>())
        .def_readwrite("uuid", &SimpleCommKitBleCharacteristic::uuid,
            "UUID of the characteristic")
        .def_readwrite("descriptors", &SimpleCommKitBleCharacteristic::descriptors_uuid,
            "List of descriptor UUIDs")
        .def_readwrite("capabilities", &SimpleCommKitBleCharacteristic::capabilities,
            "List of capability strings")
        .def_readwrite("can_read", &SimpleCommKitBleCharacteristic::can_read,
            "Whether the characteristic supports read")
        .def_readwrite("can_write_request", &SimpleCommKitBleCharacteristic::can_write_request,
            "Whether the characteristic supports write with response")
        .def_readwrite("can_write_command", &SimpleCommKitBleCharacteristic::can_write_command,
            "Whether the characteristic supports write without response")
        .def_readwrite("can_notify", &SimpleCommKitBleCharacteristic::can_notify,
            "Whether the characteristic supports notifications")
        .def_readwrite("can_indicate", &SimpleCommKitBleCharacteristic::can_indicate,
            "Whether the characteristic supports indications")
        .def("__repr__", [](const SimpleCommKitBleCharacteristic& ch) {
            return "<Characteristic uuid='" + ch.uuid + "'>";
        });
}

// ============================================================================
// Service struct
// ============================================================================
void wrap_service_struct(py::module& m) {
    py::class_<SimpleCommKitBleService>(m, "Service",
        "Represents a GATT service.")
        .def(py::init<>())
        .def_readwrite("uuid", &SimpleCommKitBleService::uuid,
            "UUID of the service")
        .def_property_readonly("data_bytes",
            [](const SimpleCommKitBleService& s) -> py::bytes {
                return py::bytes(
                    reinterpret_cast<const char*>(s.data.data()), s.data.size());
            },
            "Advertised service data as bytes")
        .def_readwrite("characteristics", &SimpleCommKitBleService::characteristics,
            "List of characteristics in this service")
        .def("__repr__", [](const SimpleCommKitBleService& s) {
            return "<Service uuid='" + s.uuid +
                   "' characteristics=" + std::to_string(s.characteristics.size()) + ">";
        });
}

// ============================================================================
// BleCentral class wrapping SimpleCommKitBleCentral
// ============================================================================
void wrap_ble_central(py::module& m) {
    py::class_<SimpleCommKitBleCentral, std::unique_ptr<SimpleCommKitBleCentral, BleCentralDeleter>>(m, "BleCentral",
        "Central BLE manager for scanning, connecting, and interacting with BLE devices.")

        .def(py::init<>(), "Create a new BleCentral instance.")

        // --- Static / Adapter discovery ---
        .def_static("bluetooth_enabled", &SimpleCommKitBleCentral::Bluetooth_Enabled,
            py::call_guard<py::gil_scoped_release>(),
            "Check if Bluetooth is enabled on the system.")

        .def("get_adapters", &SimpleCommKitBleCentral::get_Adapters,
            py::call_guard<py::gil_scoped_release>(),
            "Get a list of available Bluetooth adapters.")

        .def("get_current_adapter", &SimpleCommKitBleCentral::get_CurrentAdapter,
            py::call_guard<py::gil_scoped_release>(),
            "Get the currently selected adapter (or None).")

        .def("set_current_adapter", &SimpleCommKitBleCentral::set_CurrentAdapter,
            py::call_guard<py::gil_scoped_release>(),
            py::arg("adapter"),
            "Set the current adapter by providing an Adapter object.")

        // --- Adapter power ---
        .def("adapter_power_on", &SimpleCommKitBleCentral::adapter_Power_On,
            py::call_guard<py::gil_scoped_release>(),
            "Power on the selected adapter.")
        .def("adapter_power_off", &SimpleCommKitBleCentral::adapter_Power_Off,
            py::call_guard<py::gil_scoped_release>(),
            "Power off the selected adapter.")
        .def("adapter_is_powered", &SimpleCommKitBleCentral::adapter_Is_Powered,
            py::call_guard<py::gil_scoped_release>(),
            "Check if the selected adapter is powered on.")

        // --- Adapter callbacks ---
        .def("adapter_set_callback_on_power_on",
            [](SimpleCommKitBleCentral& self, py::function callback) {
                self.adapter_Set_Callback_On_Power_On([callback = holdCallback(std::move(callback))]() {
                    py::gil_scoped_acquire gil;
                    (*callback)();
                });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback invoked when adapter powers on.")
        .def("adapter_set_callback_on_power_off",
            [](SimpleCommKitBleCentral& self, py::function callback) {
                self.adapter_Set_Callback_On_Power_Off([callback = holdCallback(std::move(callback))]() {
                    py::gil_scoped_acquire gil;
                    (*callback)();
                });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback invoked when adapter powers off.")
        .def("adapter_set_callback_on_scan_start",
            [](SimpleCommKitBleCentral& self, py::function callback) {
                self.adapter_Set_Callback_On_Scan_Start([callback = holdCallback(std::move(callback))]() {
                    py::gil_scoped_acquire gil;
                    (*callback)();
                });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback invoked when scanning starts.")
        .def("adapter_set_callback_on_scan_stop",
            [](SimpleCommKitBleCentral& self, py::function callback) {
                self.adapter_Set_Callback_On_Scan_Stop([callback = holdCallback(std::move(callback))]() {
                    py::gil_scoped_acquire gil;
                    (*callback)();
                });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback invoked when scanning stops.")
        .def("adapter_set_callback_on_scan_found",
            [](SimpleCommKitBleCentral& self, py::function callback) {
                self.adapter_Set_Callback_On_Scan_Found(
                    [callback = holdCallback(std::move(callback))](SimpleCommKitBlePeripheral p) {
                        py::gil_scoped_acquire gil;
                        (*callback)(p);
                    });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback invoked when a peripheral is found during scanning.")
        .def("adapter_set_callback_on_scan_updated",
            [](SimpleCommKitBleCentral& self, py::function callback) {
                self.adapter_Set_Callback_On_Scan_Updated(
                    [callback = holdCallback(std::move(callback))](SimpleCommKitBlePeripheral p) {
                        py::gil_scoped_acquire gil;
                        (*callback)(p);
                    });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback invoked when a peripheral's information is updated during scanning.")

        // --- Scanning ---
        .def("adapter_scan_start", &SimpleCommKitBleCentral::adapter_Scan_Start,
            py::call_guard<py::gil_scoped_release>(),
            "Start scanning for BLE peripherals.")
        .def("adapter_scan_stop", &SimpleCommKitBleCentral::adapter_Scan_Stop,
            py::call_guard<py::gil_scoped_release>(),
            "Stop scanning for BLE peripherals.")
        .def("adapter_scan_for", &SimpleCommKitBleCentral::adapter_Scan_For,
            py::call_guard<py::gil_scoped_release>(),
            py::arg("timeout_ms"),
            "Scan for peripherals for the specified duration (ms).")
        .def("adapter_scan_is_active", &SimpleCommKitBleCentral::adapter_Scan_Is_Active,
            py::call_guard<py::gil_scoped_release>(),
            "Check if currently scanning.")
        .def("adapter_initialized", &SimpleCommKitBleCentral::adapter_Initialized,
            py::call_guard<py::gil_scoped_release>(),
            "Check if the adapter is initialized.")

        // --- Scan results ---
        .def("adapter_get_scan_results", &SimpleCommKitBleCentral::adapter_Get_Scan_Results,
            py::call_guard<py::gil_scoped_release>(),
            "Get peripherals discovered in the last scan.")
        .def("adapter_get_paired_peripherals",
            &SimpleCommKitBleCentral::adapter_Get_Paired_Peripherals,
            py::call_guard<py::gil_scoped_release>(),
            "Get all paired peripherals.")
        .def("adapter_get_connected_peripherals",
            &SimpleCommKitBleCentral::adapter_Get_Connected_Peripherals,
            py::call_guard<py::gil_scoped_release>(),
            "Get all currently connected peripherals.")

        // --- Peripheral selection ---
        .def("set_current_peripheral", &SimpleCommKitBleCentral::set_CurrentPeripheral,
            py::call_guard<py::gil_scoped_release>(),
            py::arg("peripheral"),
            "Select a peripheral for subsequent operations.")

        // --- Peripheral info ---
        .def("peripheral_get_tx_power", &SimpleCommKitBleCentral::peripheral_Get_Tx_Power,
            py::call_guard<py::gil_scoped_release>(),
            "Get the TX power of the current peripheral (dBm).")
        .def("peripheral_get_mtu", &SimpleCommKitBleCentral::peripheral_Get_Mtu,
            py::call_guard<py::gil_scoped_release>(),
            "Get the negotiated MTU of the current peripheral.")

        // --- Connection ---
        .def("peripheral_connect", &SimpleCommKitBleCentral::peripheral_Connect,
            py::call_guard<py::gil_scoped_release>(),
            "Connect to the current peripheral.")
        .def("peripheral_disconnect", &SimpleCommKitBleCentral::peripheral_Disconnect,
            py::call_guard<py::gil_scoped_release>(),
            "Disconnect from the current peripheral.")
        .def("peripheral_is_connected", &SimpleCommKitBleCentral::peripheral_Is_Connected,
            py::call_guard<py::gil_scoped_release>(),
            "Check if the current peripheral is connected.")
        .def("peripheral_is_connectable", &SimpleCommKitBleCentral::peripheral_Is_Connectable,
            py::call_guard<py::gil_scoped_release>(),
            "Check if the current peripheral is connectable.")
        .def("peripheral_is_paired", &SimpleCommKitBleCentral::peripheral_Is_Paired,
            py::call_guard<py::gil_scoped_release>(),
            "Check if the current peripheral is paired.")
        .def("peripheral_unpair", &SimpleCommKitBleCentral::peripheral_Unpair,
            py::call_guard<py::gil_scoped_release>(),
            "Unpair the current peripheral.")

        // --- Connection callbacks ---
        .def("peripheral_set_callback_on_connected",
            [](SimpleCommKitBleCentral& self, py::function callback) {
                self.peripheral_Set_Callback_On_Connected([callback = holdCallback(std::move(callback))]() {
                    py::gil_scoped_acquire gil;
                    (*callback)();
                });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback invoked when the peripheral connects.")
        .def("peripheral_set_callback_on_disconnected",
            [](SimpleCommKitBleCentral& self, py::function callback) {
                self.peripheral_Set_Callback_On_Disconnected([callback = holdCallback(std::move(callback))]() {
                    py::gil_scoped_acquire gil;
                    (*callback)();
                });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set callback invoked when the peripheral disconnects.")

        // --- Services ---
        .def("peripheral_services", &SimpleCommKitBleCentral::peripheral_Services,
            py::call_guard<py::gil_scoped_release>(),
            "Get all GATT services of the current peripheral.")

        // --- GATT: Read ---
        .def("peripheral_read",
            [](SimpleCommKitBleCentral& self, const std::string& service_uuid,
               const std::string& char_uuid) -> py::bytes {
                std::vector<uint8_t> data;
                { py::gil_scoped_release release; data = self.peripheral_Read(service_uuid, char_uuid); }
                return py::bytes(reinterpret_cast<const char*>(data.data()), data.size());
            },
            py::arg("service_uuid"), py::arg("char_uuid"),
            "Read a characteristic value. Returns bytes.")

        // --- GATT: Write ---
        .def("peripheral_write_request",
            [](SimpleCommKitBleCentral& self, const std::string& service_uuid,
               const std::string& char_uuid, py::bytes data) {
                std::string str = data;
                std::vector<uint8_t> vec(str.begin(), str.end());
                py::gil_scoped_release release;
                self.peripheral_Write_Request(service_uuid, char_uuid, vec);
            },
            py::arg("service_uuid"), py::arg("char_uuid"), py::arg("data"),
            "Write data to a characteristic with response. Data is bytes.")

        .def("peripheral_write_command",
            [](SimpleCommKitBleCentral& self, const std::string& service_uuid,
               const std::string& char_uuid, py::bytes data) {
                std::string str = data;
                std::vector<uint8_t> vec(str.begin(), str.end());
                py::gil_scoped_release release;
                self.peripheral_Write_Command(service_uuid, char_uuid, vec);
            },
            py::arg("service_uuid"), py::arg("char_uuid"), py::arg("data"),
            "Write data to a characteristic without response. Data is bytes.")

        // --- GATT: Notify / Indicate ---
        // NOTE(thread-safety): BLE backends may copy/destroy the callback
        // std::function from background BLE threads.  To avoid GIL errors
        // (inc_ref/dec_ref without the GIL), we wrap the py::object in a
        // shared_ptr whose custom deleter acquires the GIL before destruction.
        .def("peripheral_notify",
            [](SimpleCommKitBleCentral& self, const std::string& service_uuid,
               const std::string& char_uuid, py::function callback) {
                auto cb = holdCallback(std::move(callback));
                py::gil_scoped_release release;
                self.peripheral_Notify(service_uuid, char_uuid,
                    [cb](std::vector<uint8_t> payload) {
                        py::gil_scoped_acquire gil;
                        (*cb)(py::bytes(reinterpret_cast<const char*>(payload.data()), payload.size()));
                    });
            },
            py::arg("service_uuid"), py::arg("char_uuid"), py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Subscribe to notifications on a characteristic. "
            "Callback receives bytes payload.")

        .def("peripheral_indicate",
            [](SimpleCommKitBleCentral& self, const std::string& service_uuid,
               const std::string& char_uuid, py::function callback) {
                auto cb = holdCallback(std::move(callback));
                py::gil_scoped_release release;
                self.peripheral_Indicate(service_uuid, char_uuid,
                    [cb](std::vector<uint8_t> payload) {
                        py::gil_scoped_acquire gil;
                        (*cb)(py::bytes(reinterpret_cast<const char*>(payload.data()), payload.size()));
                    });
            },
            py::arg("service_uuid"), py::arg("char_uuid"), py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Subscribe to indications on a characteristic. "
            "Callback receives bytes payload.")

        .def("peripheral_unsubscribe",
            &SimpleCommKitBleCentral::peripheral_Unsubscribe,
            py::call_guard<py::gil_scoped_release>(),
            py::arg("service_uuid"), py::arg("char_uuid"),
            "Unsubscribe from notifications/indications on a characteristic.")

        // --- GATT: Descriptor Read ---
        .def("peripheral_read_descriptor",
            [](SimpleCommKitBleCentral& self, const std::string& service_uuid,
               const std::string& char_uuid, const std::string& descriptor_uuid) -> py::bytes {
                std::vector<uint8_t> data;
                { py::gil_scoped_release release; data = self.peripheral_Read(service_uuid, char_uuid, descriptor_uuid); }
                return py::bytes(reinterpret_cast<const char*>(data.data()), data.size());
            },
            py::arg("service_uuid"), py::arg("char_uuid"), py::arg("descriptor_uuid"),
            "Read a descriptor value. Returns bytes.")

        // --- GATT: Descriptor Write ---
        .def("peripheral_write_descriptor",
            [](SimpleCommKitBleCentral& self, const std::string& service_uuid,
               const std::string& char_uuid, const std::string& descriptor_uuid,
               py::bytes data) {
                std::string str = data;
                std::vector<uint8_t> vec(str.begin(), str.end());
                py::gil_scoped_release release;
                self.peripheral_Write(service_uuid, char_uuid, descriptor_uuid, vec);
            },
            py::arg("service_uuid"), py::arg("char_uuid"), py::arg("descriptor_uuid"),
            py::arg("data"),
            "Write data to a descriptor. Data is bytes.")

        // --- Error handling ---
        .def("set_callback_error",
            [](SimpleCommKitBleCentral& self, py::function callback) {
                self.set_Callback_Error([callback = holdCallback(std::move(callback))](int error_code) {
                    py::gil_scoped_acquire gil;
                    (*callback)(error_code);
                });
            },
            py::arg("callback"),
            py::keep_alive<1, 2>(),
            "Set a callback to receive error notifications. "
            "Callback receives error_code (int) and you can use "
            "get_error_description(error_code) to get a human-readable string.");
}

// ============================================================================
// Module entry point
// ============================================================================
PYBIND11_MODULE(_SimpleCommKitPyBle, m) {
    m.doc() = R"pbdoc(
        SimpleCommKitPyBle - Python bindings for SimpleCommKit BLE

        Provides a cross-platform BLE API for Python.
    )pbdoc";

    // Version / utility
    m.def("get_error_description", &SimpleCommKitErrorMap::GetErrorDescription,
          py::arg("error_code"),
          "Get a human-readable description for an error code.");

    m.def("get_version", &SimpleCommKit::getSimpleCommKitVersion,
        "Get the SimpleCommKit library version string.");

    // Wrap types
    wrap_enums(m);
    wrap_adapter_struct(m);
    wrap_peripheral_struct(m);
    wrap_characteristic_struct(m);
    wrap_service_struct(m);

    // Wrap main class
    wrap_ble_central(m);
}
