"""
SimpleCommKitPyTcp - Python bindings for SimpleCommKit TCP

Cross-platform TCP client and server toolkit for Python.
Based on SimpleCommKit's C++ TCP module.

Quick Start (Client)::

    from simple_comm_kit_tcp import TcpClient

    client = TcpClient()
    client.set_callback_on_message(lambda data: print(f"Received: {data.hex()}"))
    client.connect("127.0.0.1", 8080)
    client.send(b"Hello Server!")
    client.disconnect()

Quick Start (Server)::

    from simple_comm_kit_tcp import TcpServer

    server = TcpServer()
    server.set_callback_on_client_connected(lambda cid: print(f"Client {cid} connected"))
    server.set_callback_on_message(lambda cid, data: print(f"From {cid}: {data.hex()}"))
    server.start(8080)
    # ... accept connections ...
    server.stop()
"""

import os as _os
import sys as _sys

# Register package directory for DLL loading (needed for Windows)

from ._SimpleCommKitPyTcp import get_version
__version__ = get_version()

_pkg_dir = _os.path.dirname(_os.path.abspath(__file__))
if _sys.platform == "win32" and _os.path.isdir(_pkg_dir):
    try:
        _os.add_dll_directory(_pkg_dir)
    except OSError:
        pass  # Directory may already be registered

from ._SimpleCommKitPyTcp import (
    # Version / Utility
    get_error_description,

    # Structs
    TcpReconnectSetting,
    TlsSetting,

    # Classes
    TcpClient,
    TcpServer,
)

__all__ = [
    "get_error_description",
    "TcpReconnectSetting",
    "TlsSetting",
    "TcpClient",
    "TcpServer",
]
