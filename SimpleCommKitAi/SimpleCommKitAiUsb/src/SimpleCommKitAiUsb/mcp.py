"""SimpleCommKitAiUsb MCP Server - Expose USB operations as MCP tools for AI agents."""

from fastmcp import FastMCP
import argparse
from typing import Dict, List, Optional

from SimpleCommKitAiUsb import SimpleCommKitUsb, UsbDeviceInfo, get_error_description


class UsbState:
    """Holds global USB state shared across MCP tools."""

    def __init__(self) -> None:
        self.usb: Optional[SimpleCommKitUsb] = None
        self.device_cache: List = []
        self._initialized: bool = False
        self._hotplug_active: bool = False
        self._read_buffer: Dict[str, List[Dict]] = {}

    def init(self) -> bool:
        """Lazy-initialize USB hardware. Safe to call multiple times."""
        if self._initialized:
            return self.usb is not None
        try:
            self.usb = SimpleCommKitUsb()
            self.usb.set_callback_error(lambda code: print(
                f"[SimpleCommKitAiUsb] Error {code}: {get_error_description(code)}"
            ))
            if not self.usb.init():
                print("[SimpleCommKitAiUsb] libusb init failed")
                self._initialized = True
                return False
            self._initialized = True
        except Exception as e:
            print(f"[SimpleCommKitAiUsb] USB init failed: {e}")
            self._initialized = True
        return self.usb is not None

    def _ensure_ready(self) -> None:
        """Raise if USB is not initialized."""
        if not self.init():
            raise RuntimeError("USB hardware not available (libusb not found or init failed)")

    def _ensure_usb(self) -> SimpleCommKitUsb:
        self._ensure_ready()
        return self.usb

    def _device_to_dict(self, d) -> Dict:
        """Convert UsbDeviceInfo to a plain dict."""
        return {
            "path": d.path,
            "vendor_id": f"0x{d.vendor_id:04X}",
            "product_id": f"0x{d.product_id:04X}",
            "manufacturer_string": d.manufacturer_string,
            "product_string": d.product_string,
            "serial_number": d.serial_number,
            "bus_number": d.bus_number,
            "device_address": d.device_address,
        }

    def _setup_read_callback(self, path: str):
        """Register a read callback that buffers data for this path."""
        usb = self._ensure_usb()

        def read_callback(dev_info: UsbDeviceInfo, data: bytes) -> None:
            if path not in self._read_buffer:
                self._read_buffer[path] = []
            self._read_buffer[path].append({
                "path": dev_info.path,
                "data_hex": data.hex(),
                "data_utf8": data.decode("utf-8", errors="ignore"),
                "data_length": len(data),
            })

        usb.set_callback_on_read(read_callback)


usb_state = UsbState()

mcp = FastMCP(name="SimpleCommKitAiUsb MCP Server")


# ============================================================
# MCP Tools
# ============================================================

@mcp.tool(
    name="get_available_devices",
    description="List available USB devices on the host. Optionally filter by vendor_id and product_id. "
                "Returns device path, VID/PID, manufacturer, product, serial, bus and address info.",
    tags={"usb", "read", "discovery"},
    annotations={
        "title": "Get Available Devices",
        "readOnlyHint": True,
        "idempotentHint": True,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "discovery"},
)
def get_available_devices(vendor_id: int = 0, product_id: int = 0) -> List[Dict]:
    """List all available USB devices, optionally filtered by VID/PID."""
    usb_state._ensure_ready()
    usb = usb_state._ensure_usb()
    devices = SimpleCommKitUsb.get_available_devices(vendor_id, product_id)
    usb_state.device_cache = devices
    return [usb_state._device_to_dict(d) for d in devices]


@mcp.tool(
    name="open_device",
    description="Open a USB device for communication. "
                "You must provide either a path (e.g. '1:3' for bus:address) "
                "or a combination of vendor_id + product_id + optional serial_number.",
    tags={"usb", "open", "connection"},
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
) -> Dict:
    """Open a USB device by path or by VID/PID/serial."""
    usb_state._ensure_ready()
    usb = usb_state._ensure_usb()

    if not usb.init():
        raise RuntimeError("USB init failed")

    opened_path = ""

    if path:
        if usb.is_open(path):
            return {"message": f"Device already open: {path}", "path": path}
        usb_state._setup_read_callback(path)
        usb_state._read_buffer.setdefault(path, [])
        if usb.open(path):
            opened_path = path
        else:
            raise RuntimeError(f"Failed to open {path}")
    elif vendor_id != 0 and product_id != 0:
        if usb.open(vendor_id, product_id, serial_number):
            paths = usb.get_open_paths()
            opened_path = paths[-1] if paths else ""
        else:
            raise RuntimeError(
                f"No USB device matched VID={vendor_id:#06x} PID={product_id:#06x}"
            )
        if opened_path:
            usb_state._setup_read_callback(opened_path)
            usb_state._read_buffer.setdefault(opened_path, [])
    else:
        raise RuntimeError(
            "Must provide either 'path' or both 'vendor_id' and 'product_id'"
        )

    return {"message": f"Opened device", "path": opened_path}


@mcp.tool(
    name="close_device",
    description="Close a USB device. Provide a path to close a specific device, "
                "or omit to close all open devices.",
    tags={"usb", "close", "connection"},
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
    """Close a specific USB device or all devices."""
    usb_state._ensure_ready()
    usb = usb_state._ensure_usb()

    if path:
        usb.close(path)
        usb_state._read_buffer.pop(path, None)
        return {"message": f"Closed device: {path}"}
    else:
        usb.close()
        usb_state._read_buffer.clear()
        return {"message": "Closed all devices"}


@mcp.tool(
    name="claim_interface",
    description="Claim a USB interface on an open device. Required before performing transfers. "
                "interface_number defaults to 0.",
    tags={"usb", "config"},
    annotations={
        "title": "Claim Interface",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "config"},
)
def claim_interface(path: str, interface_number: int = 0) -> Dict:
    """Claim a USB interface."""
    usb_state._ensure_ready()
    usb = usb_state._ensure_usb()

    if not usb.is_open(path):
        raise RuntimeError(f"Device not open: {path}")

    if usb.claim_interface(path, interface_number):
        return {"message": f"Claimed interface {interface_number} on {path}"}
    else:
        raise RuntimeError(f"Failed to claim interface {interface_number} on {path}")


@mcp.tool(
    name="release_interface",
    description="Release a previously claimed USB interface.",
    tags={"usb", "config"},
    annotations={
        "title": "Release Interface",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "config"},
)
def release_interface(path: str, interface_number: int = 0) -> Dict:
    """Release a USB interface."""
    usb_state._ensure_ready()
    usb = usb_state._ensure_usb()

    if not usb.is_open(path):
        raise RuntimeError(f"Device not open: {path}")

    if usb.release_interface(path, interface_number):
        return {"message": f"Released interface {interface_number} on {path}"}
    else:
        raise RuntimeError(f"Failed to release interface {interface_number} on {path}")


@mcp.tool(
    name="control_transfer",
    description="Perform a USB control transfer. "
                "bm_request_type: e.g. 0x80 (device-to-host, standard, device), 0x00 (host-to-device). "
                "b_request: e.g. 0x06 (GET_DESCRIPTOR), 0x00 (GET_STATUS). "
                "w_value, w_index: request-specific parameters. "
                "Length: expected data length for IN transfers (ignored if data_hex is provided). "
                "Returns data as hex string on success.",
    tags={"usb", "read", "write", "io"},
    annotations={
        "title": "Control Transfer",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "io"},
)
def control_transfer(
    path: str,
    bm_request_type: int,
    b_request: int,
    w_value: int,
    w_index: int = 0,
    data_hex: str = "",
    length: int = 64,
    timeout: int = 1000,
) -> Dict:
    """Perform a USB control transfer."""
    usb_state._ensure_ready()
    usb = usb_state._ensure_usb()

    if not usb.is_open(path):
        raise RuntimeError(f"Device not open: {path}")

    try:
        if data_hex:
            buf = bytes.fromhex(data_hex)
        else:
            buf = bytes(length)

        result = usb.control_transfer(path, bm_request_type, b_request,
                                       w_value, w_index, buf, timeout)
        return {
            "message": f"Control transfer completed ({len(result)} bytes)",
            "data_hex": result.hex() if result else "",
            "length": len(result),
        }
    except Exception as exc:
        raise RuntimeError(f"Control transfer failed: {exc}") from exc


@mcp.tool(
    name="bulk_transfer_out",
    description="Perform a USB bulk OUT transfer. "
                "endpoint: OUT endpoint address (e.g. 0x01, 0x02). "
                "data_hex: hex string of data to send. "
                "Returns number of bytes transferred.",
    tags={"usb", "write", "io"},
    annotations={
        "title": "Bulk Transfer OUT",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "io"},
)
def bulk_transfer_out(
    path: str,
    endpoint: int,
    data_hex: str,
    timeout: int = 1000,
) -> Dict:
    """Perform a USB bulk OUT transfer."""
    usb_state._ensure_ready()
    usb = usb_state._ensure_usb()

    if not usb.is_open(path):
        raise RuntimeError(f"Device not open: {path}")

    try:
        data_bytes = bytes.fromhex(data_hex)
    except ValueError as exc:
        raise RuntimeError(f"Invalid hex string: {exc}") from exc

    try:
        transferred = usb.bulk_transfer_out(path, endpoint, data_bytes, timeout)
        if transferred < 0:
            raise RuntimeError(f"Bulk OUT transfer returned error code {transferred}")
        return {"message": f"Transferred {transferred} byte(s)", "bytes_transferred": transferred}
    except Exception as exc:
        raise RuntimeError(f"Bulk OUT transfer failed: {exc}") from exc


@mcp.tool(
    name="bulk_transfer_in",
    description="Perform a USB bulk IN transfer. "
                "endpoint: IN endpoint address (e.g. 0x81, 0x82). "
                "length: number of bytes to read. "
                "Returns received data as hex string.",
    tags={"usb", "read", "io"},
    annotations={
        "title": "Bulk Transfer IN",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "io"},
)
def bulk_transfer_in(
    path: str,
    endpoint: int,
    length: int = 64,
    timeout: int = 1000,
) -> Dict:
    """Perform a USB bulk IN transfer."""
    usb_state._ensure_ready()
    usb = usb_state._ensure_usb()

    if not usb.is_open(path):
        raise RuntimeError(f"Device not open: {path}")

    try:
        data = usb.bulk_transfer_in(path, endpoint, length, timeout)
        return {
            "message": f"Received {len(data)} byte(s)",
            "data_hex": data.hex() if data else "",
            "length": len(data) if data else 0,
        }
    except Exception as exc:
        raise RuntimeError(f"Bulk IN transfer failed: {exc}") from exc


@mcp.tool(
    name="interrupt_transfer_out",
    description="Perform a USB interrupt OUT transfer. "
                "endpoint: OUT endpoint address (e.g. 0x01). "
                "data_hex: hex string of data to send.",
    tags={"usb", "write", "io"},
    annotations={
        "title": "Interrupt Transfer OUT",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "io"},
)
def interrupt_transfer_out(
    path: str,
    endpoint: int,
    data_hex: str,
    timeout: int = 1000,
) -> Dict:
    """Perform a USB interrupt OUT transfer."""
    usb_state._ensure_ready()
    usb = usb_state._ensure_usb()

    if not usb.is_open(path):
        raise RuntimeError(f"Device not open: {path}")

    try:
        data_bytes = bytes.fromhex(data_hex)
    except ValueError as exc:
        raise RuntimeError(f"Invalid hex string: {exc}") from exc

    try:
        transferred = usb.interrupt_transfer_out(path, endpoint, data_bytes, timeout)
        if transferred < 0:
            raise RuntimeError(f"Interrupt OUT transfer returned error code {transferred}")
        return {"message": f"Transferred {transferred} byte(s)", "bytes_transferred": transferred}
    except Exception as exc:
        raise RuntimeError(f"Interrupt OUT transfer failed: {exc}") from exc


@mcp.tool(
    name="interrupt_transfer_in",
    description="Perform a USB interrupt IN transfer. "
                "endpoint: IN endpoint address (e.g. 0x81). "
                "length: number of bytes to read. "
                "Returns received data as hex string.",
    tags={"usb", "read", "io"},
    annotations={
        "title": "Interrupt Transfer IN",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "io"},
)
def interrupt_transfer_in(
    path: str,
    endpoint: int,
    length: int = 64,
    timeout: int = 1000,
) -> Dict:
    """Perform a USB interrupt IN transfer."""
    usb_state._ensure_ready()
    usb = usb_state._ensure_usb()

    if not usb.is_open(path):
        raise RuntimeError(f"Device not open: {path}")

    try:
        data = usb.interrupt_transfer_in(path, endpoint, length, timeout)
        return {
            "message": f"Received {len(data)} byte(s)",
            "data_hex": data.hex() if data else "",
            "length": len(data) if data else 0,
        }
    except Exception as exc:
        raise RuntimeError(f"Interrupt IN transfer failed: {exc}") from exc


@mcp.tool(
    name="start_read_poll",
    description="Start continuous read polling on a USB endpoint. "
                "The endpoint (e.g. 0x81) will be polled at the configured interval. "
                "Data is buffered — use 'get_read_data' to retrieve it. "
                "Use 'stop_read_poll' to stop.",
    tags={"usb", "read", "io", "lifecycle"},
    annotations={
        "title": "Start Read Poll",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "io"},
)
def start_read_poll(path: str, endpoint: int) -> Dict:
    """Start continuous read polling on an endpoint."""
    usb_state._ensure_ready()
    usb = usb_state._ensure_usb()

    if not usb.is_open(path):
        raise RuntimeError(f"Device not open: {path}")

    usb.start_read_poll(path, endpoint)
    return {"message": f"Read poll started on {path} endpoint 0x{endpoint:02X}"}


@mcp.tool(
    name="stop_read_poll",
    description="Stop continuous read polling on a device.",
    tags={"usb", "read", "io", "lifecycle"},
    annotations={
        "title": "Stop Read Poll",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "io"},
)
def stop_read_poll(path: str) -> Dict:
    """Stop continuous read polling."""
    usb_state._ensure_ready()
    usb = usb_state._ensure_usb()

    usb.stop_read_poll(path)
    return {"message": f"Read poll stopped on {path}"}


@mcp.tool(
    name="get_read_data",
    description="Retrieve and clear buffered read data for a device. "
                "Data is buffered after calling open_device or start_read_poll. "
                "Returns list of {path, data_hex, data_utf8, data_length} dicts.",
    tags={"usb", "read", "io"},
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
    if path not in usb_state._read_buffer:
        return []
    samples = usb_state._read_buffer[path]
    usb_state._read_buffer[path] = []
    return samples


@mcp.tool(
    name="start_hotplug",
    description="Start USB hotplug monitoring. "
                "Uses native libusb callbacks when available, falls back to polling otherwise. "
                "Provide vendor_id and product_id to filter (0 = all devices). "
                "Hotplug events are printed to console and the device cache is updated automatically.",
    tags={"usb", "hotplug", "monitor"},
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
    """Start USB hotplug detection."""
    usb_state._ensure_ready()
    usb = usb_state._ensure_usb()

    if poll_interval_ms > 0:
        usb.set_hotplug_poll_interval(poll_interval_ms)

    def hotplug_callback(added, removed):
        added_dicts = [usb_state._device_to_dict(d) for d in added]
        removed_dicts = [usb_state._device_to_dict(d) for d in removed]
        print(f"[SimpleCommKitAiUsb] Hotplug: +{len(added)} -{len(removed)}")

    usb.set_callback_on_hotplug(hotplug_callback)
    usb.start_hotplug(vendor_id, product_id)
    usb_state._hotplug_active = True
    return {
        "message": "Hotplug detection started",
        "vendor_id": vendor_id,
        "product_id": product_id,
        "poll_interval_ms": usb.get_hotplug_poll_interval(),
    }


@mcp.tool(
    name="stop_hotplug",
    description="Stop USB hotplug monitoring.",
    tags={"usb", "hotplug", "monitor"},
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
    """Stop USB hotplug detection."""
    usb_state._ensure_ready()
    usb = usb_state._ensure_usb()
    usb.stop_hotplug()
    usb_state._hotplug_active = False
    return {"message": "Hotplug detection stopped"}


@mcp.tool(
    name="get_device_list",
    description="Get the cached list of USB devices (populated by init or start_hotplug).",
    tags={"usb", "read", "status"},
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
    usb_state._ensure_ready()
    usb = usb_state._ensure_usb()
    devices = usb.get_device_list()
    usb_state.device_cache = devices
    return [usb_state._device_to_dict(d) for d in devices]


@mcp.tool(
    name="get_open_paths",
    description="Get a list of currently open USB device paths and their status.",
    tags={"usb", "read", "status"},
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
    usb_state._ensure_ready()
    usb = usb_state._ensure_usb()
    paths = usb.get_open_paths()
    hotplug_active = usb.is_hotplug_active() if hasattr(usb, 'is_hotplug_active') else usb_state._hotplug_active
    return {
        "open_paths": paths,
        "open_count": len(paths),
        "hotplug_active": hotplug_active,
    }


@mcp.tool(
    name="set_read_config",
    description="Configure read polling parameters. "
                "Set poll_interval_ms (how often to poll, in ms) and/or "
                "data_length (bytes to read per poll, default 64).",
    tags={"usb", "config"},
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
    poll_interval_ms: int = 0,
    data_length: int = 0,
) -> Dict:
    """Configure read polling parameters globally."""
    usb_state._ensure_ready()
    usb = usb_state._ensure_usb()

    if poll_interval_ms > 0:
        usb.set_read_poll_interval(poll_interval_ms)
    if data_length > 0:
        usb.set_read_data_length(data_length)

    return {
        "message": "Read config updated",
        "read_poll_interval_ms": usb.get_read_poll_interval(),
        "read_data_length": usb.get_read_data_length(),
    }


def main() -> None:
    parser = argparse.ArgumentParser(description="SimpleCommKitAiUsb MCP Server")
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
    parser.add_argument("--port", type=int, default=8003, help="Port to bind to")
    args = parser.parse_args()

    # Ensure UsbState is initialized before the MCP server starts accepting requests.
    usb_state.init()

    if args.transport in ("http", "sse", "streamable-http"):
        mcp.run(transport=args.transport, host=args.host, port=args.port)
    else:
        mcp.run()


if __name__ == "__main__":
    main()
