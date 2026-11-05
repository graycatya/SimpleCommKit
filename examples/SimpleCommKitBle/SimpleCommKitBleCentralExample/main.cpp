#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <limits>
#include <mutex>
#include <SimpleCommKitBleCentral.h>
#include "SimpleCommKitErrorMap.hpp"
#include "SimpleCommKitTestUtils.hpp"

using namespace SimpleCommKit;

// Build each message privately, then publish it under one lock. Error and
// connection callbacks share the same lock with normal menu output.
class ConsoleMessage {
    std::ostringstream text;
    std::ostream& destination;
public:
    explicit ConsoleMessage(std::ostream& destination) : destination(destination) {}
    ~ConsoleMessage() {
        static std::mutex mutex;
        std::lock_guard<std::mutex> lock(mutex);
        destination << text.str() << std::flush;
    }
    template<class T> ConsoleMessage& operator<<(const T& value) { text << value; return *this; }
    ConsoleMessage& operator<<(std::ostream& (*manipulator)(std::ostream&)) { manipulator(text); return *this; }
};
ConsoleMessage consoleOut() { return ConsoleMessage(std::cout); }
ConsoleMessage consoleError() { return ConsoleMessage(std::cerr); }

void printSeparator(const std::string& title) {
    consoleOut() << "\n========================================\nTesting: " << title
                 << "\n========================================\n\n";
}

// Consume the entire command line so trailing text cannot become another
// command. EOF/fail must leave the menu instead of reusing an unread value.
bool readChoice(char& choice) {
    if (!(std::cin >> choice)) {
        consoleOut() << "\n[INFO] Input ended; finishing operations and disconnecting." << std::endl;
        return false;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return true;
}

int main() {
    consoleOut() << "=== SimpleCommKitBleCentral ===" << std::endl;
    std::atomic<bool> connected_callback_triggered{false};
    std::atomic<bool> disconnected_callback_triggered{false};
    std::atomic<int> notification_count{0};
    SimpleCommKitBleCentral central;
    central.set_Callback_Error([](SimpleCommKit::ErrorCode error) {
        consoleError() << "[BLE ERROR] " << error << " : "
                  << SimpleCommKit::SimpleCommKitErrorMap::GetErrorDescription(error) << std::endl;
    });
    auto adapters = central.get_Adapters();
    consoleOut() << "Found " << adapters.size() << " adapter(s)" << std::endl;
    for (size_t i = 0; i < adapters.size(); ++i) {
        consoleOut() << "  Adapter " << i << ":" << std::endl;
        consoleOut() << "    Identifier: " << adapters[i].dev_identifier << std::endl;
        consoleOut() << "    Address: " << adapters[i].dev_address << std::endl;
    }

    if(adapters.empty()) {
        consoleError() << "No Bluetooth adapters available; aborting." << std::endl;
        return EXIT_FAILURE;
    }
    auto selection = SimpleCommKitTestUtils::getUserInputInt("Please select a Adapter", adapters.size() - 1);
    if (!selection.has_value()) {
        return EXIT_FAILURE;
    }
    central.set_CurrentAdapter(adapters[selection.value()]);
    central.adapter_Set_Callback_On_Power_On([]() {
        consoleOut() << "[CALLBACK] Adapter powered on" << std::endl;
    });
    central.adapter_Set_Callback_On_Power_Off([]() {
        consoleOut() << "[CALLBACK] Adapter powered off" << std::endl;
    });
    central.adapter_Set_Callback_On_Scan_Start([]() {
        consoleOut() << "[CALLBACK] Scan started" << std::endl;
    });
    central.adapter_Set_Callback_On_Scan_Stop([]() {
        consoleOut() << "[CALLBACK] Scan stopped" << std::endl;
    });
    central.adapter_Set_Callback_On_Scan_Found([](SimpleCommKitBlePeripheral peripheral) {
        consoleOut() << "[CALLBACK] Scan Found - Device: " << peripheral.address
                           << " (" << peripheral.identifier << ") RSSI: " << peripheral.rssi << std::endl;
    });
    central.adapter_Set_Callback_On_Scan_Updated([](SimpleCommKitBlePeripheral peripheral) {
        consoleOut() << "[CALLBACK] Scan updated: " << peripheral.address
                           << " (" << peripheral.identifier << ") RSSI: " << peripheral.rssi << std::endl;
    });
    if(!central.adapter_Is_Powered())
    {
        central.adapter_Power_On();
    }
    if(!central.Bluetooth_Enabled())
    {
        consoleError() << "Bluetooth is not enabled. Please enable Bluetooth and try again." << std::endl;
    }

    central.adapter_Scan_For(15000);

    std::vector<SimpleCommKitBlePeripheral> scan_results =  central.adapter_Get_Scan_Results();
    for (size_t i = 0; i < scan_results.size(); ++i) {
        consoleOut() << "  Device " << i << ":" << std::endl;
        consoleOut() << "    Identifier: " << scan_results[i].identifier << std::endl;
        consoleOut() << "    Address: " << scan_results[i].address << std::endl;

        std::string addr_type_str;
        switch (scan_results[i].address_type) {
            case SimpleCommKitBlePeripheralAddressType::PUBLIC:
                addr_type_str = "PUBLIC";
                break;
            case SimpleCommKitBlePeripheralAddressType::RANDOM:
                addr_type_str = "RANDOM";
                break;
            case SimpleCommKitBlePeripheralAddressType::UNSPECIFIED:
                addr_type_str = "UNSPECIFIED";
                break;
            default:
                addr_type_str = "UNKNOWN";
                break;
        }
        consoleOut() << "    Address Type: " << addr_type_str << std::endl;
        consoleOut() << "    RSSI: " << scan_results[i].rssi << std::endl;
        consoleOut() << "    Manufacturer Data: ";
        if (!scan_results[i].manufacturer.empty()) {
            for (const auto& [company_id, data] : scan_results[i].manufacturer) {
                consoleOut() << "CompanyID: 0x" << std::hex << company_id << std::dec << ", Data: ";
                for (const auto& byte : data) {
                    consoleOut() << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte) << " ";
                }
                consoleOut() << std::dec;
            }
        } else {
            consoleOut() << "None";
        }
        consoleOut() << std::endl;
    }

    if(scan_results.empty()) {
        consoleError() << "No peripherals found; aborting." << std::endl;
        return EXIT_FAILURE;
    }
    selection = SimpleCommKitTestUtils::getUserInputInt("Please select a Peripheral", scan_results.size() - 1);
    if (!selection.has_value()) {
        return EXIT_FAILURE;
    }

    // ============================================
    // Device Connection, Service Discovery, and Characteristic Read/Write
    // ============================================
    printSeparator("Peripheral Connection and Service Discovery");

    // Set the selected peripheral
    central.set_CurrentPeripheral(scan_results[selection.value()]);
    consoleOut() << "Selected peripheral: " << scan_results[selection.value()].identifier
              << " (" << scan_results[selection.value()].address << ")" << std::endl;

    // Set connection callbacks. State above outlives central during shutdown.

    central.peripheral_Set_Callback_On_Connected([&connected_callback_triggered]() {
        consoleOut() << "[CALLBACK] Peripheral Connected" << std::endl;
        connected_callback_triggered.store(true);
    });

    central.peripheral_Set_Callback_On_Disconnected([&disconnected_callback_triggered]() {
        consoleOut() << "[CALLBACK] Peripheral Disconnected" << std::endl;
        disconnected_callback_triggered.store(true);
    });

    // Connect to peripheral
    consoleOut() << "Connecting to peripheral..." << std::endl;
    central.peripheral_Connect();

    // Wait for connection to establish
    consoleOut() << "Waiting for connection..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    // Check connection status
    if (central.peripheral_Is_Connected()) {
        consoleOut() << "[PASS] Peripheral connected successfully" << std::endl;
    } else {
        consoleError() << "[FAIL] Peripheral did not connect; aborting GATT tests." << std::endl;
        return EXIT_FAILURE;
    }

    // Optional connectability/MTU/TX power queries are not required for a GATT
    // session and may be unsupported. Do not print their failure values as metrics.

    // ============================================
    // Service Discovery
    // ============================================
    printSeparator("Service Discovery");

    consoleOut() << "Discovering services..." << std::endl;
    auto services = central.peripheral_Services();

    if (services.empty()) {
        consoleOut() << "[WARN] No services discovered. The device may still be connecting or has no discoverable services." << std::endl;
    } else {
        consoleOut() << "Discovered " << services.size() << " service(s)" << std::endl;

        for (size_t i = 0; i < services.size(); ++i) {
            consoleOut() << "\n  Service " << i << ":" << std::endl;
            consoleOut() << "    UUID: " << services[i].uuid << std::endl;
            consoleOut() << "    Data length: " << services[i].data.size() << " bytes";
            if (!services[i].data.empty()) {
                consoleOut() << " (";
                for (size_t j = 0; j < std::min(services[i].data.size(), size_t(10)); ++j) {
                    consoleOut() << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(services[i].data[j]) << " ";
                }
                if (services[i].data.size() > 10) consoleOut() << "...";
                consoleOut() << std::dec << ")";
            }
            consoleOut() << std::endl;

            consoleOut() << "    Characteristics (" << services[i].characteristics.size() << "):" << std::endl;
            for (size_t j = 0; j < services[i].characteristics.size(); ++j) {
                const auto& ch = services[i].characteristics[j];
                consoleOut() << "      [" << j << "] UUID: " << ch.uuid << std::endl;
                consoleOut() << "           Capabilities: ";
                bool first = true;
                if (ch.can_read) { consoleOut() << "READ"; first = false; }
                if (ch.can_write_request) { consoleOut() << (first ? "" : ", ") << "WRITE-REQUEST"; first = false; }
                if (ch.can_write_command) { consoleOut() << (first ? "" : ", ") << "WRITE-COMMAND"; first = false; }
                if (ch.can_notify) { consoleOut() << (first ? "" : ", ") << "NOTIFY"; first = false; }
                if (ch.can_indicate) { consoleOut() << (first ? "" : ", ") << "INDICATE"; first = false; }
                consoleOut() << std::endl;
                consoleOut() << "           Descriptors: " << ch.descriptors_uuid.size() << std::endl;
            }
        }
    }

    // ============================================
    // Characteristic Read/Write Operations
    // ============================================
    if (!services.empty()) {
        printSeparator("Characteristic Operations");

        // Select a service
        selection = SimpleCommKitTestUtils::getUserInputInt("Select a Service for operations", services.size() - 1);
        if (!selection.has_value()) {
            consoleOut() << "Skipping characteristic operations" << std::endl;
        } else {
            auto& selected_service = services[selection.value()];

            if (selected_service.characteristics.empty()) {
                consoleOut() << "[WARN] No characteristics in selected service" << std::endl;
            } else {
                // Select a characteristic
                auto char_selection = SimpleCommKitTestUtils::getUserInputInt("Select a Characteristic", selected_service.characteristics.size() - 1);
                if (!char_selection.has_value()) {
                    consoleOut() << "Skipping characteristic operations" << std::endl;
                } else {
                    auto& selected_char = selected_service.characteristics[char_selection.value()];
                    std::string service_uuid = selected_service.uuid;
                    std::string char_uuid = selected_char.uuid;

                    consoleOut() << "\nSelected: Service=" << service_uuid << ", Characteristic=" << char_uuid << std::endl;

                    // Show capabilities summary
                    consoleOut() << "  Capabilities: ";
                    bool first_cap = true;
                    if (selected_char.can_read) { consoleOut() << "READ"; first_cap = false; }
                    if (selected_char.can_write_request) { consoleOut() << (first_cap ? "" : ", ") << "WRITE-REQUEST"; first_cap = false; }
                    if (selected_char.can_write_command) { consoleOut() << (first_cap ? "" : ", ") << "WRITE-COMMAND"; first_cap = false; }
                    if (selected_char.can_notify) { consoleOut() << (first_cap ? "" : ", ") << "NOTIFY"; first_cap = false; }
                    if (selected_char.can_indicate) { consoleOut() << (first_cap ? "" : ", ") << "INDICATE"; first_cap = false; }
                    consoleOut() << std::endl;

                    // ============================================
                    // Step 1: Register Notifications/Indications FIRST
                    // ============================================
                    bool subscribed = false;

                    if (selected_char.can_notify || selected_char.can_indicate) {
                        printSeparator("Characteristic Notification/Indication");

                        consoleOut() << "Subscribe to notifications/indications? (y/n): ";
                        char subscribe_choice{};
                        if (readChoice(subscribe_choice) && (subscribe_choice == 'y' || subscribe_choice == 'Y')) {
                            if (selected_char.can_notify) {
                                consoleOut() << "Subscribing to NOTIFY..." << std::endl;
                                central.peripheral_Notify(service_uuid, char_uuid,
                                    [&notification_count](std::vector<uint8_t> payload) {
                                        std::ostringstream message;
                                        message << "[NOTIFY] Received " << payload.size() << " bytes: ";
                                        for (const auto& byte : payload)
                                            message << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte) << " ";
                                        consoleOut() << message.str() << std::endl;
                                        notification_count.fetch_add(1);
                                    });
                                consoleOut() << "[INFO] Notify subscription requested; check BLE errors" << std::endl;
                            }

                            if (selected_char.can_indicate) {
                                consoleOut() << "Subscribing to INDICATE..." << std::endl;
                                central.peripheral_Indicate(service_uuid, char_uuid,
                                    [&notification_count](std::vector<uint8_t> payload) {
                                        std::ostringstream message;
                                        message << "[INDICATE] Received " << payload.size() << " bytes: ";
                                        for (const auto& byte : payload)
                                            message << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte) << " ";
                                        consoleOut() << message.str() << std::endl;
                                        notification_count.fetch_add(1);
                                    });
                                consoleOut() << "[INFO] Indicate subscription requested; check BLE errors" << std::endl;
                            }
                            subscribed = true;
                        }
                    } else {
                        consoleOut() << "[INFO] Characteristic does not support NOTIFY/INDICATE" << std::endl;
                    }

                    // ============================================
                    // Step 2: Interactive Read / Write Operations
                    // ============================================
                    bool running = true;
                    while (running && std::cin) {
                        if (disconnected_callback_triggered.load() || !central.peripheral_Is_Connected()) {
                            consoleOut() << "[WARN] Peripheral disconnected; leaving characteristic operations." << std::endl;
                            break;
                        }
                        printSeparator("Read / Write Operations");
                        consoleOut() << "Choose operation:" << std::endl;
                        if (selected_char.can_read) {
                            consoleOut() << "  (r) Read characteristic value" << std::endl;
                        }
                        if (selected_char.can_write_request || selected_char.can_write_command) {
                            consoleOut() << "  (w) Write characteristic value" << std::endl;
                        }
                        consoleOut() << "  (q) Quit to next step" << std::endl;
                        if (subscribed) {
                            consoleOut() << "  Notifications received so far: " << notification_count.load() << std::endl;
                        }
                        consoleOut() << "Enter choice: ";
                        char op_choice{};
                        if (!readChoice(op_choice)) break;
                        if (disconnected_callback_triggered.load() || !central.peripheral_Is_Connected()) {
                            consoleOut() << "[WARN] Peripheral disconnected; command skipped." << std::endl;
                            break;
                        }

                        if (op_choice == 'r' && selected_char.can_read) {
                            // Read operation
                            consoleOut() << "Reading characteristic value..." << std::endl;
                            try {
                                auto read_data = central.peripheral_Read(service_uuid, char_uuid);
                                if (disconnected_callback_triggered.load() || !central.peripheral_Is_Connected()) {
                                    consoleOut() << "[WARN] Peripheral disconnected during read; no successful result confirmed." << std::endl;
                                    break;
                                }
                                if (read_data.empty()) {
                                    consoleOut() << "[INFO] No data returned. The value may be empty or the read may have failed; check BLE errors." << std::endl;
                                } else {
                                    std::ostringstream message;
                                    message << "Read " << read_data.size() << " bytes: ";
                                    for (const auto byte : read_data)
                                        message << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte) << " ";
                                    consoleOut() << message.str() << std::endl;
                                }
                            } catch (const std::exception& e) {
                                consoleOut() << "[ERROR] Read failed: " << e.what() << std::endl;
                            }
                        } else if (op_choice == 'w' && (selected_char.can_write_request || selected_char.can_write_command)) {
                            // Prompt user for hex data input
                            consoleOut() << "Enter hex data to write (e.g., AA BB CC DD): ";
                            std::string hex_input;
                            if (!std::getline(std::cin, hex_input)) {
                                consoleOut() << "\n[INFO] Input ended; skipping write and disconnecting." << std::endl;
                                break;
                            }

                            std::vector<uint8_t> write_data;
                            std::istringstream hex_stream(hex_input);
                            std::string byte_str;
                            while (hex_stream >> byte_str) {
                                try {
                                    uint8_t byte_val = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
                                    write_data.push_back(byte_val);
                                } catch (...) {
                                    consoleOut() << "[WARN] Invalid hex byte: " << byte_str << ", skipping" << std::endl;
                                }
                            }

                            if (write_data.empty()) {
                                consoleOut() << "[WARN] No valid data entered, write skipped" << std::endl;
                                continue;
                            }

                            // Print data being written
                            consoleOut() << "Writing " << write_data.size() << " bytes: ";
                            for (const auto& byte : write_data) {
                                consoleOut() << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte) << " ";
                            }
                            consoleOut() << std::dec << std::endl;

                            // Choose write method if both supported
                            if (selected_char.can_write_request && selected_char.can_write_command) {
                                consoleOut() << "Choose write method: (1) WRITE-REQUEST (with response), (2) WRITE-COMMAND (without response): ";
                                char write_choice{};
                                if (!readChoice(write_choice)) break;

                                if (write_choice == '1') {
                                    try {
                                        central.peripheral_Write_Request(service_uuid, char_uuid, write_data);
                                        consoleOut() << "[INFO] Write Request submitted; check BLE errors" << std::endl;
                                    } catch (const std::exception& e) {
                                        consoleOut() << "[ERROR] Write Request failed: " << e.what() << std::endl;
                                    }
                                } else if (write_choice == '2') {
                                    try {
                                        central.peripheral_Write_Command(service_uuid, char_uuid, write_data);
                                        consoleOut() << "[INFO] Write Command submitted; check BLE errors" << std::endl;
                                    } catch (const std::exception& e) {
                                        consoleOut() << "[ERROR] Write Command failed: " << e.what() << std::endl;
                                    }
                                } else {
                                    consoleOut() << "[WARN] Invalid choice, write skipped" << std::endl;
                                }
                            } else if (selected_char.can_write_request) {
                                try {
                                    central.peripheral_Write_Request(service_uuid, char_uuid, write_data);
                                    consoleOut() << "[INFO] Write Request submitted; check BLE errors" << std::endl;
                                } catch (const std::exception& e) {
                                    consoleOut() << "[ERROR] Write Request failed: " << e.what() << std::endl;
                                }
                            } else if (selected_char.can_write_command) {
                                try {
                                    central.peripheral_Write_Command(service_uuid, char_uuid, write_data);
                                    consoleOut() << "[INFO] Write Command submitted; check BLE errors" << std::endl;
                                } catch (const std::exception& e) {
                                    consoleOut() << "[ERROR] Write Command failed: " << e.what() << std::endl;
                                }
                            }
                        } else if (op_choice == 'q') {
                            running = false;
                        } else {
                            consoleOut() << "[WARN] Invalid choice or operation not supported" << std::endl;
                        }
                    }

                    // ============================================
                    // Step 3: Unsubscribe
                    // ============================================
                    if (subscribed && !disconnected_callback_triggered.load() && central.peripheral_Is_Connected()) {
                        consoleOut() << "\nUnsubscribing..." << std::endl;
                        central.peripheral_Unsubscribe(service_uuid, char_uuid);
                        consoleOut() << "[INFO] Unsubscribe requested. Total notifications received: " << notification_count.load() << std::endl;
                    }

                    // ============================================
                    // Step 4: Descriptor Operations
                    // ============================================
                    if (std::cin && !disconnected_callback_triggered.load() && central.peripheral_Is_Connected()
                        && !selected_char.descriptors_uuid.empty()) {
                        printSeparator("Descriptor Operations");
                        consoleOut() << "This characteristic has " << selected_char.descriptors_uuid.size()
                                  << " descriptor(s)" << std::endl;

                        auto desc_selection = SimpleCommKitTestUtils::getUserInputInt(
                            "Select a Descriptor (or skip)", selected_char.descriptors_uuid.size() - 1);

                        if (desc_selection.has_value()) {
                            std::string desc_uuid = selected_char.descriptors_uuid[desc_selection.value()];
                            consoleOut() << "Selected descriptor: " << desc_uuid << std::endl;

                            // Write descriptor
                            printSeparator("Descriptor Write");
                            consoleOut() << "Enter hex data to write to descriptor (e.g., 01 00): ";
                            std::string desc_hex_input;
                            std::getline(std::cin, desc_hex_input);

                            std::vector<uint8_t> desc_write_data;
                            std::istringstream desc_hex_stream(desc_hex_input);
                            std::string desc_byte_str;
                            while (desc_hex_stream >> desc_byte_str) {
                                try {
                                    uint8_t byte_val = static_cast<uint8_t>(std::stoul(desc_byte_str, nullptr, 16));
                                    desc_write_data.push_back(byte_val);
                                } catch (...) {
                                    consoleOut() << "[WARN] Invalid hex byte: " << desc_byte_str << ", skipping" << std::endl;
                                }
                            }

                            if (desc_write_data.empty()) {
                                consoleOut() << "[WARN] No valid data entered, descriptor write skipped" << std::endl;
                            } else {
                                consoleOut() << "Writing " << desc_write_data.size() << " bytes to descriptor..." << std::endl;
                                try {
                                    central.peripheral_Write(service_uuid, char_uuid, desc_uuid, desc_write_data);
                                    consoleOut() << "[INFO] Descriptor write submitted; check BLE errors" << std::endl;
                                } catch (const std::exception& e) {
                                    consoleOut() << "[ERROR] Descriptor write failed: " << e.what() << std::endl;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // ============================================
    // Disconnect
    // ============================================
    printSeparator("Disconnect");
    if (central.peripheral_Is_Connected()) {
        consoleOut() << "Disconnecting peripheral..." << std::endl;
        central.peripheral_Disconnect();
        consoleOut() << "[PASS] Disconnect request sent" << std::endl;

        // Wait for disconnection
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        if (!central.peripheral_Is_Connected()) {
            consoleOut() << "[PASS] Peripheral disconnected successfully" << std::endl;
        }
    } else {
        consoleOut() << "[INFO] Peripheral already disconnected" << std::endl;
    }

    consoleOut() << "\n=== All Connection/Service/Characteristic Tests Completed ===" << std::endl;

    return 0;
}
