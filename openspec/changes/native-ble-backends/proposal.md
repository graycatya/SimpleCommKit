# Proposal

## Why

当前 BLE 模块依赖 SimpleBLE 0.12.1，其许可限制不符合项目的开源依赖要求。改为自行实现系统原生 BLE 后端，保留调用方接口，并让依赖与分发条件可核验。

## What Changes

- 保留 `SimpleCommKitBleCentral.h` 中现有类、结构体、方法签名和回调形式，以内部统一后端替换 SimpleBLE 类型。
- 分别实现 Windows WinRT、Linux BlueZ D-Bus、macOS/iOS CoreBluetooth、Android BluetoothGatt 后端。
- 明确平台不支持、权限拒绝、超时、断连和销毁行为，通过现有错误回调传递错误，必要时增加错误码。
- 删除 SimpleBLE 下载、链接及构建补丁，更新 Python、AI、示例和构建文档中的依赖说明。
- 建立各平台能力矩阵和依赖许可证清单；分平台完成编译与硬件验收。

## Capabilities

### New Capabilities

- `ble-central`: 现有 BLE Central 接口的扫描、连接、GATT、错误与生命周期契约，以及平台差异行为。
- `ble-distribution`: BLE 的原生平台构建、可选模块隔离及依赖许可证交付要求。

### Modified Capabilities

无。仓库尚无已归档的 OpenSpec 规格。

## Impact

涉及 `src/SimpleCommKitBle`、`src/SimpleCommKitUtil` 错误映射、根与模块 CMake、`cmake/ExecuteDownloadProjects.cmake`，以及 BLE Python/AI 模块、示例和相关文档。公共接口保持源码兼容，不承诺现有二进制文件无需重新编译。新增平台内部源文件及 Android Java/JNI 集成；不引入 Rust、Qt 或替代跨平台 BLE 协议栈。
