#include <SimpleCommKitWebSocket.h>
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

int main() {
    SimpleCommKitWebSocketClient client;

    // ---------------------------------------------------------------
    // Register callbacks
    // ---------------------------------------------------------------
    client.setCallback_OnOpen([]() {
        std::cout << "[+] WebSocket opened." << std::endl;
    });
    client.setCallback_OnClose([]() {
        std::cout << "[-] WebSocket closed." << std::endl;
    });
    client.setCallback_OnMessage([](const std::vector<uint8_t>& data) {
        std::cout << "[Msg] len=" << data.size() << "  hex=";
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
    client.setCallback_OnError([](ErrorCode error) {
        std::cerr << "[Error] " << SimpleCommKitErrorMap::GetErrorDescription(error)
                  << " (code: 0x" << std::hex << error << std::dec << ")" << std::endl;
    });

    // ---------------------------------------------------------------
    // Connection setup
    // ---------------------------------------------------------------
    std::string url = "ws://127.0.0.1:9090";

    std::cout << "=== SimpleCommKit WebSocket Client ===" << std::endl;
    std::cout << "(url format: ws://host:port/path  or  wss://host:port/path)" << std::endl;

    std::string input;
    std::cout << "Server URL [default " << url << "]: ";
    std::getline(std::cin, input);
    if (!input.empty()) { url = trim(input); }

    // Timeout
    int timeout = 3000;
    std::cout << "Connect timeout (ms) [default " << timeout << "]: ";
    std::getline(std::cin, input);
    if (!input.empty()) { timeout = std::stoi(input); }
    client.setConnectTimeout(timeout);

    // TLS (for custom certs; wss:// already auto-enables TLS)
    std::string useTls;
    std::cout << "Use explicit TLS certs? (y/n) [n]: ";
    std::getline(std::cin, useTls);
    if (useTls == "y" || useTls == "Y") {
        SimpleCommKitWebSocketTlsSetting tls;
        std::cout << "  CA cert file (for verifying server, empty=none): ";
        std::getline(std::cin, input);
        if (!input.empty()) tls.ca_file = trim(input);

        std::cout << "  Client cert file (empty=none): ";
        std::getline(std::cin, input);
        if (!input.empty()) tls.crt_file = trim(input);

        std::cout << "  Client key file (empty=none): ";
        std::getline(std::cin, input);
        if (!input.empty()) tls.key_file = trim(input);

        std::string verify;
        std::cout << "  Verify server cert? (y/n) [n]: ";
        std::getline(std::cin, verify);
        tls.verify_peer = (verify == "y" || verify == "Y");

        if (client.enableTls(tls)) {
            std::cout << "TLS enabled (custom certs)." << std::endl;
        } else {
            std::cerr << "Warning: enableTls failed!" << std::endl;
        }
    }

    // Reconnect
    std::string useReconnect;
    std::cout << "Enable auto-reconnect? (y/n) [n]: ";
    std::getline(std::cin, useReconnect);
    if (useReconnect == "y" || useReconnect == "Y") {
        SimpleCommKitWebSocketReconnectSetting reconn;
        reconn.min_delay_ms = 1000;
        reconn.max_delay_ms = 10000;
        reconn.delay_policy = 2;   // exponential backoff
        reconn.max_retry_cnt = 0;  // unlimited
        client.setReconnect(reconn);
        std::cout << "Auto-reconnect enabled." << std::endl;
    }

    // Ping interval
    int pingMs = 0;
    std::cout << "Ping interval (ms, 0=disable) [default " << pingMs << "]: ";
    std::getline(std::cin, input);
    if (!input.empty()) { pingMs = std::stoi(input); }
    if (pingMs > 0) {
        client.setPingInterval(pingMs);
        std::cout << "Ping interval set to " << pingMs << " ms." << std::endl;
    }

    // ---------------------------------------------------------------
    // Connect
    // ---------------------------------------------------------------
    std::cout << "Connecting to " << url << " ..." << std::endl;
    if (!client.open(url)) {
        std::cerr << "Failed to initiate connection!" << std::endl;
        return 1;
    }
    // Give the async connect a moment to settle
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "Connection status: " << (client.isConnected() ? "connected" : "pending...") << std::endl;
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
                          << "  send (s) <msg>     - send text message\n"
                          << "  sendb (sb) <hex>   - send hex bytes (e.g. 48 65 6C 6C 6F)\n"
                          << "  status (st)        - show connection status\n"
                          << "  reopen             - close & reconnect\n"
                          << "  quit (q)           - close & exit\n";
            }
            else if (cmd == "status" || cmd == "st") {
                std::cout << "Connected: " << (client.isConnected() ? "yes" : "no") << std::endl;
            }
            else if (cmd == "reopen") {
                client.close();
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                std::cout << "Reconnecting to " << url << " ..." << std::endl;
                if (!client.open(url)) {
                    std::cerr << "Failed to reconnect!" << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                std::cout << "Connected: " << (client.isConnected() ? "yes" : "no") << std::endl;
            }
            else if (cmd == "quit" || cmd == "q") {
                break;
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
                std::cout << "Sent " << bytes.size() << " bytes (ret=" << ret << ")" << std::endl;
            }
            else if (cmd.rfind("send ", 0) == 0 || cmd.rfind("s ", 0) == 0) {
                std::string msg = cmd.rfind("send ", 0) == 0
                    ? cmd.substr(5) : cmd.substr(2);
                if (msg.empty()) {
                    std::cerr << "Usage: send <message>" << std::endl;
                    continue;
                }
                int ret = client.send(msg);
                std::cout << "Sent (ret=" << ret << ")" << std::endl;
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
