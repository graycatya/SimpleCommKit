"""SimpleCommKitAiUdpServer MCP Server - Expose UDP server operations as MCP tools."""

from fastmcp import FastMCP
import argparse
from typing import Dict, List, Optional
import threading

from SimpleCommKitAiUdpServer import UdpServer, get_error_description


class UdpServerState:
    def __init__(self) -> None:
        self.server: Optional[UdpServer] = None
        self.messages: List[Dict] = []
        self._lock = threading.Lock()

    def ensure_server(self) -> UdpServer:
        if self.server is None:
            self.server = UdpServer()
            self.server.set_callback_on_error(lambda code: print(
                f"[SimpleCommKitAiUdpServer] Error {code}: {get_error_description(code)}"))
            self.server.set_callback_on_message(
                lambda host, port, data: self._store_message(host, port, data))
        return self.server

    def _store_message(self, host: str, port: int, data: bytes) -> None:
        with self._lock:
            self.messages.append({
                "from_host": host, "from_port": port,
                "data_hex": data.hex(), "data_utf8": data.decode("utf-8", errors="ignore"),
            })


state = UdpServerState()
mcp = FastMCP(name="SimpleCommKitAiUdpServer MCP Server")


@mcp.tool(name="udp_server_start", description="Start a UDP server listening on the given port.",
          tags={"udp", "server", "start"},
          annotations={"title": "UDP Server Start", "readOnlyHint": False},
          meta={"version": "1.0", "role": "server"})
def udp_server_start(port: int, host: str = "0.0.0.0") -> Dict[str, str]:
    server = state.ensure_server()
    try:
        result = server.start(port, host)
    except Exception as exc:
        raise RuntimeError(f"Server start failed: {exc}") from exc
    return {"message": f"Server started on {host}:{port}" if result else "Server start failed",
            "host": host, "port": str(port)}


@mcp.tool(name="udp_server_stop", description="Stop the running UDP server.",
          tags={"udp", "server", "stop"},
          annotations={"title": "UDP Server Stop", "readOnlyHint": False},
          meta={"version": "1.0", "role": "server"})
def udp_server_stop() -> Dict[str, str]:
    if state.server is None:
        raise RuntimeError("Server not initialized")
    try:
        state.server.stop()
    except Exception as exc:
        raise RuntimeError(f"Server stop failed: {exc}") from exc
    return {"message": "Server stopped"}


@mcp.tool(name="udp_server_status", description="Check whether the UDP server is running.",
          tags={"udp", "server", "read", "status"},
          annotations={"title": "UDP Server Status", "readOnlyHint": True, "idempotentHint": True},
          meta={"version": "1.0", "role": "status"})
def udp_server_status() -> Dict:
    if state.server is None:
        return {"running": False}
    return {
        "running": state.server.is_running(),
        "host": state.server.host() if state.server.is_running() else "",
        "port": state.server.port() if state.server.is_running() else 0,
    }


@mcp.tool(name="udp_server_send_to", description="Send a UDP datagram from the server to a specific host:port.",
          tags={"udp", "server", "send"},
          annotations={"title": "UDP Server Send To", "readOnlyHint": False},
          meta={"version": "1.0", "role": "data"})
def udp_server_send_to(host: str, port: int, data: str, is_hex: bool = False) -> Dict:
    if state.server is None or not state.server.is_running():
        raise RuntimeError("Server not running")
    try:
        if is_hex:
            payload = bytes.fromhex(data)
            result = state.server.send_to(host, port, payload)
        else:
            result = state.server.send_to_text(host, port, data)
    except ValueError as exc:
        raise RuntimeError(f"Invalid hex string: {exc}") from exc
    except Exception as exc:
        raise RuntimeError(f"Send failed: {exc}") from exc
    return {"bytes_sent": result, "host": host, "port": port}


@mcp.tool(name="udp_server_broadcast", description="Broadcast a UDP datagram to 255.255.255.255.",
          tags={"udp", "server", "broadcast"},
          annotations={"title": "UDP Server Broadcast", "readOnlyHint": False},
          meta={"version": "1.0", "role": "data"})
def udp_server_broadcast(data: str, is_hex: bool = False) -> Dict:
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


@mcp.tool(name="udp_server_get_messages", description="Retrieve and clear all buffered datagrams.",
          tags={"udp", "server", "read"},
          annotations={"title": "UDP Server Get Messages", "readOnlyHint": False},
          meta={"version": "1.0", "role": "data"})
def udp_server_get_messages() -> List[Dict]:
    with state._lock:
        msgs = state.messages[:]
        state.messages.clear()
    return msgs


def main() -> None:
    parser = argparse.ArgumentParser(description="SimpleCommKitAiUdpServer MCP Server")
    parser.add_argument("--transport", default="stdio",
                        choices=["stdio", "http", "sse", "streamable-http"])
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8102)
    args = parser.parse_args()
    if args.transport in ("http", "sse", "streamable-http"):
        mcp.run(transport=args.transport, host=args.host, port=args.port)
    else:
        mcp.run()


if __name__ == "__main__":
    main()
