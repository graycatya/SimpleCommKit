# BLE 原生后端实施与能力矩阵

## 当前阶段

本次提交实现公共私有后端契约、调度与假后端测试，以及 **Linux BlueZ 实验路径**。
它不是全平台迁移完成的发布版本。默认仍使用原有 SimpleBLE；Windows 已提供扫描及连接/GATT 实验实现（见下文）；Apple、Android
原生后端未实现，不允许通过原生开关静默回退。最终移除 SimpleBLE 的任务保持未完成。

## 构建与验证

```sh
cmake -S . -B build/native-ble \
  -DENABLE_SIMPLECOMMKIT_BLE=ON -DSIMPLECOMMKIT_BLE_NATIVE=ON \
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
| macOS / iOS | 待固定 SDK、部署版本及 OBJCXX 工具链 | 原生未实现 | 未验证 |
| Android | 待固定 API/NDK、Java 打包及 Context 初始化入口 | 原生未实现 | 未验证 |

## 公共契约

- `SimpleCommKitBleCentral.h` 的类、结构体、签名保持不变；新增错误码从原 BLE 子码 15 开始。
- Linux 适配器 `dev_identifier` 使用对象路径，`dev_address` 使用系统地址。设备选择优先使用非空地址及地址类型；内部缓存按对象路径去重。
- 对未来不提供地址的平台，`identifier` 必须承载可往返的原生身份，address 为空且类型为 UNSPECIFIED。现有结构无法同时增加独立的展示名称，内部保留 name；这项展示差异需在该平台交付时说明。
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
断连与重连的全部竞态，以及 Windows 剩余故障/数据会话验收和 Apple/Android 平台的原生实现。
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

启用 `SIMPLECOMMKIT_BLE_NATIVE=ON` 时，Windows 使用 C++/WinRT 后端，不下载或链接 SimpleBLE、libdbus。
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
