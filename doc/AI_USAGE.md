# SimpleCommKitAi 使用指南

> **适用版本**: 0.1.1  
> **适用对象**: AI Agent 开发者 / MCP 客户端集成者  
> **前置阅读**: [AI_DEPLOY.md](./AI_DEPLOY.md) — 部署与安装  

---

## 目录

- [快速开始](#快速开始)
- [传输模式详解](#传输模式详解)
- [模块工具参考](#模块工具参考)
  - [BLE 蓝牙](#1-ble-蓝牙-16-工具)
  - [HID 人机接口设备](#2-hid-人机接口设备-11-工具)
  - [SerialPort 串口](#3-serialport-串口-12-工具)
  - [USB 设备](#4-usb-设备-23-工具)
  - [TCP Client](#5-tcp-client-6-工具)
  - [TCP Server](#6-tcp-server-6-工具)
  - [UDP Client](#7-udp-client-5-工具)
  - [UDP Server](#8-udp-server-5-工具)
  - [WebSocket Client](#9-websocket-client-8-工具)
  - [WebSocket Server](#10-websocket-server-7-工具)
  - [MQTT Client](#11-mqtt-client-12-工具)
- [典型工作流](#典型工作流)
- [错误处理](#错误处理)
- [C++ vs Python 版本对比](#c-vs-python-版本对比)
- [常见问题](#常见问题)

---

## 快速开始

### 方式一：C++ Fastmcpp（推荐生产环境）

```bash
# BLE MCP Server（SSE 模式，端口 8003）
simplecommkitaible-fastmcpp --transport sse --port 8003
```

```json
{
  "mcpServers": {
    "simplecommkitaible-fastmcpp": {
      "command": "simplecommkitaible-fastmcpp"
    }
  }
}
```

### 方式二：Python MCP（推荐快速原型 / Web 集成）

```bash
# 先确保 pybind11 绑定已构建并安装（参见 AI_DEPLOY.md）
# 安装 Python MCP 模块
cd SimpleCommKitAi/SimpleCommKitAiBle && pip install -e .

# 运行 MCP Server
simplecommkitaible-mcp --transport sse --port 8003

# 同时运行 HTTP REST API（另一个终端）
simplecommkitaible-http --host 127.0.0.1 --port 8003
# 访问 http://127.0.0.1:8003/docs 查看 Swagger 文档
```

```json
{
  "mcpServers": {
    "simplecommkitaible-mcp": {
      "command": "simplecommkitaible-mcp"
    }
  }
}
```

### 3. 开始使用

重启 AI 客户端后，即可通过自然语言与硬件交互：

> "扫描附近的蓝牙设备，列出它们的地址和名称。"

AI Agent 会自动调用 `scan_for` 工具获取设备列表。

---

## 传输模式详解

每个 SimpleCommKitAi Fastmcpp 服务支持 4 种传输模式：

### stdio（默认）

- **原理**: 通过标准输入/输出进行 JSON-RPC 通信
- **适用场景**: 本地 MCP 客户端（Claude Desktop、Cursor 默认方式）
- **优点**: 零网络开销，最安全
- **缺点**: 仅限本地进程间通信

```bash
simplecommkitaible-fastmcpp
```

### SSE (Server-Sent Events)

- **原理**: HTTP 长连接 + 事件流推送
- **端点**: `GET /sse`（事件流），`POST /messages`（请求）
- **适用场景**: 远程 AI Agent、Web UI、跨机器访问
- **优点**: 支持远程访问，浏览器原生支持

```bash
simplecommkitaible-fastmcpp --transport sse --host 0.0.0.0 --port 8003
```

### Streamable HTTP

- **原理**: HTTP POST + `Mcp-Session-Id` 头（MCP 2025 规范）
- **端点**: `POST /mcp`
- **适用场景**: 生产环境，新规范推荐

```bash
simplecommkitaible-fastmcpp --transport streamable-http --port 8003
```

### HTTP

- **原理**: 简单 HTTP POST JSON-RPC
- **端点**: `POST /`
- **适用场景**: 调试、简单脚本集成

```bash
simplecommkitaible-fastmcpp --transport http --port 8003
# 测试: curl -X POST http://127.0.0.1:8003/ -H "Content-Type: application/json" -d '{"jsonrpc":"2.0","method":"tools/list","id":1}'
```

---

## 模块工具参考

---

### 1. BLE 蓝牙 (16 工具)

**可执行文件**: `simplecommkitaible-fastmcpp`  
**默认端口**: `8003`  
**平台**: Windows (WinRT) • macOS (CoreBluetooth) • Linux (BlueZ)  

#### 工具列表

| # | 工具名称 | 参数 | 说明 |
|:--|----------|------|------|
| 1 | `bluetooth_enabled` | (无) | 检查蓝牙是否已启用 |
| 2 | `get_adapters` | (无) | 列出可用的蓝牙适配器 |
| 3 | `scan_for` | `duration_ms` (int, 5000) | 扫描附近的 BLE 外设 |
| 4 | `connect` | `address` (str, 必需) | 连接到指定地址的外设 |
| 5 | `disconnect` | `address` (str, 必需) | 断开指定外设的连接 |
| 6 | `connected_devices` | (无) | 列出当前已连接的外设 |
| 7 | `services` | `address` (str, 必需) | 列出 GATT 服务和特征 |
| 8 | `read` | `address` (str), `service_uuid` (str), `char_uuid` (str) | 读取特征值 |
| 9 | `write_request` | `address`, `service_uuid`, `char_uuid`, `data` (hex str) | 写入（带响应） |
| 10 | `write_command` | `address`, `service_uuid`, `char_uuid`, `data` (hex str) | 写入（无响应） |
| 11 | `notify` | `address`, `service_uuid`, `char_uuid` | 订阅通知 |
| 12 | `indicate` | `address`, `service_uuid`, `char_uuid` | 订阅指示 |
| 13 | `get_notifications` | `address` (str) | 获取缓冲的通知数据 |
| 14 | `unsubscribe` | `address`, `service_uuid`, `char_uuid` | 取消订阅 |
| 15 | `descriptor_read` | `address`, `service_uuid`, `char_uuid`, `desc_uuid` | 读取描述符 |
| 16 | `descriptor_write` | `address`, `service_uuid`, `char_uuid`, `desc_uuid`, `data` (hex) | 写入描述符 |

#### 快速上手流程

```
1. get_adapters           → 确认蓝牙适配器可用
2. scan_for(duration_ms=5000) → 发现附近设备
3. connect(address="AA:BB:CC:DD:EE:FF") → 连接到设备
4. services(address="...") → 查看设备提供的 GATT 服务
5. read(address="...", service_uuid="...", char_uuid="...") → 读取数据
6. notify(address="...", ...) → 订阅实时数据推送
7. get_notifications(address="...") → 获取实时数据
8. disconnect(address="...") → 断开连接
```

---

### 2. HID 人机接口设备 (11 工具)

**可执行文件**: `simplecommkitaihid-fastmcpp`  
**默认端口**: `8004`  
**平台**: Windows • macOS • Linux  

| # | 工具名称 | 参数 | 说明 |
|:--|----------|------|------|
| 1 | `enumerate_hid` | `vid` (hex), `pid` (hex) | 枚举 HID 设备 |
| 2 | `open_hid` | `path` (str), `vid` (hex), `pid` (hex) | 打开 HID 设备 |
| 3 | `close_hid` | `path` (str) | 关闭 HID 设备 |
| 4 | `hid_status` | `path` (str) | 检查连接状态 |
| 5 | `hid_write` | `path`, `report_id` (int, 0), `data` (hex) | 写入输出报告 |
| 6 | `hid_read` | `path`, `timeout_ms` (int) | 读取输入报告 |
| 7 | `hid_get_feature` | `path`, `report_id` (int) | 获取 Feature 报告 |
| 8 | `hid_send_feature` | `path`, `report_id` (int), `data` (hex) | 发送 Feature 报告 |
| 9 | `hid_get_manufacturer` | `path` (str) | 获取制造商字符串 |
| 10 | `hid_get_product` | `path` (str) | 获取产品字符串 |
| 11 | `hid_get_serial` | `path` (str) | 获取序列号字符串 |

#### 快速上手流程

```
1. enumerate_hid()           → 列出所有 HID 设备
2. open_hid(path="...")       → 打开目标设备
3. hid_write(data="...")      → 写入数据
4. hid_read(timeout_ms=1000)  → 读取响应
5. close_hid(path="...")      → 关闭设备
```

---

### 3. SerialPort 串口 (12 工具)

**可执行文件**: `simplecommkitaiserialport-fastmcpp`  
**默认端口**: `8005`  
**平台**: Windows • macOS • Linux  

| # | 工具名称 | 参数 | 说明 |
|:--|----------|------|------|
| 1 | `list_ports` | (无) | 列出所有可用串口 |
| 2 | `open_port` | `port` (str), `baud_rate` (int, 9600), `data_bits` (int, 8), `parity` (str, "none"), `stop_bits` (int, 1) | 打开串口 |
| 3 | `close_port` | `port` (str) | 关闭串口 |
| 4 | `close_all_ports` | (无) | 关闭所有已打开的串口 |
| 5 | `is_port_open` | `port` (str) | 检查串口是否已打开 |
| 6 | `send_data` | `port`, `data` (str), `is_hex` (bool, false) | 发送数据 |
| 7 | `read_data` | `port`, `timeout_ms` (int, 1000) | 同步读取数据 |
| 8 | `start_read_poll` | `port`, `interval_ms` (int, 100) | 启动持续读取 |
| 9 | `stop_read_poll` | `port` (str) | 停止持续读取 |
| 10 | `get_read_buffer` | `port` (str) | 获取缓冲的读取数据 |
| 11 | `set_dtr` | `port`, `enable` (bool) | 设置 DTR 信号 |
| 12 | `set_rts` | `port`, `enable` (bool) | 设置 RTS 信号 |

---

### 4. USB 设备 (23 工具)

**可执行文件**: `simplecommkitaiusb-fastmcpp`  
**默认端口**: `8010`  
**平台**: Windows (WinUSB) • macOS (IOKit) • Linux (usbfs)  

| # | 工具名称 | 参数 | 说明 |
|:--|----------|------|------|
| 1 | `get_available_devices` | `vid` (hex), `pid` (hex) | 枚举 USB 设备 |
| 2 | `open` | `path` (str), `vid`, `pid`, `serial` | 打开 USB 设备 |
| 3 | `close` | (无) | 关闭设备 |
| 4 | `is_open` | (无) | 检查是否已打开 |
| 5 | `get_device_list` | (无) | 获取缓存设备列表 |
| 6 | `get_device_interfaces` | (无) | 获取设备接口信息 |
| 7 | `get_interface_endpoints` | `interface_num` (int) | 获取端点信息 |
| 8 | `find_endpoints_by_type` | `transfer_type` (str: bulk/interrupt/isoch) | 按类型查找端点 |
| 9 | `auto_discover_endpoints` | `transfer_type` (str) | 自动发现 IN/OUT 端点 |
| 10 | `claim_interface` | `interface_num` (int) | 声明接口 |
| 11 | `release_interface` | `interface_num` (int) | 释放接口 |
| 12 | `control_transfer` | `bmRequestType`, `bRequest`, `wValue`, `wIndex`, `data` (hex), `wLength` | 控制传输 |
| 13 | `bulk_transfer` | `endpoint` (hex), `data` (hex), `direction` (in/out) | 批量传输 |
| 14 | `interrupt_transfer` | `endpoint` (hex), `data` (hex), `direction` (in/out) | 中断传输 |
| 15 | `isochronous_transfer` | `endpoint` (hex), `data` (hex), `direction`, `num_packets` | 同步传输 |
| 16 | `start_read_poll` | `endpoint` (hex), `interval_ms` (int) | 启动持续读取 |
| 17 | `stop_read_poll` | `endpoint` (hex) | 停止持续读取 |
| 18 | `get_read_data` | `endpoint` (hex) | 获取读取缓冲 |
| 19 | `start_hotplug` | `interval_ms` (int) | 启动热插拔检测 |
| 20 | `stop_hotplug` | (无) | 停止热插拔检测 |
| 21 | `get_hotplug_data` | (无) | 获取热插拔事件 |
| 22 | `set_read_poll_interval` | `endpoint` (hex), `interval_ms` (int) | 设置读取间隔 |
| 23 | `get_error` | (无) | 获取并清除错误 |

#### 快速上手流程

```
1. get_available_devices()                    → 发现 USB 设备
2. open(path="1:3")                           → 打开设备
3. claim_interface(interface_num=0)            → 声明接口
4. auto_discover_endpoints(transfer_type="bulk") → 自动发现端点
5. bulk_transfer(endpoint=0x81, data="...", direction="in") → 传输数据
6. start_read_poll(endpoint=0x81)             → 持续监控
7. get_read_data(endpoint=0x81)               → 获取数据
8. release_interface(0) + close()             → 清理
```

---

### 5. TCP Client (6 工具)

**可执行文件**: `simplecommkitaitcpclient-fastmcpp`  
**默认端口**: `8006`  

| # | 工具名称 | 参数 | 说明 |
|:--|----------|------|------|
| 1 | `tcp_connect` | `host` (str), `port` (int, 8080) | 连接到 TCP 服务器 |
| 2 | `tcp_disconnect` | (无) | 断开连接 |
| 3 | `tcp_status` | (无) | 检查连接状态 |
| 4 | `tcp_send` | `data` (str), `is_hex` (bool, false) | 发送数据 |
| 5 | `tcp_get_messages` | (无) | 获取收到的消息 |
| 6 | `tcp_set_reconnect` | `min_delay_ms`, `max_delay_ms`, `delay_policy`, `max_retry_cnt` | 配置自动重连 |

#### 快速上手流程

```
1. tcp_connect(host="127.0.0.1", port=8080)
2. tcp_send(data="Hello, Server")
3. tcp_get_messages()
4. tcp_disconnect()
```

---

### 6. TCP Server (6 工具)

**可执行文件**: `simplecommkitaitcpserver-fastmcpp`  
**默认端口**: `8007`  

| # | 工具名称 | 参数 | 说明 |
|:--|----------|------|------|
| 1 | `tcp_server_start` | `port` (int, 8080), `host` (str, "0.0.0.0") | 启动 TCP 服务器 |
| 2 | `tcp_server_stop` | (无) | 停止服务器 |
| 3 | `tcp_server_status` | (无) | 检查服务器状态 |
| 4 | `tcp_server_send` | `client_id` (int), `data` (str), `is_hex` (bool, false) | 向指定客户端发送 |
| 5 | `tcp_server_broadcast` | `data` (str), `is_hex` (bool, false) | 向所有客户端广播 |
| 6 | `tcp_server_get_messages` | (无) | 获取收到的消息 |

#### 快速上手流程

```
1. tcp_server_start(port=8080)
2. tcp_server_get_messages()        ← 查看客户端连接和消息
3. tcp_server_broadcast(data="Hello clients!")
4. tcp_server_send(client_id=1, data="Private message")
5. tcp_server_stop()
```

---

### 7. UDP Client (5 工具)

**可执行文件**: `simplecommkitaiudpclient-fastmcpp`  
**默认端口**: `8008`  

| # | 工具名称 | 参数 | 说明 |
|:--|----------|------|------|
| 1 | `udp_connect` | `host` (str), `port` (int) | 设置目标地址 |
| 2 | `udp_send` | `data` (str), `is_hex` (bool, false) | 发送 UDP 数据报 |
| 3 | `udp_get_messages` | (无) | 获取收到的数据报 |
| 4 | `udp_bind` | `port` (int) | 绑定本地端口 |
| 5 | `udp_close` | (无) | 关闭套接字 |

---

### 8. UDP Server (5 工具)

**可执行文件**: `simplecommkitaiupdserver-fastmcpp`  
**默认端口**: `8009`  

| # | 工具名称 | 参数 | 说明 |
|:--|----------|------|------|
| 1 | `udp_server_start` | `port` (int) | 启动 UDP 服务器 |
| 2 | `udp_server_stop` | (无) | 停止服务器 |
| 3 | `udp_server_status` | (无) | 检查服务器状态 |
| 4 | `udp_server_send` | `host`, `port`, `data` (str), `is_hex` (bool, false) | 发送到指定客户端 |
| 5 | `udp_server_get_messages` | (无) | 获取收到的消息 |

---

### 9. WebSocket Client (8 工具)

**可执行文件**: `simplecommkitaiwebsocketclient-fastmcpp`  
**默认端口**: `8011`  

| # | 工具名称 | 参数 | 说明 |
|:--|----------|------|------|
| 1 | `ws_connect` | `url` (str) | 连接到 WebSocket 服务器 |
| 2 | `ws_disconnect` | (无) | 断开连接 |
| 3 | `ws_status` | (无) | 检查连接状态 |
| 4 | `ws_send` | `data` (str), `is_hex` (bool, false) | 发送消息 |
| 5 | `ws_get_messages` | (无) | 获取收到的消息 |
| 6 | `ws_set_reconnect` | `min_delay_ms`, `max_delay_ms`, `delay_policy`, `max_retry_cnt` | 配置自动重连 |
| 7 | `ws_set_connect_timeout` | `timeout_ms` (int, 5000) | 设置连接超时 |
| 8 | `ws_set_ping_interval` | `interval_ms` (int) | 设置 Ping 间隔（0=禁用） |

#### 快速上手流程

```
1. ws_connect(url="ws://echo.websocket.org")
2. ws_send(data="Hello WebSocket")
3. ws_get_messages()
4. ws_disconnect()
```

---

### 10. WebSocket Server (7 工具)

**可执行文件**: `simplecommkitaiwebsocketserver-fastmcpp`  
**默认端口**: `8012`  

| # | 工具名称 | 参数 | 说明 |
|:--|----------|------|------|
| 1 | `ws_server_start` | `port` (int, 8080) | 启动 WebSocket 服务器 |
| 2 | `ws_server_stop` | (无) | 停止服务器 |
| 3 | `ws_server_status` | (无) | 检查服务器状态 |
| 4 | `ws_server_send` | `client_id` (int), `data` (str), `is_hex` (bool) | 向指定客户端发送 |
| 5 | `ws_server_broadcast` | `data` (str), `is_hex` (bool) | 向所有客户端广播 |
| 6 | `ws_server_get_messages` | (无) | 获取收到的消息 |
| 7 | `ws_server_disconnect_client` | `client_id` (int) | 断开指定客户端 |

---

### 11. MQTT Client (12 工具)

**可执行文件**: `simplecommkitaimqttclient-fastmcpp`  
**默认端口**: `8013`  

| # | 工具名称 | 参数 | 说明 |
|:--|----------|------|------|
| 1 | `mqtt_connect` | `host` (str), `port` (int, 1883), `client_id` (str), `use_ssl` (bool, false) | 连接到 MQTT Broker |
| 2 | `mqtt_disconnect` | (无) | 断开连接 |
| 3 | `mqtt_status` | (无) | 获取连接状态 |
| 4 | `mqtt_publish` | `topic` (str), `data` (str), `qos` (int, 0), `retain` (bool, false) | 发布消息 |
| 5 | `mqtt_subscribe` | `topic` (str), `qos` (int, 0) | 订阅主题 |
| 6 | `mqtt_unsubscribe` | `topic` (str) | 取消订阅 |
| 7 | `mqtt_get_messages` | `topic` (str) | 获取缓冲消息 |
| 8 | `mqtt_set_reconnect` | `min_delay_ms`, `max_delay_ms`, `delay_policy`, `max_retry_cnt` | 配置重连 |
| 9 | `mqtt_set_will` | `topic`, `data`, `qos` (int), `retain` (bool) | 设置遗嘱消息 |
| 10 | `mqtt_set_auth` | `username` (str), `password` (str) | 设置认证凭据 |
| 11 | `mqtt_set_connect_timeout` | `timeout_ms` (int) | 设置连接超时 |
| 12 | `mqtt_get_subscriptions` | (无) | 获取当前订阅列表 |

#### 快速上手流程

```
1. mqtt_connect(host="test.mosquitto.org", port=1883)
2. mqtt_subscribe(topic="sensor/temperature", qos=1)
3. mqtt_publish(topic="sensor/temperature", data="25.5", qos=1)
4. mqtt_get_messages(topic="sensor/temperature")
5. mqtt_disconnect()
```

---

## 典型工作流

### 场景 1：自动化固件测试（串口 + USB）

```
1. list_ports()                    → 找到设备串口
2. open_port(port="COM3", baud_rate=115200)
3. get_available_devices(vid="0x1234", pid="0x5678") → 确认 USB 设备在线
4. send_data(data="AT+VERSION\r\n")
5. read_data(timeout_ms=1000)      → 读取固件版本
6. send_data(data="AT+TEST\r\n")
7. get_read_buffer(port="COM3")    → 持续监控测试输出
8. close_port(port="COM3")
```

### 场景 2：IoT 传感器数据采集（BLE + MQTT）

```
1. scan_for(duration_ms=5000)      → 发现 BLE 传感器
2. connect(address="AA:BB:CC:DD:EE:FF")
3. services(address="...")         → 找到传感器数据特征
4. notify(address="...", ...)      → 订阅实时传感器数据
5. mqtt_connect(host="broker.local")
6. mqtt_publish(topic="sensor/data", data="...") → 转发到 MQTT
7. [循环] get_notifications(address="...") → 获取传感器更新
8. unsubscribe(...) + disconnect(...) + mqtt_disconnect()
```

### 场景 3：网络服务健康检查（TCP + HTTP）

```
1. tcp_connect(host="192.168.1.100", port=3306)    → 检查 MySQL
   tcp_status() → "connected"
   tcp_disconnect()
2. tcp_connect(host="192.168.1.101", port=6379)    → 检查 Redis
   tcp_send(data="PING\r\n")
   tcp_get_messages() → "+PONG"
3. ws_connect(url="ws://192.168.1.102:8080/health") → 检查 WebSocket
   ws_status() → "connected"
```

---

## 错误处理

所有 SimpleCommKitAi 模块使用统一的 `SimpleCommKitErrorMap` 进行错误码转换。

### 错误返回格式

```json
{
  "isError": true,
  "content": [{
    "type": "text",
    "text": "Error: Device not found"
  }]
}
```

### 常见错误与排查

| 错误信息 | 可能原因 | 解决方案 |
|----------|----------|----------|
| `Bluetooth not enabled` | 蓝牙未开启 | 检查系统蓝牙开关 |
| `Device not found` | 设备未连接或路径错误 | 重新枚举设备列表 |
| `Connection timeout` | 目标主机不可达或端口错误 | 检查网络连通性和端口号 |
| `Permission denied` | 权限不足（Linux USB/HID） | 添加 udev 规则或以 sudo 运行 |
| `Interface already claimed` | USB 接口已被占用 | 释放接口后重试 |
| `Not connected` | 未建立连接即调用 send/get | 先调用 connect/start 方法 |

### 调试模式

设置环境变量以获取更详细的日志输出：

```bash
export FASTMPP_DEBUG=1
simplecommkitaible-fastmcpp --transport sse --port 8003
```

---

## Python HTTP REST API 使用

Python 版本独有的 `*-http` 命令提供 REST API + Swagger 文档 + SSE 事件流，适合 Web 集成和快速调试。

### 启动

```bash
# 启动 HTTP REST API（以 TCP Client 为例）
simplecommkitaictcpclient-http --host 127.0.0.1 --port 8006

# 访问 Swagger 交互式文档
# http://127.0.0.1:8006/docs
```

### 通用 REST API 端点

以 `simplecommkitaictcpclient-http` 为例，见 [`AI_DEPLOY.md` Python HTTP REST API 端点参考](./AI_DEPLOY.md#python-http-rest-api-端点参考)。

所有 11 个模块的 HTTP 模式遵循统一的 RESTful 风格：

```bash
# 连接
curl -X POST http://127.0.0.1:8006/connect -H "Content-Type: application/json" -d '{"host":"127.0.0.1","port":8080}'

# 状态查询
curl http://127.0.0.1:8006/status

# 发送数据
curl -X POST http://127.0.0.1:8006/send -H "Content-Type: application/json" -d '{"data":"Hello"}'

# 获取消息
curl http://127.0.0.1:8006/messages

# 断开
curl -X POST http://127.0.0.1:8006/disconnect

# SSE 实时事件流
curl -N http://127.0.0.1:8006/events/stream
```

### Python 客户端集成示例

```python
# 直接使用 Python SDK 调用远程 MCP 服务
import asyncio
from mcp.client.session import ClientSession
from mcp.client.sse import sse_client

async def ble_scan():
    async with sse_client("http://127.0.0.1:8003/sse") as (read, write):
        async with ClientSession(read, write) as session:
            await session.initialize()

            # 扫描 BLE 设备
            result = await session.call_tool("scan_for", {"duration_ms": 5000})
            print("Scan results:", result)

            # 连接到设备
            result = await session.call_tool("connect", {
                "address": "AA:BB:CC:DD:EE:FF"
            })
            print("Connect result:", result)

            # 读取数据
            result = await session.call_tool("read", {
                "address": "AA:BB:CC:DD:EE:FF",
                "service_uuid": "0000180a-0000-1000-8000-00805f9b34fb",
                "char_uuid": "00002a29-0000-1000-8000-00805f9b34fb"
            })
            print("Device info:", result)

asyncio.run(ble_scan())
```

---

## C++ vs Python 版本对比

| 特性 | C++ Fastmcpp | Python |
|------|:------------:|:------:|
| **性能** | 原生 C++，最低延迟 | 通过 pybind11 调用 C++ 库 |
| **内存** | 极小（< 10MB 每进程） | Python 运行时 + 库 (~50MB+) |
| **启动速度** | 即时（< 100ms） | 较慢（Python 解释器启动） |
| **依赖** | 仅依赖系统 C 库 | 需要 Python + pip 包 |
| **MCP Server** | ✅ stdio / SSE / HTTP / Streamable | ✅ stdio / SSE / HTTP / Streamable |
| **REST API** | ❌ | ✅ (`*-http` 命令 + Swagger + SSE 事件流) |
| **部署** | 单文件二进制 | pip install + 虚拟环境 |
| **可调试性** | GDB / LLDB | pdb / IDE 断点 |
| **交叉编译** | ✅ Android / iOS / 嵌入式 | 取决于目标平台 Python 支持 |
| **适用场景** | 生产环境、嵌入设备、低资源目标 | 快速原型、Web 集成、脚本自动化 |

**推荐选择**：
- **生产部署 / 嵌入设备** → C++ Fastmcpp（单文件、极低资源消耗）
- **Web 后台 / API 集成** → Python（REST + Swagger 文档开箱即用）
- **快速验证 / 原型开发** → Python（可调试、易于修改工具逻辑）

---

## 常见问题

### Q: 如何在 Docker 中运行？

参见 [AI_DEPLOY.md#docker-部署](./AI_DEPLOY.md#docker-部署)。

### Q: 多个 MCP 服务如何统一管理？

使用批量启动脚本（见 [AI_DEPLOY.md](#批量启动脚本)）或 systemd 服务组。

### Q: BLE 扫描不到设备怎么办？

1. 确认蓝牙已启用：`bluetooth_enabled`
2. Linux 下确认 `bluetoothd` 服务运行：`systemctl status bluetooth`
3. 增加扫描时间：`scan_for(duration_ms=10000)`
4. 确保设备处于可被发现模式

### Q: USB 设备提示 "Permission denied"？

Linux 下添加 udev 规则：

```bash
# /etc/udev/rules.d/99-simplecommkit-usb.rules
SUBSYSTEM=="usb", ATTR{idVendor}=="1234", ATTR{idProduct}=="5678", MODE="0666"
sudo udevadm control --reload-rules
sudo udevadm trigger
```

### Q: 如何只构建部分模块？

```bash
# 仅构建 TCP 相关
cmake .. -DENABLE_SIMPLECOMMKITAI_FASTMCPP=ON -DENABLE_SIMPLECOMMKIT_TCP=ON
cmake --build . --target simplecommkitaitcpclient-fastmcpp simplecommkitaitcpserver-fastmcpp
```

### Q: 可以同时运行多个模块的 MCP 服务吗？

可以。每个模块默认使用不同端口（8003–8013），互不冲突。

---

## 相关文档

- [AI_DEPLOY.md](./AI_DEPLOY.md) — 部署指南（构建、安装、系统服务、Docker、安全）
- [BUILD.md](./BUILD.md) — 项目整体构建指南
- [SimpleCommKitAi 各模块 README](../SimpleCommKitAi/)
