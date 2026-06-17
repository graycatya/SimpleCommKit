#pragma once

#include <fastmcpp/app.hpp>

namespace SimpleCommKitAiTcpClientFastmcpp
{

/// Register all 6 TCP client MCP tools on the given FastMCP application instance.
/// Mirrors the Python `mcp.py` tool decorators in SimpleCommKitAiTcpClient.
void register_tools(fastmcpp::FastMCP& app);

} // namespace SimpleCommKitAiTcpClientFastmcpp
