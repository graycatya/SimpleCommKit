#pragma once

#include <fastmcpp/app.hpp>

namespace SimpleCommKitAiWebSocketClientFastmcpp
{

/// Register all 8 WebSocket client MCP tools on the given FastMCP application instance.
/// Mirrors the Python MCP tools in SimpleCommKitAiWebSocketClient.
void register_tools(fastmcpp::FastMCP& app);

} // namespace SimpleCommKitAiWebSocketClientFastmcpp
