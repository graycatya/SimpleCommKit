"""Basic import and structure tests for SimpleCommKitAiBle."""

import pytest


def test_package_import():
    """Test that the package can be imported."""
    import SimpleCommKitAiBle
    assert SimpleCommKitAiBle.__version__ == "0.1.0"


def test_http_module_import(mock_ble_module):
    """Test that http module can be imported (with mocked BLE)."""
    from SimpleCommKitAiBle import http
    assert hasattr(http, "app")
    assert hasattr(http, "main")


def test_mcp_module_import(mock_ble_module):
    """Test that mcp module can be imported (with mocked BLE)."""
    from SimpleCommKitAiBle import mcp
    assert hasattr(mcp, "mcp")
    assert hasattr(mcp, "main")


def test_package_exports(mock_ble_module):
    """Test that package exports expected names (BleCentral, Adapter, etc.)."""
    from SimpleCommKitAiBle import BleCentral, Adapter, Peripheral, get_error_description
    assert BleCentral is not None
    assert Adapter is not None
    assert Peripheral is not None
    assert callable(get_error_description)


def test_http_module_attributes():
    """Test http module has expected classes and functions."""
    import importlib
    with pytest.MonkeyPatch.context() as mp:
        mp.setattr("SimpleCommKitAiBle.BleCentral", object)
        mp.setattr("SimpleCommKitAiBle.Adapter", object)
        mp.setattr("SimpleCommKitAiBle.Peripheral", object)
        mp.setattr("SimpleCommKitAiBle.get_error_description", lambda x: "")
        mod = importlib.import_module("SimpleCommKitAiBle.http")
        assert hasattr(mod, "BleState")
        assert hasattr(mod, "WriteRequest")
        assert hasattr(mod, "app")
