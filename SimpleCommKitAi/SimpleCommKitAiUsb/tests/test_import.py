# Test imports for SimpleCommKitAiUsb
def test_package_import():
    """Test that the package can be imported."""
    import SimpleCommKitAiUsb
    assert SimpleCommKitAiUsb.__version__ is not None  # version from VERSION file


def test_submodules_import():
    """Test that http and mcp submodules can be imported."""
    from SimpleCommKitAiUsb import http
    from SimpleCommKitAiUsb import mcp
    assert http is not None
    assert mcp is not None


def test_exports():
    """Test that expected names are exported."""
    import SimpleCommKitAiUsb
    assert hasattr(SimpleCommKitAiUsb, "__version__")
    assert hasattr(SimpleCommKitAiUsb, "get_error_description")
