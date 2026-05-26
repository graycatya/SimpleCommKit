"""SimpleCommKitAiBle MCP Server - Expose BLE operations as MCP tools for AI agents."""

from fastmcp import FastMCP
import argparse
from typing import Dict, List, Optional

from SimpleCommKitAiBle import BleCentral, Adapter, Peripheral, get_error_description


class BleState:
    """Holds global BLE state shared across MCP tools."""

    def __init__(self) -> None:
        self.central = None
        self.adapters: List[Adapter] = []
        self.current_adapter: Optional[Adapter] = None
        self.scan_results: List[Peripheral] = []
        self.connected: Dict[str, bool] = {}
        self.notifications: Dict[str, List[Dict]] = {}
        self._services_cache: Optional[List] = None
        self._initialized: bool = False

    def init(self) -> bool:
        """Lazy-initialize BLE hardware. Safe to call multiple times.
        Returns True if BLE is ready, False otherwise."""
        if self._initialized:
            return bool(self.central and self.current_adapter)
        try:
            self.central = BleCentral()
            self.central.set_callback_error(lambda code: print(
                f"[SimpleCommKitAiBle] Error {code}: {get_error_description(code)}"
            ))
            self.adapters = self.central.get_adapters()
            if self.adapters:
                self.current_adapter = self.adapters[0]
                self.central.set_current_adapter(self.current_adapter)
                self._setup_scan_callbacks()
                self._power_on_adapter()
            self._initialized = True
        except Exception as e:
            print(f"[SimpleCommKitAiBle] BLE init failed: {e}")
            self._initialized = True  # Don't retry, just report not ready
        return bool(self.central and self.current_adapter)

    def _ensure_ready(self) -> None:
        """Raise if BLE is not initialized (no adapter / no hardware)."""
        if not self.init():
            raise RuntimeError("BLE hardware not available (no adapter found)")

    def _setup_scan_callbacks(self) -> None:
        """Register scan callbacks so m_peripherals gets populated during scan."""
        self.central.adapter_set_callback_on_scan_start(lambda: None)
        self.central.adapter_set_callback_on_scan_stop(lambda: None)
        self.central.adapter_set_callback_on_scan_found(lambda p: None)
        self.central.adapter_set_callback_on_scan_updated(lambda p: None)

    def _power_on_adapter(self) -> None:
        """Power on the adapter if not already on."""
        if not self.central.adapter_is_powered():
            self.central.adapter_power_on()

    def refresh_adapters(self) -> None:
        if not self.init():
            return
        self.adapters = self.central.get_adapters()
        if self.adapters and not self.current_adapter:
            self.current_adapter = self.adapters[0]
            self.central.set_current_adapter(self.current_adapter)
            self._setup_scan_callbacks()
            self._power_on_adapter()

    def _ensure_services_cache(self) -> List:
        """Load services and cache them for property lookups."""
        if self._services_cache is None and self.central.peripheral_is_connected():
            self._services_cache = self.central.peripheral_services()
        return self._services_cache or []

    def check_char_property(self, char_uuid: str, prop: str) -> bool:
        """Check if a characteristic supports a given operation. Caches services."""
        for svc in self._ensure_services_cache():
            for ch in svc.characteristics:
                if ch.uuid == char_uuid:
                    return getattr(ch, prop, False)
        return True  # unknown characteristic → let the C++ layer decide

    def invalidate_services_cache(self) -> None:
        self._services_cache = None


ble_state = BleState()
mcp = FastMCP(name="SimpleCommKitAiBle MCP Server")


@mcp.tool(
    name="bluetooth_enabled",
    description="Check if Bluetooth is enabled on the host system.",
    tags={"adapter", "ble", "read", "status"},
    annotations={
        "title": "Bluetooth Enabled",
        "readOnlyHint": True,
        "idempotentHint": True,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "discovery"},
)
def bluetooth_enabled() -> Dict[str, bool]:
    """Check if Bluetooth is enabled and available."""
    enabled = BleCentral.bluetooth_enabled()
    return {"enabled": enabled}


@mcp.tool(
    name="get_adapters",
    description="List available Bluetooth adapters on the host.",
    tags={"adapter", "ble", "read"},
    annotations={
        "title": "Get Adapters",
        "readOnlyHint": True,
        "idempotentHint": True,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "discovery"},
)
def get_adapters() -> List[Dict[str, str]]:
    """List all available Bluetooth adapters."""
    ble_state.refresh_adapters()
    return [
        {"identifier": a.identifier, "address": a.address}
        for a in ble_state.adapters
    ]


@mcp.tool(
    name="scan_for",
    description="Scan for nearby BLE peripherals using the selected adapter.",
    tags={"adapter", "scan", "ble", "read"},
    annotations={
        "title": "Scan For",
        "readOnlyHint": True,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "discovery"},
)
def scan_for(timeout_ms: int = 5000) -> List[Dict]:
    """Scan for nearby BLE devices."""
    ble_state.refresh_adapters()
    if not ble_state.current_adapter:
        raise RuntimeError("No Bluetooth adapter available")

    ble_state.central.adapter_scan_for(timeout_ms)
    ble_state.scan_results = ble_state.central.adapter_get_scan_results()

    results: List[Dict] = []
    for p in ble_state.scan_results:
        manufacturer_data: Dict[str, str] = {}
        for company_id, data in p.manufacturer_data.items():
            manufacturer_data[str(company_id)] = data.hex()

        results.append({
            "identifier": p.identifier,
            "address": p.address,
            "rssi": p.rssi,
            "address_type": int(p.address_type),
            "manufacturer_data": manufacturer_data,
        })
    return results


@mcp.tool(
    name="connect",
    description="Connect to a BLE peripheral by address or identifier (name). "
                "Must be a device from the last scan results. "
                "Call scan_for first.",
    tags={"peripheral", "connect", "ble"},
    annotations={
        "title": "Connect",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "connection"},
)
def connect(address: str) -> Dict[str, str]:
    """Connect to a peripheral from the last scan results.

    address: MAC address (e.g. "AA:BB:CC:DD:EE:FF") or device identifier/name.
    """
    ble_state._ensure_ready()
    if address in ble_state.connected and ble_state.connected[address]:
        return {"message": f"Already connected to {address}", "address": address}

    if not ble_state.scan_results:
        raise RuntimeError(
            "No scan results available. Call scan_for first to discover nearby devices."
        )

    target: Optional[Peripheral] = None
    for p in ble_state.scan_results:
        if p.address == address:
            target = p
            break

    if not target:
        available = [f"{p.identifier} ({p.address})" for p in ble_state.scan_results]
        raise RuntimeError(
            f"Device '{address}' not found in scan results. "
            f"Available devices: {available}"
        )

    ble_state.central.set_current_peripheral(target)
    ble_state.invalidate_services_cache()
    try:
        ble_state.central.peripheral_connect()
    except Exception as exc:
        raise RuntimeError(f"Failed to connect: {exc}") from exc

    ble_state.connected[address] = True
    ble_state.notifications.setdefault(address, [])
    return {"message": f"Connected to {target.identifier}", "address": address}


@mcp.tool(
    name="disconnect",
    description="Disconnect from a BLE peripheral by address.",
    tags={"peripheral", "disconnect", "ble"},
    annotations={
        "title": "Disconnect",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "connection"},
)
def disconnect(address: str) -> Dict[str, str]:
    """Disconnect from a connected peripheral."""
    ble_state._ensure_ready()
    if address not in ble_state.connected:
        raise RuntimeError("Device not found in connected list")

    try:
        ble_state.central.peripheral_disconnect()
    except Exception as exc:
        raise RuntimeError(f"Failed to disconnect: {exc}") from exc

    ble_state.connected.pop(address, None)
    ble_state.notifications.pop(address, None)
    ble_state.invalidate_services_cache()
    return {"message": f"Disconnected from {address}", "address": address}


@mcp.tool(
    name="connected_devices",
    description="List currently connected BLE peripherals (by querying the adapter). "
                "Use this to check which devices are connected at any time.",
    tags={"peripheral", "ble", "read", "status"},
    annotations={
        "title": "Connected Devices",
        "readOnlyHint": True,
        "idempotentHint": True,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "discovery"},
)
def connected_devices() -> List[Dict]:
    """Return a list of all currently connected peripherals."""
    ble_state.refresh_adapters()
    if not ble_state.current_adapter:
        return []

    devices = ble_state.central.adapter_get_connected_peripherals()
    return [
        {
            "identifier": p.identifier,
            "address": p.address,
            "rssi": p.rssi,
        }
        for p in devices
    ]


@mcp.tool(
    name="services",
    description="List GATT services and characteristics on a connected peripheral. "
                "Each characteristic includes properties: can_read, can_write_request, "
                "can_write_command, can_notify, can_indicate. "
                "Use these to decide which operations are valid.",
    tags={"peripheral", "gatt", "ble", "read"},
    annotations={
        "title": "Services",
        "readOnlyHint": True,
        "idempotentHint": True,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "gatt"},
)
def services(address: str) -> Dict:
    """List services and characteristics for a connected device."""
    ble_state._ensure_ready()
    if address not in ble_state.connected:
        raise RuntimeError("Device not found or not connected")

    if not ble_state.central.peripheral_is_connected():
        return {"address": address, "connected": False}

    ble_state._services_cache = ble_state.central.peripheral_services()
    svc_list = []
    for svc in ble_state._services_cache:
        chars = [
            {
                "uuid": ch.uuid,
                "can_read": ch.can_read,
                "can_write_request": ch.can_write_request,
                "can_write_command": ch.can_write_command,
                "can_notify": ch.can_notify,
                "can_indicate": ch.can_indicate,
            }
            for ch in svc.characteristics
        ]
        svc_list.append({"uuid": svc.uuid, "characteristics": chars})

    return {
        "address": address,
        "connected": True,
        "mtu": ble_state.central.peripheral_get_mtu(),
        "services": svc_list,
    }


@mcp.tool(
    name="read",
    description="Read a characteristic value from a connected peripheral. "
                "IMPORTANT: Only call this if the characteristic's can_read is True. "
                "Check with the 'services' tool first.",
    tags={"peripheral", "gatt", "ble", "read"},
    annotations={
        "title": "Read",
        "readOnlyHint": True,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "gatt"},
)
def read(address: str, service_uuid: str, char_uuid: str) -> Dict[str, str]:
    """Read a characteristic value from a connected device."""
    ble_state._ensure_ready()
    if address not in ble_state.connected:
        raise RuntimeError("Device not found")

    if not ble_state.check_char_property(char_uuid, "can_read"):
        raise RuntimeError(
            f"Characteristic '{char_uuid}' does not support read. "
            f"Use 'services' to check which operations are supported."
        )

    try:
        data = ble_state.central.peripheral_read(service_uuid, char_uuid)
    except Exception as exc:
        raise RuntimeError(f"Read failed: {exc}") from exc

    return {
        "service_uuid": service_uuid,
        "char_uuid": char_uuid,
        "data_hex": data.hex(),
        "data_utf8": data.decode("utf-8", errors="ignore"),
    }


@mcp.tool(
    name="write_request",
    description="Write data to a characteristic with response (write request). Data must be a hex string. "
                "IMPORTANT: Only call this if the characteristic's can_write_request is True. "
                "Check with the 'services' tool first.",
    tags={"peripheral", "gatt", "ble", "write"},
    annotations={
        "title": "Write Request",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "gatt"},
)
def write_request(address: str, service_uuid: str, char_uuid: str, data: str) -> Dict[str, str]:
    """Write data to a characteristic with response. Data is a hex string."""
    ble_state._ensure_ready()
    if address not in ble_state.connected:
        raise RuntimeError("Device not found")

    if not ble_state.check_char_property(char_uuid, "can_write_request"):
        raise RuntimeError(
            f"Characteristic '{char_uuid}' does not support write_request. "
            f"Use 'services' to check which operations are supported."
        )

    try:
        data_bytes = bytes.fromhex(data)
    except ValueError as exc:
        raise RuntimeError(f"Invalid hex string: {exc}") from exc

    try:
        ble_state.central.peripheral_write_request(service_uuid, char_uuid, data_bytes)
    except Exception as exc:
        raise RuntimeError(f"Write failed: {exc}") from exc

    return {"message": "Write successful"}


@mcp.tool(
    name="write_command",
    description="Write data to a characteristic without response (write command). Data must be a hex string. "
                "IMPORTANT: Only call this if the characteristic's can_write_command is True. "
                "Check with the 'services' tool first.",
    tags={"peripheral", "gatt", "ble", "write"},
    annotations={
        "title": "Write Command",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "gatt"},
)
def write_command(address: str, service_uuid: str, char_uuid: str, data: str) -> Dict[str, str]:
    """Write data to a characteristic without response. Data is a hex string."""
    ble_state._ensure_ready()
    if address not in ble_state.connected:
        raise RuntimeError("Device not found")

    if not ble_state.check_char_property(char_uuid, "can_write_command"):
        raise RuntimeError(
            f"Characteristic '{char_uuid}' does not support write_command. "
            f"Use 'services' to check which operations are supported."
        )

    try:
        data_bytes = bytes.fromhex(data)
    except ValueError as exc:
        raise RuntimeError(f"Invalid hex string: {exc}") from exc

    try:
        ble_state.central.peripheral_write_command(service_uuid, char_uuid, data_bytes)
    except Exception as exc:
        raise RuntimeError(f"Write command failed: {exc}") from exc

    return {"message": "Write command successful"}


@mcp.tool(
    name="descriptor_read",
    description="Read a descriptor value from a connected peripheral. "
                "Descriptors contain additional metadata for a characteristic "
                "(e.g., Client Characteristic Configuration).",
    tags={"peripheral", "gatt", "ble", "descriptor", "read"},
    annotations={
        "title": "Descriptor Read",
        "readOnlyHint": True,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "gatt"},
)
def descriptor_read(address: str, service_uuid: str, char_uuid: str, desc_uuid: str) -> Dict[str, str]:
    """Read a descriptor value from a connected device."""
    ble_state._ensure_ready()
    if address not in ble_state.connected:
        raise RuntimeError("Device not found")

    try:
        data = ble_state.central.peripheral_read_descriptor(service_uuid, char_uuid, desc_uuid)
    except Exception as exc:
        raise RuntimeError(f"Descriptor read failed: {exc}") from exc

    return {
        "service_uuid": service_uuid,
        "char_uuid": char_uuid,
        "descriptor_uuid": desc_uuid,
        "data_hex": data.hex(),
        "data_utf8": data.decode("utf-8", errors="ignore"),
    }


@mcp.tool(
    name="descriptor_write",
    description="Write data to a descriptor on a connected peripheral. "
                "Data must be a hex string (e.g., '0100' to enable notifications on a CCCD).",
    tags={"peripheral", "gatt", "ble", "descriptor", "write"},
    annotations={
        "title": "Descriptor Write",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "gatt"},
)
def descriptor_write(address: str, service_uuid: str, char_uuid: str,
                     desc_uuid: str, data: str) -> Dict[str, str]:
    """Write data to a descriptor. Data is a hex string."""
    ble_state._ensure_ready()
    if address not in ble_state.connected:
        raise RuntimeError("Device not found")

    try:
        data_bytes = bytes.fromhex(data)
    except ValueError as exc:
        raise RuntimeError(f"Invalid hex string: {exc}") from exc

    try:
        ble_state.central.peripheral_write_descriptor(service_uuid, char_uuid, desc_uuid, data_bytes)
    except Exception as exc:
        raise RuntimeError(f"Descriptor write failed: {exc}") from exc

    return {"message": "Descriptor write successful"}


@mcp.tool(
    name="notify",
    description="Subscribe to notifications on a characteristic. "
                "Data is buffered and can be retrieved with get_notifications. "
                "IMPORTANT: Only call this if the characteristic's can_notify is True. "
                "Check with the 'services' tool first. "
                "Call unsubscribe when done.",
    tags={"peripheral", "gatt", "ble", "notify"},
    annotations={
        "title": "Notify",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "gatt"},
)
def notify(address: str, service_uuid: str, char_uuid: str) -> Dict[str, str]:
    """Subscribe to notifications. Data is buffered in the background."""
    ble_state._ensure_ready()
    if address not in ble_state.connected:
        raise RuntimeError("Device not found")

    if not ble_state.check_char_property(char_uuid, "can_notify"):
        raise RuntimeError(
            f"Characteristic '{char_uuid}' does not support notify. "
            f"Use 'services' to check which operations are supported."
        )

    def notification_callback(payload: bytes) -> None:
        if address in ble_state.notifications:
            ble_state.notifications[address].append({
                "service": service_uuid,
                "characteristic": char_uuid,
                "data_hex": payload.hex(),
                "data_utf8": payload.decode("utf-8", errors="ignore"),
                "type": "notification",
            })

    try:
        ble_state.central.peripheral_notify(service_uuid, char_uuid, notification_callback)
    except Exception as exc:
        raise RuntimeError(f"Notify failed: {exc}") from exc

    return {"message": "Subscribed to notifications"}


@mcp.tool(
    name="indicate",
    description="Subscribe to indications on a characteristic. "
                "Data is buffered and can be retrieved with get_notifications. "
                "IMPORTANT: Only call this if the characteristic's can_indicate is True. "
                "Check with the 'services' tool first. "
                "Call unsubscribe when done.",
    tags={"peripheral", "gatt", "ble", "indicate"},
    annotations={
        "title": "Indicate",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "gatt"},
)
def indicate(address: str, service_uuid: str, char_uuid: str) -> Dict[str, str]:
    """Subscribe to indications. Data is buffered in the background."""
    ble_state._ensure_ready()
    if address not in ble_state.connected:
        raise RuntimeError("Device not found")

    if not ble_state.check_char_property(char_uuid, "can_indicate"):
        raise RuntimeError(
            f"Characteristic '{char_uuid}' does not support indicate. "
            f"Use 'services' to check which operations are supported."
        )

    def indication_callback(payload: bytes) -> None:
        if address in ble_state.notifications:
            ble_state.notifications[address].append({
                "service": service_uuid,
                "characteristic": char_uuid,
                "data_hex": payload.hex(),
                "data_utf8": payload.decode("utf-8", errors="ignore"),
                "type": "indication",
            })

    try:
        ble_state.central.peripheral_indicate(service_uuid, char_uuid, indication_callback)
    except Exception as exc:
        raise RuntimeError(f"Indicate failed: {exc}") from exc

    return {"message": "Subscribed to indications"}


@mcp.tool(
    name="get_notifications",
    description="Retrieve and clear all buffered notifications and indications for a connected peripheral.",
    tags={"peripheral", "gatt", "ble", "read"},
    annotations={
        "title": "Get Notifications",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "gatt"},
)
def get_notifications(address: str) -> List[Dict[str, str]]:
    """Return all buffered notifications/indications and clear the buffer."""
    if address not in ble_state.notifications:
        return []

    samples = ble_state.notifications[address]
    ble_state.notifications[address] = []
    return samples


@mcp.tool(
    name="unsubscribe",
    description="Unsubscribe from notifications or indications on a characteristic.",
    tags={"peripheral", "gatt", "ble"},
    annotations={
        "title": "Unsubscribe",
        "readOnlyHint": False,
        "destructiveHint": False,
        "idempotentHint": False,
        "openWorldHint": True,
    },
    meta={"version": "1.0", "role": "gatt"},
)
def unsubscribe(address: str, service_uuid: str, char_uuid: str) -> Dict[str, str]:
    """Unsubscribe from notifications or indications on a characteristic."""
    ble_state._ensure_ready()
    if address not in ble_state.connected:
        raise RuntimeError("Device not found")

    try:
        ble_state.central.peripheral_unsubscribe(service_uuid, char_uuid)
    except Exception as exc:
        raise RuntimeError(f"Unsubscribe failed: {exc}") from exc

    return {"message": "Unsubscribed"}


def main() -> None:
    parser = argparse.ArgumentParser(description="SimpleCommKitAiBle MCP Server")
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
    parser.add_argument("--port", type=int, default=8000, help="Port to bind to")
    args = parser.parse_args()

    # Ensure BleState is initialized before the MCP server starts accepting requests.
    # This prevents "Received request before initialization was complete" warnings
    # from FastMCP when a client connects too quickly in http/sse transport modes.
    ble_state.init()

    if args.transport in ("http", "sse", "streamable-http"):
        mcp.run(transport=args.transport, host=args.host, port=args.port)
    else:
        mcp.run()


if __name__ == "__main__":
    main()
