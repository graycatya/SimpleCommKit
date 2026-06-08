"""
Type stubs for SimpleCommKitPyHid - Python bindings for SimpleCommKit HID
"""
from typing import Callable, Dict, List, Optional, Union
from enum import IntEnum


def get_error_description(error_code: int) -> str:
    """Get a human-readable description for an error code."""
    ...


class HidBusType(IntEnum):
    """HID bus type enumeration."""
    UNKNOWN: int = 0
    USB: int = 1
    BLUETOOTH: int = 2
    I2C: int = 3
    SPI: int = 4


class HidDeviceInfo:
    """HID device information structure."""
    interface_number: int
    manufacturer_string: str
    product_string: str
    release_number: int
    bus_type: int
    serial_number: str
    path: str

    def __init__(self) -> None: ...


class SimpleCommKitHid:
    """Cross-platform HID manager for enumerating, opening, and interacting with HID devices."""

    def __init__(self) -> None: ...

    # --- Static ---
    @staticmethod
    def get_available_devices(vendor_id: int = 0, product_id: int = 0) -> List[HidDeviceInfo]:
        """Enumerate available HID devices, optionally filtered by VID/PID."""
        ...

    # --- Lifecycle ---
    def init(self, vendor_id: int = 0, product_id: int = 0) -> bool:
        """Initialize the HID subsystem."""
        ...

    def exit(self) -> None:
        """Shutdown the HID subsystem."""
        ...

    def open(self, path_or_vid: Union[str, int], product_id_or_readable: Union[int, bool] = True,
             serial_number: str = "", readable: bool = True) -> bool:
        """Open a device by path, or by VID/PID/serial."""
        ...

    def close(self, path: Optional[str] = None) -> None:
        """Close a specific device or all devices."""
        ...

    def is_open(self, path: Optional[str] = None) -> bool:
        """Check if at least one device or a specific device is open."""
        ...

    # --- I/O ---
    def write(self, data_or_path: Union[str, bytes], data: Optional[bytes] = None) -> int:
        """Write a report to a device. The first argument may be a path string or data bytes."""
        ...

    def send_feature_report(self, data_or_path: Union[str, bytes], data: Optional[bytes] = None) -> int:
        """Send a feature report to a device."""
        ...

    # --- Hotplug ---
    def start_hotplug(self, vendor_id: int, product_id: int) -> None:
        """Start hotplug detection (polling-based)."""
        ...

    def stop_hotplug(self) -> None:
        """Stop hotplug detection."""
        ...

    def is_hotplug_active(self) -> bool:
        """Check if hotplug detection is active."""
        ...

    def set_hotplug_poll_interval(self, ms: int) -> None:
        """Set hotplug poll interval in milliseconds."""
        ...

    def get_hotplug_poll_interval(self) -> int:
        """Get hotplug poll interval in milliseconds."""
        ...

    # --- Read poll configuration (global) ---
    def set_read_poll_interval(self, path_or_ms: Union[str, int], ms: Optional[int] = None) -> None:
        """Set global or per-device read poll interval (ms)."""
        ...

    def get_read_poll_interval(self, path: Optional[str] = None) -> int:
        """Get global or per-device read poll interval (ms)."""
        ...

    def set_read_data_length(self, path_or_length: Union[str, int], length: Optional[int] = None) -> None:
        """Set global or per-device read data length."""
        ...

    def get_read_data_length(self, path: Optional[str] = None) -> int:
        """Get global or per-device read data length."""
        ...

    # --- Device list ---
    def get_open_paths(self) -> List[str]:
        """Get list of currently open device paths."""
        ...

    def get_device_list(self) -> List[HidDeviceInfo]:
        """Get cached device list (populated by init / start_hotplug)."""
        ...

    # --- Callbacks ---
    def set_callback_on_read(self, callback: Callable[[bytes, str], None]) -> None:
        """Set callback for incoming read data (receives bytes, device_path)."""
        ...

    def set_callback_on_hotplug(self, callback: Callable[[List[HidDeviceInfo], List[HidDeviceInfo]], None]) -> None:
        """Set callback for hotplug events (added_devices, removed_devices)."""
        ...

    def set_callback_error(self, callback: Callable[[int], None]) -> None:
        """Set a callback to receive error notifications.
        Callback receives error_code (int); use get_error_description(error_code) for details."""
        ...
