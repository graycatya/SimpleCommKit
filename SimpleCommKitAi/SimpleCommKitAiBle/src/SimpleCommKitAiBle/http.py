"""SimpleCommKitAiBle REST Server - HTTP API for BLE device control."""

try:
    from fastapi import FastAPI, HTTPException, Request
    from fastapi.responses import StreamingResponse
    from pydantic import BaseModel
    import uvicorn
except ImportError:
    print("Dependencies for SimpleCommKitAiBle HTTP server are not installed.")
    print("Please install them using: pip install simple-comm-kit-ai-ble[http]")
    exit(1)

import asyncio
from contextlib import asynccontextmanager
from typing import List, Dict
import argparse
import json
import queue
import threading

from SimpleCommKitAiBle import BleCentral, Adapter, Peripheral, get_error_description
from SimpleCommKitAiBle import __version__
class BleState:
    """Holds global BLE state shared across endpoints."""

    def __init__(self):
        self.central = None
        self.adapters: List[Adapter] = []
        self.scan_results: List[Peripheral] = []
        self.connected: Dict[str, bool] = {}
        self.notifications: Dict[str, List[Dict]] = {}
        self._scan_callbacks_set = False
        self._services_cache = None
        self._initialized: bool = False

    def init(self) -> bool:
        """Lazy-initialize BLE hardware. Safe to call multiple times."""
        if self._initialized:
            return bool(self.central and self.adapters)
        try:
            self.central = BleCentral()
            self.central.set_callback_error(lambda code: print(
                f"[SimpleCommKitAiBle-http] Error {code}: {get_error_description(code)}"
            ))
            self.adapters = self.central.get_adapters()
            if self.adapters:
                self.central.set_current_adapter(self.adapters[0])
                self._setup_scan_callbacks()
                self._power_on_adapter()
            self._initialized = True
        except Exception as e:
            print(f"[SimpleCommKitAiBle-http] BLE init failed: {e}")
            self._initialized = True
        return bool(self.central and self.adapters)
    
    def _ensure_ready(self) -> None:
        if not self.init():
            raise HTTPException(status_code=503, detail="BLE hardware not available (no adapter found)")

    def _setup_scan_callbacks(self):
        """Register scan callbacks so m_peripherals gets populated during scan."""
        self.central.adapter_set_callback_on_scan_start(lambda: print(
            "[SimpleCommKitAiBle-http] Scan started"
        ))
        self.central.adapter_set_callback_on_scan_stop(lambda: print(
            "[SimpleCommKitAiBle-http] Scan stopped"
        ))
        self.central.adapter_set_callback_on_scan_found(lambda p: print(
            f"[SimpleCommKitAiBle-http] Scan found: {p.identifier} ({p.address}) RSSI: {p.rssi}"
        ))
        self.central.adapter_set_callback_on_scan_updated(lambda p: print(
            f"[SimpleCommKitAiBle-http] Scan updated: {p.identifier} RSSI: {p.rssi}"
        ))
        self._scan_callbacks_set = True

    def _power_on_adapter(self):
        """Power on the adapter if not already on."""
        if self.central.adapter_is_powered():
            print(f"[SimpleCommKitAiBle-http] Adapter already powered on")
        else:
            print(f"[SimpleCommKitAiBle-http] Powering on adapter...")
            self.central.adapter_power_on()
            print(f"[SimpleCommKitAiBle-http] Adapter powered on: {self.central.adapter_is_powered()}")

    def refresh_adapters(self):
        if not self.init():
            return
        self.adapters = self.central.get_adapters()
        if self.adapters and not self._has_current_adapter():
            self.central.set_current_adapter(self.adapters[0])
            self._setup_scan_callbacks()
            self._power_on_adapter()

    def _has_current_adapter(self) -> bool:
        return self.central.get_current_adapter() is not None

    def _ensure_services_cache(self):
        if self._services_cache is None and self.central.peripheral_is_connected():
            self._services_cache = self.central.peripheral_services()
        return self._services_cache or []

    def check_char_property(self, char_uuid: str, prop: str) -> bool:
        for svc in self._ensure_services_cache():
            for ch in svc.characteristics:
                if ch.uuid == char_uuid:
                    return getattr(ch, prop, False)
        return True  # unknown → let the C++ layer decide

    def invalidate_services_cache(self):
        self._services_cache = None


ble_state = BleState()

# ---------------------------------------------------------------------------
# SSE streaming state (thread-safe bridge between BLE callbacks and async SSE)
# ---------------------------------------------------------------------------
_sse_queues: Dict[str, queue.Queue] = {}      # address → thread-safe Queue
_sse_events: Dict[str, threading.Event] = {}   # address → wake-up Event


def _push_to_sse(address: str, notification: Dict) -> None:
    """Push a notification to any active SSE listener for this address."""
    if address in _sse_queues:
        _sse_queues[address].put(notification)
    if address in _sse_events:
        _sse_events[address].set()


@asynccontextmanager
async def lifespan(app: FastAPI):
    print(f"Adapters found: {[a.identifier for a in ble_state.adapters]}")
    if not ble_state.adapters:
        print("No bluetooth adapter found!")
    yield


app = FastAPI(
    title="SimpleCommKitAiBle API",
    description="REST API to control BLE devices using SimpleCommKitPyBle",
    version=__version__,
    lifespan=lifespan,
)


class WriteRequest(BaseModel):
    data: str  # Hex string


@app.get("/")
def root():
    return {"message": "SimpleCommKitAiBle API is running"}


@app.get("/adapters")
def get_adapters():
    ble_state.refresh_adapters()
    return [{"identifier": a.identifier, "address": a.address} for a in ble_state.adapters]


@app.post("/scan")
def scan(timeout_ms: int = 5000):
    if not ble_state.adapters:
        raise HTTPException(status_code=500, detail="No Bluetooth adapter available")

    # Ensure adapter is selected
    current = ble_state.central.get_current_adapter()
    if current is None and ble_state.adapters:
        ble_state.central.set_current_adapter(ble_state.adapters[0])
        current = ble_state.adapters[0]
    print(f"[scan] Using adapter: {current.identifier if current else 'None'}")

    try:
        ble_state.central.adapter_scan_for(timeout_ms)
    except Exception as e:
        print(f"[scan] adapter_scan_for error: {e}")
        raise HTTPException(status_code=500, detail=f"Scan failed: {e}")

    ble_state.scan_results = ble_state.central.adapter_get_scan_results()
    print(f"[scan] Found {len(ble_state.scan_results)} device(s)")

    results = []
    for p in ble_state.scan_results:
        man_data = {}
        for company_id, data in p.manufacturer_data.items():
            man_data[str(company_id)] = data.hex()

        results.append({
            "identifier": p.identifier,
            "address": p.address,
            "rssi": p.rssi,
            "address_type": int(p.address_type),
            "manufacturer_data": man_data,
        })
    return results


@app.post("/connect/{address}")
def connect(address: str):
    if address in ble_state.connected and ble_state.connected[address]:
        return {"message": f"Already connected to {address}", "address": address}

    if not ble_state.scan_results:
        raise HTTPException(
            status_code=404,
            detail="No scan results available. Call /scan first to discover nearby devices.",
        )

    target = None
    for p in ble_state.scan_results:
        if p.address == address:
            target = p
            break

    if not target:
        available = [f"{p.identifier} ({p.address})" for p in ble_state.scan_results]
        raise HTTPException(
            status_code=404,
            detail=f"Device '{address}' not found in scan results. Available: {available}",
        )

    ble_state.central.set_current_peripheral(target)
    ble_state.invalidate_services_cache()
    try:
        ble_state.central.peripheral_connect()
        ble_state.connected[address] = True
        ble_state.notifications.setdefault(address, [])
        return {"message": f"Connected to {target.identifier}", "address": address}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to connect: {str(e)}")


@app.post("/disconnect/{address}")
def disconnect(address: str):
    if address not in ble_state.connected:
        raise HTTPException(status_code=404, detail="Device not found")

    try:
        ble_state.central.peripheral_disconnect()
        ble_state.connected.pop(address, None)
        ble_state.notifications.pop(address, None)
        ble_state.invalidate_services_cache()
        return {"message": f"Disconnected from {address}"}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to disconnect: {str(e)}")


@app.get("/devices/connected")
def get_connected_devices():
    """List all currently connected BLE peripherals."""
    if not ble_state.adapters:
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


@app.get("/device/{address}")
def get_device_info(address: str):
    if address not in ble_state.connected:
        raise HTTPException(status_code=404, detail="Device not found or not connected")

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


@app.post("/device/{address}/read/{service_uuid}/{char_uuid}")
def read_characteristic(address: str, service_uuid: str, char_uuid: str):
    if address not in ble_state.connected:
        raise HTTPException(status_code=404, detail="Device not found")

    if not ble_state.central.peripheral_is_connected():
        raise HTTPException(status_code=400, detail="Device not connected")

    if not ble_state.check_char_property(char_uuid, "can_read"):
        raise HTTPException(
            status_code=400,
            detail=f"Characteristic '{char_uuid}' does not support read.",
        )

    try:
        data = ble_state.central.peripheral_read(service_uuid, char_uuid)
        return {
            "service_uuid": service_uuid,
            "char_uuid": char_uuid,
            "data_hex": data.hex(),
            "data_utf8": data.decode("utf-8", errors="ignore"),
        }
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Read failed: {str(e)}")


@app.post("/device/{address}/write/{service_uuid}/{char_uuid}")
def write_characteristic(address: str, service_uuid: str, char_uuid: str, request: WriteRequest):
    if address not in ble_state.connected:
        raise HTTPException(status_code=404, detail="Device not found")

    if not ble_state.central.peripheral_is_connected():
        raise HTTPException(status_code=400, detail="Device not connected")

    if not ble_state.check_char_property(char_uuid, "can_write_request"):
        raise HTTPException(
            status_code=400,
            detail=f"Characteristic '{char_uuid}' does not support write_request.",
        )

    try:
        data_bytes = bytes.fromhex(request.data)
        ble_state.central.peripheral_write_request(service_uuid, char_uuid, data_bytes)
        return {"message": "Write successful"}
    except ValueError:
        raise HTTPException(status_code=400, detail="Invalid hex string")
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Write failed: {str(e)}")


@app.post("/device/{address}/write_command/{service_uuid}/{char_uuid}")
def write_command_characteristic(address: str, service_uuid: str, char_uuid: str, request: WriteRequest):
    if address not in ble_state.connected:
        raise HTTPException(status_code=404, detail="Device not found")

    if not ble_state.central.peripheral_is_connected():
        raise HTTPException(status_code=400, detail="Device not connected")

    if not ble_state.check_char_property(char_uuid, "can_write_command"):
        raise HTTPException(
            status_code=400,
            detail=f"Characteristic '{char_uuid}' does not support write_command.",
        )

    try:
        data_bytes = bytes.fromhex(request.data)
        ble_state.central.peripheral_write_command(service_uuid, char_uuid, data_bytes)
        return {"message": "Write command successful"}
    except ValueError:
        raise HTTPException(status_code=400, detail="Invalid hex string")
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Write command failed: {str(e)}")


@app.post("/device/{address}/read_descriptor/{service_uuid}/{char_uuid}/{desc_uuid}")
def read_descriptor(address: str, service_uuid: str, char_uuid: str, desc_uuid: str):
    """Read a descriptor value from a connected peripheral."""
    if address not in ble_state.connected:
        raise HTTPException(status_code=404, detail="Device not found")

    if not ble_state.central.peripheral_is_connected():
        raise HTTPException(status_code=400, detail="Device not connected")

    try:
        data = ble_state.central.peripheral_read_descriptor(service_uuid, char_uuid, desc_uuid)
        return {
            "service_uuid": service_uuid,
            "char_uuid": char_uuid,
            "descriptor_uuid": desc_uuid,
            "data_hex": data.hex(),
            "data_utf8": data.decode("utf-8", errors="ignore"),
        }
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Descriptor read failed: {str(e)}")


@app.post("/device/{address}/write_descriptor/{service_uuid}/{char_uuid}/{desc_uuid}")
def write_descriptor(address: str, service_uuid: str, char_uuid: str, desc_uuid: str, request: WriteRequest):
    """Write data to a descriptor on a connected peripheral."""
    if address not in ble_state.connected:
        raise HTTPException(status_code=404, detail="Device not found")

    if not ble_state.central.peripheral_is_connected():
        raise HTTPException(status_code=400, detail="Device not connected")

    try:
        data_bytes = bytes.fromhex(request.data)
        ble_state.central.peripheral_write_descriptor(service_uuid, char_uuid, desc_uuid, data_bytes)
        return {"message": "Descriptor write successful"}
    except ValueError:
        raise HTTPException(status_code=400, detail="Invalid hex string")
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Descriptor write failed: {str(e)}")


@app.post("/device/{address}/notify/{service_uuid}/{char_uuid}")
def notify_characteristic(address: str, service_uuid: str, char_uuid: str):
    if address not in ble_state.connected:
        raise HTTPException(status_code=404, detail="Device not found")

    if not ble_state.central.peripheral_is_connected():
        raise HTTPException(status_code=400, detail="Device not connected")

    def notification_callback(payload: bytes):
        item = {
            "service": service_uuid,
            "characteristic": char_uuid,
            "data_hex": payload.hex(),
            "data_utf8": payload.decode("utf-8", errors="ignore"),
            "type": "notification",
        }
        if address in ble_state.notifications:
            ble_state.notifications[address].append(item)
        _push_to_sse(address, item)

    try:
        ble_state.central.peripheral_notify(service_uuid, char_uuid, notification_callback)
        return {"message": "Subscribed to notifications"}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Notify failed: {str(e)}")


@app.post("/device/{address}/indicate/{service_uuid}/{char_uuid}")
def indicate_characteristic(address: str, service_uuid: str, char_uuid: str):
    if address not in ble_state.connected:
        raise HTTPException(status_code=404, detail="Device not found")

    if not ble_state.central.peripheral_is_connected():
        raise HTTPException(status_code=400, detail="Device not connected")

    def indication_callback(payload: bytes):
        item = {
            "service": service_uuid,
            "characteristic": char_uuid,
            "data_hex": payload.hex(),
            "data_utf8": payload.decode("utf-8", errors="ignore"),
            "type": "indication",
        }
        if address in ble_state.notifications:
            ble_state.notifications[address].append(item)
        _push_to_sse(address, item)

    try:
        ble_state.central.peripheral_indicate(service_uuid, char_uuid, indication_callback)
        return {"message": "Subscribed to indications"}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Indicate failed: {str(e)}")


@app.post("/device/{address}/unsubscribe/{service_uuid}/{char_uuid}")
def unsubscribe_characteristic(address: str, service_uuid: str, char_uuid: str):
    if address not in ble_state.connected:
        raise HTTPException(status_code=404, detail="Device not found")

    if not ble_state.central.peripheral_is_connected():
        raise HTTPException(status_code=400, detail="Device not connected")

    try:
        ble_state.central.peripheral_unsubscribe(service_uuid, char_uuid)
        return {"message": "Unsubscribed from notifications"}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Unsubscribe failed: {str(e)}")


@app.get("/device/{address}/notifications")
def get_notifications(address: str):
    if address not in ble_state.notifications:
        return []

    notifs = ble_state.notifications[address]
    ble_state.notifications[address] = []
    return notifs


# ---------------------------------------------------------------------------
# SSE streaming endpoint — real-time notify / indicate push
# ---------------------------------------------------------------------------
@app.get("/device/{address}/notifications/stream")
async def notification_stream(address: str, request: Request):
    """SSE endpoint: stream real-time BLE notifications and indications.

    Keeps the HTTP connection open and pushes each notification/indication
    as it arrives from the BLE peripheral.  Format: text/event-stream (SSE).

    Usage:
        curl -N http://127.0.0.1:8000/device/AA:BB:CC:DD/notifications/stream

    Or in JavaScript:
        const es = new EventSource('/device/AA:BB:CC:DD/notifications/stream');
        es.onmessage = (e) => console.log(JSON.parse(e.data));
    """
    if address not in ble_state.connected:
        raise HTTPException(status_code=404, detail="Device not connected")

    # Create thread-safe Queue + Event for this SSE client
    q: queue.Queue = queue.Queue()
    event = threading.Event()
    _sse_queues[address] = q
    _sse_events[address] = event

    async def event_generator():
        try:
            while True:
                # Check if client disconnected
                if await request.is_disconnected():
                    break

                # Wait for data with 30s timeout (triggers keepalive)
                event.wait(timeout=30)
                event.clear()

                # Drain all accumulated items
                drained = False
                while True:
                    try:
                        item = q.get_nowait()
                        drained = True
                        yield f"event: notification\ndata: {json.dumps(item, ensure_ascii=False)}\n\n"
                    except queue.Empty:
                        break

                # Send keepalive comment if nothing was drained
                if not drained:
                    yield ": keepalive\n\n"

        except asyncio.CancelledError:
            pass
        finally:
            _sse_queues.pop(address, None)
            _sse_events.pop(address, None)

    return StreamingResponse(
        event_generator(),
        media_type="text/event-stream",
        headers={
            "Cache-Control": "no-cache",
            "Connection": "keep-alive",
            "X-Accel-Buffering": "no",  # disable nginx buffering
        },
    )


def main():
    parser = argparse.ArgumentParser(description="SimpleCommKitAiBle REST Server")
    parser.add_argument("--host", default="127.0.0.1", help="Host to bind to")
    parser.add_argument("--port", type=int, default=8000, help="Port to bind to")
    args = parser.parse_known_args()[0]

    uvicorn.run(app, host=args.host, port=args.port)


if __name__ == "__main__":
    main()
