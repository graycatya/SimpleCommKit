#pragma once

#include <fastmcpp/app.hpp>

namespace SimpleCommKitAiUdpServerFastmcpp
{

/// Register all 6 UDP server MCP tools on the given FastMCP application instance.
/// Mirrors the Python `mcp.py` tool decorators in SimpleCommKitAiUdpServer.
void register_tools(fastmcpp::FastMCP& app);

} // namespace SimpleCommKitAiUdpServerFastmcpp
