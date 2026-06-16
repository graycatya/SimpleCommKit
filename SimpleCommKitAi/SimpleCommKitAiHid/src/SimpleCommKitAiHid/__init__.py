"""
SimpleCommKitAiHid - AI-friendly HID toolkit powered by SimpleCommKitPyHid

Exposes HID operations as MCP tools and REST API for AI agents and scripts.

Provides:
- MCP Server: HID operations as MCP tools for AI clients (Cursor, Claude Code, etc.)
- HTTP Server: REST API for remote HID device control
"""

# Re-export core HID types from SimpleCommKitPyHid for convenience

from SimpleCommKitPyHid import __version__

try:
    from SimpleCommKitPyHid import (
        SimpleCommKitHid,
        HidDeviceInfo,
        HidBusType,
        get_error_description,
    )
except ImportError:
    # Allow import without SimpleCommKitPyHid for testing/development
    SimpleCommKitHid = None       # type: ignore
    HidDeviceInfo = None          # type: ignore
    HidBusType = None             # type: ignore
    get_error_description = lambda code: f"Unknown error {code}"  # type: ignore

__all__ = [
    "__version__",
    "SimpleCommKitHid",
    "HidDeviceInfo",
    "HidBusType",
    "get_error_description",
]
