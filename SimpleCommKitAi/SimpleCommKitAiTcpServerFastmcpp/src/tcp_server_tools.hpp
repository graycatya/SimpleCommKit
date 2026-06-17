#pragma once

#include <fastmcpp/app.hpp>

namespace SimpleCommKitAiTcpServerFastmcpp
{

/// Register all 6 TCP server MCP tools on the given FastMCP application instance.
/// Mirrors the Python `mcp.py` tool decorators in SimpleCommKitAiTcpServer.
void register_tools(fastmcpp::FastMCP& app);

} // namespace SimpleCommKitAiTcpServerFastmcpp
