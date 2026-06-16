"""
SimpleCommKitAiUdpClient - AI-friendly UDP client toolkit powered by SimpleCommKitPyUdp
"""

from SimpleCommKitPyUdp import __version__

try:
    from SimpleCommKitPyUdp import UdpClient, get_error_description
except ImportError:
    UdpClient = None
    get_error_description = lambda code: f"Unknown error {code}"

__all__ = ["__version__", "UdpClient", "get_error_description"]
