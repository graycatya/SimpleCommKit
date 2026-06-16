"""SimpleCommKitAiWebSocketClient MCP Server - Expose WebSocket client operations as MCP tools."""

from fastmcp import FastMCP
import argparse
from typing import Dict, List, Optional
import threading

from SimpleCommKitAiWebSocketClient import WebSocketClient, WsReconnectSetting, WsTlsSetting, get_error_description


class WsClientState:
    def __init__(self) -> None:
        self.client: Optional[WebSocketClient] = None
        self.messages: List[Dict] = []
        self._lock = threading.Lock()

    def ensure_client(self) -> WebSocketClient:
        if self.client is None:
            self.client = WebSocketClient()
            self.client.set_callback_on_error(lambda code: print(
                f"[SimpleCommKitAiWebSocketClient] Error {code}: {get_error_description(code)}"))
            self.client.set_callback_on_open(
                lambda: print("[SimpleCommKitAiWebSocketClient] Connected"))
            self.client.set_callback_on_close(
                lambda: print("[SimpleCommKitAiWebSocketClient] Disconnected"))
            self.client.set_callback_on_message(lambda data: self._store_message(data))
        return self.client

    def _store_message(self, data: bytes) -> None:
        with self._lock:
            self.messages.append({"data_hex": data.hex(), "data_utf8": data.decode("utf-8", errors="ignore")})


state = WsClientState()
mcp = FastMCP(name="SimpleCommKitAiWebSocketClient MCP Server")


@mcp.tool(name="ws_connect", description="Connect WebSocket client to a URL (ws:// or wss://).",
          tags={"websocket", "client", "connect"},
          annotations={"title": "WebSocket Connect", "readOnlyHint": False},
          meta={"version": "1.0", "role": "connection"})
def ws_connect(url: str) -> Dict[str, str]:
    client = state.ensure_client()
    try:
        result = client.open(url)
    except Exception as exc:
        raise RuntimeError(f"Connect failed: {exc}") from exc
    return {"message": f"Connected to {url}" if result else "Connect failed", "url": url}


@mcp.tool(name="ws_disconnect", description="Close the WebSocket connection.",
          tags={"websocket", "client", "disconnect"},
          annotations={"title": "WebSocket Disconnect", "readOnlyHint": False},
          meta={"version": "1.0", "role": "connection"})
def ws_disconnect() -> Dict[str, str]:
    if state.client is None:
        raise RuntimeError("Client not initialized")
    try:
        state.client.close()
    except Exception as exc:
        raise RuntimeError(f"Disconnect failed: {exc}") from exc
    return {"message": "Disconnected"}


@mcp.tool(name="ws_status", description="Check whether the WebSocket client is connected.",
          tags={"websocket", "client", "read", "status"},
          annotations={"title": "WebSocket Status", "readOnlyHint": True, "idempotentHint": True},
          meta={"version": "1.0", "role": "status"})
def ws_status() -> Dict[str, bool]:
    if state.client is None:
        return {"connected": False}
    return {"connected": state.client.is_connected()}


@mcp.tool(name="ws_send", description="Send data through the WebSocket connection. Data can be text or hex-encoded bytes.",
          tags={"websocket", "client", "send"},
          annotations={"title": "WebSocket Send", "readOnlyHint": False},
          meta={"version": "1.0", "role": "data"})
def ws_send(data: str, is_hex: bool = False) -> Dict:
    if state.client is None or not state.client.is_connected():
        raise RuntimeError("Client not connected")
    try:
        if is_hex:
            payload = bytes.fromhex(data)
            result = state.client.send(payload)
        else:
            result = state.client.send_text(data)
    except ValueError as exc:
        raise RuntimeError(f"Invalid hex string: {exc}") from exc
    except Exception as exc:
        raise RuntimeError(f"Send failed: {exc}") from exc
    return {"bytes_sent": result}


@mcp.tool(name="ws_get_messages", description="Retrieve and clear all buffered messages.",
          tags={"websocket", "client", "read"},
          annotations={"title": "WebSocket Get Messages", "readOnlyHint": False},
          meta={"version": "1.0", "role": "data"})
def ws_get_messages() -> List[Dict[str, str]]:
    with state._lock:
        msgs = state.messages[:]
        state.messages.clear()
    return msgs


@mcp.tool(name="ws_set_reconnect", description="Configure automatic reconnection for the WebSocket client.",
          tags={"websocket", "client", "config"},
          annotations={"title": "WebSocket Set Reconnect", "readOnlyHint": False},
          meta={"version": "1.0", "role": "config"})
def ws_set_reconnect(min_delay_ms: int = 1000, max_delay_ms: int = 10000,
                     delay_policy: int = 2, max_retry_cnt: int = 0) -> Dict[str, str]:
    client = state.ensure_client()
    setting = WsReconnectSetting()
    setting.min_delay_ms = min_delay_ms
    setting.max_delay_ms = max_delay_ms
    setting.delay_policy = delay_policy
    setting.max_retry_cnt = max_retry_cnt
    try:
        client.set_reconnect(setting)
    except Exception as exc:
        raise RuntimeError(f"Set reconnect failed: {exc}") from exc
    return {"message": "Reconnect configured"}


@mcp.tool(name="ws_set_ping_interval", description="Set the WebSocket ping interval in milliseconds.",
          tags={"websocket", "client", "config"},
          annotations={"title": "WebSocket Set Ping Interval", "readOnlyHint": False},
          meta={"version": "1.0", "role": "config"})
def ws_set_ping_interval(ms: int) -> Dict[str, str]:
    client = state.ensure_client()
    try:
        client.set_ping_interval(ms)
    except Exception as exc:
        raise RuntimeError(f"Set ping interval failed: {exc}") from exc
    return {"message": f"Ping interval set to {ms}ms"}


def main() -> None:
    parser = argparse.ArgumentParser(description="SimpleCommKitAiWebSocketClient MCP Server")
    parser.add_argument("--transport", default="stdio",
                        choices=["stdio", "http", "sse", "streamable-http"])
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8003)
    args = parser.parse_args()
    if args.transport in ("http", "sse", "streamable-http"):
        mcp.run(transport=args.transport, host=args.host, port=args.port)
    else:
        mcp.run()


if __name__ == "__main__":
    main()
