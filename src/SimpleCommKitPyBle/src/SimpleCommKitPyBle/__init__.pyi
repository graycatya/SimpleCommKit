"""
Type stubs for SimpleCommKitPyBle - Python bindings for SimpleCommKit BLE
"""
from typing import Callable, Dict, List, Optional
from enum import Enum, IntEnum

def get_error_description(error_code: int) -> str:
    """Get a human-readable description for an error code."""
    ...

class PeripheralAddressType(IntEnum):
    """Bluetooth address type enumeration."""
    PUBLIC: int = 0
    RANDOM: int = 1
    UNSPECIFIED: int = 2

class Adapter:
    """Represents a Bluetooth adapter."""
    identifier: str
    address: str
    def __init__(self) -> None: ...

class Peripheral:
    """Represents a BLE peripheral device."""
    identifier: str
    address: str
    address_type: PeripheralAddressType
    rssi: int
    @property
    def manufacturer_data(self) -> Dict[int, bytes]:
        """Manufacturer data as dict of {company_id: bytes}"""
        ...
    def __init__(self) -> None: ...

class Characteristic:
    """Represents a GATT characteristic."""
    uuid: str
    descriptors: List[str]
    capabilities: List[str]
    can_read: bool
    can_write_request: bool
    can_write_command: bool
    can_notify: bool
    can_indicate: bool
    def __init__(self) -> None: ...

class Service:
    """Represents a GATT service."""
    uuid: str
    characteristics: List[Characteristic]
    @property
    def data_bytes(self) -> bytes:
        """Advertised service data as bytes"""
        ...
    def __init__(self) -> None: ...

class BleCentral:
    """Central BLE manager for scanning, connecting, and interacting with BLE devices."""

    def __init__(self) -> None: ...

    @staticmethod
    def bluetooth_enabled() -> bool:
        """Check if Bluetooth is enabled on the system."""
        ...

    def get_adapters(self) -> List[Adapter]:
        """Get a list of available Bluetooth adapters."""
        ...

    def get_current_adapter(self) -> Optional[Adapter]:
        """Get the currently selected adapter (or None)."""
        ...

    def set_current_adapter(self, adapter: Adapter) -> None:
        """Set the current adapter by providing an Adapter object."""
        ...

    def adapter_power_on(self) -> None:
        """Power on the selected adapter."""
        ...

    def adapter_power_off(self) -> None:
        """Power off the selected adapter."""
        ...

    def adapter_is_powered(self) -> bool:
        """Check if the selected adapter is powered on."""
        ...

    def adapter_set_callback_on_power_on(self, callback: Callable[[], None]) -> None:
        """Set callback invoked when adapter powers on."""
        ...

    def adapter_set_callback_on_power_off(self, callback: Callable[[], None]) -> None:
        """Set callback invoked when adapter powers off."""
        ...

    def adapter_set_callback_on_scan_start(self, callback: Callable[[], None]) -> None:
        """Set callback invoked when scanning starts."""
        ...

    def adapter_set_callback_on_scan_stop(self, callback: Callable[[], None]) -> None:
        """Set callback invoked when scanning stops."""
        ...

    def adapter_set_callback_on_scan_found(self, callback: Callable[[Peripheral], None]) -> None:
        """Set callback invoked when a peripheral is found during scanning."""
        ...

    def adapter_set_callback_on_scan_updated(self, callback: Callable[[Peripheral], None]) -> None:
        """Set callback invoked when a peripheral's information is updated during scanning."""
        ...

    def adapter_scan_start(self) -> None:
        """Start scanning for BLE peripherals."""
        ...

    def adapter_scan_stop(self) -> None:
        """Stop scanning for BLE peripherals."""
        ...

    def adapter_scan_for(self, timeout_ms: int) -> None:
        """Scan for peripherals for the specified duration (ms)."""
        ...

    def adapter_scan_is_active(self) -> bool:
        """Check if currently scanning."""
        ...

    def adapter_initialized(self) -> bool:
        """Check if the adapter is initialized."""
        ...

    def adapter_get_scan_results(self) -> List[Peripheral]:
        """Get peripherals discovered in the last scan."""
        ...

    def adapter_get_paired_peripherals(self) -> List[Peripheral]:
        """Get all paired peripherals."""
        ...

    def adapter_get_connected_peripherals(self) -> List[Peripheral]:
        """Get all currently connected peripherals."""
        ...

    def set_current_peripheral(self, peripheral: Peripheral) -> None:
        """Select a peripheral for subsequent operations."""
        ...

    def peripheral_get_tx_power(self) -> int:
        """Get the TX power of the current peripheral (dBm)."""
        ...

    def peripheral_get_mtu(self) -> int:
        """Get the negotiated MTU of the current peripheral."""
        ...

    def peripheral_connect(self) -> None:
        """Connect to the current peripheral."""
        ...

    def peripheral_disconnect(self) -> None:
        """Disconnect from the current peripheral."""
        ...

    def peripheral_is_connected(self) -> bool:
        """Check if the current peripheral is connected."""
        ...

    def peripheral_is_connectable(self) -> bool:
        """Check if the current peripheral is connectable."""
        ...

    def peripheral_is_paired(self) -> bool:
        """Check if the current peripheral is paired."""
        ...

    def peripheral_unpair(self) -> None:
        """Unpair the current peripheral."""
        ...

    def peripheral_set_callback_on_connected(self, callback: Callable[[], None]) -> None:
        """Set callback invoked when the peripheral connects."""
        ...

    def peripheral_set_callback_on_disconnected(self, callback: Callable[[], None]) -> None:
        """Set callback invoked when the peripheral disconnects."""
        ...

    def peripheral_services(self) -> List[Service]:
        """Get all GATT services of the current peripheral."""
        ...

    def peripheral_read(self, service_uuid: str, char_uuid: str) -> bytes:
        """Read a characteristic value. Returns bytes."""
        ...

    def peripheral_write_request(self, service_uuid: str, char_uuid: str, data: bytes) -> None:
        """Write data to a characteristic with response. Data is bytes."""
        ...

    def peripheral_write_command(self, service_uuid: str, char_uuid: str, data: bytes) -> None:
        """Write data to a characteristic without response. Data is bytes."""
        ...

    def peripheral_notify(self, service_uuid: str, char_uuid: str, callback: Callable[[bytes], None]) -> None:
        """Subscribe to notifications on a characteristic. Callback receives bytes payload."""
        ...

    def peripheral_indicate(self, service_uuid: str, char_uuid: str, callback: Callable[[bytes], None]) -> None:
        """Subscribe to indications on a characteristic. Callback receives bytes payload."""
        ...

    def peripheral_unsubscribe(self, service_uuid: str, char_uuid: str) -> None:
        """Unsubscribe from notifications/indications on a characteristic."""
        ...

    def peripheral_read_descriptor(self, service_uuid: str, char_uuid: str, descriptor_uuid: str) -> bytes:
        """Read a descriptor value. Returns bytes."""
        ...

    def peripheral_write_descriptor(self, service_uuid: str, char_uuid: str, descriptor_uuid: str, data: bytes) -> None:
        """Write data to a descriptor. Data is bytes."""
        ...

    def set_callback_error(self, callback: Callable[[int], None]) -> None:
        """Set a callback to receive error notifications.
        Callback receives error_code (int); use get_error_description(error_code) for details."""
        ...
