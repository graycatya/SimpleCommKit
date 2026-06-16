"""
Type stubs for SimpleCommKitPyWebSocket - Python bindings for SimpleCommKit WebSocket
"""
from typing import Callable

def get_error_description(error_code: int) -> str:
    """Get a human-readable description for an error code."""
    ...

class WsReconnectSetting:
    """Reconnect configuration for WebSocket client."""
    min_delay_ms: int
    max_delay_ms: int
    delay_policy: int
    max_retry_cnt: int
    def __init__(self) -> None: ...

class WsTlsSetting:
    """TLS/SSL configuration for WebSocket connections (wss://)."""
    crt_file: str
    key_file: str
    ca_file: str
    ca_path: str
    verify_peer: bool
    def __init__(self) -> None: ...

class WebSocketClient:
    """WebSocket client for connecting to ws:// or wss:// endpoints."""

    def __init__(self) -> None: ...

    def open(self, url: str) -> bool:
        """Connect to a WebSocket server URL."""
        ...

    def close(self) -> None:
        """Close the WebSocket connection."""
        ...

    def is_connected(self) -> bool:
        """Check if currently connected."""
        ...

    def send(self, data: bytes) -> int:
        """Send binary data. Returns bytes sent or error code."""
        ...

    def send_text(self, data: str) -> int:
        """Send text data. Returns bytes sent or error code."""
        ...

    def set_connect_timeout(self, timeout_ms: int) -> None:
        """Set the connection timeout in milliseconds."""
        ...

    def set_reconnect(self, setting: WsReconnectSetting) -> None:
        """Configure automatic reconnection."""
        ...

    def disable_reconnect(self) -> None:
        """Disable automatic reconnection."""
        ...

    def set_ping_interval(self, ms: int) -> None:
        """Set the ping interval in milliseconds."""
        ...

    def enable_tls(self, setting: WsTlsSetting) -> bool:
        """Enable TLS with custom certificate settings. Call BEFORE open()."""
        ...

    def enable_tls_default(self) -> bool:
        """Enable TLS with default platform certificates. Call BEFORE open()."""
        ...

    def set_callback_on_open(self, callback: Callable[[], None]) -> None:
        """Set callback invoked when WebSocket connection is opened."""
        ...

    def set_callback_on_close(self, callback: Callable[[], None]) -> None:
        """Set callback invoked when WebSocket connection is closed."""
        ...

    def set_callback_on_message(self, callback: Callable[[bytes], None]) -> None:
        """Set callback invoked when a WebSocket message is received."""
        ...

    def set_callback_on_error(self, callback: Callable[[int], None]) -> None:
        """Set callback for error notifications. Callback receives error_code (int);
        use get_error_description(error_code) for details."""
        ...

class WebSocketServer:
    """WebSocket server that accepts client connections."""

    def __init__(self) -> None: ...

    def start(self, port: int, host: str = "0.0.0.0") -> bool:
        """Start listening on host:port."""
        ...

    def stop(self) -> None:
        """Stop the server and disconnect all clients."""
        ...

    def is_running(self) -> bool:
        """Check if the server is currently running."""
        ...

    def send_to(self, client_id: int, data: bytes) -> int:
        """Send binary data to a specific client by ID."""
        ...

    def send_to_text(self, client_id: int, data: str) -> int:
        """Send text data to a specific client by ID."""
        ...

    def broadcast(self, data: bytes) -> int:
        """Broadcast binary data to all connected clients."""
        ...

    def broadcast_text(self, data: str) -> int:
        """Broadcast text data to all connected clients."""
        ...

    def connection_num(self) -> int:
        """Get the current number of connected clients."""
        ...

    def port(self) -> int:
        """Get the port the server is listening on."""
        ...

    def host(self) -> str:
        """Get the host the server is bound to."""
        ...

    def set_thread_num(self, num: int) -> None:
        """Set the number of I/O threads."""
        ...

    def set_max_connection_num(self, num: int) -> None:
        """Set the maximum number of concurrent connections."""
        ...

    def enable_tls(self, setting: WsTlsSetting) -> bool:
        """Enable TLS with custom certificate settings. Call BEFORE start()."""
        ...

    def enable_tls_default(self) -> bool:
        """Enable TLS with default platform certificates. Call BEFORE start()."""
        ...

    def set_callback_on_client_connected(self, callback: Callable[[int], None]) -> None:
        """Set callback invoked when a client connects. Callback receives client_id."""
        ...

    def set_callback_on_client_disconnected(self, callback: Callable[[int], None]) -> None:
        """Set callback invoked when a client disconnects. Callback receives client_id."""
        ...

    def set_callback_on_message(self, callback: Callable[[int, bytes], None]) -> None:
        """Set callback invoked when a message is received from a client.
        Callback receives (client_id, bytes)."""
        ...

    def set_callback_on_error(self, callback: Callable[[int], None]) -> None:
        """Set callback for error notifications. Callback receives error_code (int);
        use get_error_description(error_code) for details."""
        ...
