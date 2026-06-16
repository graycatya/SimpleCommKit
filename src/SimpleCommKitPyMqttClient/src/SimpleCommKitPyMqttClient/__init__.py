"""
SimpleCommKitPyMqttClient - Python bindings for SimpleCommKit MQTT

Cross-platform MQTT client toolkit for Python.
Based on SimpleCommKit's C++ MQTT module.

Quick Start::

    from simple_comm_kit_mqtt import MqttClient

    client = MqttClient()
    client.set_client_id("py-mqtt-001")
    client.set_callback_on_message(lambda topic, data: print(f"{topic}: {data.hex()}"))
    client.set_callback_on_connected(lambda: print("Connected!"))

    client.connect("localhost", 1883)

    import time
    time.sleep(0.5)  # wait for CONNACK

    client.subscribe("test/topic", qos=1)
    client.publish_text("test/topic", "Hello from Python!")

    client.disconnect()
"""

import os as _os
import sys as _sys

# Register package directory for DLL loading (needed for Windows)

from ._SimpleCommKitPyMqttClient import get_version
__version__ = get_version()

_pkg_dir = _os.path.dirname(_os.path.abspath(__file__))
if _sys.platform == "win32" and _os.path.isdir(_pkg_dir):
    try:
        _os.add_dll_directory(_pkg_dir)
    except OSError:
        pass  # Directory may already be registered

from ._SimpleCommKitPyMqttClient import (
    get_error_description,
    MqttReconnectSetting,
    MqttTlsSetting,
    MqttWillMessage,
    MqttClient,
)

__all__ = [
    "get_error_description",
    "MqttReconnectSetting",
    "MqttTlsSetting",
    "MqttWillMessage",
    "MqttClient",
]
