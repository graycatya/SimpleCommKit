"""
SimpleCommKitPyUsb - Python bindings for SimpleCommKit USB (single-device mode)

Cross-platform USB device toolkit for Python.
Based on SimpleCommKit's C++ USB module (libusb).

Supports: control, bulk, interrupt, and isochronous transfers.

Quick Start::

    from simple_comm_kit_usb import SimpleCommKitUsb

    usb = SimpleCommKitUsb()

    # Enumerate available USB devices
    devices = SimpleCommKitUsb.get_available_devices()
    for d in devices:
        print(f"Found: {d.product_string} at {d.path}")

    # Init and open
    usb.init()
    usb.open(devices[0].path)

    # Claim interface and do a control transfer
    usb.claim_interface(0)
    data = bytes([0] * 64)
    usb.control_transfer(0x80, 0x06, 0x0100, 0x0000, data)

    # Bulk transfer
    usb.bulk_transfer(0x01, bytes([0x01, 0x02, 0x03]))

    # Isochronous transfer (OUT)
    total, data, results = usb.isochronous_transfer(0x01, b'hello', 1, [5])

    # Start continuous read poll (callback-driven)
    usb.set_callback_on_read(lambda dev_info, data: print(f"[{dev_info.path}] {data.hex()}"))
    usb.start_read_poll(0x81)

    # Cleanup
    usb.stop_read_poll()
    usb.close()
    usb.exit()
"""

import os as _os
import sys as _sys

from ._SimpleCommKitPyUsb import get_version
__version__ = get_version()

_pkg_dir = _os.path.dirname(_os.path.abspath(__file__))
if _sys.platform == "win32" and _os.path.isdir(_pkg_dir):
    try:
        _os.add_dll_directory(_pkg_dir)
    except OSError:
        pass

from ._SimpleCommKitPyUsb import (
    get_error_description,
    UsbDeviceInfo,
    IsochronousPacketResult,
    SimpleCommKitUsb,
)

__all__ = [
    "get_error_description",
    "UsbDeviceInfo",
    "IsochronousPacketResult",
    "SimpleCommKitUsb",
]
