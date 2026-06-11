# Tests for MCP tools (AiUsb)
import pytest
from unittest.mock import MagicMock, patch
import sys
from conftest import MockUsbDeviceInfo, MockSimpleCommKitUsb, mock_usb_module

# Inject mock before importing the mcp module
mock_mod = mock_usb_module()
sys.modules["SimpleCommKitPyUsb"] = mock_mod

# Now import — it will use our mock
import SimpleCommKitAiUsb.mcp as usb_mcp


@pytest.fixture(autouse=True)
def reset_state():
    """Reset global state before each test."""
    usb_mcp.usb_state._initialized = False
    usb_mcp.usb_state._hotplug_active = False
    usb_mcp.usb_state._read_buffer.clear()
    yield


def test_get_available_devices():
    result = usb_mcp.get_available_devices()
    assert isinstance(result, list)
    assert len(result) > 0
    assert "path" in result[0]
    assert "vendor_id" in result[0]
    assert "product_id" in result[0]


def test_open_device_by_path():
    result = usb_mcp.open_device(path="1:3")
    assert result["path"] == "1:3"


def test_open_device_by_vidpid():
    result = usb_mcp.open_device(vendor_id=0x1234, product_id=0x5678)
    assert result["path"]


def test_open_device_no_args():
    with pytest.raises(RuntimeError, match="Must provide"):
        usb_mcp.open_device()


def test_close_device():
    usb_mcp.open_device(path="1:3")
    result = usb_mcp.close_device("1:3")
    assert "Closed" in result["message"]


def test_close_all():
    usb_mcp.open_device(path="1:3")
    usb_mcp.open_device(path="2:4")
    result = usb_mcp.close_device()
    assert "Closed all" in result["message"]


def test_claim_interface():
    usb_mcp.open_device(path="1:3")
    result = usb_mcp.claim_interface("1:3", 0)
    assert "Claimed" in result["message"]


def test_release_interface():
    usb_mcp.open_device(path="1:3")
    result = usb_mcp.release_interface("1:3", 0)
    assert "Released" in result["message"]


def test_control_transfer():
    usb_mcp.open_device(path="1:3")
    result = usb_mcp.control_transfer("1:3", 0x80, 0x06, 0x0100, length=18)
    assert result["length"] is not None


def test_bulk_transfer_out():
    usb_mcp.open_device(path="1:3")
    result = usb_mcp.bulk_transfer_out("1:3", 0x01, "ABCDEF")
    assert result["bytes_transferred"] == 3


def test_bulk_transfer_in():
    usb_mcp.open_device(path="1:3")
    result = usb_mcp.bulk_transfer_in("1:3", 0x81, 64)
    assert "length" in result


def test_interrupt_transfer_out():
    usb_mcp.open_device(path="1:3")
    result = usb_mcp.interrupt_transfer_out("1:3", 0x01, "FF")
    assert result["bytes_transferred"] == 1


def test_interrupt_transfer_in():
    usb_mcp.open_device(path="1:3")
    result = usb_mcp.interrupt_transfer_in("1:3", 0x81, 8)
    assert "data_hex" in result


def test_start_stop_read_poll():
    usb_mcp.open_device(path="1:3")
    result = usb_mcp.start_read_poll("1:3", 0x81)
    assert "Read poll started" in result["message"]

    result = usb_mcp.stop_read_poll("1:3")
    assert "stopped" in result["message"]


def test_get_read_data_empty():
    result = usb_mcp.get_read_data("1:3")
    assert result == []


def test_start_stop_hotplug():
    result = usb_mcp.start_hotplug(0x1234, 0x5678)
    assert result["vendor_id"] == 0x1234

    result = usb_mcp.stop_hotplug()
    assert "stopped" in result["message"]


def test_get_device_list():
    result = usb_mcp.get_device_list()
    assert isinstance(result, list)
    assert len(result) > 0
    assert "path" in result[0]


def test_get_open_paths():
    usb_mcp.open_device(path="1:3")
    result = usb_mcp.get_open_paths()
    assert result["open_count"] >= 1
    assert "1:3" in result["open_paths"]


def test_set_read_config():
    result = usb_mcp.set_read_config(poll_interval_ms=50, data_length=128)
    assert result["read_poll_interval_ms"] == 50
    assert result["read_data_length"] == 128
