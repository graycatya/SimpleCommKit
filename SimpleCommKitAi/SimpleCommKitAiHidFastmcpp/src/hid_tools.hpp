#pragma once

#include <fastmcpp/app.hpp>

namespace SimpleCommKitAiHidFastmcpp
{

/// Register all 11 HID MCP tools on the given FastMCP application instance.
/// Mirrors the Python `mcp.py` tool decorators in SimpleCommKitAiHid.
void register_tools(fastmcpp::FastMCP& app);

} // namespace SimpleCommKitAiHidFastmcpp
