# SimpleCommKit 构建指南

> **版本**: 0.1.1  
> **构建系统**: CMake ≥ 3.19  
> **C++ 标准**: C++17  
> **支持平台**: Windows / Linux / macOS / iOS / Android  

---

## 目录

- [前置要求](#前置要求)
- [第三方依赖](#第三方依赖)
- [CMake 构建选项](#cmake-构建选项)
- [模块与依赖关系](#模块与依赖关系)
- [快速构建](#快速构建)
- [构建示例](#构建示例)
- [平台特定说明](#平台特定说明)
- [输出产物](#输出产物)
- [常见问题](#常见问题)

---

## 前置要求

| 工具 / 环境 | 最低版本 | 说明 |
|------------|---------|------|
| **CMake** | 3.19 | 构建配置工具 |
| **C++ 编译器** | 支持 C++17 | MSVC 2019+ / GCC 9+ / Clang 7+ / Apple Clang 12+ |
| **Git** | 任意版本 | 用于 CMake 下载第三方依赖 |
| **Python** | 3.7+ | 仅在启用 Python 绑定时需要（`ENABLE_SIMPLECOMMKITPYBIND=ON`） |

### 各平台推荐编译器

| 平台 | 推荐编译器 |
|------|-----------|
| **Windows** | Visual Studio 2019 / 2022 (MSVC 19.x) |
| **Linux** | GCC 9+ 或 Clang 9+ |
| **macOS** | Apple Clang (Xcode 12+) |
| **iOS** | Apple Clang (Xcode 12+) |
| **Android** | Android NDK r21+ (Clang) |

---

## 第三方依赖

项目通过 CMake 脚本自动下载和管理所有第三方依赖，**无需手动安装**。依赖会下载到 `3rdparty/` 目录，压缩包存放在 `3rdparty-download/`。

### 依赖清单

| 依赖库 | 版本 | 用途 | 关联模块 |
|--------|------|------|----------|
| **libhv** | 1.3.4 | 跨平台网络库（TCP/UDP/WebSocket/MQTT） | Tcp, Udp, WebSocket, MqttClient |
| **libuv** | 1.52.0 | libhv 底层事件循环 | Tcp, Udp, WebSocket, MqttClient |
| **simpleble** | 0.12.1 | 跨平台 BLE 蓝牙库 | Ble |
| **CSerialPort** | 4.3.3 | 跨平台串口通信库 | SerialPort |
| **hidapi** | 0.15.0 | 跨平台 HID 设备库 | Hid |
| **libusb-cmake** | 1.0.29-0 | USB 设备通信库（CMake 封装） | Usb |
| **pybind11** | 3.0.4 | Python 绑定（可选） | PyBle, PySerialPort, PyHid, PyUsb, PyTcp, PyUdp, PyWebSocket, PyMqttClient |
| **fastmcpp** | main | C++ MCP Server（可选） | SimpleCommKitAi × 11 fastmcpp 服务 |

> **注意**: 仅已启用的模块对应的依赖会被下载和构建。

---

## CMake 构建选项

```
-D<选项名>=ON|OFF
```

### 全局选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `SIMPLECOMMKIT_BUILD_SHARED_LIBS` | `OFF` | 构建动态库（.so / .dll），默认构建静态库 |
| `SIMPLECOMMKIT_EXAMPLES` | `OFF` | 构建所有模块的示例程序 |
| `ENABLE_SIMPLECOMMKITAI_FASTMCPP` | `OFF` | 启用 AI Fastmcpp C++ 原生 MCP Server |

### 模块启用选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `ENABLE_SIMPLECOMMKIT_BLE` | `OFF` | BLE 蓝牙模块 |
| `ENABLE_SIMPLECOMMKIT_HID` | `OFF` | HID 人机接口设备模块（会自动启用 USB） |
| `ENABLE_SIMPLECOMMKIT_USB` | `OFF` | USB 直接通信模块 |
| `ENABLE_SIMPLECOMMKIT_SERIALPORT` | `OFF` | 串口通信模块 |
| `ENABLE_SIMPLECOMMKIT_TCP` | `ON` | TCP 网络模块（Client + Server） |
| `ENABLE_SIMPLECOMMKIT_UDP` | `ON` | UDP 网络模块（Client + Server） |
| `ENABLE_SIMPLECOMMKIT_WEBSOCKET` | `ON` | WebSocket 模块（Client + Server） |
| `ENABLE_SIMPLECOMMKIT_MQTTCLIENT` | `OFF` | MQTT 客户端模块 |

### 扩展选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `ENABLE_SIMPLECOMMKITPYBIND` | `OFF` | 启用 Python 绑定（pybind11），为每个协议模块生成 Python 扩展 |
| `WITH_SIMPLECOMMKIT_OPENSSL` | `OFF` | 启用 OpenSSL / TLS 支持（应用于 TCP、WebSocket、MQTT 及 Fastmcpp） |

### 代理选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `ENABLE_PROXY` | `OFF` | 启用下载代理 |
| `DEFAULT_HTTP_PROXY` | `""` | 默认 HTTP 代理地址 |
| `DEFAULT_HTTPS_PROXY` | `""` | 默认 HTTPS 代理地址 |

---

## 模块与依赖关系

```
                        SimpleCommKitUtil (工具库 / 错误码 / 版本)
                                │
          ┌─────────────────────┼──────────────────────────┐
          │           │         │         │        │       │
       Ble      SerialPort    Hid     Usb    Tcp/Udp/WS/Mqtt
          │           │         │         │        │
      simpleble   CSerialPort  hidapi   libusb    libhv
                                                  (含 libuv)
```

每个通信模块都依赖 `SimpleCommKitUtil`（错误码映射、版本信息、导出宏），再分别链接各自的第三方底层库。

---

## 快速构建

### 1. 克隆仓库

```bash
git clone <repository-url>
cd SimpleCommKit
```

### 2. 配置 CMake（默认：TCP + UDP + WebSocket）

```bash
mkdir build && cd build
cmake ..
```

### 3. 编译

```bash
cmake --build . --config Release
```

---

## 构建示例

### 仅构建默认模块（TCP + UDP + WebSocket）

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

### 启用 BLE + SerialPort + 示例

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DENABLE_SIMPLECOMMKIT_BLE=ON \
         -DENABLE_SIMPLECOMMKIT_SERIALPORT=ON \
         -DSIMPLECOMMKIT_EXAMPLES=ON
cmake --build . --config Release
```

### 启用所有模块

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DENABLE_SIMPLECOMMKIT_BLE=ON \
         -DENABLE_SIMPLECOMMKIT_HID=ON \
         -DENABLE_SIMPLECOMMKIT_USB=ON \
         -DENABLE_SIMPLECOMMKIT_SERIALPORT=ON \
         -DENABLE_SIMPLECOMMKIT_TCP=ON \
         -DENABLE_SIMPLECOMMKIT_UDP=ON \
         -DENABLE_SIMPLECOMMKIT_WEBSOCKET=ON \
         -DENABLE_SIMPLECOMMKIT_MQTTCLIENT=ON
cmake --build . --config Release
```

### 构建动态库

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DSIMPLECOMMKIT_BUILD_SHARED_LIBS=ON
cmake --build . --config Release
```

### 构建 Python 绑定

```bash
# 前提：安装 Python 3 开发头文件
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DENABLE_SIMPLECOMMKITPYBIND=ON \
         -DENABLE_SIMPLECOMMKIT_BLE=ON
cmake --build . --config Release
```

### 构建 AI Fastmcpp MCP Server

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DENABLE_SIMPLECOMMKITAI_FASTMCPP=ON \
         -DENABLE_SIMPLECOMMKIT_BLE=ON
cmake --build . --config Release
```

### 启用 TLS/SSL 支持

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DWITH_SIMPLECOMMKIT_OPENSSL=ON \
         -DENABLE_SIMPLECOMMKIT_MQTTCLIENT=ON
cmake --build . --config Release
```

### 使用代理下载

```bash
mkdir build && cd build
cmake .. -DENABLE_PROXY=ON \
         -DDEFAULT_HTTP_PROXY="http://proxy.example.com:8080"
cmake --build . --config Release
```

---

## 平台特定说明

### Windows (MSVC)

```powershell
# Visual Studio 2019 / 2022
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
```

- 推荐使用 **x64** 平台
- MSVC 编译器级别设置为 `/W4`

### Linux

```bash
# 安装必要的系统依赖
sudo apt-get install -y build-essential cmake git    # Debian/Ubuntu
# 或
sudo yum install -y gcc-c++ cmake git                # CentOS/RHEL

# 蓝牙开发依赖（BLE 模块）
sudo apt-get install -y libdbus-1-dev                # BlueZ D-Bus

# USB 开发依赖（USB/HID 模块）
sudo apt-get install -y libusb-1.0-0-dev
sudo apt-get install -y libudev-dev

# OpenSSL (TLS 支持)
sudo apt-get install -y libssl-dev

# 构建
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release -j$(nproc)
```

- 编译器选项：`-Wall -fPIC -Wno-stringop-overflow`
- 建议使用 `-j$(nproc)` 并行加速编译

### macOS

```bash
# 安装 Xcode Command Line Tools（如未安装）
xcode-select --install

# 使用 Homebrew 安装可选依赖
brew install cmake openssl

# 构建
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release -j$(sysctl -n hw.ncpu)
```

### iOS

iOS 使用专用工具链 `cmake/ios.toolchain.cmake`。串口、HID、USB 模块会自动禁用（iOS 不支持），其余网络模块（TCP、UDP、WebSocket）可正常使用。

#### 真机 (arm64) — 默认

```bash
cmake -B build_ios -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake
cmake --build build_ios
```

#### 模拟器 (Apple Silicon Mac 推荐)

```bash
cmake -B build_sim \
  -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake \
  -DPLATFORM=SIMULATORARM64
cmake --build build_sim
```

#### 模拟器 (Intel Mac)

```bash
cmake -B build_sim \
  -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake \
  -DPLATFORM=SIMULATOR64
cmake --build build_sim
```

#### FAT 静态库 (真机 + 模拟器二合一)

> 需要 CMake 3.14+ 和 Xcode generator

```bash
cmake -B build_fat -G Xcode \
  -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake \
  -DPLATFORM=OS64COMBINED
cmake --build build_fat --config Release
cmake --install build_fat --config Release
```

#### iOS 工具链参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `PLATFORM` | `OS64` | `OS64` / `SIMULATORARM64` / `SIMULATOR64` / `OS64COMBINED` |
| `DEPLOYMENT_TARGET` | `15.0` | 最低 iOS 部署版本 |
| `ENABLE_BITCODE` | `OFF` | Bitcode（Xcode 14+ 已废弃） |
| `ENABLE_ARC` | `ON` | 自动引用计数 |
| `ENABLE_SIMPLECOMMKIT_BLE` | `OFF` | BLE 蓝牙（需额外集成 CoreBluetooth） |

> **注意**：iOS 上可用的模块为 TCP、UDP、WebSocket、MQTT。BLE 需要额外集成 iOS 原生 CoreBluetooth 框架，其他模块（串口/HID/USB/Python绑定/AI Fastmcpp）在 iOS 平台不可用。

### Android

使用 Android NDK 工具链进行交叉编译：

```bash
# Linux / macOS
mkdir build-android && cd build-android
cmake .. -G "Ninja" \
         -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
         -DANDROID_ABI=arm64-v8a \
         -DANDROID_PLATFORM=android-24 \
         -DCMAKE_BUILD_TYPE=Release
ninja
```

#### Windows 上构建 Android

在 Windows 上必须使用 **Ninja 生成器**（不能使用 MSBuild / Visual Studio 生成器），并清理 VS 自带的 Android 环境变量以避免干扰：

```powershell
# 1. 清理 VS Android 环境变量（避免覆盖 NDK 配置）
$env:ANDROID_HOME = ""
$env:ANDROID_SDK_ROOT = ""
$env:NDK_ROOT = ""

# 2. 配置与编译（必须指定 -G "Ninja"）
mkdir build-android && cd build-android
cmake .. -G "Ninja" `
  -DCMAKE_TOOLCHAIN_FILE="D:\androidsdk\ndk\25.1.8937393\build\cmake\android.toolchain.cmake" `
  -DANDROID_ABI=arm64-v8a `
  -DANDROID_PLATFORM=android-24 `
  -DCMAKE_BUILD_TYPE=Release
ninja
```

#### Android 构建注意事项

| 注意项 | 说明 |
|--------|------|
| **NDK 版本** | 推荐 NDK r23+（r25 已测试通过） |
| **生成器** | 必须使用 Ninja（`-G "Ninja"`），MSBuild 会错误使用 VS 自带 NDK |
| **Android API Level** | `android-24` 是最低推荐版本。`android-25` 会被自动降级为 `android-24` |
| **pthread** | Android 上 pthread 已内置于 bionic libc，不能用 `-lpthread`，已自动处理 |
| **libnativehelper** | NDK r23+ 已移除该库，JNI 符号桩已内置于 simpleble 的 Android 后端 |
| **BLE 可执行文件** | BLE 示例在 Android 上需运行在 JVM 环境中（通过 `app_process` 等方式），无法直接执行 |
| **MSVC 环境变量** | 如果同时安装了 Visual Studio，需清理 `ANDROID_HOME`、`ANDROID_SDK_ROOT`、`NDK_ROOT` 环境变量 |

#### Android 适配修改说明

以下针对 NDK r23+ 的兼容性修改已内置于项目中：

| 文件 | 修改内容 | 原因 |
|------|----------|------|
| `3rdparty/simpleble/simpleble/CMakeLists.txt:392` | 移除 `nativehelper` 链接 | NDK r23+ 已移除 `libnativehelper` 库 |
| `3rdparty/simpleble/simpleble/CMakeLists.txt` | 新增 `JniStubs.cpp` 源文件 | 为 NDK r23+ 移除的 `JNI_GetCreatedJavaVMs` 提供符号桩，详见下方代码 |
| `3rdparty/CSerialPort/lib/CMakeLists.txt:74` | `UNIX` → `UNIX AND NOT ANDROID` | Android 的 pthread 在 bionic libc 中，不存在独立的 `libpthread.so` |

**`JniStubs.cpp`** — 位置：`3rdparty/simpleble/simpleble/src/backends/android/JniStubs.cpp`

该文件为 NDK r23+ 移除的 JNI Invocation API 函数提供链接时符号桩。
在真实 Android 设备上运行时，`libart.so`（Android 运行时）会先加载并提供真正实现，此桩函数不会被实际调用。

```cpp
/**
 * @file JniStubs.cpp
 * @brief Stubs for JNI Invocation API functions removed from NDK r23+.
 *
 * Prior to NDK r23, libnativehelper provided static stubs for JNI Invocation
 * API functions (JNI_GetCreatedJavaVMs, etc.). These were removed in NDK r23
 * because they are expected to be provided by the Android runtime (libart.so)
 * at load time.
 *
 * For static libraries linked into executables, the linker needs all symbols
 * resolved at link time. These stubs satisfy the linker; the real
 * implementations from libart.so take precedence at runtime.
 */

#include <jni.h>

extern "C" {

jint JNI_GetCreatedJavaVMs(JavaVM** vmBuf, jsize bufLen, jsize* nVMs) {
    (void)vmBuf;
    (void)bufLen;
    if (nVMs) *nVMs = 0;
    return JNI_ERR;
}

} // extern "C"
```

---

## 输出产物

构建输出位于 `build/` 目录下：

```
build/
├── install/                              # 安装目录（CMAKE_INSTALL_PREFIX）
│   ├── include/
│   │   ├── SimpleCommKit.h               # 总括头文件（自动生成）
│   │   ├── SimpleCommKitVersion.h        # 版本头文件（自动生成）
│   │   ├── SimpleCommKitErrorMap.hpp     # 错误码头文件
│   │   ├── SimpleCommKitExport.h         # 导出宏头文件
│   │   ├── SimpleCommKitTcp.h            # TCP 头文件
│   │   ├── SimpleCommKitUdp.h            # UDP 头文件
│   │   ├── SimpleCommKitWebSocket.h      # WebSocket 头文件
│   │   └── ...                           # 根据启用的模块
│   ├── lib/
│   │   ├── libSimpleCommKitUtil.a        # 工具库
│   │   ├── libSimpleCommKitTcp.a         # TCP 库
│   │   └── ...                           # 各模块静态库
│   └── bin/
│       └── ...                           # 动态库（如启用）
├── 3rdparty/                             # 下载的第三方库源码
├── 3rdparty-download/                    # 下载的第三方库压缩包
└── include/                              # 构建时生成的公共头文件
```

### 使用方式

集成到你的 CMake 项目中：

```cmake
find_package(SimpleCommKit REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE
    SimpleCommKit::SimpleCommKitTcp
    SimpleCommKit::SimpleCommKitUtil
)
```

或者直接包含源目录构建：

```cmake
add_subdirectory(path/to/SimpleCommKit)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE
    SimpleCommKitTcp
    SimpleCommKitUtil
)
```

---

## 常见问题

### Q: 下载依赖失败怎么办？

第三方依赖从 GitHub 下载，如遇网络问题：

1. **使用代理**：
   ```bash
   cmake .. -DENABLE_PROXY=ON -DDEFAULT_HTTP_PROXY="http://your-proxy:port"
   ```

2. **手动下载**：手动下载对应版本的压缩包，放入 `3rdparty-download/` 目录，重新运行 cmake。

### Q: 如何清理构建？

```bash
rm -rf build/
```

重新执行 cmake 配置即可。注意这也会删除已下载的第三方依赖（位于 `build/3rdparty/` 下）。

### Q: HID 和 USB 的关系？

启用 HID 模块（`ENABLE_SIMPLECOMMKIT_HID=ON`）会自动启用 USB 模块（`ENABLE_SIMPLECOMMKIT_USB=ON`），因为 HID 是 USB 设备的一种。

### Q: 如何在 Debug 模式下构建？

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug
```

### Q: 为什么 MQTT 模块默认不启用？

MQTT 需要 libhv 构建时额外启用 MQTT 协议支持，默认不开启以减小网络库体积。启用后 libhv 会以 `WITH_MQTT=ON` 重新构建。

### Q: 支持交叉编译吗？

支持。通过设置 CMake 工具链文件 (`-DCMAKE_TOOLCHAIN_FILE=toolchain.cmake`) 即可实现交叉编译，如 Android NDK、嵌入式 Linux 等。

### Q: Android 构建时提示 "The C compiler is not able to compile a simple test program" 怎么办？

这通常是 MSBuild 接管了 Android 构建并使用 VS 自带 NDK 导致的。解决方案：

1. **必须使用 Ninja 生成器**：`cmake .. -G "Ninja" ...`
2. **清理 VS 环境变量**：
   ```powershell
   $env:ANDROID_HOME = ""
   $env:ANDROID_SDK_ROOT = ""
   $env:NDK_ROOT = ""
   ```
3. 清理旧的构建目录后重新配置

### Q: Python 绑定如何使用？

启用 `ENABLE_SIMPLECOMMKITPYBIND=ON` 后构建，会在输出目录生成 Python 扩展模块（如 `_SimpleCommKitPyBle.so`），可直接在 Python 中 `import` 使用：

```python
from SimpleCommKitPyBle import BleCentral
```

---

## 项目结构速览

```
SimpleCommKit/
├── CMakeLists.txt              # 根构建文件
├── VERSION                     # 版本号（唯一版本来源）
├── cmake/                      # CMake 辅助模块
│   ├── DownloadUnzipProject.cmake   # 通用下载/解压函数
│   ├── ExecuteDownloadProjects.cmake # 第三方依赖定义与下载
│   └── ios.toolchain.cmake      # iOS 交叉编译工具链
├── src/
│   ├── SimpleCommKitUtil/      # 工具模块（错误码 / 版本 / 导出宏）
│   ├── SimpleCommKitBle/       # BLE 蓝牙
│   ├── SimpleCommKitSerialPort/# 串口通信
│   ├── SimpleCommKitHid/       # HID 设备
│   ├── SimpleCommKitUsb/       # USB 通信
│   ├── SimpleCommKitTcp/       # TCP 客户端/服务器
│   ├── SimpleCommKitUdp/       # UDP 客户端/服务器
│   ├── SimpleCommKitWebSocket/ # WebSocket 客户端/服务器
│   ├── SimpleCommKitMqttClient/# MQTT 客户端
│   └── SimpleCommKitPy*/       # Python 绑定（可选）
├── examples/                   # 示例程序（11 个）
├── SimpleCommKitAi/            # AI/FastMCP 集成（可选）
├── doc/                        # 文档
└── LICENSE                     # MIT 许可证
```
