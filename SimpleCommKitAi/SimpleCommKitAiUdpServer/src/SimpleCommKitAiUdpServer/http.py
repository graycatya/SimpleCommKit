"""SimpleCommKitAiUdpServer REST Server - HTTP API for UDP server control."""

try:
    from fastapi import FastAPI, HTTPException, Request
    from fastapi.responses import StreamingResponse
    from pydantic import BaseModel
    import uvicorn
except ImportError:
    print("Dependencies for SimpleCommKitAiUdpServer HTTP server are not installed.")
    exit(1)

import asyncio
from contextlib import asynccontextmanager
from typing import List, Dict
import argparse, json, queue, threading

from SimpleCommKitAiUdpServer import UdpServer, get_error_description
from SimpleCommKitAiUdpServer import __version__
class UdpServerState:
    def __init__(self):
        self.server: UdpServer = None
        self.messages: List[Dict] = []
        self._lock = threading.Lock()

    def ensure_server(self) -> UdpServer:
        if self.server is None:
            self.server = UdpServer()
            self.server.set_callback_on_error(lambda code: print(
                f"[SimpleCommKitAiUdpServer-http] Error {code}: {get_error_description(code)}"))
            self.server.set_callback_on_message(
                lambda host, port, data: self._store_message(host, port, data))
        return self.server

    def _store_message(self, host: str, port: int, data: bytes) -> None:
        item = {"from_host": host, "from_port": port,
                "data_hex": data.hex(), "data_utf8": data.decode("utf-8", errors="ignore")}
        with self._lock:
            self.messages.append(item)
        self._push_sse("message", item)

    def _push_sse(self, event_type: str, data: Dict) -> None:
        for q in _sse_queues.values():
            q.put({"event": event_type, "data": data})
        for ev in _sse_events.values():
            ev.set()


state = UdpServerState()
_sse_queues: Dict[str, queue.Queue] = {}
_sse_events: Dict[str, threading.Event] = {}


@asynccontextmanager
async def lifespan(app: FastAPI):
    print("[SimpleCommKitAiUdpServer-http] Starting...")
    yield


app = FastAPI(title="SimpleCommKitAiUdpServer API",
              description="REST API to control UDP server using SimpleCommKitPyUdp",
              version=__version__, lifespan=lifespan)


class StartRequest(BaseModel):
    host: str = "0.0.0.0"
    port: int = 8080


class SendRequest(BaseModel):
    host: str = "127.0.0.1"
    port: int = 8080
    data: str = ""
    is_hex: bool = False


class BroadcastRequest(BaseModel):
    data: str = ""
    is_hex: bool = False


@app.get("/")
def root():
    return {"message": "SimpleCommKitAiUdpServer API is running"}


@app.post("/start")
def start_server(req: StartRequest):
    server = state.ensure_server()
    try:
        result = server.start(req.port, req.host)
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Server start failed: {e}")
    return {"running": result, "host": req.host, "port": req.port}


@app.post("/stop")
def stop_server():
    if state.server is None:
        raise HTTPException(status_code=400, detail="Server not initialized")
    try:
        state.server.stop()
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Server stop failed: {e}")
    return {"message": "Server stopped"}


@app.get("/status")
def server_status():
    if state.server is None:
        return {"running": False}
    return {
        "running": state.server.is_running(),
        "host": state.server.host() if state.server.is_running() else "",
        "port": state.server.port() if state.server.is_running() else 0,
    }


@app.post("/send")
def send_data(req: SendRequest):
    if state.server is None or not state.server.is_running():
        raise HTTPException(status_code=400, detail="Server not running")
    try:
        if req.is_hex:
            payload = bytes.fromhex(req.data)
            result = state.server.send_to(req.host, req.port, payload)
        else:
            result = state.server.send_to_text(req.host, req.port, req.data)
    except ValueError:
        raise HTTPException(status_code=400, detail="Invalid hex string")
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Send failed: {e}")
    return {"bytes_sent": result, "host": req.host, "port": req.port}


@app.post("/broadcast")
def broadcast(req: BroadcastRequest):
    if state.server is None or not state.server.is_running():
        raise HTTPException(status_code=400, detail="Server not running")
    try:
        if req.is_hex:
            payload = bytes.fromhex(req.data)
            result = state.server.broadcast(payload)
        else:
            result = state.server.broadcast_text(req.data)
    except ValueError:
        raise HTTPException(status_code=400, detail="Invalid hex string")
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Broadcast failed: {e}")
    return {"bytes_sent": result}


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
    parser = argparse.ArgumentParser(description="SimpleCommKitAiUdpServer REST Server")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8102)
    args = parser.parse_known_args()[0]
    uvicorn.run(app, host=args.host, port=args.port)


if __name__ == "__main__":
    main()
