"""SimpleCommKitAiHid MCP Server - Expose HID operations as MCP tools for AI agents."""

from fastmcp import FastMCP
import argparse
from typing import Dict, List, Optional

from SimpleCommKitAiHid import SimpleCommKitHid, HidDeviceInfo, HidBusType, get_error_description


class HidState:
    """Holds global HID state shared across MCP tools."""

    def __init__(self) -> None:
        self.hid: Optional[SimpleCommKitHid] = None
        self.device_cache: List = []
        self._initialized: bool = False
        self._hotplug_active: bool = False
        self._read_buffer: Dict[str, List[Dict]] = {}

    def init(self) -> bool:
        """Lazy-initialize HID hardware. Safe to call multiple times.
        Returns True if HID is ready, False otherwise."""
        if self._initialized:
            return self.hid is not None
        try:
            self.hid = SimpleCommKitHid()
            self.hid.set_callback_error(lambda code: print(
                f"[SimpleCommKitAiHid] Error {code}: {get_error_description(code)}"
            ))
            self._initialized = True
        except Exception as e:
            print(f"[SimpleCommKitAiHid] HID init failed: {e}")
            self._initialized = True
        return self.hid is not None

    def _ensure_ready(self) -> None:
        """Raise if HID is not initialized."""
        if not self.init():
            raise RuntimeError("HID hardware not available")

    def _ensure_hid(self) -> SimpleCommKitHid:
        self._ensure_ready()
        return self.hid

    def _device_to_dict(self, d) -> Dict:
        """Convert HidDeviceInfo to a plain dict."""
        return {
            "path": d.path,
            "manufacturer_string": d.manufacturer_string,
            "product_string": d.product_string,
            "serial_number": d.serial_number,
            "bus_type": int(d.bus_type) if hasattr(d, 'bus_type') else 0,
            "bus_type_name": _bus_type_name(int(d.bus_type) if hasattr(d, 'bus_type') else 0),
            "interface_number": d.interface_number,
            "release_number": d.release_number,
        }

    def _setup_read_callback(self, path: str):
        """Register a read callback that buffers data for this path."""
        hid = self._ensure_hid()

        def read_callback(data: bytes, device_path: str) -> None:
            if path not in self._read_buffer:
                self._read_buffer[path] = []
            self._read_buffer[path].append({
                "path": path,
                "data_hex": data.hex(),
                "data_utf8": data.decode("utf-8", errors="ignore"),
                "data_length": len(data),
            })

        hid.set_callback_on_read(read_callback)


hid_state = HidState()


def _bus_type_name(bus_type: int) -> str:
    """Convert bus type int to human-readable name."""
    names = {0: "UNKNOWN", 1: "USB", 2: "BLUETOOTH", 3: "I2C", 4: "SPI"}
    return names.get(bus_type, f"UNKNOWN({bus_type})")


mcp = FastMCP(name="SimpleCommKitAiHid MCP Server")


@mcp.tool(
    name="get_available_devices",
    description="List available HID (Human Interface Device) devices on the host. "
                "Optionally filter by vendor_id and product_id.",
    tags={"hid", "usb", "read", "discovery"},
    annotations={
        "title": "Get Available Devices",
        "readOnlyHint": True,
        "idempotentHint": True,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "discovery"},
)
def get_available_devices(vendor_id: int = 0, product_id: int = 0) -> List[Dict]:
    """List all available HID devices, optionally filtered by VID/PID."""
    hid_state._ensure_ready()
    hid = hid_state._ensure_hid()
    devices = hid.get_available_devices(vendor_id, product_id)
    hid_state.device_cache = devices
    return [hid_state._device_to_dict(d) for d in devices]


@mcp.tool(
    name="open_device",
    description="Open a HID device for communication. "
                "You must provide either a path (e.g. '/dev/hidraw0' on Linux, a path on macOS/Windows) "
                "or a combination of vendor_id + product_id + optional serial_number. "
                "Set readable=False for write-only mode.",
    tags={"hid", "open", "connection"},
    annotations={
        "title": "Open Device",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "connection"},
)
def open_device(
    path: str = "",
    vendor_id: int = 0,
    product_id: int = 0,
    serial_number: str = "",
    readable: bool = True,
) -> Dict:
    """Open a HID device by path or by VID/PID/serial.

    path: Platform-specific device path (takes priority if provided).
    vendor_id, product_id: USB VID/PID to match (used when path is empty).
    serial_number: Optional serial number filter.
    readable: Whether to enable read mode (default True).

    Returns the open device path on success.
    """
    hid_state._ensure_ready()
    hid = hid_state._ensure_hid()

    if not hid.init():
        raise RuntimeError("HID init failed")

    opened_path = ""

    if path:
        if hid.is_open(path):
            return {"message": f"Device already open: {path}", "path": path}
        # Set read callback BEFORE open to avoid missing initial data
        if readable:
            hid_state._setup_read_callback(path)
            hid_state._read_buffer.setdefault(path, [])
        if hid.open(path, readable):
            opened_path = path
        else:
            raise RuntimeError(f"Failed to open {path}")
    elif vendor_id != 0 and product_id != 0:
        if hid.open(vendor_id, product_id, serial_number, readable):
            paths = hid.get_open_paths()
            opened_path = paths[-1] if paths else ""
        else:
            raise RuntimeError(
                f"No HID device matched VID={vendor_id:#06x} PID={product_id:#06x}"
            )
        # Set callback after open for VID/PID (path is unknown before open)
        if opened_path and readable:
            hid_state._setup_read_callback(opened_path)
            hid_state._read_buffer.setdefault(opened_path, [])
    else:
        raise RuntimeError(
            "Must provide either 'path' or both 'vendor_id' and 'product_id'"
        )

    return {"message": f"Opened device", "path": opened_path}


@mcp.tool(
    name="close_device",
    description="Close a HID device. Provide a path to close a specific device, "
                "or omit to close all open devices.",
    tags={"hid", "close", "connection"},
    annotations={
        "title": "Close Device",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "connection"},
)
def close_device(path: str = "") -> Dict:
    """Close a specific HID device or all devices."""
    hid_state._ensure_ready()
    hid = hid_state._ensure_hid()

    if path:
        hid.close(path)
        hid_state._read_buffer.pop(path, None)
        return {"message": f"Closed device: {path}"}
    else:
        hid.close()
        hid_state._read_buffer.clear()
        return {"message": "Closed all devices"}


@mcp.tool(
    name="write",
    description="Write a report to a HID device. Data must be a hex string "
                "(e.g. '00FF' or '01 02 AA').",
    tags={"hid", "write", "io"},
    annotations={
        "title": "Write",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "io"},
)
def write(path: str, data: str) -> Dict:
    """Write a report to an open HID device."""
    hid_state._ensure_ready()
    hid = hid_state._ensure_hid()

    if not hid.is_open(path):
        raise RuntimeError(f"Device not open: {path}")

    try:
        data_bytes = bytes.fromhex(data)
    except ValueError as exc:
        raise RuntimeError(f"Invalid hex string: {exc}") from exc

    try:
        written = hid.write(path, data_bytes)
        if written < 0:
            raise RuntimeError("Write failed")
        return {"message": f"Wrote {written} byte(s)", "path": path, "bytes_written": written}
    except Exception as exc:
        raise RuntimeError(f"Write failed: {exc}") from exc


@mcp.tool(
    name="send_feature_report",
    description="Send a HID feature report to a device. Data must be a hex string.",
    tags={"hid", "write", "io", "feature"},
    annotations={
        "title": "Send Feature Report",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "io"},
)
def send_feature_report(path: str, data: str) -> Dict:
    """Send a feature report to an open HID device."""
    hid_state._ensure_ready()
    hid = hid_state._ensure_hid()

    if not hid.is_open(path):
        raise RuntimeError(f"Device not open: {path}")

    try:
        data_bytes = bytes.fromhex(data)
    except ValueError as exc:
        raise RuntimeError(f"Invalid hex string: {exc}") from exc

    try:
        sent = hid.send_feature_report(path, data_bytes)
        if sent < 0:
            raise RuntimeError("Feature report failed")
        return {"message": f"Feature report sent ({sent} byte(s))", "path": path}
    except Exception as exc:
        raise RuntimeError(f"Feature report failed: {exc}") from exc


@mcp.tool(
    name="start_hotplug",
    description="Start HID device hotplug detection (polling-based). "
                "Provide vendor_id and product_id to filter, or 0 for all devices. "
                "When hotplug is active, call get_hotplug_events to retrieve connection/disconnection events.",
    tags={"hid", "hotplug", "monitor"},
    annotations={
        "title": "Start Hotplug",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "monitor"},
)
def start_hotplug(
    vendor_id: int = 0,
    product_id: int = 0,
    poll_interval_ms: int = 1000,
) -> Dict:
    """Start HID device hotplug detection."""
    hid_state._ensure_ready()
    hid = hid_state._ensure_hid()

    if not hid.init(vendor_id, product_id):
        raise RuntimeError("HID init failed")

    if poll_interval_ms > 0:
        hid.set_hotplug_poll_interval(poll_interval_ms)

    def hotplug_callback(added, removed):
        added_dicts = [hid_state._device_to_dict(d) for d in added]
        removed_dicts = [hid_state._device_to_dict(d) for d in removed]
        print(f"[SimpleCommKitAiHid] Hotplug: +{len(added)} -{len(removed)}")

    hid.set_callback_on_hotplug(hotplug_callback)
    hid.start_hotplug(vendor_id, product_id)
    hid_state._hotplug_active = True
    return {
        "message": "Hotplug detection started",
        "vendor_id": vendor_id,
        "product_id": product_id,
        "poll_interval_ms": hid.get_hotplug_poll_interval(),
    }


@mcp.tool(
    name="stop_hotplug",
    description="Stop HID device hotplug detection.",
    tags={"hid", "hotplug", "monitor"},
    annotations={
        "title": "Stop Hotplug",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": True,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "monitor"},
)
def stop_hotplug() -> Dict:
    """Stop HID device hotplug detection."""
    hid_state._ensure_ready()
    hid = hid_state._ensure_hid()
    hid.stop_hotplug()
    hid_state._hotplug_active = False
    return {"message": "Hotplug detection stopped"}


@mcp.tool(
    name="get_device_list",
    description="Get the cached list of devices (populated by open_device or start_hotplug).",
    tags={"hid", "read", "status"},
    annotations={
        "title": "Get Device List",
        "readOnlyHint": True,
        "idempotentHint": True,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "discovery"},
)
def get_device_list() -> List[Dict]:
    """Return the cached device list."""
    hid_state._ensure_ready()
    hid = hid_state._ensure_hid()
    devices = hid.get_device_list()
    hid_state.device_cache = devices
    return [hid_state._device_to_dict(d) for d in devices]


@mcp.tool(
    name="get_open_paths",
    description="Get a list of currently open HID device paths.",
    tags={"hid", "read", "status"},
    annotations={
        "title": "Get Open Paths",
        "readOnlyHint": True,
        "idempotentHint": True,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "status"},
)
def get_open_paths() -> Dict:
    """Return list of open device paths and their status."""
    hid_state._ensure_ready()
    hid = hid_state._ensure_hid()
    paths = hid.get_open_paths()
    hotplug_active = hid.is_hotplug_active() if hasattr(hid, 'is_hotplug_active') else hid_state._hotplug_active
    return {
        "open_paths": paths,
        "open_count": len(paths),
        "hotplug_active": hotplug_active,
    }


@mcp.tool(
    name="get_read_data",
    description="Retrieve and clear buffered read data for a device. "
                "Data is buffered after calling open_device with readable=True.",
    tags={"hid", "read", "io"},
    annotations={
        "title": "Get Read Data",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "io"},
)
def get_read_data(path: str) -> List[Dict]:
    """Return all buffered read data and clear the buffer."""
    if path not in hid_state._read_buffer:
        return []
    samples = hid_state._read_buffer[path]
    hid_state._read_buffer[path] = []
    return samples


@mcp.tool(
    name="set_read_config",
    description="Configure read polling parameters for a HID device. "
                "Set poll_interval_ms (how often to poll, in ms) and/or "
                "data_length (bytes to read per poll, default 64).",
    tags={"hid", "config"},
    annotations={
        "title": "Set Read Config",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "config"},
)
def set_read_config(
    path: str,
    poll_interval_ms: int = 0,
    data_length: int = 0,
) -> Dict:
    """Configure read polling for a device."""
    hid_state._ensure_ready()
    hid = hid_state._ensure_hid()

    if poll_interval_ms > 0:
        hid.set_read_poll_interval(path, poll_interval_ms)
    if data_length > 0:
        hid.set_read_data_length(path, data_length)

    return {
        "message": "Read config updated",
        "path": path,
        "read_poll_interval_ms": hid.get_read_poll_interval(path),
        "read_data_length": hid.get_read_data_length(path),
    }


def main() -> None:
    parser = argparse.ArgumentParser(description="SimpleCommKitAiHid MCP Server")
    parser.add_argument(
        "--transport",
        default="stdio",
        choices=["stdio", "http", "sse", "streamable-http"],
        help=(
            "Transport protocol: 'stdio' (local), 'streamable-http' (recommended for remote),"
            " 'http' or 'sse' (legacy, may show init warnings)"
        ),
    )
    parser.add_argument("--host", default="127.0.0.1", help="Host to bind to")
    parser.add_argument("--port", type=int, default=8002, help="Port to bind to")
    args = parser.parse_args()

    # Ensure HidState is initialized before the MCP server starts accepting requests.
    hid_state.init()

    if args.transport in ("http", "sse", "streamable-http"):
        mcp.run(transport=args.transport, host=args.host, port=args.port)
    else:
        mcp.run()


if __name__ == "__main__":
    main()
