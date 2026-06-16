"""SimpleCommKitAiUdpClient MCP Server - Expose UDP client operations as MCP tools."""

from fastmcp import FastMCP
import argparse
from typing import Dict, List, Optional
import threading

from SimpleCommKitAiUdpClient import UdpClient, get_error_description


class UdpClientState:
    def __init__(self) -> None:
        self.client: Optional[UdpClient] = None
        self.messages: List[Dict] = []
        self._lock = threading.Lock()

    def ensure_client(self) -> UdpClient:
        if self.client is None:
            self.client = UdpClient()
            self.client.set_callback_on_error(lambda code: print(
                f"[SimpleCommKitAiUdpClient] Error {code}: {get_error_description(code)}"))
            self.client.set_callback_on_message(lambda data: self._store_message(data))
        return self.client

    def _store_message(self, data: bytes) -> None:
        with self._lock:
            self.messages.append({"data_hex": data.hex(), "data_utf8": data.decode("utf-8", errors="ignore")})


state = UdpClientState()
mcp = FastMCP(name="SimpleCommKitAiUdpClient MCP Server")


@mcp.tool(name="udp_open", description="Open a UDP client socket for sending and receiving datagrams.",
          tags={"udp", "client", "open"},
          annotations={"title": "UDP Open", "readOnlyHint": False},
          meta={"version": "1.0", "role": "connection"})
def udp_open(local_port: int = 0, local_host: str = "0.0.0.0") -> Dict[str, str]:
    client = state.ensure_client()
    try:
        result = client.open(local_port, local_host)
    except Exception as exc:
        raise RuntimeError(f"Open failed: {exc}") from exc
    return {"message": f"Socket opened on {local_host}:{local_port}" if result else "Open failed",
            "host": local_host, "port": str(local_port)}


@mcp.tool(name="udp_close", description="Close the UDP client socket.",
          tags={"udp", "client", "close"},
          annotations={"title": "UDP Close", "readOnlyHint": False},
          meta={"version": "1.0", "role": "connection"})
def udp_close() -> Dict[str, str]:
    if state.client is None:
        raise RuntimeError("Client not initialized")
    try:
        state.client.close()
    except Exception as exc:
        raise RuntimeError(f"Close failed: {exc}") from exc
    return {"message": "Socket closed"}


@mcp.tool(name="udp_status", description="Check whether the UDP client socket is open.",
          tags={"udp", "client", "read", "status"},
          annotations={"title": "UDP Status", "readOnlyHint": True, "idempotentHint": True},
          meta={"version": "1.0", "role": "status"})
def udp_status() -> Dict[str, bool]:
    if state.client is None:
        return {"open": False}
    return {"open": state.client.is_open()}


@mcp.tool(name="udp_send_to", description="Send a UDP datagram to a specific host:port.",
          tags={"udp", "client", "send"},
          annotations={"title": "UDP Send To", "readOnlyHint": False},
          meta={"version": "1.0", "role": "data"})
def udp_send_to(host: str, port: int, data: str, is_hex: bool = False) -> Dict:
    if state.client is None or not state.client.is_open():
        raise RuntimeError("Client socket not open")
    try:
        if is_hex:
            payload = bytes.fromhex(data)
            result = state.client.send_to(host, port, payload)
        else:
            result = state.client.send_to_text(host, port, data)
    except ValueError as exc:
        raise RuntimeError(f"Invalid hex string: {exc}") from exc
    except Exception as exc:
        raise RuntimeError(f"Send failed: {exc}") from exc
    return {"bytes_sent": result, "host": host, "port": port}


@mcp.tool(name="udp_get_messages", description="Retrieve and clear all buffered datagrams.",
          tags={"udp", "client", "read"},
          annotations={"title": "UDP Get Messages", "readOnlyHint": False},
          meta={"version": "1.0", "role": "data"})
def udp_get_messages() -> List[Dict[str, str]]:
    with state._lock:
        msgs = state.messages[:]
        state.messages.clear()
    return msgs


def main() -> None:
    parser = argparse.ArgumentParser(description="SimpleCommKitAiUdpClient MCP Server")
    parser.add_argument("--transport", default="stdio",
                        choices=["stdio", "http", "sse", "streamable-http"])
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8002)
    args = parser.parse_args()
    if args.transport in ("http", "sse", "streamable-http"):
        mcp.run(transport=args.transport, host=args.host, port=args.port)
    else:
        mcp.run()


if __name__ == "__main__":
    main()
