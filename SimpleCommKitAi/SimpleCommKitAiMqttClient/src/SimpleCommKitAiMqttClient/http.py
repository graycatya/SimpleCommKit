"""SimpleCommKitAiMqttClient REST Server - HTTP API for MQTT client control."""

try:
    from fastapi import FastAPI, HTTPException, Request
    from fastapi.responses import StreamingResponse
    from pydantic import BaseModel
    import uvicorn
except ImportError:
    print("Dependencies for SimpleCommKitAiMqttClient HTTP server are not installed.")
    exit(1)

import asyncio
from contextlib import asynccontextmanager
from typing import List, Dict
import argparse, json, queue, threading

from SimpleCommKitAiMqttClient import MqttClient, MqttReconnectSetting, get_error_description
from SimpleCommKitAiMqttClient import __version__
class MqttState:
    def __init__(self):
        self.client: MqttClient = None
        self.messages: List[Dict] = []
        self._lock = threading.Lock()

    def ensure_client(self) -> MqttClient:
        if self.client is None:
            self.client = MqttClient()
            self.client.set_callback_on_error(lambda code: print(
                f"[SimpleCommKitAiMqttClient-http] Error {code}: {get_error_description(code)}"))
            self.client.set_callback_on_connected(
                lambda: print("[SimpleCommKitAiMqttClient-http] Connected"))
            self.client.set_callback_on_disconnected(
                lambda: print("[SimpleCommKitAiMqttClient-http] Disconnected"))
            self.client.set_callback_on_message(lambda topic, data: self._store_message(topic, data))
        return self.client

    def _store_message(self, topic: str, data: bytes) -> None:
        item = {"topic": topic, "data_hex": data.hex(), "data_utf8": data.decode("utf-8", errors="ignore")}
        with self._lock:
            self.messages.append(item)
        self._push_sse("message", item)

    def _push_sse(self, event_type: str, data: Dict) -> None:
        for q in _sse_queues.values():
            q.put({"event": event_type, "data": data})
        for ev in _sse_events.values():
            ev.set()


state = MqttState()
_sse_queues: Dict[str, queue.Queue] = {}
_sse_events: Dict[str, threading.Event] = {}


@asynccontextmanager
async def lifespan(app: FastAPI):
    print("[SimpleCommKitAiMqttClient-http] Starting...")
    yield


app = FastAPI(title="SimpleCommKitAiMqttClient API",
              description="REST API to control MQTT client using SimpleCommKitPyMqttClient",
              version=__version__, lifespan=lifespan)


class ConnectRequest(BaseModel):
    host: str = "localhost"
    port: int = 1883
    client_id: str = ""
    use_ssl: bool = False


class PublishRequest(BaseModel):
    topic: str = ""
    data: str = ""
    qos: int = 0
    retain: bool = False
    is_hex: bool = False


class SubscribeRequest(BaseModel):
    topic: str = ""
    qos: int = 0


class UnsubscribeRequest(BaseModel):
    topic: str = ""


class ReconnectRequest(BaseModel):
    min_delay_ms: int = 1000
    max_delay_ms: int = 10000
    delay_policy: int = 2
    max_retry_cnt: int = 0


@app.get("/")
def root():
    return {"message": "SimpleCommKitAiMqttClient API is running"}


@app.post("/connect")
def connect(req: ConnectRequest):
    client = state.ensure_client()
    if req.client_id:
        client.set_client_id(req.client_id)
    try:
        if req.use_ssl:
            result = client.connect_ssl(req.host, req.port)
        else:
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


@app.post("/publish")
def publish(req: PublishRequest):
    if state.client is None or not state.client.is_connected():
        raise HTTPException(status_code=400, detail="Client not connected")
    try:
        if req.is_hex:
            payload = bytes.fromhex(req.data)
            result = state.client.publish(req.topic, payload, req.qos, req.retain)
        else:
            result = state.client.publish_text(req.topic, req.data, req.qos, req.retain)
    except ValueError:
        raise HTTPException(status_code=400, detail="Invalid hex string")
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Publish failed: {e}")
    return {"message_id": result, "topic": req.topic, "qos": req.qos}


@app.post("/subscribe")
def subscribe(req: SubscribeRequest):
    if state.client is None or not state.client.is_connected():
        raise HTTPException(status_code=400, detail="Client not connected")
    try:
        result = state.client.subscribe(req.topic, req.qos)
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Subscribe failed: {e}")
    return {"topic": req.topic, "qos": req.qos, "message_id": result}


@app.post("/unsubscribe")
def unsubscribe(req: UnsubscribeRequest):
    if state.client is None or not state.client.is_connected():
        raise HTTPException(status_code=400, detail="Client not connected")
    try:
        result = state.client.unsubscribe(req.topic)
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Unsubscribe failed: {e}")
    return {"topic": req.topic}


@app.get("/messages")
def get_messages():
    with state._lock:
        msgs = state.messages[:]
        state.messages.clear()
    return msgs


@app.post("/reconnect")
def set_reconnect(req: ReconnectRequest):
    client = state.ensure_client()
    setting = MqttReconnectSetting()
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
    parser = argparse.ArgumentParser(description="SimpleCommKitAiMqttClient REST Server")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8004)
    args = parser.parse_known_args()[0]
    uvicorn.run(app, host=args.host, port=args.port)


if __name__ == "__main__":
    main()
