"""
SimpleCommKitAiSerialPort - AI-friendly Serial Port toolkit powered by simple_comm_kit_serialport

Exposes serial port operations as MCP tools and REST API for AI agents and scripts.

Provides:
- MCP Server: SerialPort operations as MCP tools for AI clients (Cursor, Claude Code, etc.)
- HTTP Server: REST API for remote serial port control
"""

# Re-export core SerialPort types from simple_comm_kit_serialport for convenience

from SimpleCommKitPySerialPort import __version__

try:
    from SimpleCommKitPySerialPort import (
        SerialPort,
        SerialPortInfo,
        BaudRate,
        Parity,
        DataBits,
        StopBits,
        FlowControl,
        get_error_description,
    )
except ImportError:
    # Allow import without SimpleCommKitPySerialPort for testing/development
    SerialPort = None           # type: ignore
    SerialPortInfo = None       # type: ignore
    BaudRate = None             # type: ignore
    Parity = None               # type: ignore
    DataBits = None             # type: ignore
    StopBits = None             # type: ignore
    FlowControl = None          # type: ignore
    get_error_description = lambda code: f"Unknown error {code}"  # type: ignore

__all__ = [
    "__version__",
    "SerialPort",
    "SerialPortInfo",
    "BaudRate",
    "Parity",
    "DataBits",
    "StopBits",
    "FlowControl",
    "get_error_description",
]
