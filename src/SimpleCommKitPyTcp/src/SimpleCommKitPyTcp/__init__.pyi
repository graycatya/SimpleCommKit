"""
Type stubs for SimpleCommKitPyTcp - Python bindings for SimpleCommKit TCP
"""
from typing import Callable, Optional

def get_error_description(error_code: int) -> str:
    """Get a human-readable description for an error code."""
    ...

class TcpReconnectSetting:
    """Reconnect configuration for TCP client."""
    min_delay_ms: int
    max_delay_ms: int
    delay_policy: int
    max_retry_cnt: int
    def __init__(self) -> None: ...

class TlsSetting:
    """TLS/SSL configuration for TCP connections."""
    crt_file: str
    key_file: str
    ca_file: str
    ca_path: str
    verify_peer: bool
    def __init__(self) -> None: ...

class TcpClient:
    """TCP client for connecting to a remote TCP server."""

    def __init__(self) -> None: ...

    def connect(self, host: str, port: int) -> bool:
        """Connect to a TCP server at host:port."""
        ...

    def disconnect(self) -> None:
        """Disconnect from the server."""
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

    def set_reconnect(self, setting: TcpReconnectSetting) -> None:
        """Configure automatic reconnection."""
        ...

    def disable_reconnect(self) -> None:
        """Disable automatic reconnection."""
        ...

    def enable_tls(self, setting: TlsSetting) -> bool:
        """Enable TLS with custom certificate settings. Call BEFORE connect()."""
        ...

    def enable_tls_default(self) -> bool:
        """Enable TLS with default platform certificates. Call BEFORE connect()."""
        ...

    def set_callback_on_connected(self, callback: Callable[[], None]) -> None:
        """Set callback invoked when connection is established."""
        ...

    def set_callback_on_disconnected(self, callback: Callable[[], None]) -> None:
        """Set callback invoked when connection is lost."""
        ...

    def set_callback_on_message(self, callback: Callable[[bytes], None]) -> None:
        """Set callback invoked when data is received."""
        ...

    def set_callback_on_error(self, callback: Callable[[int], None]) -> None:
        """Set callback for error notifications. Callback receives error_code (int);
        use get_error_description(error_code) for details."""
        ...

class TcpServer:
    """TCP server that accepts client connections."""

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

    def enable_tls(self, setting: TlsSetting) -> bool:
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
        """Set callback invoked when data is received from a client.
        Callback receives (client_id, bytes)."""
        ...

    def set_callback_on_error(self, callback: Callable[[int], None]) -> None:
        """Set callback for error notifications. Callback receives error_code (int);
        use get_error_description(error_code) for details."""
        ...
