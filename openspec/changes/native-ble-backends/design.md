# Design

## Context

动机见 proposal.md。现有公共入口是 `SimpleCommKitBleCentral.h`，使用 C++17、PImpl、同步读写和 `std::function` 回调；SimpleBLE 类型存在于私有实现以及公共方法的实现文件中。公共设备结构包含 address，但 Apple 平台不能按 MAC 地址建立统一身份模型。现有错误码数量不足以区分所有平台失败。

## Goals / Non-Goals

**Goals:** 公共接口源码兼容；系统 API 类型不泄漏至头文件；每个平台能独立构建、测试和记录能力限制；取消和销毁不会留下悬空回调。

**Non-Goals:** 本次设计不增加 Peripheral/GATT Server 或经典蓝牙，不实现 HCI 主机协议栈，不承诺跨平台 MAC、配对和电源管理完全一致，不引入 Rust 或 Qt。本文描述后续实现，当前变更仅交付规格与任务。

## Decisions

### 1. 私有统一后端与工厂

```text
SimpleCommKitBle 公共接口
          ↓
SimpleCommKitBleCentralPrivate（兼容适配、错误映射）
          ↓
内部 IBleCentralBackend / 平台工厂
          ↓
Windows       Linux          macOS/iOS        Android
WinRT         BlueZ D-Bus    CoreBluetooth    BluetoothGatt
```

计划在 `src/SimpleCommKitBle/src/backends/` 下设置公共私有契约、工厂及 windows/linux/apple/android 目录。后端只使用项目私有值类型、设备句柄和统一事件，不出现 SimpleBLE 类型。运行时内部能力表记录电源控制、配对查询、解除配对、MTU、TX power、适配器选择等能力。工厂在编译时选择平台，不向公共接口新增后端参数。相比在每个公共方法中堆叠平台宏，此方式便于独立验证；相比 btleplug，避免额外语言工具链。

### 2. 平台实现与依赖边界

| 平台 | 实现 | 构建与依赖策略 |
|---|---|---|
| Windows | C++/WinRT 的设备枚举、广播、GATT 与 Radio API | Windows SDK；仅链接实现需要的系统库，权限不足明确失败 |
| Linux | BlueZ 的 ObjectManager、Adapter1、Device1、GattService1、GattCharacteristic1、GattDescriptor1 | 采用 libdbus-1 访问系统总线，由系统 BlueZ 服务处理蓝牙；构建时检测开发包及其实际许可证 |
| macOS/iOS | Objective-C++ 封装 CBCentralManager、CBPeripheral | 仅该平台启用 OBJCXX，链接 CoreBluetooth/Foundation，记录隐私描述和应用生命周期要求 |
| Android | Java BluetoothLeScanner、BluetoothGatt、JNI 桥接 | Java 组件随应用打包，使用应用 Context；运行时权限由应用取得，库检测并报告缺失 |

Android 增加独立的初始化/Context 传递集成入口，不修改现有 Central 方法签名。最低 OS、SDK、BlueZ 和编译器版本由各后端第一项构建验证固定并写入能力矩阵，版本调整不能改变规格定义的行为。

### 3. 异步实现与同步兼容

各后端拥有串行操作上下文；Android 每个连接的 GATT 请求排队。同步方法通过带 deadline 的完成对象等待，不在系统事件线程等待。连接、发现、读写、订阅默认上限 30 秒，作为内部可集中调整的常量；扫描时间由既有 timeout_ms 决定，负值报告无效参数，0 表示立即结束。

用户回调通过库拥有的事件队列分发，在锁外调用；回调中可读状态、停止扫描或请求断连。会等待当前事件上下文的重入操作报告错误而不阻塞。销毁执行取消、解绑系统监听、清空用户回调并释放任务；通过共享状态与弱引用防止访问已释放对象。回调内销毁不能等待自己，当前回调可退出但不得再启动其他回调。

### 4. 身份、状态和错误

状态至少覆盖未选择适配器、就绪、扫描、连接中、已连接和关闭中；使用连接代次丢弃旧设备的迟到事件。适配器/设备选择采用原生稳定身份与内部映射，address 不作为所有平台的主键；Apple address 留空，保留 address_type=UNSPECIFIED。展示名称与身份分离，不把设备名称当唯一标识。

保留现有错误码值，新增不支持、权限拒绝、超时、无效参数、取消、重入及 GATT 失败等错误并补齐描述。失败调用一次错误回调，查询返回现有类型的空/false/0，void 方法只报告错误；空结果的含义由错误回调区分。不支持电源、配对或硬件指标时不得伪造成功或数值。

服务 UUID 统一大小写及标准长格式，保留通知/指示和两种写入语义。重复服务或特征 UUID 在公共参数不足以唯一定位时返回明确的歧义错误，不任意选取。缓存随断连失效。

### 5. 构建与许可核验

使用现有 BLE 开关控制依赖检测，平台库采用 PRIVATE 链接并正确处理静态消费者的传递链接要求。BLE 关闭时不要求蓝牙 SDK、D-Bus 或 Java 组件；不支持的平台启用 BLE 时配置失败并说明原因。后续最终版本彻底删除 SimpleBLE 下载与补丁，不保留静默回退。

交付 `doc/BLE_BACKENDS.md` 能力/构建矩阵与 `doc/BLE_DEPENDENCIES.md` 清单。清单逐项记录版本、来源、SPDX（如适用）、官方许可原文、修改及分发要求，区分系统服务、SDK、头文件、链接库和打包组件。libdbus 的具体授权选择在引入时依据安装版本原文核验；发现非商业或用途限制依赖时停止引入并更换依赖。保留所需 notices，不把使用原生 API 等同于自动完成许可核验。

## Risks / Trade-offs

- 多平台维护成本 → Windows、Linux、Apple、Android 分阶段交付，每阶段具有自身测试与文档。
- 公共接口无法表达所有原生能力 → 明确错误和平台矩阵，保持接口而不制造能力一致的假象。
- 系统权限和后台限制 → 真机覆盖拒绝授权、蓝牙关闭和应用恢复，不自动弹出未约定的权限流程。
- 同步等待与异步事件产生死锁 → 独立分发、有限超时、重入检测与销毁竞态测试。
- 宏观源码兼容不能证明行为兼容 → 现有示例和 Python/AI 绑定构建，加公共接口回归测试。

## Migration Plan

先固定公共接口编译基线与能力矩阵，再实现统一后端和可注入假后端。依次完成 Windows、Linux、Apple、Android，每个平台编译及硬件记录均通过后，再移除最终的 SimpleBLE 构建路径并验证全部消费者。开发中的未完成平台不得发布为可用后端；BLE 关闭构建始终可用。回退采用版本控制恢复原发布版本，并保留原版本许可说明；不在新版本自动回退至 SimpleBLE。
