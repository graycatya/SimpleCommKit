#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <SimpleCommKitBleCentral.h>
#include "SimpleCommKitErrorMap.hpp"
#include "SimpleCommKitTestUtils.hpp"

using namespace SimpleCommKit;

void printSeparator(const std::string& title) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Testing: " << title << std::endl;
    std::cout << "========================================\n" << std::endl;
}

int main() {
    std::cout << "=== SimpleCommKitBleCentral ===" << std::endl;
    SimpleCommKitBleCentral central;
    auto adapters = central.get_Adapters();
    std::cout << "Found " << adapters.size() << " adapter(s)" << std::endl;
    for (size_t i = 0; i < adapters.size(); ++i) {
        std::cout << "  Adapter " << i << ":" << std::endl;
        std::cout << "    Identifier: " << adapters[i].dev_identifier << std::endl;
        std::cout << "    Address: " << adapters[i].dev_address << std::endl;
    }

    auto selection = SimpleCommKitTestUtils::getUserInputInt("Please select a Adapter", adapters.size() - 1);
    if (!selection.has_value()) {
        return EXIT_FAILURE;
    }
    central.set_CurrentAdapter(adapters[selection.value()]);
    central.set_Callback_Error([](SimpleCommKit::ErrorCode error) {
        std::cout << "[CALLBACK] Error triggered: " << static_cast<int>(error) << " : " << SimpleCommKit::SimpleCommKitErrorMap::GetErrorDescription(error)  << std::endl;
    });
    central.adapter_Set_Callback_On_Power_On([]() {
        std::cout << "[CALLBACK] Adapter powered on" << std::endl;
    });
    central.adapter_Set_Callback_On_Power_Off([]() {
        std::cout << "[CALLBACK] Adapter powered off" << std::endl;
    });
    central.adapter_Set_Callback_On_Scan_Start([]() {
        std::cout << "[CALLBACK] Scan started" << std::endl;
    });
    central.adapter_Set_Callback_On_Scan_Stop([]() {
        std::cout << "[CALLBACK] Scan stopped" << std::endl;
    });
    central.adapter_Set_Callback_On_Scan_Found([](SimpleCommKitBlePeripheral peripheral) {
        std::cout << "[CALLBACK] Scan Found - Device: " << peripheral.address
                           << " (" << peripheral.identifier << ") RSSI: " << peripheral.rssi << std::endl;
    });
    central.adapter_Set_Callback_On_Scan_Updated([](SimpleCommKitBlePeripheral peripheral) {
        std::cout << "[CALLBACK] Scan updated: " << peripheral.address
                           << " (" << peripheral.identifier << ") RSSI: " << peripheral.rssi << std::endl;
    });
    if(!central.adapter_Is_Powered())
    {
        central.adapter_Power_On();
    }
    if(!central.Bluetooth_Enabled())
    {
        std::cerr << "Bluetooth is not enabled. Please enable Bluetooth and try again." << std::endl;
    }

    central.adapter_Scan_For(15000);

    std::vector<SimpleCommKitBlePeripheral> scan_results =  central.adapter_Get_Scan_Results();
    for (size_t i = 0; i < scan_results.size(); ++i) {
        std::cout << "  Device " << i << ":" << std::endl;
        std::cout << "    Identifier: " << scan_results[i].identifier << std::endl;
        std::cout << "    Address: " << scan_results[i].address << std::endl;

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
        std::cout << "    Address Type: " << addr_type_str << std::endl;
        std::cout << "    RSSI: " << scan_results[i].rssi << std::endl;
        std::cout << "    Manufacturer Data: ";
        if (!scan_results[i].manufacturer.empty()) {
            for (const auto& [company_id, data] : scan_results[i].manufacturer) {
                std::cout << "CompanyID: 0x" << std::hex << company_id << std::dec << ", Data: ";
                for (const auto& byte : data) {
                    std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte) << " ";
                }
                std::cout << std::dec;
            }
        } else {
            std::cout << "None";
        }
        std::cout << std::endl;
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
    std::cout << "Selected peripheral: " << scan_results[selection.value()].identifier
              << " (" << scan_results[selection.value()].address << ")" << std::endl;

    // Set connection callbacks
    std::atomic<bool> connected_callback_triggered{false};
    std::atomic<bool> disconnected_callback_triggered{false};

    central.peripheral_Set_Callback_On_Connected([&connected_callback_triggered]() {
        std::cout << "[CALLBACK] Peripheral Connected" << std::endl;
        connected_callback_triggered.store(true);
    });

    central.peripheral_Set_Callback_On_Disconnected([&disconnected_callback_triggered]() {
        std::cout << "[CALLBACK] Peripheral Disconnected" << std::endl;
        disconnected_callback_triggered.store(true);
    });

    // Connect to peripheral
    std::cout << "Connecting to peripheral..." << std::endl;
    central.peripheral_Connect();

    // Wait for connection to establish
    std::cout << "Waiting for connection..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    // Check connection status
    if (central.peripheral_Is_Connected()) {
        std::cout << "[PASS] Peripheral connected successfully" << std::endl;
    } else {
        std::cout << "[WARN] Peripheral connection may have failed or is still in progress" << std::endl;
        std::cout << "       Connection callback triggered: " << (connected_callback_triggered.load() ? "YES" : "NO") << std::endl;
    }

    // Check connectable status
    bool is_connectable = central.peripheral_Is_Connectable();
    std::cout << "Peripheral is connectable: " << (is_connectable ? "YES" : "NO") << std::endl;

    // Get TX Power and MTU
    int16_t tx_power = central.peripheral_Get_Tx_Power();
    std::cout << "TX Power: " << tx_power << " dBm" << std::endl;

    uint16_t mtu = central.peripheral_Get_Mtu();
    std::cout << "MTU: " << mtu << " bytes" << std::endl;

    // ============================================
    // Service Discovery
    // ============================================
    printSeparator("Service Discovery");

    std::cout << "Discovering services..." << std::endl;
    auto services = central.peripheral_Services();

    if (services.empty()) {
        std::cout << "[WARN] No services discovered. The device may still be connecting or has no discoverable services." << std::endl;
    } else {
        std::cout << "Discovered " << services.size() << " service(s)" << std::endl;

        for (size_t i = 0; i < services.size(); ++i) {
            std::cout << "\n  Service " << i << ":" << std::endl;
            std::cout << "    UUID: " << services[i].uuid << std::endl;
            std::cout << "    Data length: " << services[i].data.size() << " bytes";
            if (!services[i].data.empty()) {
                std::cout << " (";
                for (size_t j = 0; j < std::min(services[i].data.size(), size_t(10)); ++j) {
                    std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(services[i].data[j]) << " ";
                }
                if (services[i].data.size() > 10) std::cout << "...";
                std::cout << std::dec << ")";
            }
            std::cout << std::endl;

            std::cout << "    Characteristics (" << services[i].characteristics.size() << "):" << std::endl;
            for (size_t j = 0; j < services[i].characteristics.size(); ++j) {
                const auto& ch = services[i].characteristics[j];
                std::cout << "      [" << j << "] UUID: " << ch.uuid << std::endl;
                std::cout << "           Capabilities: ";
                bool first = true;
                if (ch.can_read) { std::cout << "READ"; first = false; }
                if (ch.can_write_request) { std::cout << (first ? "" : ", ") << "WRITE-REQUEST"; first = false; }
                if (ch.can_write_command) { std::cout << (first ? "" : ", ") << "WRITE-COMMAND"; first = false; }
                if (ch.can_notify) { std::cout << (first ? "" : ", ") << "NOTIFY"; first = false; }
                if (ch.can_indicate) { std::cout << (first ? "" : ", ") << "INDICATE"; first = false; }
                std::cout << std::endl;
                std::cout << "           Descriptors: " << ch.descriptors_uuid.size() << std::endl;
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
            std::cout << "Skipping characteristic operations" << std::endl;
        } else {
            auto& selected_service = services[selection.value()];

            if (selected_service.characteristics.empty()) {
                std::cout << "[WARN] No characteristics in selected service" << std::endl;
            } else {
                // Select a characteristic
                auto char_selection = SimpleCommKitTestUtils::getUserInputInt("Select a Characteristic", selected_service.characteristics.size() - 1);
                if (!char_selection.has_value()) {
                    std::cout << "Skipping characteristic operations" << std::endl;
                } else {
                    auto& selected_char = selected_service.characteristics[char_selection.value()];
                    std::string service_uuid = selected_service.uuid;
                    std::string char_uuid = selected_char.uuid;

                    std::cout << "\nSelected: Service=" << service_uuid << ", Characteristic=" << char_uuid << std::endl;

                    // Show capabilities summary
                    std::cout << "  Capabilities: ";
                    bool first_cap = true;
                    if (selected_char.can_read) { std::cout << "READ"; first_cap = false; }
                    if (selected_char.can_write_request) { std::cout << (first_cap ? "" : ", ") << "WRITE-REQUEST"; first_cap = false; }
                    if (selected_char.can_write_command) { std::cout << (first_cap ? "" : ", ") << "WRITE-COMMAND"; first_cap = false; }
                    if (selected_char.can_notify) { std::cout << (first_cap ? "" : ", ") << "NOTIFY"; first_cap = false; }
                    if (selected_char.can_indicate) { std::cout << (first_cap ? "" : ", ") << "INDICATE"; first_cap = false; }
                    std::cout << std::endl;

                    // ============================================
                    // Step 1: Register Notifications/Indications FIRST
                    // ============================================
                    std::atomic<int> notification_count{0};
                    bool subscribed = false;

                    if (selected_char.can_notify || selected_char.can_indicate) {
                        printSeparator("Characteristic Notification/Indication");

                        std::cout << "Subscribe to notifications/indications? (y/n): ";
                        char subscribe_choice;
                        std::cin >> subscribe_choice;
                        std::cin.ignore();

                        if (subscribe_choice == 'y' || subscribe_choice == 'Y') {
                            if (selected_char.can_notify) {
                                std::cout << "Subscribing to NOTIFY..." << std::endl;
                                central.peripheral_Notify(service_uuid, char_uuid,
                                    [&notification_count](std::vector<uint8_t> payload) {
                                        std::cout << "[NOTIFY] Received " << payload.size() << " bytes: ";
                                        for (const auto& byte : payload) {
                                            std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte) << " ";
                                        }
                                        std::cout << std::dec << std::endl;
                                        notification_count.fetch_add(1);
                                    });
                                std::cout << "[PASS] Notify subscription set up" << std::endl;
                            }

                            if (selected_char.can_indicate) {
                                std::cout << "Subscribing to INDICATE..." << std::endl;
                                central.peripheral_Indicate(service_uuid, char_uuid,
                                    [&notification_count](std::vector<uint8_t> payload) {
                                        std::cout << "[INDICATE] Received " << payload.size() << " bytes: ";
                                        for (const auto& byte : payload) {
                                            std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte) << " ";
                                        }
                                        std::cout << std::dec << std::endl;
                                        notification_count.fetch_add(1);
                                    });
                                std::cout << "[PASS] Indicate subscription set up" << std::endl;
                            }
                            subscribed = true;
                        }
                    } else {
                        std::cout << "[INFO] Characteristic does not support NOTIFY/INDICATE" << std::endl;
                    }

                    // ============================================
                    // Step 2: Interactive Read / Write Operations
                    // ============================================
                    bool running = true;
                    while (running) {
                        printSeparator("Read / Write Operations");
                        std::cout << "Choose operation:" << std::endl;
                        if (selected_char.can_read) {
                            std::cout << "  (r) Read characteristic value" << std::endl;
                        }
                        if (selected_char.can_write_request || selected_char.can_write_command) {
                            std::cout << "  (w) Write characteristic value" << std::endl;
                        }
                        std::cout << "  (q) Quit to next step" << std::endl;
                        if (subscribed) {
                            std::cout << "  Notifications received so far: " << notification_count.load() << std::endl;
                        }
                        std::cout << "Enter choice: ";
                        char op_choice;
                        std::cin >> op_choice;
                        std::cin.ignore();

                        if (op_choice == 'r' && selected_char.can_read) {
                            // Read operation
                            std::cout << "Reading characteristic value..." << std::endl;
                            try {
                                auto read_data = central.peripheral_Read(service_uuid, char_uuid);
                                std::cout << "Read " << read_data.size() << " bytes: ";
                                for (const auto& byte : read_data) {
                                    std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte) << " ";
                                }
                                std::cout << std::dec << std::endl;
                                std::cout << "[PASS] Read operation completed" << std::endl;
                            } catch (const std::exception& e) {
                                std::cout << "[ERROR] Read failed: " << e.what() << std::endl;
                            }
                        } else if (op_choice == 'w' && (selected_char.can_write_request || selected_char.can_write_command)) {
                            // Prompt user for hex data input
                            std::cout << "Enter hex data to write (e.g., AA BB CC DD): ";
                            std::string hex_input;
                            std::getline(std::cin, hex_input);

                            std::vector<uint8_t> write_data;
                            std::istringstream hex_stream(hex_input);
                            std::string byte_str;
                            while (hex_stream >> byte_str) {
                                try {
                                    uint8_t byte_val = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
                                    write_data.push_back(byte_val);
                                } catch (...) {
                                    std::cout << "[WARN] Invalid hex byte: " << byte_str << ", skipping" << std::endl;
                                }
                            }

                            if (write_data.empty()) {
                                std::cout << "[WARN] No valid data entered, write skipped" << std::endl;
                                continue;
                            }

                            // Print data being written
                            std::cout << "Writing " << write_data.size() << " bytes: ";
                            for (const auto& byte : write_data) {
                                std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte) << " ";
                            }
                            std::cout << std::dec << std::endl;

                            // Choose write method if both supported
                            if (selected_char.can_write_request && selected_char.can_write_command) {
                                std::cout << "Choose write method: (1) WRITE-REQUEST (with response), (2) WRITE-COMMAND (without response): ";
                                char write_choice;
                                std::cin >> write_choice;
                                std::cin.ignore();

                                if (write_choice == '1') {
                                    try {
                                        central.peripheral_Write_Request(service_uuid, char_uuid, write_data);
                                        std::cout << "[PASS] Write Request completed" << std::endl;
                                    } catch (const std::exception& e) {
                                        std::cout << "[ERROR] Write Request failed: " << e.what() << std::endl;
                                    }
                                } else if (write_choice == '2') {
                                    try {
                                        central.peripheral_Write_Command(service_uuid, char_uuid, write_data);
                                        std::cout << "[PASS] Write Command completed" << std::endl;
                                    } catch (const std::exception& e) {
                                        std::cout << "[ERROR] Write Command failed: " << e.what() << std::endl;
                                    }
                                } else {
                                    std::cout << "[WARN] Invalid choice, write skipped" << std::endl;
                                }
                            } else if (selected_char.can_write_request) {
                                try {
                                    central.peripheral_Write_Request(service_uuid, char_uuid, write_data);
                                    std::cout << "[PASS] Write Request completed" << std::endl;
                                } catch (const std::exception& e) {
                                    std::cout << "[ERROR] Write Request failed: " << e.what() << std::endl;
                                }
                            } else if (selected_char.can_write_command) {
                                try {
                                    central.peripheral_Write_Command(service_uuid, char_uuid, write_data);
                                    std::cout << "[PASS] Write Command completed" << std::endl;
                                } catch (const std::exception& e) {
                                    std::cout << "[ERROR] Write Command failed: " << e.what() << std::endl;
                                }
                            }
                        } else if (op_choice == 'q') {
                            running = false;
                        } else {
                            std::cout << "[WARN] Invalid choice or operation not supported" << std::endl;
                        }
                    }

                    // ============================================
                    // Step 3: Unsubscribe
                    // ============================================
                    if (subscribed) {
                        std::cout << "\nUnsubscribing..." << std::endl;
                        central.peripheral_Unsubscribe(service_uuid, char_uuid);
                        std::cout << "[PASS] Unsubscribed. Total notifications received: " << notification_count.load() << std::endl;
                    }

                    // ============================================
                    // Step 4: Descriptor Operations
                    // ============================================
                    if (!selected_char.descriptors_uuid.empty()) {
                        printSeparator("Descriptor Operations");
                        std::cout << "This characteristic has " << selected_char.descriptors_uuid.size()
                                  << " descriptor(s)" << std::endl;

                        auto desc_selection = SimpleCommKitTestUtils::getUserInputInt(
                            "Select a Descriptor (or skip)", selected_char.descriptors_uuid.size() - 1);

                        if (desc_selection.has_value()) {
                            std::string desc_uuid = selected_char.descriptors_uuid[desc_selection.value()];
                            std::cout << "Selected descriptor: " << desc_uuid << std::endl;

                            // Write descriptor
                            printSeparator("Descriptor Write");
                            std::cout << "Enter hex data to write to descriptor (e.g., 01 00): ";
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
                                    std::cout << "[WARN] Invalid hex byte: " << desc_byte_str << ", skipping" << std::endl;
                                }
                            }

                            if (desc_write_data.empty()) {
                                std::cout << "[WARN] No valid data entered, descriptor write skipped" << std::endl;
                            } else {
                                std::cout << "Writing " << desc_write_data.size() << " bytes to descriptor..." << std::endl;
                                try {
                                    central.peripheral_Write(service_uuid, char_uuid, desc_uuid, desc_write_data);
                                    std::cout << "[PASS] Descriptor write completed" << std::endl;
                                } catch (const std::exception& e) {
                                    std::cout << "[ERROR] Descriptor write failed: " << e.what() << std::endl;
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
        std::cout << "Disconnecting peripheral..." << std::endl;
        central.peripheral_Disconnect();
        std::cout << "[PASS] Disconnect request sent" << std::endl;

        // Wait for disconnection
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        if (!central.peripheral_Is_Connected()) {
            std::cout << "[PASS] Peripheral disconnected successfully" << std::endl;
        }
    } else {
        std::cout << "[INFO] Peripheral already disconnected" << std::endl;
    }

    std::cout << "\n=== All Connection/Service/Characteristic Tests Completed ===" << std::endl;

    return 0;
}
