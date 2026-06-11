"""
SimpleCommKitAiUsb - AI-friendly USB toolkit powered by SimpleCommKitPyUsb

Exposes USB operations as MCP tools and REST API for AI agents and scripts.

Provides:
- MCP Server: USB operations as MCP tools for AI clients (Cursor, Claude Code, etc.)
- HTTP Server: REST API for remote USB device control
"""

__version__ = "0.1.0"

try:
    from SimpleCommKitPyUsb import (
        SimpleCommKitUsb,
        UsbDeviceInfo,
        get_error_description,
    )
except ImportError:
    SimpleCommKitUsb = None
    UsbDeviceInfo = None
    get_error_description = lambda code: f"Unknown error {code}"

__all__ = [
    "__version__",
    "SimpleCommKitUsb",
    "UsbDeviceInfo",
    "get_error_description",
]
