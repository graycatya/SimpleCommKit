#pragma once

#include <fastmcpp/app.hpp>

namespace SimpleCommKitAiBleFastmcpp
{

/// Register all 16 BLE MCP tools on the given FastMCP application instance.
/// Mirrors the Python `mcp.py` tool decorators in SimpleCommKitAiBle.
void register_tools(fastmcpp::FastMCP& app);

} // namespace SimpleCommKitAiBleFastmcpp
