"""SimpleCommKitAiTcpClient MCP Server - Expose TCP client operations as MCP tools for AI agents."""

from fastmcp import FastMCP
import argparse
from typing import Dict, List, Optional
import threading

from SimpleCommKitAiTcpClient import TcpClient, TcpReconnectSetting, TlsSetting, get_error_description


class TcpClientState:
    """Holds global TCP client state shared across MCP tools."""

    def __init__(self) -> None:
        self.client: Optional[TcpClient] = None
        self.messages: List[Dict] = []
        self._lock = threading.Lock()

    def ensure_client(self) -> TcpClient:
        if self.client is None:
            self.client = TcpClient()
            self.client.set_callback_on_error(lambda code: print(
                f"[SimpleCommKitAiTcpClient] Error {code}: {get_error_description(code)}"
            ))
            self.client.set_callback_on_connected(
                lambda: print("[SimpleCommKitAiTcpClient] Connected"))
            self.client.set_callback_on_disconnected(
                lambda: print("[SimpleCommKitAiTcpClient] Disconnected"))
            self.client.set_callback_on_message(lambda data: self._store_message(data))
        return self.client

    def _store_message(self, data: bytes) -> None:
        with self._lock:
            self.messages.append({
                "data_hex": data.hex(),
                "data_utf8": data.decode("utf-8", errors="ignore"),
            })


state = TcpClientState()
mcp = FastMCP(name="SimpleCommKitAiTcpClient MCP Server")


@mcp.tool(
    name="tcp_connect",
    description="Connect a TCP client to a remote server at the given host and port.",
    tags={"tcp", "client", "connect"},
    annotations={"title": "TCP Connect", "readOnlyHint": False, "destructiveHint": False},
    meta={"version": "1.0", "role": "connection"},
)
def tcp_connect(host: str, port: int) -> Dict[str, str]:
    client = state.ensure_client()
    try:
        result = client.connect(host, port)
    except Exception as exc:
        raise RuntimeError(f"Connect failed: {exc}") from exc
    return {"message": f"Connected to {host}:{port}" if result else "Connect failed",
            "host": host, "port": str(port)}


@mcp.tool(
    name="tcp_disconnect",
    description="Disconnect the TCP client from the server.",
    tags={"tcp", "client", "disconnect"},
    annotations={"title": "TCP Disconnect", "readOnlyHint": False, "destructiveHint": False},
    meta={"version": "1.0", "role": "connection"},
)
def tcp_disconnect() -> Dict[str, str]:
    if state.client is None:
        raise RuntimeError("Client not initialized")
    try:
        state.client.disconnect()
    except Exception as exc:
        raise RuntimeError(f"Disconnect failed: {exc}") from exc
    return {"message": "Disconnected"}


@mcp.tool(
    name="tcp_status",
    description="Check whether the TCP client is currently connected.",
    tags={"tcp", "client", "read", "status"},
    annotations={"title": "TCP Status", "readOnlyHint": True, "idempotentHint": True},
    meta={"version": "1.0", "role": "status"},
)
def tcp_status() -> Dict[str, bool]:
    if state.client is None:
        return {"connected": False}
    return {"connected": state.client.is_connected()}


@mcp.tool(
    name="tcp_send",
    description="Send data from the TCP client to the connected server. Data can be text or hex-encoded bytes.",
    tags={"tcp", "client", "send"},
    annotations={"title": "TCP Send", "readOnlyHint": False, "destructiveHint": False},
    meta={"version": "1.0", "role": "data"},
)
def tcp_send(data: str, is_hex: bool = False) -> Dict:
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


@mcp.tool(
    name="tcp_get_messages",
    description="Retrieve and clear all buffered messages received by the TCP client.",
    tags={"tcp", "client", "read"},
    annotations={"title": "TCP Get Messages", "readOnlyHint": False},
    meta={"version": "1.0", "role": "data"},
)
def tcp_get_messages() -> List[Dict[str, str]]:
    with state._lock:
        msgs = state.messages[:]
        state.messages.clear()
    return msgs


@mcp.tool(
    name="tcp_set_reconnect",
    description="Configure automatic reconnection for the TCP client.",
    tags={"tcp", "client", "config"},
    annotations={"title": "TCP Set Reconnect", "readOnlyHint": False},
    meta={"version": "1.0", "role": "config"},
)
def tcp_set_reconnect(min_delay_ms: int = 1000, max_delay_ms: int = 10000,
                      delay_policy: int = 2, max_retry_cnt: int = 0) -> Dict[str, str]:
    client = state.ensure_client()
    setting = TcpReconnectSetting()
    setting.min_delay_ms = min_delay_ms
    setting.max_delay_ms = max_delay_ms
    setting.delay_policy = delay_policy
    setting.max_retry_cnt = max_retry_cnt
    try:
        client.set_reconnect(setting)
    except Exception as exc:
        raise RuntimeError(f"Set reconnect failed: {exc}") from exc
    return {"message": "Reconnect configured"}


def main() -> None:
    parser = argparse.ArgumentParser(description="SimpleCommKitAiTcpClient MCP Server")
    parser.add_argument("--transport", default="stdio",
                        choices=["stdio", "http", "sse", "streamable-http"],
                        help="Transport protocol")
    parser.add_argument("--host", default="127.0.0.1", help="Host to bind to")
    parser.add_argument("--port", type=int, default=8001, help="Port to bind to")
    args = parser.parse_args()

    if args.transport in ("http", "sse", "streamable-http"):
        mcp.run(transport=args.transport, host=args.host, port=args.port)
    else:
        mcp.run()


if __name__ == "__main__":
    main()
