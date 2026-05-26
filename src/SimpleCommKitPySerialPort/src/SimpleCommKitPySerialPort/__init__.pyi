"""
Type stubs for SimpleCommKitPySerialPort - Python bindings for SimpleCommKit SerialPort
"""
from typing import Callable, List, Optional
from enum import IntEnum


def get_error_description(error_code: int) -> str:
    """Get a human-readable description for an error code."""
    ...


class BaudRate(IntEnum):
    """Serial port baud rate enumeration."""
    BAUD_110: int = 110
    BAUD_300: int = 300
    BAUD_600: int = 600
    BAUD_1200: int = 1200
    BAUD_2400: int = 2400
    BAUD_4800: int = 4800
    BAUD_9600: int = 9600
    BAUD_14400: int = 14400
    BAUD_19200: int = 19200
    BAUD_38400: int = 38400
    BAUD_56000: int = 56000
    BAUD_57600: int = 57600
    BAUD_115200: int = 115200
    BAUD_921600: int = 921600


class Parity(IntEnum):
    """Serial port parity enumeration."""
    NONE: int = 0
    ODD: int = 1
    EVEN: int = 2
    MARK: int = 3
    SPACE: int = 4


class DataBits(IntEnum):
    """Serial port data bits enumeration."""
    BITS_5: int = 5
    BITS_6: int = 6
    BITS_7: int = 7
    BITS_8: int = 8


class StopBits(IntEnum):
    """Serial port stop bits enumeration."""
    ONE: int = 0
    ONE_AND_HALF: int = 1
    TWO: int = 2


class FlowControl(IntEnum):
    """Serial port flow control enumeration."""
    NONE: int = 0
    HARDWARE: int = 1
    SOFTWARE: int = 2


class SerialPortInfo:
    """Represents information about an available serial port."""
    port_name: str
    description: str
    hardware_id: str
    def __init__(self) -> None: ...


class SerialPort:
    """Serial port manager for opening, reading, writing, and monitoring serial ports."""

    def __init__(self) -> None: ...

    @staticmethod
    def get_available_ports() -> List[SerialPortInfo]:
        """Get a list of available serial ports on the system."""
        ...

    def init(
        self,
        port_name: str,
        baud_rate: int = BaudRate.BAUD_9600,
        parity: Parity = Parity.NONE,
        data_bits: DataBits = DataBits.BITS_8,
        stop_bits: StopBits = StopBits.ONE,
        flow_control: FlowControl = FlowControl.NONE,
        read_buffer_size: int = 4096,
    ) -> None:
        """Initialize the serial port with the given parameters."""
        ...

    def open(self) -> bool:
        """Open the serial port. Returns True on success."""
        ...

    def close(self) -> None:
        """Close the serial port."""
        ...

    def is_open(self) -> bool:
        """Check if the serial port is currently open."""
        ...

    def read(self, size: int) -> bytes:
        """Read up to 'size' bytes from the serial port. Returns bytes."""
        ...

    def read_all(self) -> bytes:
        """Read all available bytes from the serial port. Returns bytes."""
        ...

    def write(self, data: bytes) -> int:
        """Write data to the serial port. Data is bytes. Returns number of bytes written."""
        ...

    def flush_buffers(self) -> bool:
        """Flush all buffers (read and write). Returns True on success."""
        ...

    def flush_read_buffers(self) -> bool:
        """Flush read buffer. Returns True on success."""
        ...

    def flush_write_buffers(self) -> bool:
        """Flush write buffer. Returns True on success."""
        ...

    def get_last_error(self) -> int:
        """Get the last error code."""
        ...

    def get_last_error_msg(self) -> str:
        """Get the last error message string."""
        ...

    def clear_error(self) -> None:
        """Clear the last error."""
        ...

    def set_port_name(self, port_name: str) -> None:
        """Set the serial port name (e.g. COM3, /dev/ttyUSB0)."""
        ...

    def get_port_name(self) -> str:
        """Get the serial port name."""
        ...

    def set_baud_rate(self, baud_rate: int) -> None:
        """Set the baud rate (use BaudRate enum values or any custom int)."""
        ...

    def get_baud_rate(self) -> int:
        """Get the current baud rate."""
        ...

    def set_parity(self, parity: Parity) -> None:
        """Set the parity mode (use Parity enum values)."""
        ...

    def get_parity(self) -> Parity:
        """Get the current parity mode."""
        ...

    def set_data_bits(self, data_bits: DataBits) -> None:
        """Set the data bits (use DataBits enum values)."""
        ...

    def get_data_bits(self) -> DataBits:
        """Get the current data bits."""
        ...

    def set_stop_bits(self, stop_bits: StopBits) -> None:
        """Set the stop bits (use StopBits enum values)."""
        ...

    def get_stop_bits(self) -> StopBits:
        """Get the current stop bits."""
        ...

    def set_flow_control(self, flow_control: FlowControl) -> None:
        """Set the flow control mode (use FlowControl enum values)."""
        ...

    def get_flow_control(self) -> FlowControl:
        """Get the current flow control mode."""
        ...

    def set_read_buffer_size(self, size: int) -> None:
        """Set the read buffer size in bytes."""
        ...

    def get_read_buffer_size(self) -> int:
        """Get the current read buffer size."""
        ...

    def set_dtr(self, set: bool = True) -> None:
        """Set or clear the DTR (Data Terminal Ready) line."""
        ...

    def set_rts(self, set: bool = True) -> None:
        """Set or clear the RTS (Request To Send) line."""
        ...

    def set_read_interval_timeout(self, msecs: int) -> None:
        """Set the read interval timeout in milliseconds."""
        ...

    def get_read_interval_timeout(self) -> int:
        """Get the current read interval timeout in milliseconds."""
        ...

    def set_callback_on_read(self, callback: Callable[[bytes], None]) -> None:
        """Set callback invoked when data is received. Callback receives bytes payload."""
        ...

    def set_callback_on_hotplug(self, callback: Callable[[str, bool], None]) -> None:
        """Set callback invoked on hot-plug events.
        Callback receives (port_name: str, is_add: bool)."""
        ...

    def set_callback_error(self, callback: Callable[[int], None]) -> None:
        """Set a callback to receive error notifications.
        Callback receives error_code (int); use get_error_description(error_code) for details."""
        ...
