"""SimpleCommKitAiSerialPort REST Server - HTTP API for serial port control."""

try:
    from fastapi import FastAPI, HTTPException, Request
    from fastapi.responses import StreamingResponse
    from pydantic import BaseModel
    import uvicorn
except ImportError:
    print("Dependencies for SimpleCommKitAiSerialPort HTTP server are not installed.")
    print("Please install them using: pip install SimpleCommKitAiSerialPort")
    exit(1)

import asyncio
from contextlib import asynccontextmanager
from typing import List, Dict, Optional
import argparse
import json
import queue
import threading

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


class PortState:
    """Holds global serial port state shared across endpoints."""

    def __init__(self):
        self._ports: Dict[str, SerialPort] = {}
        self._initialized: bool = False

    def init(self) -> None:
        """Lazy-init. Safe to call multiple times."""
        self._initialized = True

    @staticmethod
    def get_available_ports() -> List[Dict]:
        """Enumerate available serial ports."""
        try:
            ports = SerialPort.get_available_ports()
            return [
                {
                    "port_name": p.port_name,
                    "description": p.description,
                    "hardware_id": p.hardware_id,
                }
                for p in ports
            ]
        except Exception as e:
            return [{"error": str(e)}]

    def get_or_create(self, port_name: str) -> SerialPort:
        """Return existing SerialPort or create a new one."""
        if port_name not in self._ports:
            sp = SerialPort()
            sp.set_port_name(port_name)
            sp.set_callback_error(
                lambda code: print(
                    f"[SimpleCommKitAiSerialPort-http] Error {code}: "
                    f"{get_error_description(code)}"
                )
            )
            self._ports[port_name] = sp
        return self._ports[port_name]

    def remove(self, port_name: str) -> None:
        self._ports.pop(port_name, None)


port_state = PortState()

# ---------------------------------------------------------------------------
# SSE streaming state (thread-safe bridge between serial callbacks and async SSE)
# ---------------------------------------------------------------------------
_sse_queues: Dict[str, queue.Queue] = {}
_sse_events: Dict[str, threading.Event] = {}


def _push_to_sse(port_name: str, item: Dict) -> None:
    """Push data to any active SSE listener for this port."""
    if port_name in _sse_queues:
        _sse_queues[port_name].put(item)
    if port_name in _sse_events:
        _sse_events[port_name].set()


@asynccontextmanager
async def lifespan(app: FastAPI):
    port_state.init()
    ports = port_state.get_available_ports()
    print(f"Available ports: {[p.get('port_name', '?') for p in ports]}")
    yield


app = FastAPI(
    title="SimpleCommKitAiSerialPort API",
    description="REST API to control serial ports using simple_comm_kit_serialport",
    version="0.1.0",
    lifespan=lifespan,
)


class WriteRequest(BaseModel):
    data: str  # Hex string


class ConfigRequest(BaseModel):
    baud_rate: Optional[int] = None
    parity: Optional[int] = None       # 0=None, 1=Odd, 2=Even, 3=Mark, 4=Space
    data_bits: Optional[int] = None    # 5, 6, 7, 8
    stop_bits: Optional[int] = None    # 0=One, 1=OneAndHalf, 2=Two
    flow_control: Optional[int] = None # 0=None, 1=Hardware, 2=Software
    read_buffer_size: Optional[int] = None
    read_interval_timeout: Optional[int] = None  # ms
    dtr: Optional[bool] = None
    rts: Optional[bool] = None


class OpenRequest(BaseModel):
    baud_rate: int = 9600
    parity: int = 0
    data_bits: int = 8
    stop_bits: int = 0
    flow_control: int = 0
    read_buffer_size: int = 4096


@app.get("/")
def root():
    return {"message": "SimpleCommKitAiSerialPort API is running"}


@app.get("/ports")
def list_ports():
    """List all available serial ports on the system."""
    return port_state.get_available_ports()


@app.post("/open/{port_name:path}")
def open_port(port_name: str, request: OpenRequest = None):
    """Open a serial port with the given configuration."""
    if request is None:
        request = OpenRequest()

    sp = port_state.get_or_create(port_name)

    try:
        sp.init(
            port_name=port_name,
            baud_rate=request.baud_rate,
            parity=request.parity,
            data_bits=request.data_bits,
            stop_bits=request.stop_bits,
            flow_control=request.flow_control,
            read_buffer_size=request.read_buffer_size,
        )
        ok = sp.open()
        if not ok:
            raise HTTPException(
                status_code=500,
                detail=f"Failed to open port '{port_name}': {sp.get_last_error_msg()}",
            )
        return {"message": f"Opened {port_name}", "port_name": port_name, "baud_rate": request.baud_rate}
    except HTTPException:
        raise
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to open port: {str(e)}")


@app.post("/close/{port_name:path}")
def close_port(port_name: str):
    """Close a serial port."""
    if port_name not in port_state._ports:
        raise HTTPException(status_code=404, detail="Port not found")

    sp = port_state._ports[port_name]
    try:
        sp.close()
        port_state.remove(port_name)
        _sse_queues.pop(port_name, None)
        _sse_events.pop(port_name, None)
        return {"message": f"Closed {port_name}", "port_name": port_name}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to close port: {str(e)}")


@app.get("/port/{port_name:path}")
def get_port_info(port_name: str):
    """Get status and configuration of a serial port."""
    if port_name not in port_state._ports:
        raise HTTPException(status_code=404, detail="Port not found")

    sp = port_state._ports[port_name]
    return {
        "port_name": sp.get_port_name(),
        "is_open": sp.is_open(),
        "baud_rate": sp.get_baud_rate(),
        "parity": int(sp.get_parity()),
        "data_bits": int(sp.get_data_bits()),
        "stop_bits": int(sp.get_stop_bits()),
        "flow_control": int(sp.get_flow_control()),
        "read_buffer_size": sp.get_read_buffer_size(),
        "read_interval_timeout": sp.get_read_interval_timeout(),
        "last_error": sp.get_last_error(),
        "last_error_msg": sp.get_last_error_msg(),
    }


@app.post("/port/{port_name:path}/read")
def read_port(port_name: str, size: int = 1024):
    """Read up to 'size' bytes from the serial port."""
    if port_name not in port_state._ports:
        raise HTTPException(status_code=404, detail="Port not found")

    sp = port_state._ports[port_name]
    if not sp.is_open():
        raise HTTPException(status_code=400, detail="Port not open")

    try:
        data = sp.read(size)
        return {
            "port_name": port_name,
            "size": len(data),
            "data_hex": data.hex(),
            "data_utf8": data.decode("utf-8", errors="ignore"),
        }
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Read failed: {str(e)}")


@app.post("/port/{port_name:path}/read_all")
def read_all_port(port_name: str):
    """Read all available bytes from the serial port."""
    if port_name not in port_state._ports:
        raise HTTPException(status_code=404, detail="Port not found")

    sp = port_state._ports[port_name]
    if not sp.is_open():
        raise HTTPException(status_code=400, detail="Port not open")

    try:
        data = sp.read_all()
        return {
            "port_name": port_name,
            "size": len(data),
            "data_hex": data.hex(),
            "data_utf8": data.decode("utf-8", errors="ignore"),
        }
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Read all failed: {str(e)}")


@app.post("/port/{port_name:path}/write")
def write_port(port_name: str, request: WriteRequest):
    """Write hex data to the serial port."""
    if port_name not in port_state._ports:
        raise HTTPException(status_code=404, detail="Port not found")

    sp = port_state._ports[port_name]
    if not sp.is_open():
        raise HTTPException(status_code=400, detail="Port not open")

    try:
        data_bytes = bytes.fromhex(request.data)
        written = sp.write(data_bytes)
        return {"port_name": port_name, "bytes_written": written}
    except ValueError:
        raise HTTPException(status_code=400, detail="Invalid hex string")
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Write failed: {str(e)}")


@app.post("/port/{port_name:path}/config")
def update_config(port_name: str, request: ConfigRequest):
    """Update serial port configuration. Port should be closed before changing most settings."""
    if port_name not in port_state._ports:
        raise HTTPException(status_code=404, detail="Port not found")

    sp = port_state._ports[port_name]

    try:
        if request.baud_rate is not None:
            sp.set_baud_rate(request.baud_rate)
        if request.parity is not None:
            sp.set_parity(request.parity)
        if request.data_bits is not None:
            sp.set_data_bits(request.data_bits)
        if request.stop_bits is not None:
            sp.set_stop_bits(request.stop_bits)
        if request.flow_control is not None:
            sp.set_flow_control(request.flow_control)
        if request.read_buffer_size is not None:
            sp.set_read_buffer_size(request.read_buffer_size)
        if request.read_interval_timeout is not None:
            sp.set_read_interval_timeout(request.read_interval_timeout)
        if request.dtr is not None:
            sp.set_dtr(request.dtr)
        if request.rts is not None:
            sp.set_rts(request.rts)

        return {"message": "Configuration updated", "port_name": port_name}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Config update failed: {str(e)}")


@app.post("/port/{port_name:path}/flush")
def flush_port(port_name: str, buffer: str = "all"):
    """Flush serial port buffers. buffer: 'all', 'read', or 'write'."""
    if port_name not in port_state._ports:
        raise HTTPException(status_code=404, detail="Port not found")

    sp = port_state._ports[port_name]
    if not sp.is_open():
        raise HTTPException(status_code=400, detail="Port not open")

    try:
        if buffer == "read":
            ok = sp.flush_read_buffers()
        elif buffer == "write":
            ok = sp.flush_write_buffers()
        else:
            ok = sp.flush_buffers()

        return {"message": f"Flushed {buffer} buffers", "success": ok}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Flush failed: {str(e)}")


# ---------------------------------------------------------------------------
# SSE streaming endpoint — real-time serial data push
# ---------------------------------------------------------------------------
@app.get("/port/{port_name:path}/stream")
async def serial_stream(port_name: str, request: Request):
    """SSE endpoint: stream real-time serial port data.

    Keeps the HTTP connection open and pushes each read event
    as it arrives from the serial port.  Format: text/event-stream (SSE).

    Usage:
        curl -N http://127.0.0.1:8001/port/COM3/stream

    Or in JavaScript:
        const es = new EventSource('/port/COM3/stream');
        es.onmessage = (e) => console.log(JSON.parse(e.data));
    """
    if port_name not in port_state._ports:
        raise HTTPException(status_code=404, detail="Port not found")

    sp = port_state._ports[port_name]
    if not sp.is_open():
        raise HTTPException(status_code=400, detail="Port not open")

    # Create thread-safe Queue + Event for this SSE client
    q: queue.Queue = queue.Queue()
    event = threading.Event()
    _sse_queues[port_name] = q
    _sse_events[port_name] = event

    # Register the on_read callback
    sp.set_callback_on_read(
        lambda data: _push_to_sse(
            port_name,
            {
                "data_hex": data.hex(),
                "data_utf8": data.decode("utf-8", errors="ignore"),
            },
        )
    )

    async def event_generator():
        try:
            while True:
                if await request.is_disconnected():
                    break

                event.wait(timeout=30)
                event.clear()

                drained = False
                while True:
                    try:
                        item = q.get_nowait()
                        drained = True
                        yield f"event: data\ndata: {json.dumps(item, ensure_ascii=False)}\n\n"
                    except queue.Empty:
                        break

                if not drained:
                    yield ": keepalive\n\n"

        except asyncio.CancelledError:
            pass
        finally:
            _sse_queues.pop(port_name, None)
            _sse_events.pop(port_name, None)

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
    parser = argparse.ArgumentParser(description="SimpleCommKitAiSerialPort REST Server")
    parser.add_argument("--host", default="127.0.0.1", help="Host to bind to")
    parser.add_argument("--port", type=int, default=8001, help="Port to bind to")
    args = parser.parse_known_args()[0]

    uvicorn.run(app, host=args.host, port=args.port)


if __name__ == "__main__":
    main()
