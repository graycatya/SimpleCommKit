"""Pytest fixtures for SimpleCommKitAiHid tests."""

import pytest
from unittest.mock import MagicMock, patch


class MockHidDeviceInfo:
    """Mock HID device info."""

    def __init__(self, path="/dev/hidraw0", product_string="Mock HID Device",
                 manufacturer_string="Mock Corp", serial_number="SN001"):
        self.path = path
        self.product_string = product_string
        self.manufacturer_string = manufacturer_string
        self.serial_number = serial_number
        self.interface_number = 0
        self.release_number = 0x0100
        self.bus_type = 1  # USB


class MockSimpleCommKitHid:
    """Mock SimpleCommKitHid that simulates basic HID operations."""

    def __init__(self):
        self._devices = [MockHidDeviceInfo()]
        self._open_paths = []
        self._error_callback = None
        self._read_callback = None
        self._hotplug_callback = None
        self._hotplug_active = False
        self._hotplug_poll_interval = 1000
        self._read_poll_interval = 100
        self._read_data_length = 64

    # ---- Static ----
    def get_available_devices(self, vendor_id=0, product_id=0):
        return self._devices

    # ---- Lifecycle ----
    def init(self, vendor_id=0, product_id=0):
        return True

    def exit(self):
        pass

    def open(self, *args):
        # args: (path, readable) or (vid, pid, serial, readable)
        if isinstance(args[0], str):
            path = args[0]
        else:
            # VID/PID open → use first device path
            path = self._devices[0].path if self._devices else "/dev/hidraw0"
        self._open_paths.append(path)
        return True

    def close(self, path=None):
        if path:
            self._open_paths = [p for p in self._open_paths if p != path]
        else:
            self._open_paths.clear()

    def is_open(self, path=None):
        if path:
            return path in self._open_paths
        return len(self._open_paths) > 0

    # ---- I/O ----
    def write(self, path, data):
        return len(data)

    def send_feature_report(self, path, data):
        return len(data)

    # ---- Hotplug ----
    def start_hotplug(self, vendor_id, product_id):
        self._hotplug_active = True

    def stop_hotplug(self):
        self._hotplug_active = False

    def is_hotplug_active(self):
        return self._hotplug_active

    def set_hotplug_poll_interval(self, ms):
        self._hotplug_poll_interval = ms

    def get_hotplug_poll_interval(self):
        return self._hotplug_poll_interval

    # ---- Read config ----
    def set_read_poll_interval(self, *args):
        if isinstance(args[0], str):
            pass  # per-path
        else:
            self._read_poll_interval = args[0]

    def get_read_poll_interval(self, path=None):
        return self._read_poll_interval

    def set_read_data_length(self, *args):
        if isinstance(args[0], str):
            pass  # per-path
        else:
            self._read_data_length = args[0]

    def get_read_data_length(self, path=None):
        return self._read_data_length

    # ---- Device list ----
    def get_open_paths(self):
        return self._open_paths

    def get_device_list(self):
        return self._devices

    # ---- Callbacks ----
    def set_callback_on_read(self, callback):
        self._read_callback = callback

    def set_callback_on_hotplug(self, callback):
        self._hotplug_callback = callback

    def set_callback_error(self, callback):
        self._error_callback = callback


def get_error_description(code):
    """Mock error description."""
    return f"Error {code}"


@pytest.fixture
def mock_hid_module():
    """Fixture that patches SimpleCommKitAiHid with mock HID classes."""
    with patch("SimpleCommKitAiHid.SimpleCommKitHid", MockSimpleCommKitHid), \
         patch("SimpleCommKitAiHid.HidDeviceInfo", MockHidDeviceInfo), \
         patch("SimpleCommKitAiHid.get_error_description", get_error_description):
        yield


@pytest.fixture
def mock_hid():
    """Fixture that returns a fresh MockSimpleCommKitHid instance."""
    return MockSimpleCommKitHid()
