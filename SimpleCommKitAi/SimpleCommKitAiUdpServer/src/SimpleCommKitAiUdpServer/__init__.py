"""
SimpleCommKitAiUdpServer - AI-friendly UDP server toolkit powered by SimpleCommKitPyUdp
"""

from SimpleCommKitPyUdp import __version__

try:
    from SimpleCommKitPyUdp import UdpServer, get_error_description
except ImportError:
    UdpServer = None
    get_error_description = lambda code: f"Unknown error {code}"

__all__ = ["__version__", "UdpServer", "get_error_description"]
