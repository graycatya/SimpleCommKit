"""
SimpleCommKitAiMqttClient - AI-friendly MQTT client toolkit powered by SimpleCommKitPyMqttClient
"""

from SimpleCommKitPyMqttClient import __version__

try:
    from SimpleCommKitPyMqttClient import (
        MqttClient, MqttReconnectSetting, MqttTlsSetting, MqttWillMessage, get_error_description,
    )
except ImportError:
    MqttClient = None
    MqttReconnectSetting = None
    MqttTlsSetting = None
    MqttWillMessage = None
    get_error_description = lambda code: f"Unknown error {code}"

__all__ = ["__version__", "MqttClient", "MqttReconnectSetting", "MqttTlsSetting",
           "MqttWillMessage", "get_error_description"]
