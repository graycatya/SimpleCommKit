"""Basic import and structure tests for SimpleCommKitAiHid."""

import pytest


def test_package_import():
    """Test that the package can be imported."""
    import SimpleCommKitAiHid
    assert SimpleCommKitAiHid.__version__ == "0.1.0"


def test_http_module_import(mock_hid_module):
    """Test that http module can be imported (with mocked HID)."""
    from SimpleCommKitAiHid import http
    assert hasattr(http, "app")
    assert hasattr(http, "main")


def test_mcp_module_import(mock_hid_module):
    """Test that mcp module can be imported (with mocked HID)."""
    from SimpleCommKitAiHid import mcp
    assert hasattr(mcp, "mcp")
    assert hasattr(mcp, "main")


def test_package_exports(mock_hid_module):
    """Test that package exports expected names."""
    from SimpleCommKitAiHid import (
        SimpleCommKitHid, HidDeviceInfo, HidBusType, get_error_description
    )
    assert SimpleCommKitHid is not None
    assert HidDeviceInfo is not None
    assert HidBusType is not None
    assert callable(get_error_description)


def test_http_module_attributes():
    """Test http module has expected classes and functions."""
    import importlib
    with pytest.MonkeyPatch.context() as mp:
        mp.setattr("SimpleCommKitAiHid.SimpleCommKitHid", object)
        mp.setattr("SimpleCommKitAiHid.HidDeviceInfo", object)
        mp.setattr("SimpleCommKitAiHid.HidBusType", object)
        mp.setattr("SimpleCommKitAiHid.get_error_description", lambda x: "")
        mod = importlib.import_module("SimpleCommKitAiHid.http")
        assert hasattr(mod, "HidState")
        assert hasattr(mod, "WriteRequest")
        assert hasattr(mod, "app")
