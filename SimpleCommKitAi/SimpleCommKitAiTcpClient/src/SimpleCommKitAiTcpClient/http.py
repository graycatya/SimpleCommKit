"""SimpleCommKitAiTcpClient REST Server - HTTP API for TCP client control."""

try:
    from fastapi import FastAPI, HTTPException, Request
    from fastapi.responses import StreamingResponse
    from pydantic import BaseModel
    import uvicorn
except ImportError:
    print("Dependencies for SimpleCommKitAiTcpClient HTTP server are not installed.")
    exit(1)

import asyncio
from contextlib import asynccontextmanager
from typing import List, Dict
import argparse
import json
import queue
import threading

from SimpleCommKitAiTcpClient import TcpClient, TcpReconnectSetting, TlsSetting, get_error_description
from SimpleCommKitAiTcpClient import __version__
class TcpClientState:
    def __init__(self):
        self.client: TcpClient = None
        self.messages: List[Dict] = []
        self._lock = threading.Lock()

    def ensure_client(self) -> TcpClient:
        if self.client is None:
            self.client = TcpClient()
            self.client.set_callback_on_error(lambda code: print(
                f"[SimpleCommKitAiTcpClient-http] Error {code}: {get_error_description(code)}"))
            self.client.set_callback_on_connected(
                lambda: print("[SimpleCommKitAiTcpClient-http] Connected"))
            self.client.set_callback_on_disconnected(
                lambda: print("[SimpleCommKitAiTcpClient-http] Disconnected"))
            self.client.set_callback_on_message(lambda data: self._store_message(data))
        return self.client

    def _store_message(self, data: bytes) -> None:
        item = {"data_hex": data.hex(), "data_utf8": data.decode("utf-8", errors="ignore")}
        with self._lock:
            self.messages.append(item)
        self._push_sse("message", item)

    def _push_sse(self, event_type: str, data: Dict) -> None:
        for q in _sse_queues.values():
            q.put({"event": event_type, "data": data})
        for ev in _sse_events.values():
            ev.set()


state = TcpClientState()
_sse_queues: Dict[str, queue.Queue] = {}
_sse_events: Dict[str, threading.Event] = {}


@asynccontextmanager
async def lifespan(app: FastAPI):
    print("[SimpleCommKitAiTcpClient-http] Server starting...")
    yield


app = FastAPI(
    title="SimpleCommKitAiTcpClient API",
    description="REST API to control TCP client using SimpleCommKitPyTcp",
    version=__version__,
    lifespan=lifespan,
)


class ConnectRequest(BaseModel):
    host: str = "127.0.0.1"
    port: int = 8080


class SendRequest(BaseModel):
    data: str = ""
    is_hex: bool = False


class ReconnectRequest(BaseModel):
    min_delay_ms: int = 1000
    max_delay_ms: int = 10000
    delay_policy: int = 2
    max_retry_cnt: int = 0


@app.get("/")
def root():
    return {"message": "SimpleCommKitAiTcpClient API is running"}


@app.post("/connect")
def connect(req: ConnectRequest):
    client = state.ensure_client()
    try:
        result = client.connect(req.host, req.port)
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Connect failed: {e}")
    return {"connected": result, "host": req.host, "port": req.port}


@app.post("/disconnect")
def disconnect():
    if state.client is None:
        raise HTTPException(status_code=400, detail="Client not initialized")
    try:
        state.client.disconnect()
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Disconnect failed: {e}")
    return {"message": "Disconnected"}


@app.get("/status")
def status():
    if state.client is None:
        return {"connected": False}
    return {"connected": state.client.is_connected()}


@app.post("/send")
def send_data(req: SendRequest):
    if state.client is None or not state.client.is_connected():
        raise HTTPException(status_code=400, detail="Client not connected")
    try:
        if req.is_hex:
            payload = bytes.fromhex(req.data)
            result = state.client.send(payload)
        else:
            result = state.client.send_text(req.data)
    except ValueError:
        raise HTTPException(status_code=400, detail="Invalid hex string")
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Send failed: {e}")
    return {"bytes_sent": result}


@app.get("/messages")
def get_messages():
    with state._lock:
        msgs = state.messages[:]
        state.messages.clear()
    return msgs


@app.post("/reconnect")
def set_reconnect(req: ReconnectRequest):
    client = state.ensure_client()
    setting = TcpReconnectSetting()
    setting.min_delay_ms = req.min_delay_ms
    setting.max_delay_ms = req.max_delay_ms
    setting.delay_policy = req.delay_policy
    setting.max_retry_cnt = req.max_retry_cnt
    try:
        client.set_reconnect(setting)
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Set reconnect failed: {e}")
    return {"message": "Reconnect configured"}


@app.get("/events/stream")
async def event_stream(request: Request):
    """SSE endpoint: stream real-time TCP client events."""
    q: queue.Queue = queue.Queue()
    event = threading.Event()
    client_key = str(id(q))
    _sse_queues[client_key] = q
    _sse_events[client_key] = event

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
                        yield f"event: {item['event']}\ndata: {json.dumps(item['data'], ensure_ascii=False)}\n\n"
                    except queue.Empty:
                        break
                if not drained:
                    yield ": keepalive\n\n"
        except asyncio.CancelledError:
            pass
        finally:
            _sse_queues.pop(client_key, None)
            _sse_events.pop(client_key, None)

    return StreamingResponse(
        event_generator(),
        media_type="text/event-stream",
        headers={"Cache-Control": "no-cache", "Connection": "keep-alive", "X-Accel-Buffering": "no"},
    )


def main():
    parser = argparse.ArgumentParser(description="SimpleCommKitAiTcpClient REST Server")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8001)
    args = parser.parse_known_args()[0]
    uvicorn.run(app, host=args.host, port=args.port)


if __name__ == "__main__":
    main()
