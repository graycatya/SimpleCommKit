#pragma once

#include <fastmcpp/app.hpp>

namespace SimpleCommKitAiUdpClientFastmcpp
{

/// Register all 5 UDP client MCP tools on the given FastMCP application instance.
/// Mirrors the Python `mcp.py` tool decorators in SimpleCommKitAiUdpClient.
void register_tools(fastmcpp::FastMCP& app);

} // namespace SimpleCommKitAiUdpClientFastmcpp
