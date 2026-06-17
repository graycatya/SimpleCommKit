#pragma once

#include <fastmcpp/app.hpp>

namespace SimpleCommKitAiSerialPortFastmcpp
{

/// Register all 12+ serial-port MCP tools on the given FastMCP application instance.
/// Mirrors the Python `mcp.py` tool decorators in SimpleCommKitAiSerialPort.
void register_tools(fastmcpp::FastMCP& app);

} // namespace SimpleCommKitAiSerialPortFastmcpp
