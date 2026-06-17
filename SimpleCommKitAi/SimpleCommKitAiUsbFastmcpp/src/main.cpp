/// SimpleCommKitAiUsbFastmcpp - C++ MCP server for USB devices
///
/// Exposes 23 MCP tools for USB device enumeration, open/close, control/bulk/
/// interrupt/isochronous transfers, interface management, read-poll, hotplug,
/// and endpoint discovery.
/// Supports stdio, http, sse, and streamable-http transports.
///
/// Usage:
///   simplecommkitaiusb-fastmcpp                                  # stdio (default)
///   simplecommkitaiusb-fastmcpp --transport sse --port 8007      # SSE on port 8007
///   simplecommkitaiusb-fastmcpp --transport streamable-http      # Streamable HTTP
///   simplecommkitaiusb-fastmcpp --transport http --port 8007    # HTTP on port 8007

#include "usb_state.hpp"
#include "usb_tools.hpp"

#include <fastmcpp/app.hpp>
#include <fastmcpp/mcp/handler.hpp>
#include <fastmcpp/server/http_server.hpp>
#include <fastmcpp/server/sse_server.hpp>
#include <fastmcpp/server/stdio_server.hpp>
#include <fastmcpp/server/streamable_http_server.hpp>

#include <atomic>
#include <csignal>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

using namespace SimpleCommKitAiUsbFastmcpp;

static std::atomic<bool> g_running{true};

static void signal_handler(int)
{
    g_running = false;
}

static void print_usage(const char* prog)
{
    std::cout << "Usage: " << prog << " [OPTIONS]\n\n"
              << "Options:\n"
              << "  --transport TRANSPORT   Transport protocol (default: stdio)\n"
              << "                          Choices: stdio, http, sse, streamable-http\n"
              << "  --host HOST             Host to bind to (default: 127.0.0.1)\n"
              << "  --port PORT             Port to bind to (default: 8007)\n"
              << "  --help, -h              Show this help message\n"
              << std::endl;
}

int main(int argc, char* argv[])
{
    // -----------------------------------------------------------------------
    // Parse command line
    // -----------------------------------------------------------------------
    std::string transport = "stdio";
    std::string host      = "127.0.0.1";
    int port              = 8007;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if ((arg == "--transport" || arg == "-t") && i + 1 < argc)
        {
            transport = argv[++i];
        }
        else if ((arg == "--host" || arg == "-H") && i + 1 < argc)
        {
            host = argv[++i];
        }
        else if ((arg == "--port" || arg == "-p") && i + 1 < argc)
        {
            port = std::stoi(argv[++i]);
        }
        else if (arg == "--help" || arg == "-h")
        {
            print_usage(argv[0]);
            return 0;
        }
        else
        {
            std::cerr << "Unknown option: " << arg << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }

    // -----------------------------------------------------------------------
    // Build FastMCP app and register tools
    // -----------------------------------------------------------------------
    fastmcpp::FastMCP app("SimpleCommKitAiUsbFastmcpp MCP Server", "1.0.0",
                          std::nullopt, std::nullopt, std::nullopt,
                          std::vector<std::shared_ptr<fastmcpp::providers::Provider>>{});

    register_tools(app);

    // -----------------------------------------------------------------------
    // Create handler and start server
    // -----------------------------------------------------------------------
    auto handler = fastmcpp::mcp::make_mcp_handler(app);

    if (transport == "http")
    {
        std::cerr << "[SimpleCommKitAiUsbFastmcpp] Starting HTTP server on "
                  << host << ":" << port << std::endl;

        fastmcpp::server::HttpServerWrapper server(
            std::make_shared<fastmcpp::server::Server>(app.server()), host, port);

        if (!server.start())
        {
            std::cerr << "Failed to start HTTP server" << std::endl;
            return 1;
        }

        std::cerr << "HTTP MCP server running at http://" << host << ":" << port
                  << std::endl;
        std::cerr << "Press Ctrl+C to stop." << std::endl;

        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        while (g_running)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Cleanup all USB devices before stopping
        UsbState::instance().remove_all_devices();
        server.stop();
    }
    else if (transport == "sse")
    {
        std::cerr << "[SimpleCommKitAiUsbFastmcpp] Starting SSE server on "
                  << host << ":" << port << std::endl;

        fastmcpp::server::SseServerWrapper server(handler, host, port, "/sse",
                                                  "/messages");

        if (!server.start())
        {
            std::cerr << "Failed to start SSE server" << std::endl;
            return 1;
        }

        std::cerr << "SSE MCP server running at http://" << host << ":" << port
                  << "/sse" << std::endl;
        std::cerr << "Press Ctrl+C to stop." << std::endl;

        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        while (g_running)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Cleanup all USB devices before stopping
        UsbState::instance().remove_all_devices();
        server.stop();
    }
    else if (transport == "streamable-http")
    {
        std::cerr << "[SimpleCommKitAiUsbFastmcpp] Starting Streamable HTTP server on "
                  << host << ":" << port << std::endl;

        fastmcpp::server::StreamableHttpServerWrapper server(handler, host, port,
                                                             "/mcp");

        if (!server.start())
        {
            std::cerr << "Failed to start Streamable HTTP server" << std::endl;
            return 1;
        }

        std::cerr << "Streamable HTTP MCP server running at http://" << host
                  << ":" << port << "/mcp" << std::endl;
        std::cerr << "Press Ctrl+C to stop." << std::endl;

        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        while (g_running)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Cleanup all USB devices before stopping
        UsbState::instance().remove_all_devices();
        server.stop();
    }
    else
    {
        // Default: stdio
        std::cerr << "[SimpleCommKitAiUsbFastmcpp] Starting STDIO MCP server..." << std::endl;
        std::cerr << "Send JSON-RPC requests via stdin (one per line)." << std::endl;
        std::cerr << "Press Ctrl+D (Unix) or Ctrl+Z (Windows) to exit." << std::endl;

        fastmcpp::server::StdioServerWrapper server(handler);
        server.run(); // Blocking
    }

    std::cerr << "[SimpleCommKitAiUsbFastmcpp] Server stopped." << std::endl;
    return 0;
}
