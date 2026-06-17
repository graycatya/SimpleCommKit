#pragma once

#include <fastmcpp/app.hpp>

namespace SimpleCommKitAiUsbFastmcpp
{

/// Register all 23 USB MCP tools on the given FastMCP application instance.
/// Mirrors the Python `mcp.py` tool decorators in SimpleCommKitAiUsb.
void register_tools(fastmcpp::FastMCP& app);

} // namespace SimpleCommKitAiUsbFastmcpp
