#pragma once

#include <fastmcpp/app.hpp>

namespace SimpleCommKitAiMqttClientFastmcpp
{

/// Register all MQTT MCP tools on the given FastMCP application instance.
/// Mirrors the Python `mcp.py` tool decorators in SimpleCommKitAiMqttClient.
void register_tools(fastmcpp::FastMCP& app);

} // namespace SimpleCommKitAiMqttClientFastmcpp
