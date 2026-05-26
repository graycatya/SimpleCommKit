"""
SimpleCommKitAiBle - AI-friendly BLE toolkit powered by SimpleCommKitPyBle

Exposes BLE operations as MCP tools and REST API for AI agents and scripts.

Provides:
- MCP Server: BleCentral operations as MCP tools for AI clients (Cursor, Claude Code, etc.)
- HTTP Server: REST API for remote BLE device control
"""

__version__ = "0.1.0"

# Re-export core BLE types from SimpleCommKitPyBle for convenience
try:
    from SimpleCommKitPyBle import (
        BleCentral,
        Adapter,
        Peripheral,
        get_error_description,
    )
except ImportError:
    # Allow import without SimpleCommKitPyBle for testing/development
    BleCentral = None       # type: ignore
    Adapter = None          # type: ignore
    Peripheral = None       # type: ignore
    get_error_description = lambda code: f"Unknown error {code}"  # type: ignore

__all__ = [
    "__version__",
    "BleCentral",
    "Adapter",
    "Peripheral",
    "get_error_description",
]
