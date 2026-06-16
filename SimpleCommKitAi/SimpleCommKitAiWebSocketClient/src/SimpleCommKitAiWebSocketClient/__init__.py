"""
SimpleCommKitAiWebSocketClient - AI-friendly WebSocket client toolkit powered by SimpleCommKitPyWebSocket
"""

from SimpleCommKitPyWebSocket import __version__

try:
    from SimpleCommKitPyWebSocket import (
        WebSocketClient, WsReconnectSetting, WsTlsSetting, get_error_description,
    )
except ImportError:
    WebSocketClient = None
    WsReconnectSetting = None
    WsTlsSetting = None
    get_error_description = lambda code: f"Unknown error {code}"

__all__ = ["__version__", "WebSocketClient", "WsReconnectSetting", "WsTlsSetting", "get_error_description"]
