"""SimpleCommKitAiUdpClient REST Server - HTTP API for UDP client control."""

try:
    from fastapi import FastAPI, HTTPException, Request
    from fastapi.responses import StreamingResponse
    from pydantic import BaseModel
    import uvicorn
except ImportError:
    print("Dependencies for SimpleCommKitAiUdpClient HTTP server are not installed.")
    exit(1)

import asyncio
from contextlib import asynccontextmanager
from typing import List, Dict
import argparse, json, queue, threading

from SimpleCommKitAiUdpClient import UdpClient, get_error_description
from SimpleCommKitAiUdpClient import __version__
class UdpClientState:
    def __init__(self):
        self.client: UdpClient = None
        self.messages: List[Dict] = []
        self._lock = threading.Lock()

    def ensure_client(self) -> UdpClient:
        if self.client is None:
            self.client = UdpClient()
            self.client.set_callback_on_error(lambda code: print(
                f"[SimpleCommKitAiUdpClient-http] Error {code}: {get_error_description(code)}"))
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


state = UdpClientState()
_sse_queues: Dict[str, queue.Queue] = {}
_sse_events: Dict[str, threading.Event] = {}


@asynccontextmanager
async def lifespan(app: FastAPI):
    print("[SimpleCommKitAiUdpClient-http] Starting...")
    yield


app = FastAPI(title="SimpleCommKitAiUdpClient API",
              description="REST API to control UDP client using SimpleCommKitPyUdp",
              version=__version__, lifespan=lifespan)


class OpenRequest(BaseModel):
    local_port: int = 0
    local_host: str = "0.0.0.0"


class SendRequest(BaseModel):
    host: str = "127.0.0.1"
    port: int = 8080
    data: str = ""
    is_hex: bool = False


@app.get("/")
def root():
    return {"message": "SimpleCommKitAiUdpClient API is running"}


@app.post("/open")
def open_socket(req: OpenRequest):
    client = state.ensure_client()
    try:
        result = client.open(req.local_port, req.local_host)
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Open failed: {e}")
    return {"open": result, "host": req.local_host, "port": req.local_port}


@app.post("/close")
def close_socket():
    if state.client is None:
        raise HTTPException(status_code=400, detail="Client not initialized")
    try:
        state.client.close()
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Close failed: {e}")
    return {"message": "Socket closed"}


@app.get("/status")
def status():
    if state.client is None:
        return {"open": False}
    return {"open": state.client.is_open()}


@app.post("/send")
def send_data(req: SendRequest):
    if state.client is None or not state.client.is_open():
        raise HTTPException(status_code=400, detail="Client socket not open")
    try:
        if req.is_hex:
            payload = bytes.fromhex(req.data)
            result = state.client.send_to(req.host, req.port, payload)
        else:
            result = state.client.send_to_text(req.host, req.port, req.data)
    except ValueError:
        raise HTTPException(status_code=400, detail="Invalid hex string")
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Send failed: {e}")
    return {"bytes_sent": result, "host": req.host, "port": req.port}


@app.get("/messages")
def get_messages():
    with state._lock:
        msgs = state.messages[:]
        state.messages.clear()
    return msgs


@app.get("/events/stream")
async def event_stream(request: Request):
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
        event_generator(), media_type="text/event-stream",
        headers={"Cache-Control": "no-cache", "Connection": "keep-alive", "X-Accel-Buffering": "no"},
    )


def main():
    parser = argparse.ArgumentParser(description="SimpleCommKitAiUdpClient REST Server")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8002)
    args = parser.parse_known_args()[0]
    uvicorn.run(app, host=args.host, port=args.port)


if __name__ == "__main__":
    main()
