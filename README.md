# SimpleCommKit

**SimpleCommKit** 是一个跨平台的统一通信接口库 (C++20)，为 **8 种常见通信协议** 提供一致的 API，覆盖从硬件层 (BLE、USB、串口) 到网络层 (TCP、UDP、WebSocket、MQTT) 的全场景通信需求。

> **三层架构**：C++ 核心库 → Python 绑定 (pybind11) → AI 工具层 (MCP Server + REST API)

---

## 特性

- **统一 API 风格** — 所有模块共享一致的命名规范与回调模式，降低学习成本
- **跨平台支持** — 兼容 Windows / macOS / Linux
- **C++20 标准** — 现代 C++ 设计，PIMPL 模式隐藏实现细节
- **TLS/SSL 加密** — TCP、WebSocket、MQTT 均支持 TLS 安全连接
- **自动重连** — TCP、WebSocket、MQTT 支持可配置的固定/线性/指数退避重连策略
- **Python 绑定** — 通过 pybind11 提供完整的 Python 接口
- **AI 友好** — 内置 MCP Server 与 REST API，让 AI 助手可直接操控通信设备

---

## 支持的协议

| 协议 | 模块 | 角色 | 底层库 |
|------|------|------|--------|
| BLE (蓝牙低功耗) | `SimpleCommKitBle` | Central | simpleble |
| 串口 | `SimpleCommKitSerialPort` | 读写 | CSerialPort |
| HID | `SimpleCommKitHid` | 读写 | hidapi |
| USB | `SimpleCommKitUsb` | Host | libusb |
| TCP | `SimpleCommKitTcp` | Client + Server | libhv |
| UDP | `SimpleCommKitUdp` | Client + Server | libhv |
| WebSocket | `SimpleCommKitWebSocket` | Client + Server | libhv |
| MQTT | `SimpleCommKitMqttClient` | Client | libhv |

---

## 架构概览

```
┌──────────────────────────────────────────────────────────┐
│                    AI 工具层 (SimpleCommKitAi)             │
│   ┌──────────┐  ┌──────────┐  ┌──────────┐               │
│   │MCP Server│  │REST API  │  │  Skills  │  ... x8 协议   │
│   └────┬─────┘  └────┬─────┘  └────┬─────┘               │
├────────┼─────────────┼─────────────┼─────────────────────┤
│        │   Python 绑定 (pybind11)   │                      │
│   ┌────┴─────┐  ┌────┴─────┐  ┌────┴─────┐               │
│   │ PyBle    │  │ PyTcp    │  │ PyMqtt   │  ... x8 协议   │
│   └────┬─────┘  └────┬─────┘  └────┬─────┘               │
├────────┼─────────────┼─────────────┼─────────────────────┤
│        │        C++ 核心库         │                      │
│   ┌────┴────┐ ┌────┴────┐ ┌────┴────┐ ┌──────────┐      │
│   │  BLE    │ │  TCP    │ │  MQTT   │ │  Util     │      │
│   │SerialPort│ │  UDP    │ │WebSocket│ │(ErrorMap) │      │
│   │  HID    │ │         │ │         │ │           │      │
│   │  USB    │ │         │ │         │ │           │      │
│   └────┬────┘ └────┬────┘ └────┬────┘ └──────────┘      │
├────────┼─────────────┼─────────────┼─────────────────────┤
│   simpleble   libusb  CSerialPort  hidapi       libhv    │
└──────────────────────────────────────────────────────────┘
```

---

## 快速开始

### 环境要求

- **CMake** >= 3.19
- **C++20** 编译器 (GCC 10+ / Clang 12+ / MSVC 2022+)
- **Python 3.8+** (可选，用于 Python 绑定和 AI 工具)

### 构建 C++ 核心库

```bash
git clone https://github.com/your-org/SimpleCommKit.git
cd SimpleCommKit

# 配置 (默认构建静态库)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 构建
cmake --build build --config Release -j$(nproc)

# 安装
cmake --install build --config Release
```

### CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `SIMPLECOMMKIT_BUILD_SHARED_LIBS` | `OFF` | 构建动态库 |
| `SIMPLECOMMKIT_EXAMPLES` | `OFF` | 构建所有示例程序 |
| `SIMPLECOMMKITBLE_EXAMPLES` | `OFF` | 构建 BLE 示例 |
| `SIMPLECOMMKITSERIALPORT_EXAMPLES` | `OFF` | 构建串口示例 |
| `SIMPLECOMMKITHID_EXAMPLES` | `OFF` | 构建 HID 示例 |
| `SIMPLECOMMKITUSB_EXAMPLES` | `OFF` | 构建 USB 示例 |
| `SIMPLECOMMKITTCP_EXAMPLES` | `OFF` | 构建 TCP 示例 |
| `SIMPLECOMMKITUDP_EXAMPLES` | `OFF` | 构建 UDP 示例 |
| `SIMPLECOMMKITWEBSOCKET_EXAMPLES` | `OFF` | 构建 WebSocket 示例 |
| `SIMPLECOMMKITMQTTCLIENT_EXAMPLES` | `OFF` | 构建 MQTT 示例 |
| `ENABLE_SIMPLECOMMKITPYBIND` | `OFF` | 启用 Python 绑定 |
| `ENABLE_PROXY` | `OFF` | 启用网络代理 |

**构建带示例**：

```bash
cmake -B build -DSIMPLECOMMKIT_EXAMPLES=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(nproc)
```

### 在 CMake 项目中集成

```cmake
add_subdirectory(path/to/SimpleCommKit)
target_link_libraries(your_app PRIVATE
    SimpleCommKitBle
    SimpleCommKitTcp
    SimpleCommKitMqttClient
    # ... 按需添加
)
```

---

## 使用示例

### BLE — 扫描并连接外设

```cpp
#include <SimpleCommKitBleCentral.h>
using namespace SimpleCommKit;

SimpleCommKitBleCentral central;

// 获取可用适配器并选择
auto adapters = central.get_Adapters();
central.set_CurrentAdapter(adapters[0]);

// 扫描回调
central.adapter_Set_Callback_On_Scan_Found([](SimpleCommKitBlePeripheral peripheral) {
    std::cout << "Found: " << peripheral.identifier
              << " RSSI: " << peripheral.rssi << std::endl;
});

// 扫描 5 秒
central.adapter_Scan_For(5000);

// 获取扫描结果并连接
auto results = central.adapter_Get_Scan_Results();
if (!results.empty()) {
    central.set_CurrentPeripheral(results[0]);
    central.peripheral_Set_Callback_On_Connected([]() {
        std::cout << "Connected!" << std::endl;
    });
    central.peripheral_Connect();

    // 枚举服务与特征
    auto services = central.peripheral_Services();
    for (auto& svc : services) {
        for (auto& ch : svc.characteristics) {
            if (ch.can_read) {
                auto data = central.peripheral_Read(svc.uuid, ch.uuid);
            }
            if (ch.can_notify) {
                central.peripheral_Notify(svc.uuid, ch.uuid,
                    [](std::vector<uint8_t> data) { /* 处理通知 */ });
            }
        }
    }
}
```

### TCP — 客户端连接与收发

```cpp
#include <SimpleCommKitTcp.h>
using namespace SimpleCommKit;

SimpleCommKitTcpClient client;

// 回调
client.setCallback_OnConnected([]() {
    std::cout << "TCP connected" << std::endl;
});
client.setCallback_OnMessage([](std::vector<uint8_t> data) {
    std::string msg(data.begin(), data.end());
    std::cout << "Received: " << msg << std::endl;
});
client.setCallback_OnError([](ErrorCode error) {
    std::cerr << SimpleCommKitErrorMap::GetErrorDescription(error) << std::endl;
});

// 配置
client.setConnectTimeout(5000);
client.setReconnect(SimpleCommKitTcpReconnectSetting{
    .min_delay_ms = 1000,
    .max_delay_ms = 30000,
    .delay_policy = SimpleCommKitReconnectDelayPolicy::Exponential,
    .max_retry_cnt = 10
});

// 连接 & 发送
if (client.connect("127.0.0.1", 8080)) {
    client.send("Hello Server!");
}

// 断开
client.disconnect();
```

### TCP — 服务器

```cpp
#include <SimpleCommKitTcp.h>
using namespace SimpleCommKit;

SimpleCommKitTcpServer server;

server.setCallback_OnConnected([](const std::string& clientId) {
    std::cout << "Client connected: " << clientId << std::endl;
});
server.setCallback_OnMessage([](const std::string& clientId,
                                 std::vector<uint8_t> data) {
    server.sendTo(clientId, "Echo: " + std::string(data.begin(), data.end()));
});
server.setCallback_OnDisconnected([](const std::string& clientId) {
    std::cout << "Client disconnected: " << clientId << std::endl;
});

server.setMaxConnections(100);
server.start(8080);
```

### UDP — 客户端收发

```cpp
#include <SimpleCommKitUdp.h>
using namespace SimpleCommKit;

SimpleCommKitUdpClient client;

client.setCallback_OnMessage([](std::vector<uint8_t> data) {
    std::string msg(data.begin(), data.end());
    std::cout << "UDP received: " << msg << std::endl;
});

client.open(0);                                 // 绑定随机本地端口
client.sendTo("127.0.0.1", 9090, "Hello UDP");  // 发送到指定地址

// 或设置默认远程地址后直接 send()
client.setRemoteAddress("127.0.0.1", 9090);
client.send("Hello again");
```

### WebSocket — 客户端

```cpp
#include <SimpleCommKitWebSocket.h>
using namespace SimpleCommKit;

SimpleCommKitWebSocketClient client;

client.setCallback_OnMessage([](std::vector<uint8_t> data) {
    std::cout << "WS received: " << std::string(data.begin(), data.end()) << std::endl;
});

client.setPingInterval(30);
client.open("ws://127.0.0.1:8080/chat");
client.send("Hello WebSocket!");
```

### MQTT — 发布与订阅

```cpp
#include <SimpleCommKitMqttClient.h>
using namespace SimpleCommKit;

SimpleCommKitMqttClient client;

client.setClientId("my_client_001");
client.setAuth("username", "password");

client.setCallback_OnMessage([](const std::string& topic,
                                 const std::vector<uint8_t>& data) {
    std::cout << "[" << topic << "] "
              << std::string(data.begin(), data.end()) << std::endl;
});

// 连接
client.connect("broker.emqx.io", 1883);

// 订阅
client.subscribe("sensor/temperature", 1);

// 发布
client.publish("sensor/temperature", "25.6°C", 1, false);

// 遗嘱消息
client.setWill(SimpleCommKitMqttWillMessage{
    .topic   = "client/status",
    .payload = "offline",
    .qos     = 1,
    .retain  = true
});
```

---

## Python 绑定

启用 `ENABLE_SIMPLECOMMKITPYBIND=ON` 后，所有协议模块都将编译为 Python 扩展。

```python
# TCP Client
from SimpleCommKitPyTcp import SimpleCommKitTcpClient

client = SimpleCommKitTcpClient()
client.setCallback_OnMessage(lambda data: print(f"Received: {data}"))
client.connect("127.0.0.1", 8080)
client.send("Hello from Python!")
```

可用的 Python 包：

| 包名 | 协议 |
|------|------|
| `SimpleCommKitPyBle` | BLE |
| `SimpleCommKitPySerialPort` | 串口 |
| `SimpleCommKitPyHid` | HID |
| `SimpleCommKitPyUsb` | USB |
| `SimpleCommKitPyTcp` | TCP |
| `SimpleCommKitPyUdp` | UDP |
| `SimpleCommKitPyWebSocket` | WebSocket |
| `SimpleCommKitPyMqttClient` | MQTT |

---

## AI 工具层

`SimpleCommKitAi/` 为每种协议提供了独立的 MCP Server 和 REST API，让 AI 助手 (Cursor / Claude Code / Windsurf 等) 可以直接操控通信设备。

### 安装 AI 工具

```bash
cd SimpleCommKitAi/SimpleCommKitAiTcpClient
pip install -e .
```

### MCP Server 模式

配置 AI 助手使用 MCP Server：

```json
{
  "mcpServers": {
    "simplecommkitaibtcpclient": {
      "command": "python",
      "args": ["-m", "SimpleCommKitAiTcpClient.mcp_server"]
    }
  }
}
```

### REST API 模式

```bash
python -m SimpleCommKitAiTcpClient.http_server --port 8000
```

```bash
curl -X POST http://localhost:8000/connect \
  -H "Content-Type: application/json" \
  -d '{"host": "127.0.0.1", "port": 8080}'
```

可用的 AI 工具包：

| 工具包 | MCP 入口 | HTTP 入口 |
|--------|----------|-----------|
| `SimpleCommKitAiBle` | ✅ | ✅ |
| `SimpleCommKitAiSerialPort` | ✅ | ✅ |
| `SimpleCommKitAiHid` | ✅ | ✅ |
| `SimpleCommKitAiUsb` | ✅ | ✅ |
| `SimpleCommKitAiTcpClient` | ✅ | ✅ |
| `SimpleCommKitAiTcpServer` | ✅ | ✅ |
| `SimpleCommKitAiUdpClient` | ✅ | ✅ |
| `SimpleCommKitAiUdpServer` | ✅ | ✅ |
| `SimpleCommKitAiWebSocketClient` | ✅ | ✅ |
| `SimpleCommKitAiWebSocketServer` | ✅ | ✅ |
| `SimpleCommKitAiMqttClient` | ✅ | ✅ |

---

## 统一错误处理

所有模块共用统一的 32 位错误码系统：

| 位段 | 含义 |
|------|------|
| Bits 31-24 | 模块 ID (0=Ble, 1=SerialPort, 2=Hid, 3=Usb, 4=Tcp, 5=Udp, 6=WebSocket, 7=Mqtt) |
| Bits 23-16 | 错误类型 |
| Bits 15-8  | 子类型 |
| Bits 7-0   | 具体错误码 |

```cpp
#include <SimpleCommKitUtil/SimpleCommKitErrorMap.hpp>

client.setCallback_OnError([](ErrorCode error) {
    std::string desc = SimpleCommKitErrorMap::GetErrorDescription(error);
    std::cerr << "Error: " << desc << std::endl;
});
```

---

## 项目结构

```
SimpleCommKit/
├── cmake/                   # CMake 模块
├── buildscripts/            # 自定义构建脚本
├── src/                     # C++ 核心库
│   ├── SimpleCommKitUtil/   # 工具类 / 错误码
│   ├── SimpleCommKitBle/    # BLE Central
│   ├── SimpleCommKitSerialPort/
│   ├── SimpleCommKitHid/
│   ├── SimpleCommKitUsb/
│   ├── SimpleCommKitTcp/    # TCP Client + Server
│   ├── SimpleCommKitUdp/    # UDP Client + Server
│   ├── SimpleCommKitWebSocket/
│   ├── SimpleCommKitMqttClient/
│   └── SimpleCommKitPy*/    # Python 绑定 (pybind11)
├── examples/                # C++ 示例程序
├── SimpleCommKitAi/         # AI 工具层 (MCP + REST)
└── 3rdparty/                # 第三方库 (自动下载)
```

---

## 第三方依赖

| 库 | 用途 | 许可 |
|----|------|------|
| [simpleble](https://github.com/OpenBluetoothToolbox/SimpleBLE) | BLE 跨平台支持 | LGPL-2.1 |
| [libusb](https://github.com/libusb/libusb) | USB 设备通信 | LGPL-2.1 |
| [CSerialPort](https://github.com/itas109/CSerialPort) | 串口通信 | LGPL-3.0 |
| [hidapi](https://github.com/libusb/hidapi) | HID 设备通信 | BSD/GPLv3 |
| [libhv](https://github.com/ithewei/libhv) | TCP/UDP/WebSocket/MQTT | BSD-3-Clause |
| [pybind11](https://github.com/pybind/pybind11) (可选) | Python 绑定 | BSD-3-Clause |

---

## 许可

本项目基于 **MIT License** 发布，详见 [LICENSE](LICENSE)。

第三方依赖库按其各自的许可协议发布。
