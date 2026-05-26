"""Pytest fixtures for SimpleCommKitAiSerialPort tests."""

import pytest
from unittest.mock import MagicMock, patch


class MockSerialPortInfo:
    """Mock serial port info."""
    def __init__(self, port_name="COM3", description="Mock Serial Port", hardware_id="USB\\VID_1234"):
        self.port_name = port_name
        self.description = description
        self.hardware_id = hardware_id


class MockSerialPort:
    """Mock SerialPort that simulates basic serial operations."""

    def __init__(self):
        self._port_name = ""
        self._is_open = False
        self._baud_rate = 9600
        self._parity = 0
        self._data_bits = 8
        self._stop_bits = 0
        self._flow_control = 0
        self._read_buffer_size = 4096
        self._read_interval_timeout = 0
        self._on_read_callback = None
        self._on_hotplug_callback = None
        self._error_callback = None
        self._last_error = 0
        self._last_error_msg = ""

    # ---- Static ----
    @staticmethod
    def get_available_ports():
        return [MockSerialPortInfo("COM3", "Mock Serial Port"), MockSerialPortInfo("COM4", "Another Mock")]

    # ---- Lifecycle ----
    def init(self, port_name, baud_rate=9600, parity=0, data_bits=8,
             stop_bits=0, flow_control=0, read_buffer_size=4096):
        self._port_name = port_name
        self._baud_rate = baud_rate
        self._parity = parity
        self._data_bits = data_bits
        self._stop_bits = stop_bits
        self._flow_control = flow_control
        self._read_buffer_size = read_buffer_size

    def open(self):
        self._is_open = True
        return True

    def close(self):
        self._is_open = False

    def is_open(self):
        return self._is_open

    # ---- I/O ----
    def read(self, size):
        return b"mock_serial_data"

    def read_all(self):
        return b"mock_all_data"

    def write(self, data):
        return len(data)

    # ---- Flush ----
    def flush_buffers(self):
        return True

    def flush_read_buffers(self):
        return True

    def flush_write_buffers(self):
        return True

    # ---- Error ----
    def get_last_error(self):
        return self._last_error

    def get_last_error_msg(self):
        return self._last_error_msg

    def clear_error(self):
        self._last_error = 0
        self._last_error_msg = ""

    # ---- Config getters/setters ----
    def set_port_name(self, name):
        self._port_name = name

    def get_port_name(self):
        return self._port_name

    def set_baud_rate(self, rate):
        self._baud_rate = rate

    def get_baud_rate(self):
        return self._baud_rate

    def set_parity(self, parity):
        self._parity = parity

    def get_parity(self):
        return self._parity

    def set_data_bits(self, bits):
        self._data_bits = bits

    def get_data_bits(self):
        return self._data_bits

    def set_stop_bits(self, bits):
        self._stop_bits = bits

    def get_stop_bits(self):
        return self._stop_bits

    def set_flow_control(self, fc):
        self._flow_control = fc

    def get_flow_control(self):
        return self._flow_control

    def set_read_buffer_size(self, size):
        self._read_buffer_size = size

    def get_read_buffer_size(self):
        return self._read_buffer_size

    def set_dtr(self, set=True):
        pass

    def set_rts(self, set=True):
        pass

    def set_read_interval_timeout(self, msecs):
        self._read_interval_timeout = msecs

    def get_read_interval_timeout(self):
        return self._read_interval_timeout

    # ---- Callbacks ----
    def set_callback_on_read(self, callback):
        self._on_read_callback = callback

    def set_callback_on_hotplug(self, callback):
        self._on_hotplug_callback = callback

    def set_callback_error(self, callback):
        self._error_callback = callback


def get_error_description(code):
    """Mock error description."""
    return f"Error {code}"


@pytest.fixture
def mock_serial_module():
    """Fixture that patches SimpleCommKitAiSerialPort with mock serial classes."""
    with patch("SimpleCommKitAiSerialPort.SerialPort", MockSerialPort), \
         patch("SimpleCommKitAiSerialPort.SerialPortInfo", MockSerialPortInfo), \
         patch("SimpleCommKitAiSerialPort.BaudRate", MagicMock()), \
         patch("SimpleCommKitAiSerialPort.Parity", MagicMock()), \
         patch("SimpleCommKitAiSerialPort.DataBits", MagicMock()), \
         patch("SimpleCommKitAiSerialPort.StopBits", MagicMock()), \
         patch("SimpleCommKitAiSerialPort.FlowControl", MagicMock()), \
         patch("SimpleCommKitAiSerialPort.get_error_description", get_error_description):
        yield


@pytest.fixture
def mock_serial_port():
    """Fixture that returns a fresh MockSerialPort instance."""
    return MockSerialPort()
