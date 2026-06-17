#pragma once

#include <fastmcpp/app.hpp>

namespace SimpleCommKitAiWebSocketServerFastmcpp
{

/// Register all 8 WebSocket server MCP tools on the given FastMCP application instance.
/// Mirrors the Python MCP tools in SimpleCommKitAiWebSocketServer.
void register_tools(fastmcpp::FastMCP& app);

} // namespace SimpleCommKitAiWebSocketServerFastmcpp
