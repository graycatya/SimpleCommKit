"""
SimpleCommKitAiWebSocketServer - AI-friendly WebSocket server toolkit powered by SimpleCommKitPyWebSocket
"""

from SimpleCommKitPyWebSocket import __version__

try:
    from SimpleCommKitPyWebSocket import WebSocketServer, WsTlsSetting, get_error_description
except ImportError:
    WebSocketServer = None
    WsTlsSetting = None
    get_error_description = lambda code: f"Unknown error {code}"

__all__ = ["__version__", "WebSocketServer", "WsTlsSetting", "get_error_description"]
