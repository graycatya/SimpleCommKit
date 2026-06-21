# SimpleCommKitAi 部署指南

> **版本**: 0.1.1  
> **构建系统**: CMake ≥ 3.19（C++）/ Python 3.8+（Python）  
> **运行模式**: MCP Server（Model Context Protocol）  
> **适用场景**: 为 AI Agent 提供原生硬件设备控制能力（BLE / 串口 / HID / USB / TCP / UDP / WebSocket / MQTT）  

---

## 目录

- [架构概览](#架构概览)
- [C++ Fastmcpp 部署](#c-fastmcpp-部署)
  - [构建](#构建)
  - [安装](#安装)
  - [作为系统服务运行](#作为系统服务运行)
  - [Docker 部署](#docker-部署)
  - [Android 交叉编译](#android-交叉编译)
- [Python 部署](#python-部署)
  - [安装](#安装-1)
  - [运行](#运行)
- [MCP 客户端配置](#mcp-客户端配置)
  - [Claude Desktop](#claude-desktop)
  - [Cursor / VS Code](#cursor--vs-code)
  - [自定义 MCP 客户端](#自定义-mcp-客户端)
- [端口分配表](#端口分配表)
- [安全建议](#安全建议)

---

## 架构概览

```
┌──────────────────────────────────────────────────┐
│                  AI Agent / MCP Client              │
│         (Claude Desktop / Cursor / 自定义客户端)      │
└──────────────┬───────────────────────────────────┘
               │ stdio / SSE / HTTP / Streamable HTTP
               ▼
┌──────────────────────────────────────────────────┐
│             SimpleCommKitAi Fastmcpp               │
│          (C++ 原生 MCP Server × 11 模块)            │
├──────────────────────────────────────────────────┤
│  fastmcpp (C++ MCP Framework)                     │
├──────────┬──────────┬──────────┬─────────────────┤
│  BLE     │ Serial   │   HID    │      USB        │
│  (16 工具) │ (12 工具) │ (11 工具) │   (23 工具)      │
├──────────┼──────────┼──────────┼─────────────────┤
│ TCP Cli  │ TCP Svr  │ UDP Cli  │  UDP Server     │
│  (6 工具) │  (6 工具) │  (5 工具) │   (5 工具)       │
├──────────┼──────────┼──────────┼─────────────────┤
│ WS Cli   │ WS Svr   │ MQTT     │                 │
│  (8 工具) │  (7 工具) │ (12 工具) │                 │
└──────────┴──────────┴──────────┴─────────────────┘
```

**版本说明**：
- **C++ Fastmcpp**: 原生编译，零 Python 依赖，最低延迟。推荐**生产环境**使用。
- **Python**: 基于 `fastmcp` SDK，提供 MCP Server + HTTP REST API。适合**快速原型**和 Python 生态集成。

---

## C++ Fastmcpp 部署

### 前置要求

| 依赖 | 版本 | 说明 |
|------|------|------|
| CMake | ≥ 3.19 | 构建配置 |
| C++ 编译器 | C++17 | GCC 9+ / Clang 7+ / MSVC 2019+ / Apple Clang 12+ |
| Git | 任意 | 下载第三方依赖 |
| Ninja | 任意 | (可选) 推荐作为生成器，尤其 Windows 上必须使用 |
| libusb-1.0 | (可选) | USB 模块需要。Linux: `libusb-1.0-0-dev`，macOS: `brew install libusb` |
| libdbus-1 | (可选) | BLE 模块在 Linux 上需要。`libdbus-1-dev` |
| OpenSSL | (可选) | TLS 支持。`libssl-dev` (Linux) 或 `brew install openssl` (macOS) |

### 构建

#### 方式一：构建全部模块

```bash
# 1. 克隆仓库
git clone <repository-url>
cd SimpleCommKit

# 2. 配置
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DENABLE_SIMPLECOMMKITAI_FASTMCPP=ON \
         -DENABLE_SIMPLECOMMKIT_BLE=ON \
         -DENABLE_SIMPLECOMMKIT_HID=ON \
         -DENABLE_SIMPLECOMMKIT_USB=ON \
         -DENABLE_SIMPLECOMMKIT_SERIALPORT=ON \
         -DENABLE_SIMPLECOMMKIT_MQTTCLIENT=ON

# 3. 编译
cmake --build . --config Release -j$(nproc)
```

#### 方式二：按需构建单个模块

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_SIMPLECOMMKITAI_FASTMCPP=ON -DENABLE_SIMPLECOMMKIT_BLE=ON
cmake --build . --target simplecommkitaible-fastmcpp
```

#### 方式三：仅构建网络模块（TCP/UDP/WebSocket，无需额外依赖）

```bash
cd SimpleCommKit
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_SIMPLECOMMKITAI_FASTMCPP=ON
cmake --build . --config Release -j$(nproc)
# 产出: simplecommkitaitcpclient-fastmcpp, simplecommkitaitcpserver-fastmcpp,
#       simplecommkitaiudpclient-fastmcpp, simplecommkitaiupdserver-fastmcpp,
#       simplecommkitaiwebsocketclient-fastmcpp, simplecommkitaiwebsocketserver-fastmcpp
```

#### Windows 注意事项

```powershell
# 必须使用 Ninja 生成器
mkdir build && cd build
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DENABLE_SIMPLECOMMKITAI_FASTMCPP=ON
ninja
```

#### Android 交叉编译

```powershell
# 详见 doc/BUILD.md 中 Android 构建章节
mkdir build-android && cd build-android
cmake .. -G "Ninja" `
  -DCMAKE_TOOLCHAIN_FILE="$env:ANDROID_NDK/build/cmake/android.toolchain.cmake" `
  -DANDROID_ABI=arm64-v8a `
  -DANDROID_PLATFORM=android-24 `
  -DCMAKE_BUILD_TYPE=Release `
  -DENABLE_SIMPLECOMMKITAI_FASTMCPP=ON
ninja
```

### 安装

构建产物位于 `build/bin/` 目录。安装到系统路径：

```bash
# 安装到 CMAKE_INSTALL_PREFIX（默认 build/install）
cmake --install . --prefix /usr/local

# 或只安装单个可执行文件
cp build/bin/simplecommkitaible-fastmcpp /usr/local/bin/
```

### 运行

#### 基础运行

```bash
# stdio 模式（默认，供 MCP 客户端进程调用）
simplecommkitaible-fastmcpp

# SSE 模式（适合远程访问 / 浏览器连接）
simplecommkitaible-fastmcpp --transport sse --host 0.0.0.0 --port 8003

# Streamable HTTP 模式（MCP 2025 规范推荐）
simplecommkitaible-fastmcpp --transport streamable-http --port 8003

# 查看帮助
simplecommkitaible-fastmcpp --help
```

**传输模式对比**：

| 模式 | 说明 | 适用场景 | 远程访问 |
|------|------|----------|:--------:|
| `stdio` | stdin/stdout JSON-RPC | 本地 MCP 客户端（Claude Desktop 默认） | ✗ |
| `sse` | Server-Sent Events | 浏览器 / 远程 AI Agent / Web UI | ✓ |
| `streamable-http` | HTTP POST + Session ID | MCP 2025 规范，适合生产环境 | ✓ |
| `http` | 简单 HTTP POST | 调试 / 简单集成 | ✓ |

### 作为系统服务运行

#### Linux (systemd)

为每个需要的模块创建 systemd 服务文件。以 BLE 为例：

```ini
# /etc/systemd/system/simplecommkitaible-fastmcpp.service
[Unit]
Description=SimpleCommKitAi BLE Fastmcpp MCP Server
After=network.target bluetooth.target

[Service]
Type=simple
ExecStart=/usr/local/bin/simplecommkitaible-fastmcpp --transport sse --host 127.0.0.1 --port 8003
Restart=always
RestartSec=5
User=nobody

# 安全加固
NoNewPrivileges=yes
ProtectSystem=strict
ProtectHome=yes
ReadWritePaths=/var/log

[Install]
WantedBy=multi-user.target
```

```bash
# 启用并启动
sudo systemctl daemon-reload
sudo systemctl enable simplecommkitaible-fastmcpp
sudo systemctl start simplecommkitaible-fastmcpp
sudo systemctl status simplecommkitaible-fastmcpp
```

#### macOS (launchd)

```xml
<!-- ~/Library/LaunchAgents/com.simplecommkit.ble-fastmcpp.plist -->
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
  "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>com.simplecommkit.ble-fastmcpp</string>
    <key>ProgramArguments</key>
    <array>
        <string>/usr/local/bin/simplecommkitaible-fastmcpp</string>
        <string>--transport</string>
        <string>sse</string>
        <string>--host</string>
        <string>127.0.0.1</string>
        <string>--port</string>
        <string>8003</string>
    </array>
    <key>RunAtLoad</key>
    <true/>
    <key>KeepAlive</key>
    <true/>
</dict>
</plist>
```

```bash
launchctl load ~/Library/LaunchAgents/com.simplecommkit.ble-fastmcpp.plist
```

#### 批量启动脚本

```bash
#!/bin/bash
# start-all-services.sh — 启动所有 SimpleCommKitAi Fastmcpp 服务

SERVICES=(
    "simplecommkitaible-fastmcpp:8003"
    "simplecommkitaihid-fastmcpp:8004"
    "simplecommkitaiserialport-fastmcpp:8005"
    "simplecommkitaitcpclient-fastmcpp:8006"
    "simplecommkitaitcpserver-fastmcpp:8007"
    "simplecommkitaiudpclient-fastmcpp:8008"
    "simplecommkitaiupdserver-fastmcpp:8009"
    "simplecommkitaiusb-fastmcpp:8010"
    "simplecommkitaiwebsocketclient-fastmcpp:8011"
    "simplecommkitaiwebsocketserver-fastmcpp:8012"
    "simplecommkitaimqttclient-fastmcpp:8013"
)

for entry in "${SERVICES[@]}"; do
    bin="${entry%%:*}"
    port="${entry##*:}"
    if command -v "$bin" &> /dev/null; then
        $bin --transport sse --host 127.0.0.1 --port "$port" &
        echo "Started $bin on port $port (PID $!)"
    fi
done

wait
```

### Docker 部署

```dockerfile
# Dockerfile
FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y \
    build-essential cmake git ninja-build \
    libdbus-1-dev libusb-1.0-0-dev libudev-dev libssl-dev \
    && rm -rf /var/lib/apt/lists/*

COPY . /src/SimpleCommKit
WORKDIR /src/SimpleCommKit

RUN mkdir build && cd build && \
    cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DENABLE_SIMPLECOMMKITAI_FASTMCPP=ON \
        -DENABLE_SIMPLECOMMKIT_BLE=ON \
        -DENABLE_SIMPLECOMMKIT_HID=ON \
        -DENABLE_SIMPLECOMMKIT_USB=ON \
        -DENABLE_SIMPLECOMMKIT_SERIALPORT=ON \
        -DENABLE_SIMPLECOMMKIT_MQTTCLIENT=ON \
    && ninja

FROM ubuntu:22.04
RUN apt-get update && apt-get install -y \
    libdbus-1-3 libusb-1.0-0 libudev1 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /src/SimpleCommKit/build/bin/* /usr/local/bin/

EXPOSE 8003-8013

ENTRYPOINT ["simplecommkitaible-fastmcpp"]
CMD ["--transport", "sse", "--host", "0.0.0.0", "--port", "8003"]
```

```bash
# 构建镜像（以 BLE 为例）
docker build -t simplecommkit-ble-fastmcpp .

# 运行容器
docker run -d --name ble-mcp \
    --network host \
    --privileged \                          # USB/BLE 硬件访问
    -v /var/run/dbus:/var/run/dbus \        # BLE (Linux BlueZ)
    simplecommkit-ble-fastmcpp
```

### Android 交叉编译

详见 [`BUILD.md`](./BUILD.md#android) 中的 Android 构建章节。构建出的可执行文件可部署到 Android 设备通过 `adb shell` 运行。

---

## Python 部署

Python 版本基于 `fastmcp` SDK + `FastAPI`，提供 **MCP Server**（`*-mcp` 命令）和 **REST API**（`*-http` 命令，含 Swagger 文档）双模式。与 C++ 版本暴露相同的 MCP 工具语义，但额外提供 HTTP REST 端点。

### Python 模块架构

```
SimpleCommKitAi Xxx/
├── pyproject.toml              # 依赖、入口点定义
├── src/SimpleCommKitAiXxx/
│   ├── __init__.py
│   ├── mcp.py                  # MCP Server（@mcp.tool 装饰器注册工具）
│   └── http.py                 # FastAPI REST Server + SSE 事件流
└── tests/
```

**关键依赖**：`fastmcp`（MCP 框架）、`fastapi`、`uvicorn`、`pydantic`

**底层桥接**：Python MCP 模块通过 pybind11 调用 C++ `SimpleCommKitPy*` 绑定模块：

```
Python MCP Server (simplecommkitaible-mcp)
       │
       ▼ fastmcp SDK
SimpleCommKitAiBle (Python 源码，@mcp.tool 装饰器)
       │
       ▼ import SimpleCommKitPyBle（pybind11 绑定）
SimpleCommKitBle (C++ 原生库)
       │
       ▼ SimpleBLE
```

### 全部 11 个 Python MCP 模块

| # | 模块目录 | MCP 命令 | HTTP 命令 | 底层绑定 |
|:--|----------|----------|-----------|----------|
| 1 | `SimpleCommKitAiBle` | `simplecommkitaible-mcp` | `simplecommkitaible-http` | `SimpleCommKitPyBle` |
| 2 | `SimpleCommKitAiHid` | `simplecommkitaihid-mcp` | `simplecommkitaihid-http` | `SimpleCommKitPyHid` |
| 3 | `SimpleCommKitAiSerialPort` | `simplecommkitaiserialport-mcp` | `simplecommkitaiserialport-http` | `SimpleCommKitPySerialPort` |
| 4 | `SimpleCommKitAiUsb` | `simplecommkitaiusb-mcp` | `simplecommkitaiusb-http` | `SimpleCommKitPyUsb` |
| 5 | `SimpleCommKitAiTcpClient` | `simplecommkitaictcpclient-mcp` | `simplecommkitaictcpclient-http` | `SimpleCommKitPyTcp` |
| 6 | `SimpleCommKitAiTcpServer` | `simplecommkitaictcpserver-mcp` | `simplecommkitaictcpserver-http` | `SimpleCommKitPyTcp` |
| 7 | `SimpleCommKitAiUdpClient` | `simplecommkitaiudpclient-mcp` | `simplecommkitaiudpclient-http` | `SimpleCommKitPyUdp` |
| 8 | `SimpleCommKitAiUdpServer` | `simplecommkitaiupdserver-mcp` | `simplecommkitaiupdserver-http` | `SimpleCommKitPyUdp` |
| 9 | `SimpleCommKitAiWebSocketClient` | `simplecommkitawebsocketclient-mcp` | `simplecommkitawebsocketclient-http` | `SimpleCommKitPyWebSocket` |
| 10 | `SimpleCommKitAiWebSocketServer` | `simplecommkitawebsocketserver-mcp` | `simplecommkitawebsocketserver-http` | `SimpleCommKitPyWebSocket` |
| 11 | `SimpleCommKitAiMqttClient` | `simplecommkitaimqttclient-mcp` | `simplecommkitaimqttclient-http` | `SimpleCommKitPyMqttClient` |

### 前置要求

- Python ≥ 3.8
- pybind11 绑定模块（需先从源码构建 C++ `SimpleCommKitPy*` 库）
- 虚拟环境（推荐）

### 安装

#### 方式一：逐个安装

```bash
# 1. 先构建全部 pybind11 绑定模块（参见 BUILD.md）
cd SimpleCommKit
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DENABLE_SIMPLECOMMKITPYBIND=ON \
         -DENABLE_SIMPLECOMMKIT_BLE=ON \
         -DENABLE_SIMPLECOMMKIT_HID=ON \
         -DENABLE_SIMPLECOMMKIT_USB=ON \
         -DENABLE_SIMPLECOMMKIT_SERIALPORT=ON \
         -DENABLE_SIMPLECOMMKIT_MQTTCLIENT=ON
cmake --build . --config Release

# 2. 创建虚拟环境
cd ..
python -m venv venv
# Linux/macOS: source venv/bin/activate
# Windows:     venv\Scripts\activate

# 3. 按需安装（以 BLE 为例）
cd SimpleCommKitAi/SimpleCommKitAiBle
pip install -e .
pip install -e ".[test]"
```

#### 方式二：批量安装全部 11 个模块

```bash
# 在虚拟环境中执行
cd SimpleCommKitAi
for dir in SimpleCommKitAiBle SimpleCommKitAiHid SimpleCommKitAiSerialPort \
           SimpleCommKitAiUsb SimpleCommKitAiTcpClient SimpleCommKitAiTcpServer \
           SimpleCommKitAiUdpClient SimpleCommKitAiUdpServer \
           SimpleCommKitAiWebSocketClient SimpleCommKitAiWebSocketServer \
           SimpleCommKitAiMqttClient; do
    echo "Installing $dir..."
    pip install -e "./$dir"
done
```

### Python 虚拟环境 + pybind11 的路径设置

pybind11 生成的 `.pyd`/`.so` 文件需要能被 Python 找到。在 Windows 上：

```powershell
# 将构建输出路径加入 PATH
$env:PATH = "E:\code\catgrayprj\SimpleCommKit\build\bin\Release;$env:PATH"
# 或设置 PYTHONPATH
$env:PYTHONPATH = "E:\code\catgrayprj\SimpleCommKit\build\bin\Release"
```

### 运行

```bash
# ======== MCP Server 模式（供 AI 客户端调用）========
simplecommkitaible-mcp                              # stdio（默认）
simplecommkitaible-mcp --transport sse --port 8003  # SSE 远程模式
simplecommkitaible-mcp --transport streamable-http  # MCP 2025 规范
simplecommkitaible-mcp --transport http --port 8003 # 简单 HTTP

# ======== HTTP REST API 模式（供 Web/脚本调用）========
simplecommkitaible-http --host 127.0.0.1 --port 8003
# 访问 http://127.0.0.1:8003/docs 查看 Swagger 交互式 API 文档
# 访问 http://127.0.0.1:8003/events/stream 获取 SSE 事件流
```

### Python vs C++ 运行方式对比

| 功能 | Python 版本 | C++ Fastmcpp |
|------|:----------:|:------------:|
| MCP Server (stdio) | `simplecommkitaible-mcp` | `simplecommkitaible-fastmcpp` |
| MCP Server (SSE) | `*-mcp --transport sse --port N` | `*-fastmcpp --transport sse --port N` |
| **REST API** | `simplecommkitaible-http --port N` | 不支持 |
| **Swagger 文档** | `http://host:N/docs` ✅ | 不支持 |
| **SSE 事件流** | `http://host:N/events/stream` ✅ | 不支持 |
| 工具名称/语义 | 与 C++ 版本**完全一致** | 与 Python 版本**完全一致** |

### Python MCP 客户端配置

```json
{
  "mcpServers": {
    "simplecommkitaible-mcp": {
      "command": "simplecommkitaible-mcp"
    },
    "simplecommkitaiserialport-mcp": {
      "command": "simplecommkitaiserialport-mcp"
    },
    "simplecommkitaictcpclient-mcp": {
      "command": "simplecommkitaictcpclient-mcp"
    }
  }
}
```

> **注意**：如使用虚拟环境，`command` 需用虚拟环境中 Python 的完整路径。例如：
> ```json
> {
>   "mcpServers": {
>     "simplecommkitaible-mcp": {
>       "command": "/path/to/venv/bin/simplecommkitaible-mcp"
>     }
>   }
> }
> ```

### Python HTTP REST API 端点参考

以 `simplecommkitaictcpclient-http` 为例，`/docs` 提供完整 Swagger 交互式文档：

| 方法 | 端点 | 说明 |
|------|------|------|
| `POST` | `/connect` | 建立连接 |
| `POST` | `/disconnect` | 断开连接 |
| `GET` | `/status` | 获取连接状态 |
| `POST` | `/send` | 发送数据 |
| `GET` | `/messages` | 获取缓冲消息 |
| `GET` | `/events/stream` | SSE 实时事件流 |
| `POST` | `/reconnect` | 配置自动重连 |

```bash
# 直接调用 REST API
curl -X POST http://127.0.0.1:8003/connect \
  -H "Content-Type: application/json" \
  -d '{"host":"127.0.0.1","port":8080}'

curl http://127.0.0.1:8003/status

curl -X POST http://127.0.0.1:8003/send \
  -H "Content-Type: application/json" \
  -d '{"data":"Hello Server"}'

curl http://127.0.0.1:8003/messages
```

---

## MCP 客户端配置

### Claude Desktop

编辑 Claude Desktop 配置文件：

- **macOS**: `~/Library/Application Support/Claude/claude_desktop_config.json`
- **Windows**: `%APPDATA%\Claude\claude_desktop_config.json`
- **Linux**: `~/.config/Claude/claude_desktop_config.json`

```json
{
  "mcpServers": {
    "simplecommkitaible-fastmcpp": {
      "command": "simplecommkitaible-fastmcpp"
    },
    "simplecommkitaiusb-fastmcpp": {
      "command": "simplecommkitaiusb-fastmcpp"
    },
    "simplecommkitaiserialport-fastmcpp": {
      "command": "simplecommkitaiserialport-fastmcpp"
    }
  }
}
```

### Cursor / VS Code

在 Cursor 中配置 `.cursor/mcp.json`，或在 VS Code 中配置 `.vscode/mcp.json`：

```json
{
  "mcpServers": {
    "simplecommkitaible-fastmcpp": {
      "command": "simplecommkitaible-fastmcpp"
    },
    "simplecommkitaitcpclient-fastmcpp": {
      "command": "simplecommkitaitcpclient-fastmcpp"
    },
    "simplecommkitaiwebsocketclient-fastmcpp": {
      "command": "simplecommkitaiwebsocketclient-fastmcpp"
    }
  }
}
```

### 自定义 MCP 客户端

```python
# 使用 mcp Python SDK 连接远程 SSE 服务
import asyncio
from mcp.client.session import ClientSession
from mcp.client.sse import sse_client

async def main():
    async with sse_client("http://127.0.0.1:8003/sse") as (read, write):
        async with ClientSession(read, write) as session:
            await session.initialize()
            tools = await session.list_tools()
            print("Available tools:", [t.name for t in tools.tools])

            result = await session.call_tool("get_adapters", {})
            print("Adapters:", result)

asyncio.run(main())
```

---

## 端口分配表

| 模块 | 二进制文件名 | 默认端口 | 协议栈 |
|------|-------------|:------:|--------|
| BLE | `simplecommkitaible-fastmcpp` | 8003 | SimpleBLE |
| HID | `simplecommkitaihid-fastmcpp` | 8004 | hidapi |
| SerialPort | `simplecommkitaiserialport-fastmcpp` | 8005 | CSerialPort |
| TCP Client | `simplecommkitaitcpclient-fastmcpp` | 8006 | libhv |
| TCP Server | `simplecommkitaitcpserver-fastmcpp` | 8007 | libhv |
| UDP Client | `simplecommkitaiudpclient-fastmcpp` | 8008 | libhv |
| UDP Server | `simplecommkitaiupdserver-fastmcpp` | 8009 | libhv |
| USB | `simplecommkitaiusb-fastmcpp` | 8010 | libusb-1.0 |
| WebSocket Client | `simplecommkitaiwebsocketclient-fastmcpp` | 8011 | libhv |
| WebSocket Server | `simplecommkitaiwebsocketserver-fastmcpp` | 8012 | libhv |
| MQTT Client | `simplecommkitaimqttclient-fastmcpp` | 8013 | libhv (MQTT) |

**端口范围**: 8003–8013，每个模块独享一个端口，避免冲突。

---

## 安全建议

1. **网络绑定**: 仅对外暴露时使用 `--host 0.0.0.0`。本地使用推荐 `--host 127.0.0.1`。
2. **防火墙**: 如对外暴露，使用防火墙限制访问来源 IP。
3. **反向代理**: 生产环境建议通过 Nginx/Caddy 反向代理，添加 TLS 和认证：
   ```nginx
   # /etc/nginx/sites-available/mcp-ble.conf
   server {
       listen 443 ssl;
       server_name mcp-ble.example.com;

       ssl_certificate /etc/ssl/certs/mcp.crt;
       ssl_certificate_key /etc/ssl/private/mcp.key;

       location / {
           proxy_pass http://127.0.0.1:8003;
           proxy_http_version 1.1;
           proxy_set_header Upgrade $http_upgrade;
           proxy_set_header Connection "upgrade";
           proxy_set_header Host $host;
           proxy_read_timeout 86400s;
           proxy_send_timeout 86400s;
       }
   }
   ```
4. **权限最小化**: 以非 root 用户运行，仅授予必要的硬件访问权限。
5. **日志审计**: 生产环境建议配置日志记录以追踪 MCP 工具调用。

---

## 相关文档

- [BUILD.md](./BUILD.md) — 项目构建指南
- [AI_USAGE.md](./AI_USAGE.md) — 使用指南与工具参考
- [SimpleCommKitAi 各模块 README](../SimpleCommKitAi/)
