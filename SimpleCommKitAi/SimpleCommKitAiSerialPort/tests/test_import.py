"""Basic import and structure tests for SimpleCommKitAiSerialPort."""

import pytest


def test_package_import():
    """Test that the package can be imported."""
    import SimpleCommKitAiSerialPort
    assert SimpleCommKitAiSerialPort.__version__ == "0.1.0"


def test_http_module_import(mock_serial_module):
    """Test that http module can be imported (with mocked SerialPort)."""
    from SimpleCommKitAiSerialPort import http
    assert hasattr(http, "app")
    assert hasattr(http, "main")


def test_mcp_module_import(mock_serial_module):
    """Test that mcp module can be imported (with mocked SerialPort)."""
    from SimpleCommKitAiSerialPort import mcp
    assert hasattr(mcp, "mcp")
    assert hasattr(mcp, "main")


def test_package_exports(mock_serial_module):
    """Test that package exports expected names."""
    from SimpleCommKitAiSerialPort import (
        SerialPort,
        SerialPortInfo,
        BaudRate,
        Parity,
        DataBits,
        StopBits,
        FlowControl,
        get_error_description,
    )
    assert SerialPort is not None
    assert SerialPortInfo is not None
    assert callable(get_error_description)


def test_http_module_attributes():
    """Test http module has expected classes and functions."""
    import importlib
    with pytest.MonkeyPatch.context() as mp:
        mp.setattr("SimpleCommKitAiSerialPort.SerialPort", object)
        mp.setattr("SimpleCommKitAiSerialPort.SerialPortInfo", object)
        mp.setattr("SimpleCommKitAiSerialPort.BaudRate", object)
        mp.setattr("SimpleCommKitAiSerialPort.Parity", object)
        mp.setattr("SimpleCommKitAiSerialPort.DataBits", object)
        mp.setattr("SimpleCommKitAiSerialPort.StopBits", object)
        mp.setattr("SimpleCommKitAiSerialPort.FlowControl", object)
        mp.setattr("SimpleCommKitAiSerialPort.get_error_description", lambda x: "")
        mod = importlib.import_module("SimpleCommKitAiSerialPort.http")
        assert hasattr(mod, "PortState")
        assert hasattr(mod, "WriteRequest")
        assert hasattr(mod, "ConfigRequest")
        assert hasattr(mod, "OpenRequest")
        assert hasattr(mod, "app")
