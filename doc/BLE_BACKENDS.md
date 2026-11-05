# BLE 原生后端实施与能力矩阵

## 当前阶段

本次提交实现公共私有后端契约、调度与假后端测试，以及 **Linux BlueZ 实验路径**。
它不是全平台迁移完成的发布版本。默认仍使用原有 SimpleBLE；Windows、Apple、Android
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
| Windows | 待固定 Windows SDK/C++/WinRT 版本 | 原生未实现 | 未验证 |
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
断连与重连的全部竞态，以及其余四个系统平台的原生实现。
因此任务 4.1–4.3 仍未整体勾选，不能声明完整 Linux 后端验收。Python 扩展的模拟扫描、读写、通知、取消订阅、常规销毁与回调内销毁已验证；AI 端到端和 Python 解释器退出的全部竞态仍需额外验证。


## 验证记录（2026-11-05）

- `build/native-ble`：Linux 原生静态库与原 C++ 示例编译通过。
- `build/native-ble-shared`：Linux 原生动态库与原 C++ 示例链接通过。
- `build/native-ble-python`：Python 3.14 扩展编译，CTest 的假后端、独立 BlueZ D-Bus 模拟、总线不可用错误路径及 Python 模拟消费者通过，四组测试各连续运行 10 次通过。
- `build/ble-isolated`：BLE/TCP/UDP/WebSocket 关闭且禁用 PkgConfig 查找时，最小库配置和构建通过；未调用第三方下载。
- `build/native-unsupported`：Generic 目标启用原生 BLE 时按预期配置失败，说明 Linux 以外后端待实现。
- `build/ble-asan`：AddressSanitizer 和 UndefinedBehaviorSanitizer 下的契约与 D-Bus 模拟测试通过（`ASAN_OPTIONS=detect_leaks=0`）。LeakSanitizer 在当前受跟踪执行环境报工具限制，不能声称完成泄漏检查。

真实蓝牙硬件会话、Windows/Apple/Android 构建以及两类 AI 服务的端到端运行均未验收。


## 真机反馈后的回归修复（2026-11-05）

用户报告 AIPIN_008D 在选择后报 BackendUnavailable，随后连续报 peripheral not set。
日志本身不能证明系统实际移除了 Device1，但新增模拟场景（StopDiscovery 后从对象表移除设备）
可复现相同选择/连接失败链。修复前该回归测试失败，修复后通过。

选择现在保留扫描缓存中的逻辑设备身份；连接时若当前 Device1 不存在，在原操作 deadline 内
重新启动私有 LE 发现，并保持到连接/服务解析完成，再释放发现会话。设备没有重新出现则超时，
系统总线/服务真正不可用仍报告 BackendUnavailable。不会把这种自动恢复视作真机验证通过。

示例在无适配器、无扫描结果或连接失败时退出，避免后续 GATT 连续错误；不再把可选 MTU、
TX power 查询失败的 0 值打印成有效指标。新增 CTest 名称为 ble_bluez_transient_device_tests。
