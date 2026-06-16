#include <SimpleCommKitUdp.h>
#include "SimpleCommKitErrorMap.hpp"

#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <unordered_map>
#include <mutex>

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

// Track known clients: key = "host:port"
struct PeerInfo {
    std::string host;
    int port;
};
static std::unordered_map<std::string, PeerInfo> g_knownPeers;
static std::mutex g_peersMutex;

static std::string peerKey(const std::string& host, int port) {
    return host + ":" + std::to_string(port);
}

int main() {
    SimpleCommKitUdpServer server;

    // ---------------------------------------------------------------
    // Register callbacks
    // ---------------------------------------------------------------
    server.setCallback_OnError([](ErrorCode error) {
        std::cerr << "[Error] " << SimpleCommKitErrorMap::GetErrorDescription(error)
                  << " (code: 0x" << std::hex << error << std::dec << ")" << std::endl;
    });

    // UDP is connectionless – OnMessage includes the sender's address
    server.setCallback_OnMessage([](const std::string& fromHost, int fromPort, const std::vector<uint8_t>& data) {
        // Track this peer
        {
            std::lock_guard<std::mutex> lock(g_peersMutex);
            std::string key = peerKey(fromHost, fromPort);
            if (g_knownPeers.find(key) == g_knownPeers.end()) {
                g_knownPeers[key] = {fromHost, fromPort};
                std::cout << "[+] New peer " << fromHost << ":" << fromPort << std::endl;
            }
        }

        // Print hex dump
        std::cout << "[Msg] from=" << fromHost << ":" << fromPort
                  << "  len=" << data.size() << "  hex=";
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
    });

    // ---------------------------------------------------------------
    // Server setup
    // ---------------------------------------------------------------
    int port = 9090;
    std::string host = "0.0.0.0";

    std::cout << "=== SimpleCommKit UDP Server ===" << std::endl;
    std::cout << "(UDP is connectionless – messages carry sender's address)" << std::endl;

    std::cout << "Bind host [default " << host << "]: ";
    std::string input;
    std::getline(std::cin, input);
    if (!input.empty()) { host = trim(input); }

    std::cout << "Bind port [default " << port << "]: ";
    std::getline(std::cin, input);
    if (!input.empty()) { port = std::stoi(input); }

    // ---------------------------------------------------------------
    // Start server
    // ---------------------------------------------------------------
    std::cout << "Starting UDP server on " << host << ":" << port << " ..." << std::endl;
    if (!server.start(port, host)) {
        std::cerr << "Failed to start server!" << std::endl;
        return 1;
    }
    std::cout << "Server is running. Type 'help' for commands." << std::endl;

    // ---------------------------------------------------------------
    // Command loop (runs in main thread)
    // ---------------------------------------------------------------
    std::string cmd;
    while (std::getline(std::cin, cmd)) {
        cmd = trim(cmd);
        if (cmd.empty()) continue;

        try {
            if (cmd == "help" || cmd == "h") {
                std::cout << "Commands:\n"
                          << "  list (l)           - list known peer addresses\n"
                          << "  send <host> <port> <msg>    - send text to a peer\n"
                          << "  sendb <host> <port> <hex>  - send hex bytes to a peer\n"
                          << "  broadcast (bc) <msg>       - broadcast text\n"
                          << "  broadcastb (bcb) <hex>     - broadcast hex\n"
                          << "  info               - show server info\n"
                          << "  stop               - stop the server\n"
                          << "  quit (q)           - stop & exit\n";
            }
            else if (cmd == "list" || cmd == "l") {
                std::lock_guard<std::mutex> lock(g_peersMutex);
                std::cout << "Known peers (" << g_knownPeers.size() << "):" << std::endl;
                for (const auto& [key, peer] : g_knownPeers) {
                    std::cout << "  " << peer.host << ":" << peer.port << std::endl;
                }
            }
            else if (cmd == "info") {
                std::cout << "Server: " << server.host() << ":" << server.port() << std::endl;
                std::cout << "Running: " << (server.isRunning() ? "yes" : "no") << std::endl;
                std::lock_guard<std::mutex> lock(g_peersMutex);
                std::cout << "Known peers: " << g_knownPeers.size() << std::endl;
            }
            else if (cmd == "stop") {
                server.stop();
                std::cout << "Server stopped. (note: UDP is stateless, restart will resume)" << std::endl;
            }
            else if (cmd == "quit" || cmd == "q") {
                break;
            }
            else if (cmd.rfind("sendb ", 0) == 0) {
                // sendb <host> <port> <hex...>
                std::string params = cmd.substr(6);
                std::istringstream iss(params);
                std::string h; int p = 0;
                if (!(iss >> h >> p)) {
                    std::cerr << "Usage: sendb <host> <port> <hex bytes...>" << std::endl;
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
                int ret = server.sendTo(h, p, bytes);
                std::cout << "Sent " << bytes.size() << " bytes -> " << h << ":" << p
                          << " (ret=" << ret << ")" << std::endl;
            }
            else if (cmd.rfind("send ", 0) == 0) {
                // send <host> <port> <msg>
                std::string params = cmd.substr(5);
                std::istringstream iss(params);
                std::string h; int p = 0;
                if (!(iss >> h >> p)) {
                    std::cerr << "Usage: send <host> <port> <message>" << std::endl;
                    continue;
                }
                std::string msg;
                std::getline(iss, msg);
                msg = trim(msg);
                int ret = server.sendTo(h, p, msg);
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
                int ret = server.broadcast(bytes);
                std::cout << "Broadcast " << bytes.size() << " bytes (ret=" << ret << ")" << std::endl;
            }
            else if (cmd.rfind("bc ", 0) == 0 || cmd.rfind("broadcast ", 0) == 0) {
                std::string msg = cmd.rfind("bc ", 0) == 0
                    ? cmd.substr(3) : cmd.substr(10);
                int ret = server.broadcast(msg);
                std::cout << "Broadcast (ret=" << ret << ")" << std::endl;
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
    server.stop();
    std::cout << "Server shut down. Goodbye." << std::endl;

    return 0;
}
