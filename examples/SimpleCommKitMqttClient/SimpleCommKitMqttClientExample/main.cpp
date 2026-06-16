#include <SimpleCommKitMqttClient.h>
#include "SimpleCommKitErrorMap.hpp"
#include "SimpleCommKitTestUtils.hpp"

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
    SimpleCommKitMqttClient client;

    // ---------------------------------------------------------------
    // Register callbacks
    // ---------------------------------------------------------------
    client.setCallback_OnConnected([]() {
        std::cout << "[+] Connected to broker (CONNACK received)." << std::endl;
    });
    client.setCallback_OnDisconnected([]() {
        std::cout << "[-] Disconnected from broker." << std::endl;
    });
    client.setCallback_OnMessage([](const std::string& topic, const std::vector<uint8_t>& data) {
        std::cout << "[Msg] topic=\"" << topic << "\"  len=" << data.size() << "  hex=";
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
    // Broker connection setup
    // ---------------------------------------------------------------
    std::string host = "127.0.0.1";
    int port = 1883;

    std::cout << "=== SimpleCommKit MQTT Client ===" << std::endl;

    // Client ID
    std::string clientId;
    std::cout << "Client ID (empty=auto): ";
    std::getline(std::cin, clientId);
    clientId = trim(clientId);
    if (!clientId.empty()) {
        client.setClientId(clientId);
    }

    // Broker host/port
    std::string input;
    std::cout << "Broker host [default " << host << "]: ";
    std::getline(std::cin, input);
    if (!input.empty()) { host = trim(input); }

    std::cout << "Broker port [default " << port << "]: ";
    std::getline(std::cin, input);
    if (!input.empty()) { port = std::stoi(input); }

    // Authentication
    std::string useAuth;
    std::cout << "Use authentication? (y/n) [n]: ";
    std::getline(std::cin, useAuth);
    if (useAuth == "y" || useAuth == "Y") {
        std::string username, password;
        std::cout << "  Username: ";
        std::getline(std::cin, username);
        std::cout << "  Password: ";
        std::getline(std::cin, password);
        client.setAuth(trim(username), trim(password));
    }

    // Will message
    std::string useWill;
    std::cout << "Set Last Will? (y/n) [n]: ";
    std::getline(std::cin, useWill);
    if (useWill == "y" || useWill == "Y") {
        SimpleCommKitMqttWillMessage will;
        std::cout << "  Will topic: ";
        std::getline(std::cin, input);
        will.topic = trim(input);

        std::cout << "  Will payload (text): ";
        std::getline(std::cin, input);
        auto str = trim(input);
        will.payload.assign(str.begin(), str.end());

        std::cout << "  Will QoS (0/1/2) [0]: ";
        std::getline(std::cin, input);
        if (!input.empty()) { will.qos = std::stoi(input); }

        std::string retain;
        std::cout << "  Will retain? (y/n) [n]: ";
        std::getline(std::cin, retain);
        will.retain = (retain == "y" || retain == "Y");

        client.setWill(will);
        std::cout << "Will set on topic \"" << will.topic << "\"" << std::endl;
    }

    // Ping / keep-alive interval
    std::cout << "Ping interval (seconds) [default 60]: ";
    std::getline(std::cin, input);
    if (!input.empty()) { client.setPingInterval(std::stoi(input)); }

    // Connect timeout
    int timeout = 5000;
    std::cout << "Connect timeout (ms) [default " << timeout << "]: ";
    std::getline(std::cin, input);
    if (!input.empty()) { timeout = std::stoi(input); }
    client.setConnectTimeout(timeout);

    // TLS
    std::string useTls;
    std::cout << "Use TLS/SSL? (y/n) [n]: ";
    std::getline(std::cin, useTls);
    bool ssl = false;
    if (useTls == "y" || useTls == "Y") {
        ssl = true;
        std::string tlsMode;
        std::cout << "  (p)lain TLS (no custom certs)  or  (c)ustom certificates? [p]: ";
        std::getline(std::cin, tlsMode);

        if (tlsMode == "c" || tlsMode == "C") {
            SimpleCommKitMqttTlsSetting tls;
            std::cout << "  CA cert file (for verifying broker, empty=none): ";
            std::getline(std::cin, input);
            if (!input.empty()) tls.ca_file = trim(input);

            std::cout << "  Client cert file (empty=none): ";
            std::getline(std::cin, input);
            if (!input.empty()) tls.crt_file = trim(input);

            std::cout << "  Client key file (empty=none): ";
            std::getline(std::cin, input);
            if (!input.empty()) tls.key_file = trim(input);

            std::string verify;
            std::cout << "  Verify broker cert? (y/n) [n]: ";
            std::getline(std::cin, verify);
            tls.verify_peer = (verify == "y" || verify == "Y");

            if (client.enableTls(tls)) {
                std::cout << "TLS enabled (custom certs)." << std::endl;
            } else {
                std::cerr << "Warning: enableTls failed!" << std::endl;
            }
        } else {
            if (client.enableTls()) {
                std::cout << "TLS enabled (platform default)." << std::endl;
            } else {
                std::cerr << "Warning: enableTls failed!" << std::endl;
            }
        }
    }

    // Reconnect
    std::string useReconnect;
    std::cout << "Enable auto-reconnect? (y/n) [n]: ";
    std::getline(std::cin, useReconnect);
    if (useReconnect == "y" || useReconnect == "Y") {
        SimpleCommKitMqttReconnectSetting reconn;
        reconn.min_delay_ms = 1000;
        reconn.max_delay_ms = 10000;
        reconn.delay_policy = 2;   // exponential backoff
        reconn.max_retry_cnt = 0;  // unlimited
        client.setReconnect(reconn);
        std::cout << "Auto-reconnect enabled." << std::endl;
    }

    // ---------------------------------------------------------------
    // Connect to broker
    // ---------------------------------------------------------------
    std::cout << "Connecting to MQTT broker at " << host << ":" << port
              << (ssl ? " (SSL)" : "") << " ..." << std::endl;
    bool connected;
    if (ssl) {
        connected = client.connectSsl(host, port);
    } else {
        connected = client.connect(host, port);
    }
    if (!connected) {
        std::cerr << "Failed to initiate connection!" << std::endl;
        return 1;
    }
    // Give CONNACK a moment to arrive
    std::this_thread::sleep_for(std::chrono::milliseconds(800));

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
                          << "  pub <topic> <msg>         - publish text message\n"
                          << "  pubb <topic> <hex...>     - publish hex bytes\n"
                          << "  pubr <topic> <msg>       - publish with retain\n"
                          << "  sub <topic> [qos]        - subscribe to a topic\n"
                          << "  unsub <topic>            - unsubscribe from a topic\n"
                          << "  status (st)              - show connection status\n"
                          << "  reconnect (rc)           - disconnect & reconnect\n"
                          << "  disconnect (dc)          - disconnect from broker\n"
                          << "  quit (q)                 - disconnect & exit\n";
            }
            else if (cmd == "status" || cmd == "st") {
                std::cout << "Connected: " << (client.isConnected() ? "yes" : "no") << std::endl;
            }
            else if (cmd == "disconnect" || cmd == "dc") {
                client.disconnect();
                std::cout << "Disconnecting..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                std::cout << "Connected: " << (client.isConnected() ? "yes" : "no") << std::endl;
            }
            else if (cmd == "reconnect" || cmd == "rc") {
                client.disconnect();
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                std::cout << "Reconnecting..." << std::endl;
                if (ssl) {
                    client.connectSsl(host, port);
                } else {
                    client.connect(host, port);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(800));
                std::cout << "Connected: " << (client.isConnected() ? "yes" : "no") << std::endl;
            }
            else if (cmd == "quit" || cmd == "q") {
                break;
            }
            // --- Publish (binary) ---
            else if (cmd.rfind("pubb ", 0) == 0) {
                // pubb <topic> <hex bytes...>
                std::istringstream iss(cmd.substr(5));
                std::string topic, hexPart;
                // Read topic (up to first space after a non-quoted token, or quoted)
                if (cmd[5] == '"') {
                    auto endQ = cmd.find('"', 6);
                    if (endQ == std::string::npos) {
                        std::cerr << "Unterminated quote in topic." << std::endl;
                        continue;
                    }
                    topic = cmd.substr(6, endQ - 6);
                    hexPart = trim(cmd.substr(endQ + 1));
                } else {
                    auto spacePos = cmd.find(' ', 5);
                    if (spacePos == std::string::npos) {
                        std::cerr << "Usage: pubb <topic> <hex bytes...>" << std::endl;
                        continue;
                    }
                    topic = cmd.substr(5, spacePos - 5);
                    hexPart = trim(cmd.substr(spacePos + 1));
                }
                if (topic.empty() || hexPart.empty()) {
                    std::cerr << "Usage: pubb <topic> <hex bytes...>" << std::endl;
                    continue;
                }
                auto bytes = parseHex(hexPart);
                if (bytes.empty()) {
                    std::cerr << "No valid hex bytes." << std::endl;
                    continue;
                }
                int ret = client.publish(topic, bytes, 0, false);
                std::cout << "Published " << bytes.size() << " bytes to \""
                          << topic << "\" (msg_id=" << ret << ")" << std::endl;
            }
            // --- Publish with retain ---
            else if (cmd.rfind("pubr ", 0) == 0) {
                auto spacePos = cmd.find(' ', 5);
                if (spacePos == std::string::npos) {
                    std::cerr << "Usage: pubr <topic> <message>" << std::endl;
                    continue;
                }
                std::string topic = cmd.substr(5, spacePos - 5);
                std::string msg = cmd.substr(spacePos + 1);
                if (topic.empty() || msg.empty()) {
                    std::cerr << "Usage: pubr <topic> <message>" << std::endl;
                    continue;
                }
                int ret = client.publish(topic, msg, 0, true);
                std::cout << "Published (retained) to \"" << topic
                          << "\" (msg_id=" << ret << ")" << std::endl;
            }
            // --- Publish (text) ---
            else if (cmd.rfind("pub ", 0) == 0) {
                auto spacePos = cmd.find(' ', 4);
                if (spacePos == std::string::npos) {
                    std::cerr << "Usage: pub <topic> <message>" << std::endl;
                    continue;
                }
                std::string topic = cmd.substr(4, spacePos - 4);
                std::string msg = cmd.substr(spacePos + 1);
                if (topic.empty() || msg.empty()) {
                    std::cerr << "Usage: pub <topic> <message>" << std::endl;
                    continue;
                }
                int ret = client.publish(topic, msg, 0, false);
                std::cout << "Published to \"" << topic
                          << "\" (msg_id=" << ret << ")" << std::endl;
            }
            // --- Subscribe ---
            else if (cmd.rfind("sub ", 0) == 0) {
                std::istringstream iss(cmd.substr(4));
                std::string topic;
                int qos = 0;
                iss >> topic;
                if (!iss.eof()) { iss >> qos; }
                if (topic.empty()) {
                    std::cerr << "Usage: sub <topic> [qos]" << std::endl;
                    continue;
                }
                int ret = client.subscribe(topic, qos);
                std::cout << "Subscribed to \"" << topic << "\" (qos=" << qos
                          << ", msg_id=" << ret << ")" << std::endl;
            }
            // --- Unsubscribe ---
            else if (cmd.rfind("unsub ", 0) == 0) {
                std::string topic = trim(cmd.substr(6));
                if (topic.empty()) {
                    std::cerr << "Usage: unsub <topic>" << std::endl;
                    continue;
                }
                int ret = client.unsubscribe(topic);
                std::cout << "Unsubscribed from \"" << topic
                          << "\" (msg_id=" << ret << ")" << std::endl;
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
    client.disconnect();
    std::cout << "MQTT client shut down. Goodbye." << std::endl;

    return 0;
}
