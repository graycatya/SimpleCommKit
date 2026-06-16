"""SimpleCommKitAiTcpServer MCP Server - Expose TCP server operations as MCP tools for AI agents."""

from fastmcp import FastMCP
import argparse
from typing import Dict, List, Optional
import threading

from SimpleCommKitAiTcpServer import TcpServer, TlsSetting, get_error_description


class TcpServerState:
    def __init__(self) -> None:
        self.server: Optional[TcpServer] = None
        self.messages: List[Dict] = []
        self.connected_clients: Dict[int, str] = {}
        self._lock = threading.Lock()

    def ensure_server(self) -> TcpServer:
        if self.server is None:
            self.server = TcpServer()
            self.server.set_callback_on_error(lambda code: print(
                f"[SimpleCommKitAiTcpServer] Error {code}: {get_error_description(code)}"))
            self.server.set_callback_on_client_connected(lambda cid: self._on_connect(cid))
            self.server.set_callback_on_client_disconnected(lambda cid: self._on_disconnect(cid))
            self.server.set_callback_on_message(lambda cid, data: self._store_message(cid, data))
        return self.server

    def _on_connect(self, cid: int) -> None:
        with self._lock:
            self.connected_clients[cid] = f"client_{cid}"

    def _on_disconnect(self, cid: int) -> None:
        with self._lock:
            self.connected_clients.pop(cid, None)

    def _store_message(self, cid: int, data: bytes) -> None:
        with self._lock:
            self.messages.append({
                "client_id": cid,
                "data_hex": data.hex(),
                "data_utf8": data.decode("utf-8", errors="ignore"),
            })


state = TcpServerState()
mcp = FastMCP(name="SimpleCommKitAiTcpServer MCP Server")


@mcp.tool(name="tcp_server_start", description="Start a TCP server listening on the given port.",
          tags={"tcp", "server", "start"},
          annotations={"title": "TCP Server Start", "readOnlyHint": False},
          meta={"version": "1.0", "role": "server"})
def tcp_server_start(port: int, host: str = "0.0.0.0") -> Dict[str, str]:
    server = state.ensure_server()
    try:
        result = server.start(port, host)
    except Exception as exc:
        raise RuntimeError(f"Server start failed: {exc}") from exc
    return {"message": f"Server started on {host}:{port}" if result else "Server start failed",
            "host": host, "port": str(port)}


@mcp.tool(name="tcp_server_stop", description="Stop the running TCP server and disconnect all clients.",
          tags={"tcp", "server", "stop"},
          annotations={"title": "TCP Server Stop", "readOnlyHint": False},
          meta={"version": "1.0", "role": "server"})
def tcp_server_stop() -> Dict[str, str]:
    if state.server is None:
        raise RuntimeError("Server not initialized")
    try:
        state.server.stop()
    except Exception as exc:
        raise RuntimeError(f"Server stop failed: {exc}") from exc
    return {"message": "Server stopped"}


@mcp.tool(name="tcp_server_status", description="Check server status and connection count.",
          tags={"tcp", "server", "read", "status"},
          annotations={"title": "TCP Server Status", "readOnlyHint": True, "idempotentHint": True},
          meta={"version": "1.0", "role": "status"})
def tcp_server_status() -> Dict:
    if state.server is None:
        return {"running": False, "connections": 0}
    return {
        "running": state.server.is_running(),
        "connections": state.server.connection_num() if state.server.is_running() else 0,
        "host": state.server.host() if state.server.is_running() else "",
        "port": state.server.port() if state.server.is_running() else 0,
    }


@mcp.tool(name="tcp_server_send", description="Send data to a specific connected client by client ID.",
          tags={"tcp", "server", "send"},
          annotations={"title": "TCP Server Send", "readOnlyHint": False},
          meta={"version": "1.0", "role": "data"})
def tcp_server_send(client_id: int, data: str, is_hex: bool = False) -> Dict:
    if state.server is None or not state.server.is_running():
        raise RuntimeError("Server not running")
    try:
        if is_hex:
            payload = bytes.fromhex(data)
            result = state.server.send_to(client_id, payload)
        else:
            result = state.server.send_to_text(client_id, data)
    except ValueError as exc:
        raise RuntimeError(f"Invalid hex string: {exc}") from exc
    except Exception as exc:
        raise RuntimeError(f"Send failed: {exc}") from exc
    return {"client_id": client_id, "bytes_sent": result}


@mcp.tool(name="tcp_server_broadcast", description="Broadcast data to all connected TCP clients.",
          tags={"tcp", "server", "broadcast"},
          annotations={"title": "TCP Server Broadcast", "readOnlyHint": False},
          meta={"version": "1.0", "role": "data"})
def tcp_server_broadcast(data: str, is_hex: bool = False) -> Dict:
    if state.server is None or not state.server.is_running():
        raise RuntimeError("Server not running")
    try:
        if is_hex:
            payload = bytes.fromhex(data)
            result = state.server.broadcast(payload)
        else:
            result = state.server.broadcast_text(data)
    except ValueError as exc:
        raise RuntimeError(f"Invalid hex string: {exc}") from exc
    except Exception as exc:
        raise RuntimeError(f"Broadcast failed: {exc}") from exc
    return {"bytes_sent": result}


@mcp.tool(name="tcp_server_get_messages", description="Retrieve and clear all buffered messages.",
          tags={"tcp", "server", "read"},
          annotations={"title": "TCP Server Get Messages", "readOnlyHint": False},
          meta={"version": "1.0", "role": "data"})
def tcp_server_get_messages() -> List[Dict]:
    with state._lock:
        msgs = state.messages[:]
        state.messages.clear()
    return msgs


def main() -> None:
    parser = argparse.ArgumentParser(description="SimpleCommKitAiTcpServer MCP Server")
    parser.add_argument("--transport", default="stdio",
                        choices=["stdio", "http", "sse", "streamable-http"])
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8101)
    args = parser.parse_args()
    if args.transport in ("http", "sse", "streamable-http"):
        mcp.run(transport=args.transport, host=args.host, port=args.port)
    else:
        mcp.run()


if __name__ == "__main__":
    main()
