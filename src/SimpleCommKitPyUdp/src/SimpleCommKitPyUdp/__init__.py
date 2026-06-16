"""
SimpleCommKitPyUdp - Python bindings for SimpleCommKit UDP

Cross-platform UDP client and server toolkit for Python.
Based on SimpleCommKit's C++ UDP module.

Quick Start (Client)::

    from simple_comm_kit_udp import UdpClient

    client = UdpClient()
    client.open()
    client.set_callback_on_message(lambda data: print(f"Received: {data.hex()}"))
    client.send_to("127.0.0.1", 8080, b"Hello!")
    client.close()

Quick Start (Server)::

    from simple_comm_kit_udp import UdpServer

    server = UdpServer()
    server.set_callback_on_message(lambda host, port, data: print(f"From {host}:{port}: {data.hex()}"))
    server.start(8080)
    # ... receive datagrams ...
    server.stop()
"""

import os as _os
import sys as _sys

# Register package directory for DLL loading (needed for Windows)

from ._SimpleCommKitPyUdp import get_version
__version__ = get_version()

_pkg_dir = _os.path.dirname(_os.path.abspath(__file__))
if _sys.platform == "win32" and _os.path.isdir(_pkg_dir):
    try:
        _os.add_dll_directory(_pkg_dir)
    except OSError:
        pass  # Directory may already be registered

from ._SimpleCommKitPyUdp import (
    # Version / Utility
    get_error_description,

    # Classes
    UdpClient,
    UdpServer,
)

__all__ = [
    "get_error_description",
    "UdpClient",
    "UdpServer",
]
