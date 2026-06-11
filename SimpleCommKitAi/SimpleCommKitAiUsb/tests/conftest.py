# SimpleCommKitAiUsb Test Config
import pytest
from unittest.mock import MagicMock


class MockUsbDeviceInfo:
    """Mock USB device info for tests."""
    def __init__(self, path="1:3", vid=0x1234, pid=0x5678,
                 manufacturer="TestMfg", product="TestDev", serial="SN001",
                 bus=1, addr=3):
        self.path = path
        self.vendor_id = vid
        self.product_id = pid
        self.manufacturer_string = manufacturer
        self.product_string = product
        self.serial_number = serial
        self.bus_number = bus
        self.device_address = addr


class MockSimpleCommKitUsb:
    """Mock SimpleCommKitUsb for testing without hardware."""

    def __init__(self):
        self._open_paths = []
        self._init_done = False
        self._hotplug_active = False
        self._on_read = None
        self._on_hotplug = None
        self._on_error = None
        self._poll_ms = 100
        self._data_len = 64

    def init(self):
        self._init_done = True
        return True

    def exit(self):
        self._init_done = False

    def open(self, path_or_vid, pid=0, serial=""):
        if isinstance(path_or_vid, str):
            self._open_paths.append(path_or_vid)
            return True
        else:
            path = f"{path_or_vid & 0xFF}:{pid & 0xFF}"
            self._open_paths.append(path)
            return True

    def close(self, path=None):
        if path:
            if path in self._open_paths:
                self._open_paths.remove(path)
        else:
            self._open_paths.clear()

    def is_open(self, path=None):
        if path:
            return path in self._open_paths
        return len(self._open_paths) > 0

    def claim_Interface(self, path, iface):
        return True

    def release_Interface(self, path, iface):
        return True

    def control_transfer(self, path, bm, br, wv, wi, data, timeout=1000):
        if hasattr(data, '__len__') and len(data) > 0:
            return bytes(len(data))
        return bytes(0)

    def bulk_transfer_out(self, path, ep, data, timeout=1000):
        return len(data)

    def bulk_transfer_in(self, path, ep, length, timeout=1000):
        return bytes(length)

    def interrupt_transfer_out(self, path, ep, data, timeout=1000):
        return len(data)

    def interrupt_transfer_in(self, path, ep, length, timeout=1000):
        return bytes(length)

    def start_read_poll(self, path, ep):
        pass

    def stop_read_poll(self, path):
        pass

    def is_read_poll_active(self):
        pass

    def start_hotplug(self, vid=0, pid=0):
        self._hotplug_active = True

    def stop_hotplug(self):
        self._hotplug_active = False

    def is_hotplug_active(self):
        return self._hotplug_active

    def set_hotplug_poll_interval(self, ms):
        pass

    def get_hotplug_poll_interval(self):
        return 1000

    def set_read_poll_interval(self, ms):
        self._poll_ms = ms

    def get_read_poll_interval(self):
        return self._poll_ms

    def set_read_data_length(self, length):
        self._data_len = length

    def get_read_data_length(self):
        return self._data_len

    def get_open_paths(self):
        return list(self._open_paths)

    def get_device_list(self):
        return [MockUsbDeviceInfo()]

    def set_callback_on_read(self, cb):
        self._on_read = cb

    def set_callback_on_hotplug(self, cb):
        self._on_hotplug = cb

    def set_callback_error(self, cb):
        self._on_error = cb

    def get_available_devices(vid=0, pid=0):
        return [MockUsbDeviceInfo()]


def mock_usb_module():
    """Create a mock for SimpleCommKitPyUsb."""
    mock = MagicMock()
    mock.SimpleCommKitUsb = MockSimpleCommKitUsb
    mock.UsbDeviceInfo = MockUsbDeviceInfo
    mock.get_error_description = lambda code: f"Error {code}"
    return mock
