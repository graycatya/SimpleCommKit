"""
SimpleCommKitPyHid - Python bindings for SimpleCommKit HID

Cross-platform HID (Human Interface Device) toolkit for Python.
Based on SimpleCommKit's C++ HID module.

Quick Start::

    from simple_comm_kit_hid import SimpleCommKitHid

    hid = SimpleCommKitHid()

    # Enumerate available HID devices
    devices = SimpleCommKitHid.get_available_devices()
    for d in devices:
        print(f"Found: {d.product_string} at {d.path}")

    # Init and open
    hid.init()
    hid.open(devices[0].path)

    # Write a report
    hid.write(bytes([0x00, 0x01, 0x02]))

    # Set read callback
    hid.set_callback_on_read(lambda data, path: print(f"Read {len(data)} bytes: {data.hex()}"))

    # Cleanup
    hid.close()
    hid.exit()
"""

import os as _os
import sys as _sys

# Register package directory for DLL loading (needed for Windows)

from ._SimpleCommKitPyHid import get_version
__version__ = get_version()

_pkg_dir = _os.path.dirname(_os.path.abspath(__file__))
if _sys.platform == "win32" and _os.path.isdir(_pkg_dir):
    try:
        _os.add_dll_directory(_pkg_dir)
    except OSError:
        pass  # Directory may already be registered

from ._SimpleCommKitPyHid import (
    # Version / Utility
    get_error_description,

    # Enum
    HidBusType,

    # Struct
    HidDeviceInfo,

    # Main class
    SimpleCommKitHid,
)

__all__ = [
    "get_error_description",
    "HidBusType",
    "HidDeviceInfo",
    "SimpleCommKitHid",
]
