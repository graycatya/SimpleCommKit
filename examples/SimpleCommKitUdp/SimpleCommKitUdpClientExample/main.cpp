#include <SimpleCommKitUdp.h>
#include "SimpleCommKitErrorMap.hpp"

#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>
#include <iomanip>

using namespace SimpleCommKit;

// Trim helper
static std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// Parse hex bytes from a space-separated string, e.g. "48 65 6C 6C 6F"
static std::vector<uint8_t> parseHex(const std::string& hexStr) {
    std::vector<uint8_t> bytes;
    std::istringstream iss(hexStr);
    std::string token;
    while (iss >> token) {
        try {
            bytes.push_back(static_cast<uint8_t>(std::stoul(token, nullptr, 16)));
        } catch (...) {
            // skip invalid
        }
    }
    return bytes;
}

// Print received data with hex + ascii preview
static void printData(const std::string& label, const std::vector<uint8_t>& data) {
    std::cout << "[" << label << "] len=" << data.size() << "  hex=";
    for (auto b : data) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(b) << " ";
    }
    std::cout << std::dec << std::endl;

    // Print as ASCII if readable
    bool readable = true;
    for (auto b : data) {
        if (b < 0x20 && b != '\r' && b != '\n' && b != '\t') { readable = false; break; }
    }
    if (readable) {
        std::cout << "      ascii: \"";
        for (auto b : data) {
            if (b == '\r') std::cout << "\\r";
            else if (b == '\n') std::cout << "\\n";
            else if (b == '\t') std::cout << "\\t";
            else std::cout << static_cast<char>(b);
        }
        std::cout << "\"" << std::endl;
    }
}

int main() {
    SimpleCommKitUdpClient client;

    // ---------------------------------------------------------------
    // Register callbacks
    // ---------------------------------------------------------------
    client.setCallback_OnMessage([](const std::vector<uint8_t>& data) {
        printData("Msg", data);
    });
    client.setCallback_OnError([](ErrorCode error) {
        std::cerr << "[Error] " << SimpleCommKitErrorMap::GetErrorDescription(error)
                  << " (code: 0x" << std::hex << error << std::dec << ")" << std::endl;
    });

    // ---------------------------------------------------------------
    // Setup
    // ---------------------------------------------------------------
    std::string remoteHost = "127.0.0.1";
    int remotePort = 9090;
    int localPort = 0;           // 0 = OS picks a free port
    std::string localHost = "0.0.0.0";

    std::cout << "=== SimpleCommKit UDP Client ===" << std::endl;
    std::cout << "(UDP is connectionless – open a local socket, then send/recv)" << std::endl;

    std::string input;

    // Local bind address
    std::cout << "Local bind host [default " << localHost << "]: ";
    std::getline(std::cin, input);
    if (!input.empty()) { localHost = trim(input); }

    std::cout << "Local bind port (0=auto) [default " << localPort << "]: ";
    std::getline(std::cin, input);
    if (!input.empty()) { localPort = std::stoi(input); }

    // Remote target
    std::cout << "Default remote host [default " << remoteHost << "]: ";
    std::getline(std::cin, input);
    if (!input.empty()) { remoteHost = trim(input); }

    std::cout << "Default remote port [default " << remotePort << "]: ";
    std::getline(std::cin, input);
    if (!input.empty()) { remotePort = std::stoi(input); }

    // Read timeout
    int timeout = 3000;
    std::cout << "Read timeout (ms) [default " << timeout << "]: ";
    std::getline(std::cin, input);
    if (!input.empty()) { timeout = std::stoi(input); }
    client.setReadTimeout(timeout);

    // ---------------------------------------------------------------
    // Open local socket
    // ---------------------------------------------------------------
    std::cout << "Opening local UDP socket on " << localHost << ":" << localPort << " ..." << std::endl;
    if (!client.open(localPort, localHost)) {
        std::cerr << "Failed to open socket!" << std::endl;
        return 1;
    }
    std::cout << "Socket open: " << (client.isOpen() ? "yes" : "no") << std::endl;

    // Set the default remote address for convenience send()
    client.setRemoteAddress(remoteHost, remotePort);
    std::cout << "Default remote set to " << remoteHost << ":" << remotePort << std::endl;
    std::cout << "Type 'help' for commands." << std::endl;

    // ---------------------------------------------------------------
    // Command loop
    // ---------------------------------------------------------------
    std::string cmd;
    while (std::getline(std::cin, cmd)) {
        cmd = trim(cmd);
        if (cmd.empty()) continue;

        try {
            if (cmd == "help" || cmd == "h") {
                std::cout << "Commands:\n"
                          << "  send (s) <msg>       - send text to default remote\n"
                          << "  sendb (sb) <hex>     - send hex bytes to default remote\n"
                          << "  sendto (st) <host> <port> <msg>      - send text to specific addr\n"
                          << "  sendtob (stb) <host> <port> <hex>    - send hex to specific addr\n"
                          << "  remote (rm) <host> <port> - change default remote\n"
                          << "  broadcast (bc) <msg>     - send text to 255.255.255.255\n"
                          << "  broadcastb (bcb) <hex>   - send hex to 255.255.255.255\n"
                          << "  status (st)        - show socket status\n"
                          << "  reopen             - close & reopen socket\n"
                          << "  quit (q)           - close & exit\n";
            }
            else if (cmd == "status" || cmd == "st") {
                std::cout << "Socket open: " << (client.isOpen() ? "yes" : "no") << std::endl;
                std::cout << "Default remote: " << remoteHost << ":" << remotePort << std::endl;
            }
            else if (cmd == "reopen") {
                client.close();
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                std::cout << "Re-opening socket..." << std::endl;
                if (client.open(localPort, localHost)) {
                    client.setRemoteAddress(remoteHost, remotePort);
                    std::cout << "Socket re-opened." << std::endl;
                } else {
                    std::cerr << "Failed to re-open socket!" << std::endl;
                }
            }
            else if (cmd == "quit" || cmd == "q") {
                break;
            }
            else if (cmd.rfind("remote ", 0) == 0 || cmd.rfind("rm ", 0) == 0) {
                std::string params = cmd.rfind("remote ", 0) == 0
                    ? cmd.substr(7) : cmd.substr(3);
                std::istringstream iss(params);
                std::string h; int p = 0;
                if (iss >> h >> p) {
                    remoteHost = h;
                    remotePort = p;
                    client.setRemoteAddress(remoteHost, remotePort);
                    std::cout << "Default remote set to " << remoteHost << ":" << remotePort << std::endl;
                } else {
                    std::cerr << "Usage: remote <host> <port>" << std::endl;
                }
            }
            else if (cmd.rfind("sendtob ", 0) == 0 || cmd.rfind("stb ", 0) == 0) {
                // sendtob <host> <port> <hex...>
                std::string params = cmd.rfind("sendtob ", 0) == 0
                    ? cmd.substr(8) : cmd.substr(4);
                std::istringstream iss(params);
                std::string h; int p = 0;
                if (!(iss >> h >> p)) {
                    std::cerr << "Usage: sendtob <host> <port> <hex bytes...>" << std::endl;
                    continue;
                }
                std::string hexRest;
                std::getline(iss, hexRest);
                hexRest = trim(hexRest);
                auto bytes = parseHex(hexRest);
                if (bytes.empty()) {
                    std::cerr << "No valid hex bytes." << std::endl;
                    continue;
                }
                int ret = client.sendTo(h, p, bytes);
                std::cout << "Sent " << bytes.size() << " bytes -> " << h << ":" << p
                          << " (ret=" << ret << ")" << std::endl;
            }
            else if (cmd.rfind("sendto ", 0) == 0 || cmd.rfind("st ", 0) == 0) {
                // sendto <host> <port> <msg>
                std::string params = cmd.rfind("sendto ", 0) == 0
                    ? cmd.substr(7) : cmd.substr(3);
                std::istringstream iss(params);
                std::string h; int p = 0;
                if (!(iss >> h >> p)) {
                    std::cerr << "Usage: sendto <host> <port> <message>" << std::endl;
                    continue;
                }
                std::string msg;
                std::getline(iss, msg);
                msg = trim(msg);
                int ret = client.sendTo(h, p, msg);
                std::cout << "Sent -> " << h << ":" << p << " (ret=" << ret << ")" << std::endl;
            }
            else if (cmd.rfind("bcb ", 0) == 0 || cmd.rfind("broadcastb ", 0) == 0) {
                std::string hexData = cmd.rfind("bcb ", 0) == 0
                    ? cmd.substr(4) : cmd.substr(11);
                auto bytes = parseHex(hexData);
                if (bytes.empty()) {
                    std::cerr << "No valid hex bytes." << std::endl;
                    continue;
                }
                int ret = client.sendTo("255.255.255.255", remotePort, bytes);
                std::cout << "Broadcast " << bytes.size() << " bytes (ret=" << ret << ")" << std::endl;
            }
            else if (cmd.rfind("bc ", 0) == 0 || cmd.rfind("broadcast ", 0) == 0) {
                std::string msg = cmd.rfind("bc ", 0) == 0
                    ? cmd.substr(3) : cmd.substr(10);
                int ret = client.sendTo("255.255.255.255", remotePort, msg);
                std::cout << "Broadcast (ret=" << ret << ")" << std::endl;
            }
            else if (cmd.rfind("sendb ", 0) == 0 || cmd.rfind("sb ", 0) == 0) {
                std::string hexData = cmd.rfind("sendb ", 0) == 0
                    ? cmd.substr(6) : cmd.substr(3);
                if (hexData.empty()) {
                    std::cerr << "Usage: sendb <hex bytes...>" << std::endl;
                    continue;
                }
                auto bytes = parseHex(hexData);
                if (bytes.empty()) {
                    std::cerr << "No valid hex bytes." << std::endl;
                    continue;
                }
                int ret = client.send(bytes);
                std::cout << "Sent " << bytes.size() << " bytes -> " << remoteHost << ":" << remotePort
                          << " (ret=" << ret << ")" << std::endl;
            }
            else if (cmd.rfind("send ", 0) == 0 || cmd.rfind("s ", 0) == 0) {
                std::string msg = cmd.rfind("send ", 0) == 0
                    ? cmd.substr(5) : cmd.substr(2);
                if (msg.empty()) {
                    std::cerr << "Usage: send <message>" << std::endl;
                    continue;
                }
                int ret = client.send(msg);
                std::cout << "Sent -> " << remoteHost << ":" << remotePort
                          << " (ret=" << ret << ")" << std::endl;
            }
            else {
                std::cerr << "Unknown command. Type 'help'." << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    // ---------------------------------------------------------------
    // Cleanup
    // ---------------------------------------------------------------
    client.close();
    std::cout << "Client shut down. Goodbye." << std::endl;

    return 0;
}
