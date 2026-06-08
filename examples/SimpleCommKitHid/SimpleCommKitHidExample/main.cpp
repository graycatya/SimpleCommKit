#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include <SimpleCommKitHid.h>
#include "SimpleCommKitErrorMap.hpp"

namespace hid = SimpleCommKit;

void printSeparator(const std::string& title) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Testing: " << title << std::endl;
    std::cout << "========================================\n" << std::endl;
}

std::string busTypeToString(int busType) {
    switch (static_cast<hid::SimpleCommKitHidBusType>(busType)) {
    case hid::HID_BUS_TYPE_USB:       return "USB";
    case hid::HID_BUS_TYPE_BLUETOOTH: return "Bluetooth";
    case hid::HID_BUS_TYPE_I2C:       return "I2C";
    case hid::HID_BUS_TYPE_SPI:       return "SPI";
    default:                          return "UNKNOWN";
    }
}

void printDeviceList(const std::vector<hid::SimpleCommKitHidDeviceInfo>& devices) {
    if (devices.empty()) {
        std::cout << "  (no devices)" << std::endl;
        return;
    }
    for (size_t i = 0; i < devices.size(); ++i) {
        std::cout << "  [" << i << "] " << devices[i].path << std::endl;
        std::cout << "       Manufacturer: " << devices[i].manufacturer_string << std::endl;
        std::cout << "       Product:      " << devices[i].product_string << std::endl;
        std::cout << "       Serial:       " << devices[i].serial_number << std::endl;
        std::cout << "       Bus:          " << busTypeToString(devices[i].bus_type) << std::endl;
        std::cout << "       Release:      0x"
                  << std::hex << devices[i].release_number << std::dec << std::endl;
        std::cout << "       Interface:    " << devices[i].interface_number << std::endl;
        std::cout << std::endl;
    }
}

int main() {
    std::cout << "=== SimpleCommKitHid Interactive Demo ===" << std::endl;

    hid::SimpleCommKitHid hidDevice;

    // Register error callback
    hidDevice.set_Callback_Error([](SimpleCommKit::ErrorCode error) {
        std::cout << "[CALLBACK] Error triggered: 0x"
                  << std::hex << static_cast<int>(error) << std::dec
                  << " : " << SimpleCommKit::SimpleCommKitErrorMap::GetErrorDescription(error)
                  << std::endl;
    });

    // ============================================
    // Initial device enumeration
    // ============================================
    printSeparator("Enumerate Available HID Devices");

    auto initialDevices = hid::SimpleCommKitHid::get_Available_Devices();

    if (initialDevices.empty()) {
        std::cout << "[INFO] No HID devices found on this system." << std::endl;
        std::cout << "[INFO] Connect a HID device and try again." << std::endl;

        std::cout << "\nTry filtering by VID/PID?" << std::endl;
        std::cout << "Enter VID (hex, or 0 to skip): ";
        unsigned short vid = 0;
        std::cin >> std::hex >> vid >> std::dec;

        if (vid != 0) {
            std::cout << "Enter PID (hex): ";
            unsigned short pid = 0;
            std::cin >> std::hex >> pid >> std::dec;

            initialDevices = hid::SimpleCommKitHid::get_Available_Devices(vid, pid);
            if (initialDevices.empty()) {
                std::cout << "[INFO] Still no devices found. Exiting." << std::endl;
                return 0;
            }
        } else {
            return 0;
        }
    }

    std::cout << "Found " << initialDevices.size() << " HID device(s):" << std::endl;
    printDeviceList(initialDevices);

    // ============================================
    // Init + Hotplug (optional) — start before device selection
    // ============================================
    printSeparator("Hotplug Detection");

    std::cout << "Enable hotplug monitoring? (y/n, default: y): ";
    char hotplugChoice = 'y';
    std::cin >> hotplugChoice;
    bool enableHotplug = (hotplugChoice != 'n' && hotplugChoice != 'N');

    if (enableHotplug) {
        hidDevice.init();

        hidDevice.set_Callback_On_HotPlug(
            [](const std::vector<hid::SimpleCommKitHidDeviceInfo>& added,
               const std::vector<hid::SimpleCommKitHidDeviceInfo>& removed) {
                std::cout << "\n[CALLBACK] Hotplug event:" << std::endl;
                if (!added.empty()) {
                    std::cout << "  Added " << added.size() << " device(s):" << std::endl;
                    for (const auto& d : added) {
                        std::cout << "    + " << d.path << " (" << d.product_string << ")" << std::endl;
                    }
                }
                if (!removed.empty()) {
                    std::cout << "  Removed " << removed.size() << " device(s):" << std::endl;
                    for (const auto& d : removed) {
                        std::cout << "    - " << d.path << " (" << d.product_string << ")" << std::endl;
                    }
                }
            });

        std::cout << "Hotplug poll interval (ms, default 1000): ";
        int hotplugPollMs = 1000;
        std::cin >> hotplugPollMs;
        if (std::cin.fail() || hotplugPollMs <= 0) {
            std::cin.clear();
            hotplugPollMs = 1000;
        }
        hidDevice.set_Hotplug_Poll_Interval(hotplugPollMs);

        hidDevice.start_Hotplug(0x0, 0x0);
        std::cout << "[INFO] Hotplug monitoring started (polling every "
                  << hotplugPollMs << " ms)" << std::endl;
    }

    // ============================================
    // Ready — go directly to interactive mode
    // ============================================
    if (!enableHotplug) {
        hidDevice.init();
    }

    // Register read callback (shared across all open devices)
    hidDevice.set_Callback_On_Read([](const hid::SimpleCommKitHidDeviceInfo& devInfo,
                                       const std::vector<uint8_t>& data) {
        std::cout << "\n[CALLBACK] Received from " << devInfo.path;
        if (!devInfo.product_string.empty()) {
            std::cout << " (" << devInfo.product_string << ")";
        }
        std::cout << ", " << data.size() << " byte(s): ";
        for (const auto& byte : data) {
            std::cout << std::hex << std::setfill('0') << std::setw(2)
                      << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl;
    });

    // ============================================
    // Interactive Write Loop
    // ============================================
    printSeparator("Interactive Mode");

    // Helper lambda to print the current open-device list
    auto printOpenDevices = [&]() {
        auto openPaths = hidDevice.get_Open_Paths();
        if (openPaths.empty()) {
            std::cout << "(no devices open)" << std::endl;
        } else {
            std::cout << "Open devices (" << openPaths.size() << "):" << std::endl;
            for (size_t i = 0; i < openPaths.size(); ++i) {
                std::cout << "  [" << i << "] " << openPaths[i] << std::endl;
            }
        }
    };

    std::cout << "Commands:" << std::endl;
    std::cout << "  w <devIdx> <hex>      - Write report to device <devIdx> (e.g. w 0 01 02 AA)" << std::endl;
    std::cout << "  wf <devIdx> <hex>     - Send feature report to device <devIdx> (e.g. wf 1 05)" << std::endl;
    std::cout << "  open <path> [r]       - Open a device (append 'r' for readable)" << std::endl;
    std::cout << "  close <devIdx>        - Close device <devIdx>" << std::endl;
    std::cout << "  scan                  - Scan available HID devices" << std::endl;
    std::cout << "  devs                  - List open devices" << std::endl;
    std::cout << "  info                  - Show device & read config" << std::endl;
    std::cout << "  poll                  - Show global read poll interval (ms)" << std::endl;
    std::cout << "  poll <ms>             - Set global read poll interval" << std::endl;
    std::cout << "  poll <devIdx> <ms>    - Set per-device read poll interval" << std::endl;
    std::cout << "  dlen                  - Show global read data length" << std::endl;
    std::cout << "  dlen <len>            - Set global read data length" << std::endl;
    std::cout << "  dlen <devIdx> <len>   - Set per-device read data length" << std::endl;
    std::cout << "  q                     - Quit" << std::endl;
    std::cout << "------------------------------------------------" << std::endl;

    printOpenDevices();
    std::cout << "------------------------------------------------" << std::endl;

    std::cin.ignore();
    std::string line;
    while (true) {
        std::cout << "\n> ";
        std::getline(std::cin, line);

        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "q" || cmd == "quit") {
            std::cout << "Exiting interactive mode..." << std::endl;
            break;
        }
        else if (cmd == "open") {
            std::string openPath;
            iss >> openPath;
            if (openPath.empty()) {
                std::cout << "Usage: open <path> [r]" << std::endl;
                continue;
            }
            std::string flag;
            iss >> flag;
            bool openReadable = (flag == "r" || flag == "R");

            std::cout << "Opening " << openPath
                      << (openReadable ? " (readable)" : " (write-only)") << "..." << std::endl;
            if (hidDevice.open(openPath, openReadable)) {
                std::cout << "[PASS] " << openPath << " opened" << std::endl;
                printOpenDevices();
            } else {
                std::cout << "[FAIL] Failed to open " << openPath << std::endl;
            }
        }
        else if (cmd == "close") {
            int devIdx = -1;
            iss >> devIdx;
            if (devIdx < 0) {
                std::cout << "Usage: close <devIdx>" << std::endl;
                continue;
            }
            auto openPaths = hidDevice.get_Open_Paths();
            if (static_cast<size_t>(devIdx) >= openPaths.size()) {
                std::cout << "[ERROR] Invalid device index." << std::endl;
                continue;
            }
            std::string targetPath = openPaths[static_cast<size_t>(devIdx)];
            std::cout << "Closing " << targetPath << "..." << std::endl;
            hidDevice.close(targetPath);
            std::cout << "[PASS] " << targetPath << " closed" << std::endl;
            printOpenDevices();
        }
        else if (cmd == "w" || cmd == "wf") {
            int devIdx = -1;
            iss >> devIdx;
            if (devIdx < 0) {
                std::cout << "Usage: " << cmd << " <devIdx> <hex bytes...>" << std::endl;
                continue;
            }

            auto openPaths = hidDevice.get_Open_Paths();
            if (static_cast<size_t>(devIdx) >= openPaths.size()) {
                std::cout << "[ERROR] Invalid device index. Open devices: 0-"
                          << (openPaths.size() > 0 ? openPaths.size() - 1 : 0) << std::endl;
                continue;
            }
            std::string targetPath = openPaths[static_cast<size_t>(devIdx)];

            std::vector<uint8_t> data;
            std::string hexStr;
            while (iss >> hexStr) {
                try {
                    uint8_t byte = static_cast<uint8_t>(std::stoul(hexStr, nullptr, 16));
                    data.push_back(byte);
                } catch (...) {
                    std::cout << "[ERROR] Invalid hex byte: " << hexStr << std::endl;
                    data.clear();
                    break;
                }
            }

            if (data.empty()) {
                std::cout << "Usage: " << cmd << " <devIdx> <hex bytes...>" << std::endl;
                continue;
            }

            if (cmd == "w") {
                std::cout << "Write [" << devIdx << "] " << targetPath << ": ";
                for (const auto& b : data) {
                    std::cout << std::hex << std::setfill('0') << std::setw(2)
                              << static_cast<int>(b) << " ";
                }
                std::cout << std::dec << std::endl;

                int written = hidDevice.write(targetPath, data);
                if (written >= 0) {
                    std::cout << "[PASS] Wrote " << written << " byte(s)" << std::endl;
                } else {
                    std::cout << "[FAIL] Write failed" << std::endl;
                }
            } else {
                std::cout << "Feature [" << devIdx << "] " << targetPath << ": ";
                for (const auto& b : data) {
                    std::cout << std::hex << std::setfill('0') << std::setw(2)
                              << static_cast<int>(b) << " ";
                }
                std::cout << std::dec << std::endl;

                int ret = hidDevice.send_Feature_Report(targetPath, data);
                if (ret >= 0) {
                    std::cout << "[PASS] Feature report sent (" << ret << " byte(s))" << std::endl;
                } else {
                    std::cout << "[FAIL] Feature report failed" << std::endl;
                }
            }
        }
        else if (cmd == "scan") {
            auto devices = hid::SimpleCommKitHid::get_Available_Devices();
            if (devices.empty()) {
                std::cout << "[INFO] No HID devices found." << std::endl;
            } else {
                std::cout << "Available devices (" << devices.size() << "):" << std::endl;
                printDeviceList(devices);
            }
        }
        else if (cmd == "devs") {
            printOpenDevices();
        }
        else if (cmd == "info") {
            auto openPaths = hidDevice.get_Open_Paths();
            std::cout << "Open count:          " << openPaths.size() << std::endl;
            std::cout << "Hotplug:             " << (hidDevice.is_Hotplug_Active() ? "ACTIVE" : "INACTIVE") << std::endl;
            std::cout << "Read poll interval:  " << hidDevice.get_Read_Poll_Interval() << " ms (global)" << std::endl;
            std::cout << "Read data length:    " << hidDevice.get_Read_Data_Length() << " bytes (global)" << std::endl;
            printOpenDevices();
        }
        else if (cmd == "poll") {
            int devIdx = -1;
            int value = -1;
            iss >> devIdx;
            if (iss.fail()) {
                // No arguments → show global
                std::cout << "Read poll interval: " << hidDevice.get_Read_Poll_Interval() << " ms (global)" << std::endl;
                continue;
            }
            iss >> value;
            if (iss.fail()) {
                // One argument → set global
                if (devIdx <= 0) {
                    std::cout << "Usage: poll <ms>  (ms > 0)" << std::endl;
                    continue;
                }
                hidDevice.set_Read_Poll_Interval(devIdx);
                std::cout << "[OK] Global read poll interval set to " << devIdx << " ms" << std::endl;
            } else {
                // Two arguments → set per-device
                if (devIdx < 0 || value <= 0) {
                    std::cout << "Usage: poll <devIdx> <ms>  (ms > 0)" << std::endl;
                    continue;
                }
                auto openPaths = hidDevice.get_Open_Paths();
                if (static_cast<size_t>(devIdx) >= openPaths.size()) {
                    std::cout << "[ERROR] Invalid device index." << std::endl;
                    continue;
                }
                std::string targetPath = openPaths[static_cast<size_t>(devIdx)];
                hidDevice.set_Read_Poll_Interval(targetPath, value);
                int actual = hidDevice.get_Read_Poll_Interval(targetPath);
                std::cout << "[OK] Read poll interval for [" << devIdx << "] " << targetPath
                          << " set to " << actual << " ms" << std::endl;
            }
        }
        else if (cmd == "dlen") {
            int devIdx = -1;
            int value = -1;
            iss >> devIdx;
            if (iss.fail()) {
                // No arguments → show global
                std::cout << "Read data length: " << hidDevice.get_Read_Data_Length() << " bytes (global)" << std::endl;
                continue;
            }
            iss >> value;
            if (iss.fail()) {
                // One argument → set global
                if (devIdx <= 0) {
                    std::cout << "Usage: dlen <length>  (length > 0)" << std::endl;
                    continue;
                }
                hidDevice.set_Read_Data_Length(devIdx);
                std::cout << "[OK] Global read data length set to " << devIdx << " bytes" << std::endl;
            } else {
                // Two arguments → set per-device
                if (devIdx < 0 || value <= 0) {
                    std::cout << "Usage: dlen <devIdx> <length>  (length > 0)" << std::endl;
                    continue;
                }
                auto openPaths = hidDevice.get_Open_Paths();
                if (static_cast<size_t>(devIdx) >= openPaths.size()) {
                    std::cout << "[ERROR] Invalid device index." << std::endl;
                    continue;
                }
                std::string targetPath = openPaths[static_cast<size_t>(devIdx)];
                hidDevice.set_Read_Data_Length(targetPath, value);
                int actual = hidDevice.get_Read_Data_Length(targetPath);
                std::cout << "[OK] Read data length for [" << devIdx << "] " << targetPath
                          << " set to " << actual << " bytes" << std::endl;
            }
        }
        else if (cmd == "help" || cmd == "?") {
            std::cout << "Commands:" << std::endl;
            std::cout << "  w <devIdx> <hex>      - Write report to device <devIdx>" << std::endl;
            std::cout << "  wf <devIdx> <hex>     - Send feature report to device <devIdx>" << std::endl;
            std::cout << "  open <path> [r]       - Open a device (append 'r' for readable)" << std::endl;
            std::cout << "  close <devIdx>        - Close device <devIdx>" << std::endl;
            std::cout << "  scan                  - Scan available HID devices" << std::endl;
            std::cout << "  devs                  - List open devices" << std::endl;
            std::cout << "  info                  - Show device & read config" << std::endl;
            std::cout << "  poll                  - Show global read poll interval (ms)" << std::endl;
            std::cout << "  poll <ms>             - Set global read poll interval" << std::endl;
            std::cout << "  poll <devIdx> <ms>    - Set per-device read poll interval" << std::endl;
            std::cout << "  dlen                  - Show global read data length" << std::endl;
            std::cout << "  dlen <len>            - Set global read data length" << std::endl;
            std::cout << "  dlen <devIdx> <len>   - Set per-device read data length" << std::endl;
            std::cout << "  q                     - Quit" << std::endl;
        }
        else {
            std::cout << "Unknown command: " << cmd << " (type 'help' for commands)" << std::endl;
        }
    }

    // ============================================
    // Cleanup
    // ============================================
    printSeparator("Close HID Device");

    std::cout << "Closing device..." << std::endl;

    hidDevice.stop_Hotplug();
    hidDevice.close();
    hidDevice.exit();

    if (!hidDevice.is_Open()) {
        std::cout << "[PASS] HID device closed successfully" << std::endl;
    } else {
        std::cout << "[WARN] Device may not have closed properly" << std::endl;
    }

    std::cout << "\n=== SimpleCommKitHid Demo Completed ===" << std::endl;
    return 0;
}
