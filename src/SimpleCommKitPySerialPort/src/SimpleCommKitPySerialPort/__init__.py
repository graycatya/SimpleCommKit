"""
SimpleCommKitPySerialPort - Python bindings for SimpleCommKit SerialPort

Cross-platform serial port toolkit for Python.
Based on SimpleCommKit's C++ SerialPort module.

Quick Start::

    from simple_comm_kit_serialport import SerialPort

    sp = SerialPort()

    # List available ports
    ports = SerialPort.get_available_ports()
    for p in ports:
        print(f"Port: {p.port_name} - {p.description}")

    # Open a serial port
    sp.init(
        port_name="COM3",
        baud_rate=BaudRate.BAUD_9600,
        parity=Parity.NONE,
        data_bits=DataBits.BITS_8,
        stop_bits=StopBits.ONE,
        flow_control=FlowControl.NONE,
    )
    if sp.open():
        # Write data
        sp.write(b"Hello Serial!")
        # Read data
        data = sp.read(100)
        print(f"Received: {data.hex()}")
        sp.close()
"""

import os as _os
import sys as _sys

# Register package directory for DLL loading (needed for Windows)

from ._SimpleCommKitPySerialPort import get_version
__version__ = get_version()

_pkg_dir = _os.path.dirname(_os.path.abspath(__file__))
if _sys.platform == "win32" and _os.path.isdir(_pkg_dir):
    try:
        _os.add_dll_directory(_pkg_dir)
    except OSError:
        pass  # Directory may already be registered

from ._SimpleCommKitPySerialPort import (
    # Version / Utility
    get_error_description,

    # Enums
    BaudRate,
    Parity,
    DataBits,
    StopBits,
    FlowControl,

    # Structs
    SerialPortInfo,

    # Main class
    SerialPort,
)

__all__ = [
    "get_error_description",
    "BaudRate",
    "Parity",
    "DataBits",
    "StopBits",
    "FlowControl",
    "SerialPortInfo",
    "SerialPort",
]
