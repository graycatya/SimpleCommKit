#include <SimpleCommKitUsb.h>
#include "SimpleCommKitErrorMap.hpp"
#include "SimpleCommKitTestUtils.hpp"

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <thread>
#include <chrono>
#include <cstring>

namespace usb = SimpleCommKit;

// ============================================================
// Helper functions
// ============================================================

static void printSeparator(const std::string& title) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  " << title << std::endl;
    std::cout << "========================================\n" << std::endl;
}

static void printDevice(const usb::SimpleCommKitUsbDeviceInfo& dev, int index = -1) {
    if (index >= 0) {
        std::cout << "  [" << index << "] ";
    } else {
        std::cout << "  - ";
    }
    std::cout << std::hex << std::setfill('0')
              << std::setw(4) << dev.vendor_id << ":"
              << std::setw(4) << dev.product_id << std::dec
              << "  bus=" << static_cast<int>(dev.bus_number)
              << " addr=" << static_cast<int>(dev.device_address)
              << "  path=" << dev.path;
    if (!dev.manufacturer_string.empty()) {
        std::cout << "  [" << dev.manufacturer_string << "]";
    }
    if (!dev.product_string.empty()) {
        std::cout << "  [" << dev.product_string << "]";
    }
    if (!dev.serial_number.empty()) {
        std::cout << "  SN=" << dev.serial_number;
    }
    std::cout << std::endl;
}

static void printDeviceList(const std::vector<usb::SimpleCommKitUsbDeviceInfo>& devices) {
    if (devices.empty()) {
        std::cout << "  (no devices found)" << std::endl;
        return;
    }
    for (size_t i = 0; i < devices.size(); ++i) {
        printDevice(devices[i], static_cast<int>(i));
    }
}

static void printHex(const std::vector<uint8_t>& data) {
    std::cout << "  (len=" << data.size() << ") ";
    for (size_t i = 0; i < data.size() && i < 32; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(data[i]) << " ";
    }
    if (data.size() > 32) std::cout << "...";
    std::cout << std::dec << std::endl;
}

static std::vector<uint8_t> parseHexString(const std::string& hex) {
    std::vector<uint8_t> result;
    std::istringstream iss(hex);
    unsigned int val;
    while (iss >> std::hex >> val) {
        if (val <= 0xFF) {
            result.push_back(static_cast<uint8_t>(val));
        }
    }
    return result;
}

static const char* transferTypeName(usb::SimpleCommKitUsbTransferType type) {
    switch (type) {
        case usb::SimpleCommKitUsbTransferType::Control:     return "Control";
        case usb::SimpleCommKitUsbTransferType::Isochronous: return "Isochronous";
        case usb::SimpleCommKitUsbTransferType::Bulk:        return "Bulk";
        case usb::SimpleCommKitUsbTransferType::Interrupt:   return "Interrupt";
        default: return "Unknown";
    }
}

static void printEndpoint(const usb::SimpleCommKitUsbEndpointInfo& ep, int index = -1) {
    const char* dir = ep.is_in ? "IN" : "OUT";
    const char* type = "?";
    if (ep.isBulk())        type = "Bulk";
    else if (ep.isInterrupt()) type = "Interrupt";
    else if (ep.isIsochronous()) type = "Isochronous";
    else if (ep.isControl()) type = "Control";

    std::cout << "  [" << (index >= 0 ? std::to_string(index) : "-") << "] "
              << "EP=0x" << std::hex << std::setw(2) << std::setfill('0')
              << static_cast<int>(ep.endpoint_address) << std::dec
              << " " << type << "/" << dir
              << " maxPkt=" << ep.max_packet_size
              << " interval=" << static_cast<int>(ep.interval)
              << std::endl;
}

static void printInterface(const usb::SimpleCommKitUsbInterfaceInfo& iface) {
    std::cout << "  Interface #" << static_cast<int>(iface.interface_number)
              << " alt=" << static_cast<int>(iface.alternate_setting)
              << " class=0x" << std::hex << std::setw(2) << std::setfill('0')
              << static_cast<int>(iface.interface_class) << std::dec
              << " endpoints=" << static_cast<int>(iface.num_endpoints);
    if (!iface.interface_string.empty()) {
        std::cout << " [" << iface.interface_string << "]";
    }
    std::cout << std::endl;
    for (size_t i = 0; i < iface.endpoints.size(); ++i) {
        printEndpoint(iface.endpoints[i], static_cast<int>(i));
    }
}

static usb::SimpleCommKitUsbTransferType parseTransferType(const std::string& s) {
    if (s == "bulk" || s == "b")        return usb::SimpleCommKitUsbTransferType::Bulk;
    if (s == "interrupt" || s == "i")   return usb::SimpleCommKitUsbTransferType::Interrupt;
    if (s == "isochronous" || s == "iso") return usb::SimpleCommKitUsbTransferType::Isochronous;
    if (s == "control" || s == "c")     return usb::SimpleCommKitUsbTransferType::Control;
    // default to bulk
    return usb::SimpleCommKitUsbTransferType::Bulk;
}

// CRC-32 (ISO-HDLC / PKZIP)
static uint32_t crc32(const std::vector<uint8_t>& data) {
    static uint32_t table[256];
    static bool init = false;
    if (!init) {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t crc = i;
            for (int j = 0; j < 8; j++) {
                crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
            }
            table[i] = crc;
        }
        init = true;
    }
    uint32_t crc = 0xFFFFFFFF;
    for (uint8_t b : data) {
        crc = table[(crc ^ b) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

// Build ConfigurationStatusQuery packet (protocol V1)
static std::vector<uint8_t> buildConfigQuery() {
    // Get current unix timestamp (4 bytes, little-endian)
    uint32_t timestamp = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());

    // Payload: CmdID(0x01) + Version(0x01) + Time(4B LE)
    std::vector<uint8_t> payload = {0x01, 0x01};
    payload.push_back(static_cast<uint8_t>(timestamp & 0xFF));
    payload.push_back(static_cast<uint8_t>((timestamp >> 8) & 0xFF));
    payload.push_back(static_cast<uint8_t>((timestamp >> 16) & 0xFF));
    payload.push_back(static_cast<uint8_t>((timestamp >> 24) & 0xFF));

    // CRC32 of payload
    uint32_t crc = crc32(payload);
    payload.push_back(static_cast<uint8_t>(crc & 0xFF));
    payload.push_back(static_cast<uint8_t>((crc >> 8) & 0xFF));
    payload.push_back(static_cast<uint8_t>((crc >> 16) & 0xFF));
    payload.push_back(static_cast<uint8_t>((crc >> 24) & 0xFF));

    // Frame: NSYNC(0xED) + Length(2B LE) + Payload
    uint16_t len = static_cast<uint16_t>(payload.size());
    std::vector<uint8_t> frame = {0xED,
                                   static_cast<uint8_t>(len & 0xFF),
                                   static_cast<uint8_t>((len >> 8) & 0xFF)};
    frame.insert(frame.end(), payload.begin(), payload.end());
    return frame;
}

// Parse response: extract readable fields
static void parseConfigResponse(const std::vector<uint8_t>& raw) {
    // Frame: NSYNC(1) + Length(2B LE) + Payload
    if (raw.size() < 4) { std::cout << "Response too short" << std::endl; return; }
    if (raw[0] != 0xED) { std::cout << "Invalid NSYNC" << std::endl; return; }

    uint16_t payloadLen = raw[1] | (raw[2] << 8);
    // Extract only the actual payload (ignore padding beyond frame length)
    auto payload = std::vector<uint8_t>(raw.begin() + 3, raw.begin() + 3 + payloadLen);

    // Payload: CmdID(1) + Body + CRC32(4)
    if (payload.size() < 6) { std::cout << "Payload too short" << std::endl; return; }
    uint8_t cmdId = payload[0];
    auto body = std::vector<uint8_t>(payload.begin() + 1, payload.end() - 4);
    
    std::cout << "  CmdID: 0x" << std::hex << (int)cmdId << std::dec << std::endl;
    if (body.size() < 120) { std::cout << "Body too short (" << body.size() << "B)" << std::endl; return; }

    auto readStr = [&](size_t off, size_t len) {
        std::string s;
        for (size_t i = off; i < off + len && body[i]; i++) s += (char)body[i];
        return s;
    };

    std::cout << "  Device Code:  " << readStr(0, 32) << std::endl;
    std::cout << "  SW Version:   " << readStr(32, 8) << std::endl;
    std::cout << "  HW Version:   " << readStr(40, 8) << std::endl;
    std::cout << "  Proto Ver:    " << (int)body[48] << std::endl;
    std::cout << "  Battery:      " << (int)body[49] << "%" << std::endl;
    std::cout << "  Battery Temp: " << (int)body[50] << " C" << std::endl;
    std::cout << "  Configured:   " << (body[51] ? "YES" : "NO") << std::endl;
    
    if (body[51]) {
        std::cout << "  Device Name:  " << readStr(52, 64) << std::endl;
        uint16_t dur = body[116] | (body[117] << 8);
        std::cout << "  Default Dur:  " << dur << "s" << std::endl;
        std::cout << "  Pwd Req:      " << (body[118] ? "YES" : "NO") << std::endl;
        std::cout << "  Encrypt Req:  " << (body[119] ? "YES" : "NO") << std::endl;
    }
}

static void showMenu() {
    std::cout << std::endl;
    std::cout << "=== SimpleCommKitUsb Interactive Demo (Single Device) ===" << std::endl;
    std::cout << "  scan                - Enumerate USB devices" << std::endl;
    std::cout << "  open <idx>          - Open device by index" << std::endl;
    std::cout << "  open_vp <vid> <pid> - Open by VID/PID" << std::endl;
    std::cout << "  close               - Close device" << std::endl;
    std::cout << "  claim <if>          - Claim interface" << std::endl;
    std::cout << "  release <if>        - Release interface" << std::endl;
    std::cout << "  ctrl <bmReq> <bReq> <wVal> <wIdx> <hexdata> - Control OUT" << std::endl;
    std::cout << "  ctrl_in <bmReq> <bReq> <wVal> <wIdx> <len> - Control IN" << std::endl;
    std::cout << "  bulk <ep> <hexdata|len> - Bulk transfer (OUT=hexdata, IN=len)" << std::endl;
    std::cout << "  intr <ep> <hexdata|len> - Interrupt transfer" << std::endl;
    std::cout << "  iso <ep> <npkts> <pktlen> <hexdata|len> - Isochronous transfer" << std::endl;
    std::cout << "  read_on <ep>        - Start continuous read poll" << std::endl;
    std::cout << "  read_off            - Stop read poll" << std::endl;
    std::cout << "  read_stat           - Check if read poll is active" << std::endl;
    std::cout << "  rpoll <ms>          - Set read poll interval" << std::endl;
    std::cout << "  rlen <len>          - Set read data length" << std::endl;
    std::cout << "  hotplug <vid> <pid> - Start hotplug (0 0 = all)" << std::endl;
    std::cout << "  hotplug_stop        - Stop hotplug" << std::endl;
    std::cout << "  hpoll <ms>          - Set hotplug poll interval" << std::endl;
    std::cout << "  info                - Show device info" << std::endl;
    std::cout << "  devs                - Show cached device list" << std::endl;
    std::cout << "  ifaces              - Show all interfaces & endpoints" << std::endl;
    std::cout << "  endpoints <if>      - Show endpoints for interface" << std::endl;
    std::cout << "  findeps <type>      - Find endpoints: bulk|interrupt|iso" << std::endl;
    std::cout << "  autodisc <type>     - Auto-discover IN/OUT endpoints" << std::endl;
    std::cout << "  query               - Query device config (RT-Thread protocol)" << std::endl;
    std::cout << "  help                - Show this menu" << std::endl;
    std::cout << "  q                   - Quit" << std::endl;
}

// ============================================================
// main
// ============================================================

int main() {
    printSeparator("SimpleCommKitUsb Example");

    usb::SimpleCommKitUsb usbDevice;

    // --- Error callback ---
    usbDevice.set_Callback_Error([](usb::ErrorCode error) {
        std::cout << "[ERROR] code=" << error
                  << " : " << usb::SimpleCommKitErrorMap::GetErrorDescription(error)
                  << std::endl;
    });

    // --- Init ---
    if (!usbDevice.init()) {
        std::cerr << "Failed to initialize libusb context" << std::endl;
        return EXIT_FAILURE;
    }

    // --- Hotplug callback ---
    usbDevice.set_Callback_On_HotPlug(
        [](const std::vector<usb::SimpleCommKitUsbDeviceInfo>& added,
           const std::vector<usb::SimpleCommKitUsbDeviceInfo>& removed) {
            if (!added.empty()) {
                std::cout << "\n[HOTPLUG] + " << added.size() << " device(s) added:" << std::endl;
                printDeviceList(added);
            }
            if (!removed.empty()) {
                std::cout << "\n[HOTPLUG] - " << removed.size() << " device(s) removed:" << std::endl;
                printDeviceList(removed);
            }
        });

    // --- Read callback ---
    usbDevice.set_Callback_On_Read(
        [](const usb::SimpleCommKitUsbDeviceInfo& dev,
           const std::vector<uint8_t>& data) {
            std::cout << "\n[READ] device=" << dev.path << " ";
            printHex(data);
        });

    // --- Interactive loop ---
    bool running = true;
    showMenu();

    while (running) {
        std::cout << "> " << std::flush;

        std::string line;
        if (!std::getline(std::cin, line) || line.empty()) continue;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "q" || cmd == "quit") {
            running = false;

        } else if (cmd == "scan") {
            auto devs = usb::SimpleCommKitUsb::get_Available_Devices();
            std::cout << "Found " << devs.size() << " USB device(s):" << std::endl;
            printDeviceList(devs);

        } else if (cmd == "open") {
            int idx = -1;
            iss >> idx;
            auto devs = usb::SimpleCommKitUsb::get_Available_Devices();
            if (idx < 0 || static_cast<size_t>(idx) >= devs.size()) {
                std::cout << "Invalid index. Run 'scan' first." << std::endl;
                continue;
            }
            if (usbDevice.open(devs[static_cast<size_t>(idx)].path)) {
                std::cout << "Opened: " << devs[static_cast<size_t>(idx)].path << std::endl;
            }

        } else if (cmd == "open_vp") {
            unsigned int vid = 0, pid = 0;
            iss >> std::hex >> vid >> pid;
            if (usbDevice.open(static_cast<unsigned short>(vid),
                               static_cast<unsigned short>(pid))) {
                std::cout << "Opened device VID:PID="
                          << std::hex << vid << ":" << pid << std::dec << std::endl;
            }

        } else if (cmd == "close") {
            if (usbDevice.is_Open()) {
                usbDevice.close();
                std::cout << "Device closed." << std::endl;
            } else {
                std::cout << "No device is open." << std::endl;
            }

        } else if (cmd == "claim") {
            int iface = 0;
            iss >> iface;
            if (usbDevice.claim_Interface(iface)) {
                std::cout << "Claimed interface " << iface << std::endl;
            }

        } else if (cmd == "release") {
            int iface = 0;
            iss >> iface;
            if (usbDevice.release_Interface(iface)) {
                std::cout << "Released interface " << iface << std::endl;
            }

        } else if (cmd == "ctrl" || cmd == "ctrl_in") {
            unsigned int bmReq = 0, bReq = 0, wVal = 0, wIdx = 0;
            std::string hexstr;
            iss >> std::hex >> bmReq >> bReq >> wVal >> wIdx;
            std::getline(iss, hexstr);

            auto data = parseHexString(hexstr);
            bool isIn = (cmd == "ctrl_in");

            if (isIn) {
                unsigned int inLen = 64;
                if (data.size() > 0) inLen = data[0];
                data.resize(inLen);
            }

            int ret = usbDevice.control_Transfer(
                static_cast<uint8_t>(bmReq | (isIn ? 0x80 : 0x00)),
                static_cast<uint8_t>(bReq),
                static_cast<uint16_t>(wVal), static_cast<uint16_t>(wIdx),
                data, 1000);
            std::cout << "Control transfer result=" << ret << std::endl;
            if (ret > 0) {
                std::cout << "Received:" << std::endl;
                printHex(data);
            }

        } else if (cmd == "bulk") {
            int ep = 0;
            std::string rest;
            iss >> std::hex >> ep;
            std::getline(iss, rest);
            
            bool is_in = (ep & 0x80) != 0;
            if (is_in) {
                // Bulk IN: parse as length
                int length = 64;
                std::istringstream riss(rest);
                riss >> length;
                std::vector<uint8_t> data(static_cast<size_t>(length));
                int ret = usbDevice.bulk_Transfer(static_cast<uint8_t>(ep), data, 1000);
                std::cout << "Bulk IN result=" << ret << std::endl;
                if (ret > 0) {
                    data.resize(static_cast<size_t>(ret));
                    printHex(data);
                }
            } else {
                // Bulk OUT: parse as hex data
                auto data = parseHexString(rest);
                if (data.empty()) {
                    std::cout << "Usage: bulk <ep> <hex bytes>" << std::endl;
                    continue;
                }
                int ret = usbDevice.bulk_Transfer(static_cast<uint8_t>(ep), data, 1000);
                std::cout << "Bulk OUT transferred " << ret << " bytes" << std::endl;
            }

        } else if (cmd == "intr") {
            int ep = 0;
            std::string rest;
            iss >> std::hex >> ep;
            std::getline(iss, rest);
            
            bool is_in = (ep & 0x80) != 0;
            if (is_in) {
                int length = 64;
                std::istringstream riss(rest);
                riss >> length;
                std::vector<uint8_t> data(static_cast<size_t>(length));
                int ret = usbDevice.interrupt_Transfer(static_cast<uint8_t>(ep), data, 1000);
                std::cout << "Interrupt IN result=" << ret << std::endl;
                if (ret > 0) {
                    data.resize(static_cast<size_t>(ret));
                    printHex(data);
                }
            } else {
                auto data = parseHexString(rest);
                if (data.empty()) {
                    std::cout << "Usage: intr <ep> <hex bytes>" << std::endl;
                    continue;
                }
                int ret = usbDevice.interrupt_Transfer(static_cast<uint8_t>(ep), data, 1000);
                std::cout << "Interrupt OUT transferred " << ret << " bytes" << std::endl;
            }

        } else if (cmd == "iso") {
            int ep = 0, npkts = 1, pktlen = 64;
            std::string rest;
            iss >> std::hex >> ep >> std::dec >> npkts >> pktlen;
            std::getline(iss, rest);

            bool is_in = (ep & 0x80) != 0;
            std::vector<int> packet_lengths(static_cast<size_t>(npkts), pktlen);
            std::vector<uint8_t> data;
            std::vector<usb::SimpleCommKitUsbIsoPacketResult> results;

            if (is_in) {
                // Isochronous IN: data will be auto-resized
                int ret = usbDevice.isochronous_Transfer(
                    static_cast<uint8_t>(ep), data, npkts, packet_lengths, results, 1000);
                std::cout << "Isochronous IN result=" << ret << std::endl;
                for (size_t i = 0; i < results.size(); ++i) {
                    std::cout << "  packet[" << i << "]: req=" << results[i].length
                              << " actual=" << results[i].actual_length
                              << " status=" << results[i].status << std::endl;
                }
                if (ret > 0) printHex(data);
            } else {
                // Isochronous OUT
                data = parseHexString(rest);
                if (data.empty()) {
                    std::cout << "Usage: iso <ep> <npkts> <pktlen> <hex bytes>" << std::endl;
                    continue;
                }
                int total_pkt = npkts * pktlen;
                if (static_cast<int>(data.size()) < total_pkt) {
                    packet_lengths.back() = static_cast<int>(data.size()) - (npkts - 1) * pktlen;
                }
                int ret = usbDevice.isochronous_Transfer(
                    static_cast<uint8_t>(ep), data, npkts, packet_lengths, results, 1000);
                std::cout << "Isochronous OUT result=" << ret << std::endl;
                for (size_t i = 0; i < results.size(); ++i) {
                    std::cout << "  packet[" << i << "]: req=" << results[i].length
                              << " actual=" << results[i].actual_length
                              << " status=" << results[i].status << std::endl;
                }
            }

        } else if (cmd == "read_on") {
            int ep = 0;
            iss >> std::hex >> ep;
            usbDevice.start_Read_Poll(static_cast<uint8_t>(ep));
            std::cout << "Read poll started on EP="
                      << std::hex << ep << std::dec << std::endl;

        } else if (cmd == "read_off") {
            usbDevice.stop_Read_Poll();
            std::cout << "Read poll stopped." << std::endl;

        } else if (cmd == "read_stat") {
            if (usbDevice.is_Read_Poll_Active()) {
                std::cout << "Read poll is ACTIVE" << std::endl;
            } else {
                std::cout << "Read poll is INACTIVE" << std::endl;
            }

        } else if (cmd == "rpoll") {
            int ms = 100;
            iss >> ms;
            usbDevice.set_Read_Poll_Interval(ms);
            std::cout << "Read poll interval set to " << ms << " ms" << std::endl;

        } else if (cmd == "rlen") {
            int len = 64;
            iss >> len;
            usbDevice.set_Read_Data_Length(len);
            std::cout << "Read data length set to " << len << std::endl;

        } else if (cmd == "hotplug") {
            unsigned int vid = 0, pid = 0;
            iss >> std::hex >> vid >> pid;
            usbDevice.start_Hotplug(static_cast<unsigned short>(vid),
                                     static_cast<unsigned short>(pid));
            std::cout << "Hotplug monitoring started for VID:PID="
                      << std::hex << vid << ":" << pid << std::dec << std::endl;

        } else if (cmd == "hotplug_stop") {
            usbDevice.stop_Hotplug();
            std::cout << "Hotplug monitoring stopped." << std::endl;

        } else if (cmd == "hpoll") {
            int ms = 1000;
            iss >> ms;
            usbDevice.set_Hotplug_Poll_Interval(ms);
            std::cout << "Hotplug poll interval set to " << ms << " ms" << std::endl;

        } else if (cmd == "info") {
            if (usbDevice.is_Open()) {
                std::cout << "Open device path: " << usbDevice.get_Open_Path() << std::endl;
            } else {
                std::cout << "No device is open." << std::endl;
            }

        } else if (cmd == "devs") {
            auto devs = usbDevice.get_Device_List();
            std::cout << "Cached devices: " << devs.size() << std::endl;
            printDeviceList(devs);

        } else if (cmd == "ifaces") {
            if (!usbDevice.is_Open()) {
                std::cout << "No device open. Use 'open' first." << std::endl;
                continue;
            }
            auto interfaces = usbDevice.get_Device_Interfaces();
            std::cout << "Device has " << interfaces.size() << " interface(s):" << std::endl;
            for (size_t i = 0; i < interfaces.size(); ++i) {
                printInterface(interfaces[i]);
            }

        } else if (cmd == "endpoints") {
            int iface = 0;
            iss >> iface;
            if (!usbDevice.is_Open()) {
                std::cout << "No device open. Use 'open' first." << std::endl;
                continue;
            }
            auto eps = usbDevice.get_Interface_Endpoints(iface);
            if (eps.empty()) {
                std::cout << "No endpoints found for interface " << iface << std::endl;
            } else {
                std::cout << "Interface " << iface << " has " << eps.size() << " endpoint(s):" << std::endl;
                for (size_t i = 0; i < eps.size(); ++i) {
                    printEndpoint(eps[i], static_cast<int>(i));
                }
            }

        } else if (cmd == "findeps") {
            std::string typeStr;
            iss >> typeStr;
            if (!usbDevice.is_Open()) {
                std::cout << "No device open. Use 'open' first." << std::endl;
                continue;
            }
            auto transferType = parseTransferType(typeStr);
            auto eps = usbDevice.find_Endpoints_By_Type(transferType);
            std::cout << "Found " << eps.size() << " "
                      << transferTypeName(transferType) << " endpoint(s):" << std::endl;
            for (size_t i = 0; i < eps.size(); ++i) {
                printEndpoint(eps[i], static_cast<int>(i));
            }

        } else if (cmd == "autodisc") {
            std::string typeStr;
            iss >> typeStr;
            if (!usbDevice.is_Open()) {
                std::cout << "No device open. Use 'open' first." << std::endl;
                continue;
            }
            auto transferType = parseTransferType(typeStr);
            uint8_t out_ep = 0, in_ep = 0;
            if (usbDevice.auto_Discover_Endpoints(transferType, out_ep, in_ep)) {
                std::cout << transferTypeName(transferType) << " endpoint auto-discovery:" << std::endl;
                if (out_ep != 0) {
                    std::cout << "  OUT: EP=0x" << std::hex << std::setw(2) << std::setfill('0')
                              << static_cast<int>(out_ep) << std::dec << std::endl;
                } else {
                    std::cout << "  OUT: (not found)" << std::endl;
                }
                if (in_ep != 0) {
                    std::cout << "  IN : EP=0x" << std::hex << std::setw(2) << std::setfill('0')
                              << static_cast<int>(in_ep) << std::dec << std::endl;
                } else {
                    std::cout << "  IN : (not found)" << std::endl;
                }
            } else {
                std::cout << "No " << transferTypeName(transferType)
                          << " endpoints found." << std::endl;
            }

        } else if (cmd == "query") {
            if (!usbDevice.is_Open()) {
                std::cout << "No device open. Use 'open' first." << std::endl;
                continue;
            }
            auto queryPkt = buildConfigQuery();
            std::cout << "Sending ConfigQuery (" << queryPkt.size() << " bytes): ";
            printHex(queryPkt);

            int ret = usbDevice.bulk_Transfer(0x02, queryPkt, 1000);
            if (ret < 0) {
                std::cout << "Send failed: " << ret << std::endl;
                continue;
            }
            std::cout << "Sent " << ret << " bytes" << std::endl;

            // Read response
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::vector<uint8_t> resp(512);
            ret = usbDevice.bulk_Transfer(0x81, resp, 2000);
            if (ret > 0) {
                resp.resize(static_cast<size_t>(ret));
                std::cout << "Response (" << ret << " bytes):" << std::endl;
                printHex(resp);
                parseConfigResponse(resp);
            } else {
                std::cout << "No response (ret=" << ret << ")" << std::endl;
            }

        } else if (cmd == "help") {
            showMenu();
        } else {
            std::cout << "Unknown command: " << cmd << std::endl;
        }
    }

    // --- Cleanup ---
    std::cout << "\nShutting down..." << std::endl;
    usbDevice.stop_Hotplug();
    usbDevice.stop_Read_Poll();
    usbDevice.close();
    usbDevice.exit();
    std::cout << "Done." << std::endl;

    return EXIT_SUCCESS;
}
