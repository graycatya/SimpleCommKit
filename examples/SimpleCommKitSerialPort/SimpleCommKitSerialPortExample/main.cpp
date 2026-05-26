#include <iostream>
#include <atomic>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include <SimpleCommKitSerialPort.h>
#include "SimpleCommKitErrorMap.hpp"
#include "SimpleCommKitTestUtils.hpp"

namespace serial = SimpleCommKit;

void printSeparator(const std::string& title) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Testing: " << title << std::endl;
    std::cout << "========================================\n" << std::endl;
}

// Convert parity enum to string for display
std::string parityToString(serial::Parity p) {
    switch (p) {
    case serial::ParityNone:  return "NONE";
    case serial::ParityOdd:   return "ODD";
    case serial::ParityEven:  return "EVEN";
    case serial::ParityMark:  return "MARK";
    case serial::ParitySpace: return "SPACE";
    default:                  return "UNKNOWN";
    }
}

// Convert data bits enum to string for display
std::string dataBitsToString(serial::DataBits d) {
    switch (d) {
    case serial::DataBits5: return "5";
    case serial::DataBits6: return "6";
    case serial::DataBits7: return "7";
    case serial::DataBits8: return "8";
    default:                return "UNKNOWN";
    }
}

// Convert stop bits enum to string for display
std::string stopBitsToString(serial::StopBits s) {
    switch (s) {
    case serial::StopOne:       return "1";
    case serial::StopOneAndHalf: return "1.5";
    case serial::StopTwo:       return "2";
    default:                    return "UNKNOWN";
    }
}

// Convert flow control enum to string for display
std::string flowControlToString(serial::FlowControl f) {
    switch (f) {
    case serial::FlowNone:     return "NONE";
    case serial::FlowHardware: return "HARDWARE";
    case serial::FlowSoftware: return "SOFTWARE";
    default:                   return "UNKNOWN";
    }
}

int main() {
    std::cout << "=== SimpleCommKitSerialPort Interactive Demo ===" << std::endl;

    serial::SimpleCommKitSerialPort serial;

    // Register error callback
    serial.set_Callback_Error([](SimpleCommKit::ErrorCode error) {
        std::cout << "[CALLBACK] Error triggered: 0x"
                  << std::hex << static_cast<int>(error) << std::dec
                  << " : " << SimpleCommKit::SimpleCommKitErrorMap::GetErrorDescription(error)
                  << std::endl;
    });

    // ============================================
    // Enumerate available serial ports
    // ============================================
    printSeparator("Enumerate Available Serial Ports");

    auto ports = serial::SimpleCommKitSerialPort::get_Available_Ports();

    if (ports.empty()) {
        std::cout << "[INFO] No serial ports found on this system." << std::endl;
        std::cout << "[INFO] If you have serial devices connected, check your drivers." << std::endl;
        return 0;
    }

    std::cout << "Found " << ports.size() << " serial port(s):" << std::endl;
    for (size_t i = 0; i < ports.size(); ++i) {
        std::cout << "  [" << i << "] " << ports[i].portName
                  << "  -  " << ports[i].description << std::endl;
    }

    // ============================================
    // Select a serial port
    // ============================================
    auto selection = SimpleCommKitTestUtils::getUserInputInt("Select a serial port", ports.size() - 1);
    if (!selection.has_value()) {
        std::cout << "Invalid selection. Exiting." << std::endl;
        return EXIT_FAILURE;
    }

    std::string selectedPort = ports[selection.value()].portName;
    std::cout << "Selected: " << selectedPort << std::endl;

    // ============================================
    // Configure serial port parameters
    // ============================================
    printSeparator("Configure Serial Port Parameters");

    // Baud rate selection
    std::cout << "Select baud rate:" << std::endl;
    std::cout << "  [0] 9600   [1] 19200   [2] 38400   [3] 57600   [4] 115200" << std::endl;
    std::cout << "  [5] 921600 [6] 230400  [7] 460800  [8] Custom" << std::endl;

    int baudRates[] = {9600, 19200, 38400, 57600, 115200, 921600, 230400, 460800};
    int baudIdx = 0;
    std::cout << "Baud rate (0-8, default 0): ";
    std::cin >> baudIdx;
    if (std::cin.fail() || baudIdx < 0 || baudIdx > 8) baudIdx = 0;

    int baudRate;
    if (baudIdx == 8) {
        std::cout << "Enter custom baud rate: ";
        std::cin >> baudRate;
        if (std::cin.fail() || baudRate <= 0) {
            std::cin.clear();
            baudRate = 9600;
        }
    } else {
        baudRate = baudRates[baudIdx];
    }
    std::cout << "  -> Baud rate: " << baudRate << std::endl;

    // Parity selection
    std::cout << "\nSelect parity:" << std::endl;
    std::cout << "  [0] None  [1] Odd  [2] Even  [3] Mark  [4] Space" << std::endl;
    int parityIdx = 0;
    std::cout << "Parity (0-4, default 0): ";
    std::cin >> parityIdx;
    if (parityIdx < 0 || parityIdx > 4) parityIdx = 0;
    serial::Parity parities[] = {serial::ParityNone, serial::ParityOdd, serial::ParityEven,
                                 serial::ParityMark, serial::ParitySpace};
    serial::Parity parity = parities[parityIdx];
    std::cout << "  -> Parity: " << parityToString(parity) << std::endl;

    // Data bits selection
    std::cout << "\nSelect data bits:" << std::endl;
    std::cout << "  [0] 5  [1] 6  [2] 7  [3] 8" << std::endl;
    int dataIdx = 3;
    std::cout << "Data bits (0-3, default 3): ";
    std::cin >> dataIdx;
    if (dataIdx < 0 || dataIdx > 3) dataIdx = 3;
    serial::DataBits dataBitsArr[] = {serial::DataBits5, serial::DataBits6,
                                      serial::DataBits7, serial::DataBits8};
    serial::DataBits dataBits = dataBitsArr[dataIdx];
    std::cout << "  -> Data bits: " << dataBitsToString(dataBits) << std::endl;

    // Stop bits selection
    std::cout << "\nSelect stop bits:" << std::endl;
    std::cout << "  [0] 1  [1] 1.5  [2] 2" << std::endl;
    int stopIdx = 0;
    std::cout << "Stop bits (0-2, default 0): ";
    std::cin >> stopIdx;
    if (stopIdx < 0 || stopIdx > 2) stopIdx = 0;
    serial::StopBits stopBitsArr[] = {serial::StopOne, serial::StopOneAndHalf, serial::StopTwo};
    serial::StopBits stopbits = stopBitsArr[stopIdx];
    std::cout << "  -> Stop bits: " << stopBitsToString(stopbits) << std::endl;

    // Flow control selection
    std::cout << "\nSelect flow control:" << std::endl;
    std::cout << "  [0] None  [1] Hardware  [2] Software" << std::endl;
    int flowIdx = 0;
    std::cout << "Flow control (0-2, default 0): ";
    std::cin >> flowIdx;
    if (flowIdx < 0 || flowIdx > 2) flowIdx = 0;
    serial::FlowControl flowArr[] = {serial::FlowNone, serial::FlowHardware, serial::FlowSoftware};
    serial::FlowControl flowControl = flowArr[flowIdx];
    std::cout << "  -> Flow control: " << flowControlToString(flowControl) << std::endl;

    // Read buffer size
    std::cout << "\nRead buffer size (default 4096): ";
    unsigned int readBufferSize = 4096;
    std::cin >> readBufferSize;
    if (std::cin.fail()) {
        std::cin.clear();
        readBufferSize = 4096;
    }
    std::cout << "  -> Read buffer size: " << readBufferSize << std::endl;

    // Display configuration summary
    printSeparator("Configuration Summary");
    std::cout << "  Port:          " << selectedPort << std::endl;
    std::cout << "  Baud Rate:     " << baudRate << std::endl;
    std::cout << "  Parity:        " << parityToString(parity) << std::endl;
    std::cout << "  Data Bits:     " << dataBitsToString(dataBits) << std::endl;
    std::cout << "  Stop Bits:     " << stopBitsToString(stopbits) << std::endl;
    std::cout << "  Flow Control:  " << flowControlToString(flowControl) << std::endl;
    std::cout << "  Buffer Size:   " << readBufferSize << std::endl;

    // ============================================
    // Initialize and open the serial port
    // ============================================
    printSeparator("Initialize and Open Serial Port");

    serial.init(selectedPort, baudRate, parity, dataBits, stopbits, flowControl, readBufferSize);
    std::cout << "Initialized." << std::endl;

    // Register data callback
    std::atomic<bool> data_received{false};
    serial.set_Callback_On_Read([&data_received](const std::vector<uint8_t>& data) {
        std::cout << "\n[CALLBACK] Received " << data.size() << " byte(s): ";
        for (const auto& byte : data) {
            std::cout << std::hex << std::setfill('0') << std::setw(2)
                      << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl;

        // Also show as ASCII if printable
        bool allPrintable = true;
        for (const auto& byte : data) {
            if (byte < 0x20 || byte > 0x7E) {
                if (byte != '\r' && byte != '\n' && byte != '\t') {
                    allPrintable = false;
                    break;
                }
            }
        }
        if (allPrintable && !data.empty()) {
            std::cout << "           ASCII: \"";
            for (const auto& byte : data) {
                if (byte == '\r') std::cout << "\\r";
                else if (byte == '\n') std::cout << "\\n";
                else if (byte == '\t') std::cout << "\\t";
                else std::cout << static_cast<char>(byte);
            }
            std::cout << "\"" << std::endl;
        }

        data_received.store(true);
    });

    // Register hot-plug callback
    serial.set_Callback_On_HotPlug([](const std::string& portName, bool isAdd) {
        std::cout << "\n[CALLBACK] Hot-plug event: " << portName
                  << (isAdd ? " ADDED" : " REMOVED") << std::endl;
    });

    std::cout << "Opening " << selectedPort << "..." << std::endl;
    if (serial.open()) {
        std::cout << "[PASS] " << selectedPort << " opened successfully" << std::endl;
    } else {
        std::cout << "[FAIL] Failed to open " << selectedPort << std::endl;
        int lastErr = serial.get_Last_Error();
        std::string lastErrMsg = serial.get_Last_Error_Msg();
        if (lastErr != 0) {
            std::cout << "       Last error: " << lastErr << " - " << lastErrMsg << std::endl;
        }
        return EXIT_FAILURE;
    }

    // Check open status
    bool isOpen = serial.is_Open();
    std::cout << "Port is open: " << (isOpen ? "YES" : "NO") << std::endl;

    // Set DTR/RTS (common for embedded devices)
    std::cout << "Setting DTR and RTS..." << std::endl;
    serial.set_Dtr(true);
    serial.set_Rts(true);

    // ============================================
    // Interactive Read/Write Loop
    // ============================================
    printSeparator("Interactive Read/Write");

    std::cout << "Commands:" << std::endl;
    std::cout << "  w <text>   - Write text to serial port" << std::endl;
    std::cout << "  wb <hex>   - Write bytes in hex (e.g. wb AA BB CC)" << std::endl;
    std::cout << "  r <N>      - Read N bytes" << std::endl;
    std::cout << "  ra         - Read all available data" << std::endl;
    std::cout << "  flush      - Flush all buffers" << std::endl;
    std::cout << "  info       - Show port info" << std::endl;
    std::cout << "  q          - Quit" << std::endl;
    std::cout << "------------------------------------------------" << std::endl;

    std::cin.ignore(); // clear the newline from previous input
    std::string line;
    while (true) {
        std::cout << "\n> ";
        std::getline(std::cin, line);

        if (line.empty()) continue;

        // Parse command
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "q" || cmd == "quit") {
            std::cout << "Exiting interactive mode..." << std::endl;
            break;
        }
        else if (cmd == "w") {
            // Write text
            std::string text;
            std::getline(iss, text);
            // Remove leading space
            if (!text.empty() && text[0] == ' ') text = text.substr(1);

            if (text.empty()) {
                std::cout << "Usage: w <text>" << std::endl;
                continue;
            }

            std::vector<uint8_t> data(text.begin(), text.end());
            int written = serial.write(data);
            if (written >= 0) {
                std::cout << "[PASS] Wrote " << written << " byte(s)" << std::endl;
            } else {
                std::cout << "[FAIL] Write failed" << std::endl;
            }
        }
        else if (cmd == "wb") {
            // Write hex bytes
            std::vector<uint8_t> data;
            std::string hexStr;
            while (iss >> hexStr) {
                try {
                    uint8_t byte = static_cast<uint8_t>(std::stoul(hexStr, nullptr, 16));
                    data.push_back(byte);
                } catch (...) {
                    std::cout << "[ERROR] Invalid hex byte: " << hexStr << std::endl;
                    break;
                }
            }

            if (data.empty()) {
                std::cout << "Usage: wb <hex bytes...>  (e.g. wb AA BB CC)" << std::endl;
                continue;
            }

            std::cout << "Writing " << data.size() << " byte(s): ";
            for (const auto& b : data) {
                std::cout << std::hex << std::setfill('0') << std::setw(2)
                          << static_cast<int>(b) << " ";
            }
            std::cout << std::dec << std::endl;

            int written = serial.write(data);
            if (written >= 0) {
                std::cout << "[PASS] Wrote " << written << " byte(s)" << std::endl;
            } else {
                std::cout << "[FAIL] Write failed" << std::endl;
            }
        }
        else if (cmd == "r") {
            // Read N bytes
            int n = 0;
            iss >> n;
            if (n <= 0) {
                std::cout << "Usage: r <N>  (N > 0)" << std::endl;
                continue;
            }

            auto data = serial.read(n);
            if (data.empty()) {
                std::cout << "[INFO] No data read (timeout or no data available)" << std::endl;
            } else {
                std::cout << "Read " << data.size() << " byte(s): ";
                for (const auto& byte : data) {
                    std::cout << std::hex << std::setfill('0') << std::setw(2)
                              << static_cast<int>(byte) << " ";
                }
                std::cout << std::dec << std::endl;

                // Print ASCII
                bool allPrintable = true;
                for (const auto& byte : data) {
                    if (byte < 0x20 || byte > 0x7E) {
                        if (byte != '\r' && byte != '\n' && byte != '\t') {
                            allPrintable = false;
                            break;
                        }
                    }
                }
                if (allPrintable) {
                    std::cout << "       ASCII: \"";
                    for (const auto& byte : data) {
                        if (byte == '\r') std::cout << "\\r";
                        else if (byte == '\n') std::cout << "\\n";
                        else if (byte == '\t') std::cout << "\\t";
                        else std::cout << static_cast<char>(byte);
                    }
                    std::cout << "\"" << std::endl;
                }
            }
        }
        else if (cmd == "ra") {
            // Read all
            auto data = serial.read_All();
            if (data.empty()) {
                std::cout << "[INFO] No data available" << std::endl;
            } else {
                std::cout << "Read all: " << data.size() << " byte(s)" << std::endl;
                for (const auto& byte : data) {
                    std::cout << std::hex << std::setfill('0') << std::setw(2)
                              << static_cast<int>(byte) << " ";
                }
                std::cout << std::dec << std::endl;
            }
        }
        else if (cmd == "flush") {
            if (serial.flush_Buffers()) {
                std::cout << "[PASS] Buffers flushed" << std::endl;
            } else {
                std::cout << "[FAIL] Flush failed" << std::endl;
            }
        }
        else if (cmd == "info") {
            std::cout << "Port:          " << serial.get_Port_Name() << std::endl;
            std::cout << "Baud Rate:     " << serial.get_Baud_Rate() << std::endl;
            std::cout << "Parity:        " << parityToString(serial.get_Parity()) << std::endl;
            std::cout << "Data Bits:     " << dataBitsToString(serial.get_Data_Bits()) << std::endl;
            std::cout << "Stop Bits:     " << stopBitsToString(serial.get_Stop_Bits()) << std::endl;
            std::cout << "Flow Control:  " << flowControlToString(serial.get_Flow_Control()) << std::endl;
            std::cout << "Buffer Size:   " << serial.get_Read_Buffer_Size() << std::endl;
            std::cout << "Interval Timeout: " << serial.get_Read_Interval_Timeout() << " ms" << std::endl;
            std::cout << "Is Open:       " << (serial.is_Open() ? "YES" : "NO") << std::endl;
        }
        else if (cmd == "help" || cmd == "?") {
            std::cout << "Commands:" << std::endl;
            std::cout << "  w <text>   - Write text" << std::endl;
            std::cout << "  wb <hex>   - Write bytes in hex" << std::endl;
            std::cout << "  r <N>      - Read N bytes" << std::endl;
            std::cout << "  ra         - Read all available" << std::endl;
            std::cout << "  flush      - Flush buffers" << std::endl;
            std::cout << "  info       - Show port info" << std::endl;
            std::cout << "  q          - Quit" << std::endl;
        }
        else {
            std::cout << "Unknown command: " << cmd << " (type 'help' for commands)" << std::endl;
        }
    }

    // ============================================
    // Cleanup: Close the serial port
    // ============================================
    printSeparator("Close Serial Port");

    std::cout << "Closing " << selectedPort << "..." << std::endl;
    serial.close();

    if (!serial.is_Open()) {
        std::cout << "[PASS] " << selectedPort << " closed successfully" << std::endl;
    } else {
        std::cout << "[WARN] Port may not have closed properly" << std::endl;
    }

    std::cout << "\n=== SimpleCommKitSerialPort Demo Completed ===" << std::endl;
    return 0;
}
