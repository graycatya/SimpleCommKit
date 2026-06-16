"""SimpleCommKitAiHid REST Server - HTTP API for HID device control."""

try:
    from fastapi import FastAPI, HTTPException, Request
    from fastapi.responses import StreamingResponse
    from pydantic import BaseModel
    import uvicorn
except ImportError:
    print("Dependencies for SimpleCommKitAiHid HTTP server are not installed.")
    print("Please install them using: pip install simple-comm-kit-ai-hid[http]")
    exit(1)

import asyncio
from contextlib import asynccontextmanager
from typing import List, Dict, Optional
import argparse
import json
import queue
import threading

from SimpleCommKitAiHid import SimpleCommKitHid, HidDeviceInfo, HidBusType, get_error_description
from SimpleCommKitAiHid import __version__
class HidState:
    """Holds global HID state shared across endpoints."""

    def __init__(self):
        self.hid: Optional[SimpleCommKitHid] = None
        self.device_cache: List = []
        self._initialized: bool = False
        self._hotplug_active: bool = False

    def init(self) -> bool:
        """Lazy-initialize HID hardware. Safe to call multiple times."""
        if self._initialized:
            return self.hid is not None
        try:
            self.hid = SimpleCommKitHid()
            self.hid.set_callback_error(lambda code: print(
                f"[SimpleCommKitAiHid-http] Error {code}: {get_error_description(code)}"
            ))
            self._initialized = True
        except Exception as e:
            print(f"[SimpleCommKitAiHid-http] HID init failed: {e}")
            self._initialized = True
        return self.hid is not None

    def _ensure_ready(self) -> None:
        if not self.init():
            raise HTTPException(status_code=503, detail="HID hardware not available")

    def _ensure_hid(self) -> SimpleCommKitHid:
        self._ensure_ready()
        return self.hid

    def refresh_devices(self) -> List[Dict]:
        """Refresh the cached device list."""
        hid = self._ensure_hid()
        self.device_cache = hid.get_available_devices()
        return [_device_to_dict(d) for d in self.device_cache]


hid_state = HidState()

# ---------------------------------------------------------------------------
# SSE streaming state (thread-safe bridge between HID callbacks and async SSE)
# ---------------------------------------------------------------------------
_sse_queues: Dict[str, queue.Queue] = {}       # path → thread-safe Queue
_sse_events: Dict[str, threading.Event] = {}    # path → wake-up Event


def _push_to_sse(path: str, data: Dict) -> None:
    """Push read data to any active SSE listener for this path."""
    if path in _sse_queues:
        _sse_queues[path].put(data)
    if path in _sse_events:
        _sse_events[path].set()


def _device_to_dict(d) -> Dict:
    """Convert HidDeviceInfo to a plain dict."""
    return {
        "path": d.path,
        "manufacturer_string": d.manufacturer_string,
        "product_string": d.product_string,
        "serial_number": d.serial_number,
        "bus_type": int(d.bus_type) if hasattr(d, 'bus_type') else 0,
        "interface_number": d.interface_number,
        "release_number": d.release_number,
    }


@asynccontextmanager
async def lifespan(app: FastAPI):
    hid_state.init()
    print(f"[SimpleCommKitAiHid-http] HID server ready")
    yield
    print(f"[SimpleCommKitAiHid-http] HID server shutting down")


app = FastAPI(
    title="SimpleCommKitAiHid API",
    description="REST API to control HID devices using SimpleCommKitPyHid",
    version=__version__,
    lifespan=lifespan,
)


class WriteRequest(BaseModel):
    data: str  # Hex string


class OpenByVidPidRequest(BaseModel):
    vendor_id: int
    product_id: int
    serial_number: str = ""
    readable: bool = True


class OpenByPathRequest(BaseModel):
    path: str
    readable: bool = True


class HotplugRequest(BaseModel):
    vendor_id: int = 0
    product_id: int = 0
    poll_interval_ms: int = 1000


@app.get("/")
def root():
    return {"message": "SimpleCommKitAiHid API is running"}


@app.get("/devices")
def get_devices(vendor_id: int = 0, product_id: int = 0):
    """List available HID devices, optionally filtered by VID/PID."""
    hid = hid_state._ensure_hid()
    devices = hid.get_available_devices(vendor_id, product_id)
    return [_device_to_dict(d) for d in devices]


@app.post("/open/{path:path}")
def open_by_path(path: str, request: OpenByPathRequest):
    """Open a HID device by path."""
    hid = hid_state._ensure_hid()
    if not hid.init():
        raise HTTPException(status_code=500, detail="HID init failed")

    if hid.is_open(path):
        return {"message": f"Device already open: {path}", "path": path}

    # Set read callback BEFORE open to avoid missing initial data
    if request.readable:
        _setup_read_callback(hid, path)

    if hid.open(path, request.readable):
        return {"message": f"Opened device: {path}", "path": path}
    else:
        raise HTTPException(status_code=500, detail=f"Failed to open {path}")


@app.post("/open")
def open_by_vid_pid(request: OpenByVidPidRequest):
    """Open a HID device by VID/PID/optional serial."""
    hid = hid_state._ensure_hid()
    if not hid.init():
        raise HTTPException(status_code=500, detail="HID init failed")

    if hid.open(request.vendor_id, request.product_id, request.serial_number, request.readable):
        paths = hid.get_open_paths()
        path = paths[-1] if paths else ""
        if path:
            _setup_read_callback(hid, path)
        return {"message": f"Opened device", "path": path}
    else:
        raise HTTPException(status_code=404, detail=f"No HID device matched VID={request.vendor_id:#06x} PID={request.product_id:#06x}")


@app.post("/close/{path:path}")
def close_device(path: str):
    """Close a specific HID device by path."""
    hid = hid_state._ensure_hid()
    hid.close(path)
    # Clean up SSE for this path
    _sse_queues.pop(path, None)
    _sse_events.pop(path, None)
    return {"message": f"Closed device: {path}"}


@app.post("/close")
def close_all():
    """Close all open HID devices."""
    hid = hid_state._ensure_hid()
    hid.close()
    _sse_queues.clear()
    _sse_events.clear()
    return {"message": "Closed all devices"}


@app.get("/open_paths")
def get_open_paths():
    """Get list of currently open device paths."""
    hid = hid_state._ensure_hid()
    return hid.get_open_paths()


@app.post("/device/{path:path}/write")
def write_device(path: str, request: WriteRequest):
    """Write a report to a HID device. Data must be a hex string."""
    hid = hid_state._ensure_hid()
    if not hid.is_open(path):
        raise HTTPException(status_code=400, detail=f"Device not open: {path}")

    try:
        data_bytes = bytes.fromhex(request.data)
    except ValueError:
        raise HTTPException(status_code=400, detail="Invalid hex string")

    try:
        written = hid.write(path, data_bytes)
        if written >= 0:
            return {"message": f"Wrote {written} byte(s)", "path": path, "bytes_written": written}
        else:
            raise HTTPException(status_code=500, detail="Write failed")
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Write failed: {str(e)}")


@app.post("/device/{path:path}/feature")
def send_feature_report(path: str, request: WriteRequest):
    """Send a feature report to a HID device. Data must be a hex string."""
    hid = hid_state._ensure_hid()
    if not hid.is_open(path):
        raise HTTPException(status_code=400, detail=f"Device not open: {path}")

    try:
        data_bytes = bytes.fromhex(request.data)
    except ValueError:
        raise HTTPException(status_code=400, detail="Invalid hex string")

    try:
        sent = hid.send_feature_report(path, data_bytes)
        if sent >= 0:
            return {"message": f"Feature report sent ({sent} byte(s))", "path": path}
        else:
            raise HTTPException(status_code=500, detail="Feature report failed")
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Feature report failed: {str(e)}")


# ---------------------------------------------------------------------------
# Hotplug endpoints
# ---------------------------------------------------------------------------
@app.post("/hotplug/start")
def start_hotplug(request: HotplugRequest):
    """Start hotplug detection (polling-based)."""
    hid = hid_state._ensure_hid()
    if not hid.init(request.vendor_id, request.product_id):
        raise HTTPException(status_code=500, detail="HID init failed")

    if request.poll_interval_ms > 0:
        hid.set_hotplug_poll_interval(request.poll_interval_ms)

    def hotplug_callback(added, removed):
        added_dicts = [_device_to_dict(d) for d in added]
        removed_dicts = [_device_to_dict(d) for d in removed]
        msg = {
            "added": added_dicts,
            "removed": removed_dicts,
        }
        # Push to all SSE listeners
        for path in list(_sse_queues.keys()):
            _push_to_sse(path, {"type": "hotplug", **msg})

    hid.set_callback_on_hotplug(hotplug_callback)
    hid.start_hotplug(request.vendor_id, request.product_id)
    hid_state._hotplug_active = True
    return {"message": "Hotplug detection started", "vendor_id": request.vendor_id, "product_id": request.product_id}


@app.post("/hotplug/stop")
def stop_hotplug():
    """Stop hotplug detection."""
    hid = hid_state._ensure_hid()
    hid.stop_hotplug()
    hid_state._hotplug_active = False
    return {"message": "Hotplug detection stopped"}


@app.get("/hotplug/status")
def hotplug_status():
    """Get hotplug detection status and configuration."""
    hid = hid_state._ensure_hid()
    return {
        "active": hid.is_hotplug_active(),
        "poll_interval_ms": hid.get_hotplug_poll_interval(),
    }


# ---------------------------------------------------------------------------
# Configuration endpoints
# ---------------------------------------------------------------------------
@app.get("/device/{path:path}/config")
def get_device_config(path: str):
    """Get read poll configuration for a device."""
    hid = hid_state._ensure_hid()
    return {
        "path": path,
        "is_open": hid.is_open(path),
        "read_poll_interval_ms": hid.get_read_poll_interval(path),
        "read_data_length": hid.get_read_data_length(path),
    }


class ConfigRequest(BaseModel):
    read_poll_interval_ms: Optional[int] = None
    read_data_length: Optional[int] = None


@app.post("/device/{path:path}/config")
def set_device_config(path: str, request: ConfigRequest):
    """Update read poll configuration for a device."""
    hid = hid_state._ensure_hid()
    if request.read_poll_interval_ms is not None:
        hid.set_read_poll_interval(path, request.read_poll_interval_ms)
    if request.read_data_length is not None:
        hid.set_read_data_length(path, request.read_data_length)
    return {"message": "Configuration updated", "path": path}


# ---------------------------------------------------------------------------
# SSE streaming endpoint — real-time read data push
# ---------------------------------------------------------------------------
def _setup_read_callback(hid, path: str):
    """Register a read callback that pushes data to SSE."""

    def read_callback(data: bytes, device_path: str):
        item = {
            "path": path,
            "data_hex": data.hex(),
            "data_utf8": data.decode("utf-8", errors="ignore"),
            "type": "read",
        }
        _push_to_sse(path, item)

    hid.set_callback_on_read(read_callback)


@app.get("/device/{path:path}/stream")
async def read_stream(path: str, request: Request):
    """SSE endpoint: stream real-time HID read data.

    Keeps the HTTP connection open and pushes each read event
    as it arrives from the HID device. Format: text/event-stream (SSE).

    Usage:
        curl -N http://127.0.0.1:8002/device//dev/hidraw0/stream
    """
    hid = hid_state._ensure_hid()
    if not hid.is_open(path):
        raise HTTPException(status_code=400, detail=f"Device not open: {path}")

    # Create thread-safe Queue + Event for this SSE client
    q: queue.Queue = queue.Queue()
    event = threading.Event()
    _sse_queues[path] = q
    _sse_events[path] = event

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
                        yield f"event: {item.get('type', 'read')}\ndata: {json.dumps(item, ensure_ascii=False)}\n\n"
                    except queue.Empty:
                        break

                # Send keepalive comment if nothing was drained
                if not drained:
                    yield ": keepalive\n\n"

        except asyncio.CancelledError:
            pass
        finally:
            _sse_queues.pop(path, None)
            _sse_events.pop(path, None)

    return StreamingResponse(
        event_generator(),
        media_type="text/event-stream",
        headers={
            "Cache-Control": "no-cache",
            "Connection": "keep-alive",
            "X-Accel-Buffering": "no",
        },
    )


def main():
    parser = argparse.ArgumentParser(description="SimpleCommKitAiHid REST Server")
    parser.add_argument("--host", default="127.0.0.1", help="Host to bind to")
    parser.add_argument("--port", type=int, default=8002, help="Port to bind to")
    args = parser.parse_known_args()[0]

    uvicorn.run(app, host=args.host, port=args.port)


if __name__ == "__main__":
    main()
