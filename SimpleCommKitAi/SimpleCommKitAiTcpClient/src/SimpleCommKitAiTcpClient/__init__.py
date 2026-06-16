"""
SimpleCommKitAiTcpClient - AI-friendly TCP client toolkit powered by SimpleCommKitPyTcp

Exposes TCP client operations as MCP tools and REST API for AI agents and scripts.
"""

from SimpleCommKitPyTcp import __version__

try:
    from SimpleCommKitPyTcp import (
        TcpClient,
        TcpReconnectSetting,
        TlsSetting,
        get_error_description,
    )
except ImportError:
    TcpClient = None       # type: ignore
    TcpReconnectSetting = None  # type: ignore
    TlsSetting = None      # type: ignore
    get_error_description = lambda code: f"Unknown error {code}"  # type: ignore

__all__ = [
    "__version__",
    "TcpClient",
    "TcpReconnectSetting",
    "TlsSetting",
    "get_error_description",
]
