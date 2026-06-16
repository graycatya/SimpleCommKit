"""SimpleCommKitAiMqttClient MCP Server - Expose MQTT operations as MCP tools."""

from fastmcp import FastMCP
import argparse
from typing import Dict, List, Optional
import threading

from SimpleCommKitAiMqttClient import MqttClient, MqttReconnectSetting, MqttTlsSetting, MqttWillMessage, get_error_description


class MqttState:
    def __init__(self) -> None:
        self.client: Optional[MqttClient] = None
        self.messages: List[Dict] = []
        self._lock = threading.Lock()

    def ensure_client(self) -> MqttClient:
        if self.client is None:
            self.client = MqttClient()
            self.client.set_callback_on_error(lambda code: print(
                f"[SimpleCommKitAiMqttClient] Error {code}: {get_error_description(code)}"))
            self.client.set_callback_on_connected(
                lambda: print("[SimpleCommKitAiMqttClient] Connected"))
            self.client.set_callback_on_disconnected(
                lambda: print("[SimpleCommKitAiMqttClient] Disconnected"))
            self.client.set_callback_on_message(lambda topic, data: self._store_message(topic, data))
        return self.client

    def _store_message(self, topic: str, data: bytes) -> None:
        with self._lock:
            self.messages.append({"topic": topic, "data_hex": data.hex(),
                                  "data_utf8": data.decode("utf-8", errors="ignore")})


state = MqttState()
mcp = FastMCP(name="SimpleCommKitAiMqttClient MCP Server")


@mcp.tool(name="mqtt_connect", description="Connect to an MQTT broker at the given host and port.",
          tags={"mqtt", "connect"},
          annotations={"title": "MQTT Connect", "readOnlyHint": False},
          meta={"version": "1.0", "role": "connection"})
def mqtt_connect(host: str, port: int = 1883, client_id: str = "", use_ssl: bool = False) -> Dict[str, str]:
    client = state.ensure_client()
    if client_id:
        client.set_client_id(client_id)
    try:
        if use_ssl:
            result = client.connect_ssl(host, port)
        else:
            result = client.connect(host, port)
    except Exception as exc:
        raise RuntimeError(f"Connect failed: {exc}") from exc
    return {"message": f"Connected to {host}:{port}" if result else "Connect failed",
            "host": host, "port": str(port)}


@mcp.tool(name="mqtt_disconnect", description="Disconnect from the MQTT broker.",
          tags={"mqtt", "disconnect"},
          annotations={"title": "MQTT Disconnect", "readOnlyHint": False},
          meta={"version": "1.0", "role": "connection"})
def mqtt_disconnect() -> Dict[str, str]:
    if state.client is None:
        raise RuntimeError("Client not initialized")
    try:
        state.client.disconnect()
    except Exception as exc:
        raise RuntimeError(f"Disconnect failed: {exc}") from exc
    return {"message": "Disconnected"}


@mcp.tool(name="mqtt_status", description="Check whether the MQTT client is connected.",
          tags={"mqtt", "read", "status"},
          annotations={"title": "MQTT Status", "readOnlyHint": True, "idempotentHint": True},
          meta={"version": "1.0", "role": "status"})
def mqtt_status() -> Dict[str, bool]:
    if state.client is None:
        return {"connected": False}
    return {"connected": state.client.is_connected()}


@mcp.tool(name="mqtt_publish", description="Publish a message to an MQTT topic. Data can be text or hex-encoded bytes.",
          tags={"mqtt", "publish", "send"},
          annotations={"title": "MQTT Publish", "readOnlyHint": False},
          meta={"version": "1.0", "role": "data"})
def mqtt_publish(topic: str, data: str, qos: int = 0, retain: bool = False, is_hex: bool = False) -> Dict:
    if state.client is None or not state.client.is_connected():
        raise RuntimeError("Client not connected")
    try:
        if is_hex:
            payload = bytes.fromhex(data)
            result = state.client.publish(topic, payload, qos, retain)
        else:
            result = state.client.publish_text(topic, data, qos, retain)
    except ValueError as exc:
        raise RuntimeError(f"Invalid hex string: {exc}") from exc
    except Exception as exc:
        raise RuntimeError(f"Publish failed: {exc}") from exc
    return {"message_id": result, "topic": topic, "qos": qos}


@mcp.tool(name="mqtt_subscribe", description="Subscribe to an MQTT topic with the given QoS level (0, 1, or 2).",
          tags={"mqtt", "subscribe"},
          annotations={"title": "MQTT Subscribe", "readOnlyHint": False},
          meta={"version": "1.0", "role": "data"})
def mqtt_subscribe(topic: str, qos: int = 0) -> Dict[str, str]:
    if state.client is None or not state.client.is_connected():
        raise RuntimeError("Client not connected")
    try:
        result = state.client.subscribe(topic, qos)
    except Exception as exc:
        raise RuntimeError(f"Subscribe failed: {exc}") from exc
    return {"message": f"Subscribed to {topic}", "topic": topic, "qos": qos, "message_id": result}


@mcp.tool(name="mqtt_unsubscribe", description="Unsubscribe from an MQTT topic.",
          tags={"mqtt", "unsubscribe"},
          annotations={"title": "MQTT Unsubscribe", "readOnlyHint": False},
          meta={"version": "1.0", "role": "data"})
def mqtt_unsubscribe(topic: str) -> Dict[str, str]:
    if state.client is None or not state.client.is_connected():
        raise RuntimeError("Client not connected")
    try:
        result = state.client.unsubscribe(topic)
    except Exception as exc:
        raise RuntimeError(f"Unsubscribe failed: {exc}") from exc
    return {"message": f"Unsubscribed from {topic}", "topic": topic}


@mcp.tool(name="mqtt_get_messages", description="Retrieve and clear all buffered MQTT messages.",
          tags={"mqtt", "read"},
          annotations={"title": "MQTT Get Messages", "readOnlyHint": False},
          meta={"version": "1.0", "role": "data"})
def mqtt_get_messages() -> List[Dict]:
    with state._lock:
        msgs = state.messages[:]
        state.messages.clear()
    return msgs


@mcp.tool(name="mqtt_set_reconnect", description="Configure automatic reconnection for the MQTT client.",
          tags={"mqtt", "config"},
          annotations={"title": "MQTT Set Reconnect", "readOnlyHint": False},
          meta={"version": "1.0", "role": "config"})
def mqtt_set_reconnect(min_delay_ms: int = 1000, max_delay_ms: int = 10000,
                       delay_policy: int = 2, max_retry_cnt: int = 0) -> Dict[str, str]:
    client = state.ensure_client()
    setting = MqttReconnectSetting()
    setting.min_delay_ms = min_delay_ms
    setting.max_delay_ms = max_delay_ms
    setting.delay_policy = delay_policy
    setting.max_retry_cnt = max_retry_cnt
    try:
        client.set_reconnect(setting)
    except Exception as exc:
        raise RuntimeError(f"Set reconnect failed: {exc}") from exc
    return {"message": "Reconnect configured"}


def main() -> None:
    parser = argparse.ArgumentParser(description="SimpleCommKitAiMqttClient MCP Server")
    parser.add_argument("--transport", default="stdio",
                        choices=["stdio", "http", "sse", "streamable-http"])
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8004)
    args = parser.parse_args()
    if args.transport in ("http", "sse", "streamable-http"):
        mcp.run(transport=args.transport, host=args.host, port=args.port)
    else:
        mcp.run()


if __name__ == "__main__":
    main()
