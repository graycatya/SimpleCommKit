"""
Type stubs for SimpleCommKitPyUdp - Python bindings for SimpleCommKit UDP
"""
from typing import Callable

def get_error_description(error_code: int) -> str:
    """Get a human-readable description for an error code."""
    ...

class UdpClient:
    """UDP client for sending and receiving datagrams."""

    def __init__(self) -> None: ...

    def open(self, local_port: int = 0, local_host: str = "0.0.0.0") -> bool:
        """Open the local UDP socket."""
        ...

    def close(self) -> None:
        """Close the UDP socket."""
        ...

    def is_open(self) -> bool:
        """Check if the socket is currently open."""
        ...

    def send_to(self, host: str, port: int, data: bytes) -> int:
        """Send binary data to a remote host:port. Returns bytes sent or error code."""
        ...

    def send_to_text(self, host: str, port: int, data: str) -> int:
        """Send text data to a remote host:port. Returns bytes sent or error code."""
        ...

    def set_remote_address(self, host: str, port: int) -> None:
        """Set a default remote address for subsequent send() calls."""
        ...

    def send(self, data: bytes) -> int:
        """Send binary data to the default remote address. Call set_remote_address() first."""
        ...

    def send_text(self, data: str) -> int:
        """Send text data to the default remote address. Call set_remote_address() first."""
        ...

    def set_read_timeout(self, timeout_ms: int) -> None:
        """Set the read timeout in milliseconds."""
        ...

    def set_callback_on_message(self, callback: Callable[[bytes], None]) -> None:
        """Set callback invoked when a datagram is received."""
        ...

    def set_callback_on_error(self, callback: Callable[[int], None]) -> None:
        """Set callback for error notifications. Callback receives error_code (int);
        use get_error_description(error_code) for details."""
        ...

class UdpServer:
    """UDP server that listens for incoming datagrams."""

    def __init__(self) -> None: ...

    def start(self, port: int, host: str = "0.0.0.0") -> bool:
        """Start listening on host:port."""
        ...

    def stop(self) -> None:
        """Stop the server."""
        ...

    def is_running(self) -> bool:
        """Check if the server is currently running."""
        ...

    def send_to(self, host: str, port: int, data: bytes) -> int:
        """Send binary data to a specific remote host:port."""
        ...

    def send_to_text(self, host: str, port: int, data: str) -> int:
        """Send text data to a specific remote host:port."""
        ...

    def broadcast(self, data: bytes) -> int:
        """Broadcast binary data to 255.255.255.255."""
        ...

    def broadcast_text(self, data: str) -> int:
        """Broadcast text data to 255.255.255.255."""
        ...

    def port(self) -> int:
        """Get the port the server is listening on."""
        ...

    def host(self) -> str:
        """Get the host the server is bound to."""
        ...

    def set_callback_on_message(self, callback: Callable[[str, int, bytes], None]) -> None:
        """Set callback invoked when a datagram is received.
        Callback receives (from_host, from_port, bytes)."""
        ...

    def set_callback_on_error(self, callback: Callable[[int], None]) -> None:
        """Set callback for error notifications. Callback receives error_code (int);
        use get_error_description(error_code) for details."""
        ...
