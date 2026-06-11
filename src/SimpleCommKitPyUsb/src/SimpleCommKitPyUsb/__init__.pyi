"""
Type stubs for SimpleCommKitPyUsb - Python bindings for SimpleCommKit USB (single-device mode)
"""
from typing import Callable, List, Optional, Union, Tuple


def get_error_description(error_code: int) -> str:
    """Get a human-readable description for an error code."""
    ...


class UsbDeviceInfo:
    """USB device information structure."""
    vendor_id: int
    product_id: int
    manufacturer_string: str
    product_string: str
    serial_number: str
    bus_number: int
    device_address: int
    path: str

    def __init__(self) -> None: ...


class IsochronousPacketResult:
    """Per-packet result for an isochronous transfer."""
    length: int
    actual_length: int
    status: int  # 0 = LIBUSB_TRANSFER_COMPLETED

    def __init__(self) -> None: ...


class SimpleCommKitUsb:
    """Cross-platform USB manager (single-device mode)."""

    def __init__(self) -> None: ...

    @staticmethod
    def get_available_devices(vendor_id: int = 0, product_id: int = 0) -> List[UsbDeviceInfo]:
        """Enumerate available USB devices, optionally filtered by VID/PID."""
        ...

    def init(self) -> bool: ...
    def exit(self) -> None: ...

    def open(self, path_or_vid: Union[str, int], product_id: int = 0,
             serial_number: str = "") -> bool:
        """Open a device by path or by VID/PID/serial."""
        ...

    def close(self) -> None:
        """Close the open device."""
        ...

    def is_open(self) -> bool:
        """Check if a device is open."""
        ...

    def claim_interface(self, interface_number: int) -> bool:
        """Claim a USB interface on the open device."""
        ...

    def release_interface(self, interface_number: int) -> bool:
        """Release a USB interface on the open device."""
        ...

    def control_transfer(self, bm_request_type: int, b_request: int,
                         w_value: int, w_index: int, data: bytes,
                         timeout: int = 1000) -> bytes: ...
    def bulk_transfer(self, endpoint: int, data_or_length: Union[bytes, int],
                      timeout: int = 1000) -> Union[int, bytes]:
        """Bulk transfer. Direction auto-detected from endpoint (bit7).
        OUT (bit7=0): pass bytes, returns transferred count.
        IN (bit7=1): pass length as int, returns received bytes."""
        ...
    def interrupt_transfer(self, endpoint: int, data_or_length: Union[bytes, int],
                           timeout: int = 1000) -> Union[int, bytes]:
        """Interrupt transfer. Direction auto-detected from endpoint (bit7).
        OUT (bit7=0): pass bytes, returns transferred count.
        IN (bit7=1): pass length as int, returns received bytes."""
        ...

    def isochronous_transfer(self, endpoint: int, data_or_length: Union[bytes, int],
                             num_packets: int, packet_lengths: List[int],
                             timeout: int = 1000
                             ) -> Tuple[int, bytes, List[IsochronousPacketResult]]:
        """Isochronous transfer. Direction auto-detected from endpoint (bit7).
        OUT (bit7=0): pass bytes as data. IN (bit7=1): pass buffer size as int.
        Returns (total_bytes, received_data, packet_results)."""
        ...

    def start_read_poll(self, endpoint: int) -> None: ...
    def stop_read_poll(self) -> None: ...
    def is_read_poll_active(self) -> bool: ...

    def start_hotplug(self, vendor_id: int = 0, product_id: int = 0) -> None: ...
    def stop_hotplug(self) -> None: ...
    def is_hotplug_active(self) -> bool: ...
    def set_hotplug_poll_interval(self, ms: int) -> None: ...
    def get_hotplug_poll_interval(self) -> int: ...

    def set_read_poll_interval(self, ms: int) -> None: ...
    def get_read_poll_interval(self) -> int: ...
    def set_read_data_length(self, length: int) -> None: ...
    def get_read_data_length(self) -> int: ...

    def get_open_path(self) -> str: ...
    def is_open_device(self) -> bool: ...
    def get_device_list(self) -> List[UsbDeviceInfo]: ...

    def set_callback_on_read(self, callback: Callable[[UsbDeviceInfo, bytes], None]) -> None: ...
    def set_callback_on_hotplug(self, callback: Callable[[List[UsbDeviceInfo], List[UsbDeviceInfo]], None]) -> None: ...
    def set_callback_error(self, callback: Callable[[int], None]) -> None: ...
