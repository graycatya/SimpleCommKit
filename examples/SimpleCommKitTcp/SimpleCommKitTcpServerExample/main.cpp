#include <SimpleCommKitTcp.h>
#include "SimpleCommKitErrorMap.hpp"
#include "SimpleCommKitTestUtils.hpp"

#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <atomic>

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

int main() {
    SimpleCommKitTcpServer server;

    // ---------------------------------------------------------------
    // Register error callback
    // ---------------------------------------------------------------
    server.setCallback_OnError([](ErrorCode error) {
        std::cerr << "[Error] " << SimpleCommKitErrorMap::GetErrorDescription(error)
                  << " (code: 0x" << std::hex << error << std::dec << ")" << std::endl;
    });

    // ---------------------------------------------------------------
    // Register client connect / disconnect callbacks
    // ---------------------------------------------------------------
    server.setCallback_OnClientConnected([](uint32_t client_id) {
        std::cout << "[+] Client connected  (id=" << client_id << ")" << std::endl;
    });
    server.setCallback_OnClientDisconnected([](uint32_t client_id) {
        std::cout << "[-] Client disconnected (id=" << client_id << ")" << std::endl;
    });

    // ---------------------------------------------------------------
    // Register message callback
    // ---------------------------------------------------------------
    server.setCallback_OnMessage([](uint32_t client_id, const std::vector<uint8_t>& data) {
        // Print hex dump
        std::cout << "[Msg] client=" << client_id << "  len=" << data.size() << "  hex=";
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

    std::cout << "=== SimpleCommKit TCP Server ===" << std::endl;

    std::cout << "Listen host [default " << host << "]: ";
    std::string input;
    std::getline(std::cin, input);
    if (!input.empty()) { host = trim(input); }

    std::cout << "Listen port [default " << port << "]: ";
    std::getline(std::cin, input);
    if (!input.empty()) { port = std::stoi(input); }

    int threadNum = 0;
    std::cout << "Worker threads (0=single-thread) [default " << threadNum << "]: ";
    std::getline(std::cin, input);
    if (!input.empty()) { threadNum = std::stoi(input); }

    server.setThreadNum(threadNum);

    // TLS
    std::string useTls;
    std::cout << "Use TLS/SSL? (y/n) [n]: ";
    std::getline(std::cin, useTls);
    if (useTls == "y" || useTls == "Y") {
        SimpleCommKitTlsSetting tls;
        std::cout << "  Server cert file (PEM): ";
        std::getline(std::cin, input);
        tls.crt_file = trim(input);

        std::cout << "  Server key file (PEM): ";
        std::getline(std::cin, input);
        tls.key_file = trim(input);

        std::string verify;
        std::cout << "  Verify client cert? (y/n) [n]: ";
        std::getline(std::cin, verify);
        if (verify == "y" || verify == "Y") {
            tls.verify_peer = true;
            std::cout << "  CA cert file (for verifying client): ";
            std::getline(std::cin, input);
            tls.ca_file = trim(input);
        }

        if (server.enableTls(tls)) {
            std::cout << "TLS enabled." << std::endl;
        } else {
            std::cerr << "Warning: enableTls failed!" << std::endl;
        }
    }

    // ---------------------------------------------------------------
    // Start server
    // ---------------------------------------------------------------
    std::cout << "Starting server on " << host << ":" << port << " ..." << std::endl;
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
                          << "  list (l)           - list connected clients\n"
                          << "  send <id> <msg>    - send text to a client\n"
                          << "  sendb <id> <hex>   - send hex bytes to a client\n"
                          << "  broadcast (bc) <msg>- broadcast text to all clients\n"
                          << "  broadcastb (bcb) <hex> - broadcast hex to all\n"
                          << "  info               - show server info\n"
                          << "  stop               - stop the server\n"
                          << "  quit (q)           - stop & exit\n";
            }
            else if (cmd == "list" || cmd == "l") {
                std::cout << "Connected clients: " << server.connectionNum() << std::endl;
            }
            else if (cmd == "info") {
                std::cout << "Server: " << server.host() << ":" << server.port() << std::endl;
                std::cout << "Clients: " << server.connectionNum() << std::endl;
                std::cout << "Running: " << (server.isRunning() ? "yes" : "no") << std::endl;
            }
            else if (cmd == "stop") {
                server.stop();
                std::cout << "Server stopped." << std::endl;
            }
            else if (cmd == "quit" || cmd == "q") {
                break;
            }
            else if (cmd.rfind("sendb ", 0) == 0) {
                // sendb <id> <hex...>
                auto space1 = cmd.find(' ', 6);
                if (space1 == std::string::npos) {
                    std::cerr << "Usage: sendb <id> <hex bytes...>" << std::endl;
                    continue;
                }
                uint32_t cid = std::stoul(cmd.substr(6, space1 - 6));
                std::string hexData = cmd.substr(space1 + 1);
                auto bytes = parseHex(hexData);
                int ret = server.sendTo(cid, bytes);
                std::cout << "Sent " << bytes.size() << " bytes -> client " << cid
                          << " (ret=" << ret << ")" << std::endl;
            }
            else if (cmd.rfind("send ", 0) == 0) {
                // send <id> <msg>
                auto space1 = cmd.find(' ', 5);
                if (space1 == std::string::npos) {
                    std::cerr << "Usage: send <id> <message>" << std::endl;
                    continue;
                }
                uint32_t cid = std::stoul(cmd.substr(5, space1 - 5));
                std::string msg = cmd.substr(space1 + 1);
                int ret = server.sendTo(cid, msg);
                std::cout << "Sent to client " << cid << " (ret=" << ret << ")" << std::endl;
            }
            else if (cmd.rfind("bcb ", 0) == 0 || cmd.rfind("broadcastb ", 0) == 0) {
                std::string hexData = cmd.rfind("bcb ", 0) == 0
                    ? cmd.substr(4) : cmd.substr(11);
                auto bytes = parseHex(hexData);
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
