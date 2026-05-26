"""
SimpleCommKitPyBle - Python bindings for SimpleCommKit BLE

Cross-platform BLE (Bluetooth Low Energy) toolkit for Python.
Based on SimpleCommKit's C++ BLE central module.

Quick Start::

    from simple_comm_kit_ble import BleCentral

    central = BleCentral()

    # Discover adapters
    adapters = central.get_adapters()
    central.set_current_adapter(adapters[0])

    # Scan for peripherals
    central.adapter_scan_for(5000)
    results = central.adapter_get_scan_results()
    for p in results:
        print(f"Found: {p.address} ({p.identifier}) RSSI: {p.rssi}")

    # Connect and explore
    central.set_current_peripheral(results[0])
    central.peripheral_connect()
    services = central.peripheral_services()
    for svc in services:
        for ch in svc.characteristics:
            if ch.can_read:
                data = central.peripheral_read(svc.uuid, ch.uuid)
                print(f"{ch.uuid}: {data.hex()}")

    central.peripheral_disconnect()
"""

import os as _os
import sys as _sys

# Register package directory for DLL loading (needed for Windows)
_pkg_dir = _os.path.dirname(_os.path.abspath(__file__))
if _sys.platform == "win32" and _os.path.isdir(_pkg_dir):
    try:
        _os.add_dll_directory(_pkg_dir)
    except OSError:
        pass  # Directory may already be registered

from ._SimpleCommKitPyBle import (
    # Version / Utility
    get_error_description,

    # Enum
    PeripheralAddressType,

    # Structs
    Adapter,
    Peripheral,
    Characteristic,
    Service,

    # Main class
    BleCentral,
)

__version__ = "0.1.0"
__all__ = [
    "get_error_description",
    "PeripheralAddressType",
    "Adapter",
    "Peripheral",
    "Characteristic",
    "Service",
    "BleCentral",
]
