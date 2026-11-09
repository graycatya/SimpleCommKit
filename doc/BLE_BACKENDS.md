# BLE 原生后端实施与能力矩阵

## iOS 更新（2026-11-09）

iOS 15+ 已复用 Apple CoreBluetooth 后端，并提供 UIKit 测试 App、签名构建入口和前台生命周期清理。
iPhone XR / iOS 18.7.9 两轮 IM948 公共接口测试通过，48 个完整传感器包及两次原参数恢复已核对。
构建、权限、前后台和剩余验收范围见 [BLE_IOS.md](BLE_IOS.md)。以下日期记录保留历史状态，以本节和平台文档为准。

## Android 更新（2026-11-07）

已新增 Kotlin/JNI 原生实验后端：扫描、BluetoothGatt 连接/服务发现、特征/描述符读写、
Notify/Indicate 订阅退订、MTU 和断连重连。CLS-AL00 / Android 12 复用原
Im948ReadProbe.hpp 完成两轮 IM948 传感器会话，原参数恢复验证通过。
实际 Write Request/Indicate 数据及故障/其它版本仍待验收；详见 [BLE_ANDROID.md](BLE_ANDROID.md)。
以下各阶段记录保留其当时的范围，本节更新其中“Android 尚未实施”的历史状态。

## Current backend selection

BLE uses only in-house Linux BlueZ D-Bus, Windows C++/WinRT, macOS/iOS CoreBluetooth and Android Kotlin/JNI backends. Enable `ENABLE_SIMPLECOMMKIT_BLE=ON`; no backend switch or SimpleBLE dependency is required. iOS 15+ integration and iPhone XR validation are documented in [BLE_IOS.md](BLE_IOS.md). Platform validation limits below still apply.

## 构建与验证

```sh
cmake -S . -B build/native-ble \
  -DENABLE_SIMPLECOMMKIT_BLE=ON \
  -DENABLE_SIMPLECOMMKIT_TCP=OFF -DENABLE_SIMPLECOMMKIT_UDP=OFF \
  -DENABLE_SIMPLECOMMKIT_WEBSOCKET=OFF -DSIMPLECOMMKIT_EXAMPLES=ON
cmake --build build/native-ble

# 独立测试不下载第三方源码；假后端测试不需要蓝牙 SDK。
cmake -S tests/ble -B build/ble-tests
cmake --build build/ble-tests
ctest --test-dir build/ble-tests --output-on-failure
```

根项目也支持 `-DSIMPLECOMMKIT_BLE_TESTS=ON`。Linux 上安装 libdbus 开发包、dbus-run-session、
python3-dbus 和 python3-gi 后，额外运行独立会话总线中的 BlueZ 模拟服务；该测试不操作系统蓝牙。
限制创建 Unix socket 的沙箱无法直接运行此项集成测试。

Python 扩展使用 `-DENABLE_SIMPLECOMMKITPYBIND=ON -DCMAKE_POSITION_INDEPENDENT_CODE=ON`。
动态库使用 `-DSIMPLECOMMKIT_BUILD_SHARED_LIBS=ON`。系统依赖通过 PRIVATE 链接，CMake 对静态库
消费者传递必要的链接依赖，不向公共头文件引入 D-Bus。

| 平台 | 构建下限/候选 | 当前证据 | 硬件验收 |
| --- | --- | --- | --- |
| Linux | C++17、CMake 3.21；libdbus >= 1.12；BlueZ 5.x 的 Adapter1/Device1/GATT API | GNU 15.2、CMake 4.2.3、libdbus 1.16.2 下静态/动态库、示例、Python 扩展编译；独立 D-Bus 模拟服务通过 | 未验证；最低 BlueZ 实机版本尚未固定 |
| Windows | Windows 10 2004 / SDK 19041+，MSVC C++17 | 扫描/连接/GATT 已实现，x64 静态库和示例构建通过 | UAF028 连接、发现、读取、订阅/退订、重连通过；写入/通知载荷/故障场景待验 |
| macOS | macOS 10.15+（arm64 11+），C++17 / Objective-C++ ARC；CoreBluetooth、Foundation | Apple Clang 21 / SDK 26.5，arm64 静态/动态库、C++ 示例与 Python 3.12 扩展通过；模拟 delegate 会话通过 | 原 C++ 示例完成 IM948-V3.05 两次连接会话、状态读取、Notify / Write Command、23 个传感器数据包及参数恢复；故障场景待验 |
| iOS | iOS 15+、C++17 / Objective-C++ ARC；系统 CoreBluetooth / Foundation | 真机 arm64 静态/动态库、签名 App；模拟器 arm64 App / x86_64 库编译通过 | iPhone XR / iOS 18.7.9 两轮 IM948、48 个传感器包、原参数恢复通过；故障与载荷验收边界见 BLE_IOS.md |
| Android | API 23+、NDK 25.1、Kotlin 2.3.10；宿主初始化及权限入口见 BLE_ANDROID.md | 两个 ABI 动态库、Kotlin AAR 和测试 APK 已接入 | Android 12 两轮 IM948 统一测试通过；其余版本/故障/生产交付待验 |

## 公共契约

- `SimpleCommKitBleCentral.h` 的类、结构体、签名保持不变；新增错误码从原 BLE 子码 15 开始。
- Linux 适配器 `dev_identifier` 使用对象路径，`dev_address` 使用系统地址。设备选择优先使用非空地址及地址类型；内部缓存按对象路径去重。
- 外设 `identifier` 显示设备名称。macOS 使用 `CBPeripheral.identifier.UUIDString` 作为 `address` 的平台虚拟地址，`address_type=UNSPECIFIED`；它不是 MAC。按虚拟地址选择和去重，同名及无名称设备仍可区分；iOS 后续接入沿用此约定。
- 同步平台操作默认 deadline 为 30 秒；取消以独立标志传播。用户回调串行、锁外执行，系统线程只传递值，不调用用户代码。
- 回调内允许查询、请求停止扫描及请求断连；可能阻塞的扫描、连接、GATT 和设备切换报告 ReentrantOperation。回调内停止/断连为排队请求。
- `adapter_Scan_For(ms)` 同步等待，等待期间不占用操作线程；负值无效，零开始后立即停止。停止幂等，缓存保留。
- 静态 `Bluetooth_Enabled()` 没有实例错误回调，失败返回 false；需诊断时使用实例方法与错误回调。
- 单个实例的操作线程与回调线程分别串行；调用方不得在销毁完成后再调用该对象。已进入的同步操作持有共享状态，销毁会取消它们；回调中销毁不会等待自己。
- 取消订阅删除对应回调并等待已进入的回调退出；排队的旧订阅事件有版本检查。所有异常在公共适配层映射一次错误；查询失败返回空/false/0。
- GATT UUID 支持 16/32/128 位输入并规范为小写长格式；重复服务/特征/描述符 UUID 不能唯一解析时报告 AmbiguousIdentity。

## Linux 公共方法能力

以下“已实现”表示有代码及所列自动化证据，**不表示真机验收完成**。

| 公共方法 | 实现与限制 | 失败行为/自动化 |
| --- | --- | --- |
| Bluetooth_Enabled | 任一适配器 Powered 为真 | 总线不可用返回 false；模拟验证 |
| get_Adapters / get_CurrentAdapter / set_CurrentAdapter | 枚举、快照、按原生键/地址选择 | 未选择/不存在/歧义明确失败；模拟验证 |
| adapter_Power_On / adapter_Power_Off / adapter_Is_Powered | Adapter1 Powered | 权限拒绝映射；状态查询模拟验证，电源切换真机待验 |
| adapter_Set_Callback_On_Power_On / On_Power_Off | 监视 Powered 改变 | 不主动申请授权；真机待验 |
| adapter_Set_Callback_On_Scan_Start / On_Scan_Stop | 单次扫描状态事件、幂等停止 | 假后端验证 |
| adapter_Set_Callback_On_Scan_Found / On_Scan_Updated | 按原生身份首次/更新分发，重复数据更新 | 假后端与 D-Bus 模拟验证 |
| adapter_Get_Scan_Results | 已扫描缓存，停止后仍可读取 | 假后端验证 |
| adapter_Get_Paired_Peripherals / Get_Connected_Peripherals | 过滤 Device1 Paired/Connected 并导入身份映射 | 无结果为空；总线异常报错；真机待验 |
| adapter_Scan_Start / Stop / For / Is_Active | LE discovery 会话，发现过滤 Transport=le | 负值无效，0 立即结束；假后端与模拟验证 |
| adapter_Initialized | 是否已成功选择适配器 | 无适配器 false；假后端验证 |
| set_CurrentPeripheral | 选择已发现/枚举的设备，停止扫描，取消旧代次 | 不存在/歧义明确失败；假后端与模拟验证 |
| peripheral_Get_Tx_Power | Device1 TxPower，属性可选 | 缺少属性 NotSupported；真机待验 |
| peripheral_Get_Mtu | 未提供统一可靠的连接 MTU 查询 | NotSupported，返回 0；模拟验证 |
| peripheral_Connect / Disconnect / Is_Connected | Device1 Connect/Disconnect；等待 ServicesResolved | deadline/取消；模拟连接验证，真机待验 |
| peripheral_Is_Connectable | Device1 无可靠通用只读连接能力字段 | NotSupported，false |
| peripheral_Is_Paired / Unpair | Device1 Paired / Adapter1 RemoveDevice | 权限映射；解除配对还会移除 BlueZ 设备对象；真机待验 |
| peripheral_Set_Callback_On_Connected / On_Disconnected | 监视原生连接状态 | 连接代次过滤；模拟验证 |
| peripheral_Services | 服务/特征/描述符及 Flags | 每次获取当前连接树，不保留断连服务缓存；模拟验证 |
| peripheral_Read（特征及描述符重载） | ReadValue | 原始字节不转文本；模拟验证 |
| peripheral_Write_Request / Write_Command | WriteValue type=request/command | 语义区分；模拟验证 |
| peripheral_Write（描述符） | GattDescriptor1 WriteValue | 原始字节；模拟验证 |
| peripheral_Notify / Indicate | StartNotify，验证对应 Flags | 同时支持 notify/indicate 时无法指定模式，明确 NotSupported；模拟 notify 验证，indicate 真机待验 |
| peripheral_Unsubscribe | StopNotify，撤销回调和排队旧事件 | 假后端与模拟验证 |
| set_Callback_Error | 异步串行错误回调，可在选择适配器前注册 | 旧值保持、新错误描述、重入和权限映射假后端验证 |

## 内部架构及尚未验收项

原生开关选择 `BleCentralNative.cpp`，不编译旧 PImpl。`IBleCentralBackend` 仅使用项目值类型；
工厂目前由 Linux 实现提供，假后端测试链接另一个工厂。`SerialExecutor` 将操作和用户事件分离，
`Generation` 取消旧操作并拒绝旧连接事件；执行器共享运行状态支持回调内销毁。

Linux 命令与监视各持有独立私有 D-Bus 连接。通知直接接收 PropertiesChanged 信号，重复值不去重；
扫描、对象移除及状态通过信号和对象树快照同步（监视循环空闲等待 50ms）。这会有额外 D-Bus 开销，
后续应通过真机压力测试确定是否改成完整增量对象缓存。

尚需验证/实现：真实权限拒绝、多适配器切换的真实状态同步、BlueZ 服务重启后的自动恢复、连接超时清理的真实系统行为、
断连与重连的全部竞态，以及 Windows 剩余故障/数据会话验收和 macOS 硬件验收及 iOS/Android 平台的原生实现。
因此任务 4.1–4.3 仍未整体勾选，不能声明完整 Linux 后端验收。Python 扩展的模拟扫描、读写、通知、取消订阅、常规销毁与回调内销毁已验证；AI 端到端和 Python 解释器退出的全部竞态仍需额外验证。


## 验证记录（2026-11-05）

- `build/native-ble`：Linux 原生静态库与原 C++ 示例编译通过。
- `build/native-ble-shared`：Linux 原生动态库与原 C++ 示例链接通过。
- `build/native-ble-python`：Python 3.14 扩展编译，CTest 的假后端、独立 BlueZ D-Bus 模拟、总线不可用错误路径及 Python 模拟消费者通过，四组测试各连续运行 10 次通过。
- `build/ble-isolated`：BLE/TCP/UDP/WebSocket 关闭且禁用 PkgConfig 查找时，最小库配置和构建通过；未调用第三方下载。
- `build/native-unsupported`：Generic 目标启用原生 BLE 时按预期配置失败，说明 Linux 以外后端待实现。
- `build/ble-asan`：AddressSanitizer 和 UndefinedBehaviorSanitizer 下的契约与 D-Bus 模拟测试通过（`ASAN_OPTIONS=detect_leaks=0`）。LeakSanitizer 在当前受跟踪执行环境报工具限制，不能声称完成泄漏检查。

上述 Linux 阶段未验收真实硬件会话、Windows/Apple/Android 构建及两类 AI 服务端到端运行；后续 Windows 3.1 证据见文末。


## 真机反馈后的回归修复（2026-11-05）

用户报告 AIPIN_008D 在选择后报 BackendUnavailable，随后连续报 peripheral not set。
日志本身不能证明系统实际移除了 Device1，但新增模拟场景（StopDiscovery 后从对象表移除设备）
可复现相同选择/连接失败链。修复前该回归测试失败，修复后通过。

选择现在保留扫描缓存中的逻辑设备身份；连接时若当前 Device1 不存在，在原操作 deadline 内
重新启动私有 LE 发现，并保持到连接/服务解析完成，再释放发现会话。设备没有重新出现则超时，
系统总线/服务真正不可用仍报告 BackendUnavailable。不会把这种自动恢复视作真机验证通过。

示例在无适配器、无扫描结果或连接失败时退出，避免后续 GATT 连续错误；不再把可选 MTU、
TX power 查询失败的 0 值打印成有效指标。新增 CTest 名称为 ble_bluez_transient_device_tests。


## Windows 3.1 实施记录（2026-11-05）

启用 `ENABLE_SIMPLECOMMKIT_BLE=ON` 时，Windows 使用 C++/WinRT 后端，不下载或链接 SimpleBLE、libdbus。
最低目标 Windows 10 2004（19041）；需 MSVC、Windows SDK 10.0.19041+ 的 C++/WinRT 头文件和 windowsapp.lib。
验证环境：MSVC 19.39.33522、SDK 10.0.22621.0、CMake 3.27.7、Ninja、x64 Debug。

- 枚举所有支持 LE 的 BluetoothAdapter；适配器标识使用系统 DeviceId，地址为大写冒号格式。
- 广播 Watcher 无指定适配器入口：仅允许选择系统默认 LE 适配器；其它适配器返回 NotSupported，multiple_adapters 为 false。
- 电源查询和状态事件已实现；电源开关返回 NotSupported，不主动申请授权或修改系统无线电状态。
- 主动扫描提供开始/停止、Found/Updated、RSSI、厂商原始数据；重复停止幂等，停止后保留缓存。
- 外设公共 identifier 显示广播 LocalName，与 Linux 的名称展示一致；本轮扫描中后续广播无名称时保留已发现名称，从未收到名称则为空。内部 id 为 `winrt:<地址类型>:<MAC>`，保留 Public/Random/Unspecified；按地址及地址类型选择设备，同名设备不会合并。随机地址轮换后视为新设备，匿名无地址广播不提供可选设备。
- 扫描 epoch 屏蔽旧 Watcher 的迟到事件，销毁撤销订阅；系统回调只投递值，公共执行器串行分发用户回调。
- E_ACCESSDENIED、ConsentRequired、DisabledByPolicy/DisabledByUser 映射 PermissionDenied；其它 HRESULT 和 Watcher 停止错误通过公共错误回调报告。扫描前 radio 非 On 返回 BackendUnavailable。
- 3.1 阶段连接/GATT 等操作尚未实现；后续 3.2 已补齐的能力及证据见下文。

打包应用须在 manifest 声明 `<DeviceCapability Name="bluetooth" />`；桌面程序仍受 Windows 隐私、策略和设备访问条件限制。应用负责 UI 授权流程，库不在后台主动弹出授权请求。
API 依据：[Watcher](https://learn.microsoft.com/en-us/uwp/api/windows.devices.bluetooth.advertisement.bluetoothleadvertisementwatcher)、[微软广播示例](https://github.com/microsoft/Windows-universal-samples/blob/main/Samples/BluetoothAdvertisement/cs/Scenario1_Watcher.xaml.cs)。

`build/native-win64` 静态库和原 C++ 示例编译链接通过，CTest 公共契约及 Windows 专项测试均通过。
专项测试覆盖身份格式、相同 MAC 不同地址类型、权限/超时/取消等错误映射、未选择适配器及重复销毁。
真机命令：`build/native-win64/tests/ble/ble_winrt_tests.exe --scan`，不会连接设备或切换蓝牙。
沙箱内枚举报 BackendUnavailable（系统找不到指定文件）；沙箱外枚举 1 个 LE 适配器且电源 On，两次三轮扫描分别发现 2/3/1 和 4/1/3 个设备。
第二次探针同时验证每轮开始/停止各一次、重复停止幂等以及停止后无迟到广播事件。
拒绝授权、蓝牙关闭及多适配器真机场景未执行，3.1 保留未整体验收；错误映射测试不替代真机验收。

## Windows 3.2 实施记录（2026-11-05）

- 连接使用扫描缓存中的地址及 Public/Random 类型解析 BluetoothLEDevice，建立 GattSession，并通过 Uncached 服务发现触发连接。成功后发送 Connected；失败、取消或超时释放本库的连接对象、服务和订阅。
- 断连释放 MaintainConnection、关闭会话、服务及设备，重复断连幂等。Windows 可能仍为其它应用保持物理连接；本库断连仅释放自身会话，不承诺强制断开其它进程的连接。
- 查询支持连接状态、配对状态、MTU；支持枚举已配对/已连接设备及显式解除配对。公共接口没有主动配对方法，应用通过 Windows UI 完成配对。电源切换、通用 connectable 和 TX power 查询仍为 NotSupported。
- 服务、特征、描述符发现采用 WinRT GATT API；支持 16/32/128 位 UUID 输入并规范化。相同 UUID 多实例无法唯一定位时返回 AmbiguousIdentity，缺失或非法 UUID 返回 InvalidArgument。
- 特征及描述符读取返回原始字节；特征 Write Request / Command 分别映射 WriteWithResponse / WriteWithoutResponse，描述符写入使用 WriteValueAsync。特征能力不支持对应操作时返回 NotSupported。
- Notify / Indicate 分别写入对应 CCCD 配置。取消订阅先撤销本地分发版本与事件令牌，再请求关闭远端 CCCD；公共层等待在途回调退出。远端关闭失败仍报告错误，本地不再分发该订阅。
- 连接 epoch、订阅版本及公共 generation 共同屏蔽旧事件。非预期断连失效 GATT/订阅状态；ServicesChanged 后丢弃旧服务缓存并在下次访问重新发现，旧订阅失效并报告 GattError，调用方需重新订阅。
- GATT AccessDenied 映射 PermissionDenied，Unreachable 映射 BackendUnavailable，ProtocolError 映射 GattError。每个异步等待遵守公共 operation deadline 和取消标记；Windows 不保证底层无线连接请求可立即取消，本库不等待其迟到结果，也不将迟到结果作为成功连接。

验证环境沿用 3.1。原目录 `build/native-win64` 静态库和交互式 C++ 示例已重新编译链接，公共契约和 Windows 专项 CTest 均通过。
专项测试增加 GATT 状态映射、短 UUID 规范化、缺失/非法/重复 UUID 拒绝、未选择外设及已过期操作检查。

真机命令：`build/native-win64/tests/ble/ble_winrt_tests.exe --connect 20:23:08:BA:F2:B9`。
针对用户选择的 UAF028，探针两次执行，每次两轮连接/断连重连成功；MTU 512、未配对。
发现服务 1800（1 个特征）和 ffb0（4 个特征）；标准设备名称 2a00 读取成功但返回空值，ffb2 的 CCCD 读取返回 2 字节，ffb5 的 CCCD 读取返回空值。
ffb2 Notify 和 ffb5 Indicate 的配置/取消成功，重复断连和取消后不再分发检查通过；测试期间通知载荷数量为 0，不能据此宣称已验证通知数据内容。

尚未真机验收：业务载荷写入（含 Request/Command 和描述符写入）、非空通知/指示数据、权限拒绝、设备离线/真实连接超时、连接中销毁/取消、服务变化及解除配对。
探针仅做标准属性/CCCD 读取和订阅配置，不自动写入未知业务数据或解除系统配对。因此任务 3.2 保持未整体验收。
API 依据：[微软 GATT 客户端示例](https://github.com/microsoft/Windows-universal-samples/blob/main/Samples/BluetoothLE/cppwinrt/Scenario2_Client.cpp)、[连接触发与取消限制](https://github.com/MicrosoftDocs/winrt-api/blob/docs/windows.devices.bluetooth/bluetoothledevice_frombluetoothaddressasync_117221987.md)。

## QR380A 定向读取诊断（2026-11-05）

目标为用户报告的 QR380A-17AC_Ble，PUBLIC 地址 66:33:CF:9A:17:AC。
项目后端在服务 1801 的特征 2a01 读取处复现断连；该设备枚举到 10 个服务，MTU 516。
为区分公共层取消和系统行为，另外使用不链接 SimpleCommKit 的独立 C++/WinRT 探针连接相同设备。
独立探针确认 1801/2a01 的 Uncached 和 Cached 读取都会出现 ConnectionStatus=Disconnected，随后返回 HRESULT 0x800704C7。
不读取时保持连接 10 秒，结束时仍为 Connected。同设备的 180a/2a29（Manufacturer Name）Uncached 读取成功，返回 3 字节且连接仍为 Connected。
因此当前证据将故障收窄到该设备的 2a01 访问路径；尚不能进一步区分设备固件、Windows 栈及底层无线交互原因。
不会自动把断连后的空结果视为成功，也不会用缓存读取、忽略错误或无限重连来掩盖故障。

定向项目后端探针用法：`ble_winrt_tests --read <MAC> <service UUID> <characteristic UUID>`。
例如：`build/native-win64/tests/ble/ble_winrt_tests.exe --read 66:33:CF:9A:17:AC 180a 2a29`。
连接探针改为单轮 15 秒扫描，以避免先前连续清空短扫描缓存导致漏掉较弱信号设备；普通 `--scan` 仍为三轮 2 秒。
独立原生探针及诊断输出保留在本地 build/native-win64/qr380-native-*，不作为公共模块源代码安装。
项目后端定向探针也通过：两轮连接/断连重连，每轮读取 180a/2a29 均返回 3 字节且保持连接。日志为 build/native-win64/qr380-backend-manufacturer.log。


## macOS CoreBluetooth 实施记录（2026-11-06）

`ENABLE_SIMPLECOMMKIT_BLE=ON` 在 Darwin 上选择 Objective-C++ ARC 后端，仅链接系统
CoreBluetooth / Foundation；不下载或链接 SimpleBLE。iOS 和 Mac Catalyst 尚未接入。
构建基线为 macOS 10.15+（Apple Silicon 为 11+）、C++17、CMake 3.21+、带对应 SDK 的 Xcode。
本机证据：macOS 26.6.2 arm64、Xcode 26.6、Apple Clang 21.0.0、macOS SDK 26.5、CMake 4.3.0。
最低系统仅是编译目标，尚未在旧版系统运行验收。

| 公共方法组 | 实现与边界 |
| --- | --- |
| Bluetooth_Enabled / 适配器枚举、选择、状态 | 使用系统默认逻辑适配器 `corebluetooth:default`；地址为空，不枚举物理控制器，不支持多适配器选择。初始化等候状态受 deadline / 取消约束；Unauthorized → PermissionDenied、Unsupported → NotSupported、PoweredOff → BackendUnavailable（电源状态查询为 false） |
| 电源开关 / 电源事件 | 开关 NotSupported；manager 状态变化映射 On/Off 事件，不主动显示蓝牙电源提醒 |
| Scan_Start / Stop / For / Is_Active / Results / Found / Updated | LE 扫描允许重复广播，按 UUID 去重；RSSI、厂商 ID 与二进制数据；每轮独立扫描 manager 和弱 delegate，停止时撤销，隔离旧扫描事件。停止幂等且保留结果 |
| 外设身份与选择 | 公共 identifier 显示广播 LocalName，缺失时使用系统名称；更新无名称时保留本轮已知名称，从未取得名称则为空。address 为 `CBPeripheral.identifier.UUIDString` 平台虚拟地址、类型 UNSPECIFIED；按 UUID 选择和去重，不用名称合并设备。连接在连接 manager 中按 UUID 重新获取句柄；UUID 不是 MAC，不承诺跨电脑或系统重置稳定 |
| Paired_Peripherals / Connected_Peripherals / Is_Paired / Unpair | NotSupported；CoreBluetooth 的已连接检索需要指定服务 UUID，现有无过滤参数接口无法完整枚举，不能将当前库会话冒充全系统连接列表 |
| Connect / Disconnect / Is_Connected / 连接事件 | 使用 CBCentralManager 连接、取消及 delegate；断连清空 GATT / 订阅。仅释放本库连接，不承诺断开其它应用。操作超时、取消或异步失败会撤销整个连接 delegate 域，隔离迟到事件；重试需重新连接 |
| Is_Connectable / Get_Tx_Power | 来自最近扫描广播；未提供字段时 NotSupported，不伪造数值 |
| Get_Mtu | NotSupported；maximumWriteValueLength 是写入长度上限，不能作为精确 ATT MTU |
| Services | 发现所有主服务、特征和描述符；UUID 规范化，保留广播中的服务数据和读、两类写入、Notify / Indicate 能力；断连缓存失效。服务变化报告 GattError 并撤销当前连接，调用方重新连接、发现和订阅 |
| Read（特征） | 原始二进制；没有 Read 能力或当前正在通知时 NotSupported，避免共用 delegate 回调将通知误认为读响应 |
| Read（描述符） | NSData 原始字节、NSString UTF-8；标准 2900 / 2902 / 2903 的 NSNumber 转为 16 位小端；无法无损转换的其它类型明确 NotSupported |
| Write_Request / Write_Command | 区分 WithResponse / WithoutResponse；检查特征能力和系统最大写入长度，超长 InvalidArgument。Command 等待系统可发送状态，受 deadline / 取消约束；无远端确认，不自动分包 |
| Write（描述符） | 系统描述符写入；2902 CCCD 直接写入 NotSupported，须用订阅接口 |
| Notify / Indicate / Unsubscribe | 检查对应属性；同时支持两种模式时 CoreBluetooth 不能显式选择，返回 NotSupported。系统确认订阅状态，取消时先撤销本地事件；公共层等待在途用户回调退出 |

```sh
cmake -S . -B build/native-macos \
  -DENABLE_SIMPLECOMMKIT_BLE=ON \
  -DENABLE_SIMPLECOMMKIT_TCP=OFF -DENABLE_SIMPLECOMMKIT_UDP=OFF \
  -DENABLE_SIMPLECOMMKIT_WEBSOCKET=OFF -DSIMPLECOMMKIT_EXAMPLES=ON \
  -DSIMPLECOMMKIT_BLE_TESTS=ON -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build/native-macos -j 4
ctest --test-dir build/native-macos --output-on-failure
# 显式真机探针：只扫描 5 秒，不连接、不写入、不解除配对。
build/native-macos/tests/ble/ble_corebluetooth_tests --scan
```

应用必须在自己的 Info.plist 提供 `NSBluetoothAlwaysUsageDescription`。
App Sandbox 应用还需 `com.apple.security.device.bluetooth` entitlement。
命令行示例和扫描探针已通过 Mach-O `__TEXT,__info_plist` 嵌入用途描述；库不会替宿主添加权限或修改
系统隐私设置。Python、AI 服务需由其实际宿主提供用途描述及权限；不能认为扩展模块中附一份 plist
就能授权 Python 解释器。首次实际调用会触发系统授权流程；`ShowPowerAlert=NO` 仅关闭电源提醒，
不会绕过或抑制系统授权。构造和销毁库对象本身延迟初始化，不访问蓝牙或触发权限。

验证范围：

- `build/native-macos`：静态库、原 C++ 示例、公共契约与 CoreBluetooth 专项模拟会话。
- `build/native-macos-shared`：动态库、原 C++ 示例及 Python 3.12 扩展编译链接；Python 导入及 20 次构造/销毁通过。
- `build/macos-ble-off`：关闭通信模块的最小库构建通过，不启用 Objective-C++ / CoreBluetooth 检测或 SimpleBLE 下载。
- 专项测试不构造真实 CBCentralManager，以 Objective-C 代用品驱动真实 delegate / 后端，验证扫描身份、去重、旧扫描隔离、二进制读写、描述符转换、订阅退订、歧义、超时、取消、操作中销毁及服务变化。
- `build/macos-ble-asan`：AddressSanitizer / UndefinedBehaviorSanitizer 下公共契约和专项模拟会话通过；`ASAN_OPTIONS=detect_leaks=0`，不宣称已完成泄漏检查。
- 真实扫描：沙箱内返回 NotSupported（65551），沙箱外返回 PermissionDenied（65553）；未取得成功扫描证据，不能判断真实设备发现和连接可用。需在系统隐私设置为实际运行宿主授予蓝牙权限后再次验证。

尚未验收：标准 GATT Read / Write Request / 描述符写入的真实载荷、蓝牙关闭/恢复、真实断连和超时、最低 OS / Intel Mac、
Python 实际蓝牙会话、两类 AI 消费者端到端、应用签名与打包及 Apple SDK 分发条款。
5.1–5.3 保持未整体验收；iOS 的编译、权限、生命周期和设备验收需要单独实施。

实现依据为本机 Apple SDK 的 `CBCentralManager.h`、`CBPeripheral.h`、`CBDescriptor.h` 与 `CBError.h`
中的官方 API 声明和注释（本次网络文档工具不可用，未声称完成在线文档核验）。


### 用户真机扫描反馈与显示约定调整（2026-11-06）

用户提供的 C++ 示例日志包含更新回调、停止扫描及 38 个设备结果，证明该用户运行环境的实际扫描路径
已成功；此证据不替代当前代理宿主的权限结果，也未覆盖连接 / GATT。
按用户要求，外设 Identifier 改为名称，Address 显示 Apple 平台 UUID 虚拟地址，类型保持 UNSPECIFIED。
不将 UUID 截断或哈希成六字节 MAC，避免地址碰撞和误认为真实硬件地址。同名设备通过 UUID 区分，
后续无名称广播保留已知名称；未获得名称的设备显示空 Identifier，仍可通过 UUID 地址选择。
iOS 后续实现采用相同字段约定，本次未启用 iOS 后端。

### IM948-V3.05 定向读取入口（2026-11-06）

原 C++ 示例新增可选参数 `--scan-seconds 1..300`、`--device-address <地址>`、`--im948-read` 和 `--im948-sensor-read`。
默认交互流程保留。IM948 模式在服务发现后确认 AE30/AE01 的 Write Command 和 AE30/AE02 的 Notify，
订阅接收后依次发送 `29`（保持连接）、`10`（状态）、三次 `11`（获取当前已订阅的功能数据），
打印原始回复、状态/电量、采样时间戳和功能标识，随后退订及断连。功能标识为零时明确提示无传感器字段；
不自动改订阅配置、校准、主动上报或持久设置。协议依据：[厂商说明书](https://www.yuque.com/cxqwork/lkw3sg/yqa3e0)。

```sh
build/native-macos/examples/SimpleCommKitBle/SimpleCommKitBleCentralExample/SimpleCommKitBleCentralExample \
  --scan-seconds 25 --device-address 529EE4D9-CF8D-9BBC-4BB4-FC7611A941AF --im948-read
```

本机一次 25 秒交互扫描已看到 `im948-V3.05`、上述 UUID、RSSI 约 -35 dBm。
后续自动选择/读取轮次尚未再次发现目标，未进入连接阶段，不能声称读取或通知已验证。
探测日志保存在 `build/native-macos/im948-read-session*.log`；后续成功结果需另外追加。


### IM948-V3.05 真机读取通过（2026-11-06）

在用户重启设备并恢复广播后，使用 **SimpleCommKitBleCentralExample**（公共 C++ 接口，原生 macOS 静态库）
完成两次实际连接/发现/通知会话，不再仅是内部后端探针：

- 目标 `im948-V3.05`，Apple UUID `529EE4D9-CF8D-9BBC-4BB4-FC7611A941AF`。
- 两轮分别发现 4 个服务，AE30 的 AE01 为 Write Command，AE02 为 Notify；还发现 AE3A、AE00 和 180F。
- 第一轮 `--im948-read`：AE01 发送 `29`，收到 `29` 回复；发送 `10` 收到 33 字节状态，电量 90%、3850 mV、传感器开启、功能标识 0。发送 `11` 并收到含时间戳的 7 字节数据。
- 第二轮 `--im948-sensor-read`：保存状态中的原参数，通过 `12` 仅把本次会话的功能标识设为 `0x0046`（含重力加速度、角速度、欧拉角），其他参数原样。
- 收到 **23 个完整的 25 字节传感器通知包**，包头 `11 46 00`，包含时间戳及 18 字节实际数据。
  示例加速度 `[0.0000, 0.0000, 9.7761] m/s²`，角速度 `[-0.6714, 0.3052, -0.4883] °/s`，
  初始欧拉角 `[0, 0, 0]°`，后续数据有变化。本轮电量状态 92%、3860 mV；读数不同时间取得，不作为精度校验。
- 结束前发送保存的原参数，并再次查询 `10`：原参数字节 1–10 完全相同，功能标识恢复为 0。
  退订、断连正常，两个进程返回 0，日志无 BLE ERROR。未执行校准、归零或保存出厂参数。

此模式仅在显式指定 `--im948-sensor-read` 时临时修改功能订阅，正常结束会恢复并读回核对。
异常退出或物理断连时无法保证在线恢复，不能把恢复失败报告为成功。
协议中的 `11` 单次回复与已开启的主动上报格式相同，无法仅凭通知内容区分来源；证据为已发送查询并收到实际数据，
不声称每一个 `11` 请求都可独立关联某一帧。

```sh
build/native-macos/examples/SimpleCommKitBle/SimpleCommKitBleCentralExample/SimpleCommKitBleCentralExample \
  --scan-seconds 25 --device-address 529EE4D9-CF8D-9BBC-4BB4-FC7611A941AF --im948-sensor-read
```

原始日志：`build/native-macos/im948-read-session-4.log`、`build/native-macos/im948-sensor-read.log`。
日志逐包检查确认所有 `0x0046` 帧为 25 字节且前后原参数一致；重新构建静态/动态示例、公共契约与 macOS 模拟测试通过。
尚未把本次 Notify + Write Command 传感器会话当作标准 Read / Write Request、Indicate、全部故障或压力场景的验收。
