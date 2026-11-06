# Android 原生 BLE（2026-11-07）

当前已实现 Kotlin/JNI 初始化、扫描、BluetoothGatt 连接、服务发现、特征/描述符读写、
Notify/Indicate 订阅退订、MTU 协商和断连重连。Android 12 IM948 真机统一测试通过。
后台运行、权限/电源故障、其它 Android 版本及生产分发仍需验收。

## 构建与打包

最低 Android 6.0 / API 23；Kotlin 2.3.10 编译为 JVM 1.8 字节码，compileSdk 35。
本机验证 NDK 25.1.8937393、CMake 3.27.7、JDK 21，arm64-v8a 和 x86_64
Debug 动态库。原生路径不下载/链接 SimpleBLE，也不要求 Qt 或第三方 Java 依赖。

```powershell
python scripts/build_android_ble.py --sdk D:/androidsdk --ndk D:/androidsdk/ndk/25.1.8937393 --java-home "D:/app/android studio/jbr" --kotlin-home "D:/app/android studio/plugins/Kotlin/kotlinc"
```

输出 `build/android-ble-aar/SimpleCommKitBle-debug.aar`，包含 Kotlin 编译的 classes、
manifest、混淆保留规则、两个 ABI 的 BLE/Util 共享库及 libc++_shared.so。
可重复传入 `--abi arm64-v8a` 等参数选择 ABI。生产打包、16 KB 页兼容性和
商店分发要求尚未验收，本次 Debug AAR 供集成测试。

宿主 App 把 AAR 放入 libs 并添加 `implementation(files("libs/SimpleCommKitBle-debug.aar"))`。
如果已有其它 NDK 模块，应使用相同 NDK 与 `c++_shared`，统一运行时版本。
原生宿主仍需配置本项目头文件和 native 链接；业务使用现有 C++ 公共接口。
采用静态库链接到宿主 `.so` 时，须保留 AndroidBackend 对象及 JNI 导出符号，
并加载该宿主库；默认示例使用 AAR 中的动态库。

## 初始化与权限

加载库后，在 Application 或 Activity 初始化一次：

```kotlin
System.loadLibrary("SimpleCommKitUtil")
System.loadLibrary("SimpleCommKitBle")
org.simplecommkit.ble.NativeBle.initialize(applicationContext)
```

仅持有 Application Context，不保存 Activity；不要实现另一个 `JNI_OnLoad`
来替代此初始化入口。未初始化调用 C++ API 会报告 BackendUnavailable，不会隐式弹窗。
JNI 类保留进程生命周期，每个后端的 Java 实例引用在 shutdown 时释放。

宿主负责在 UI 线程请求并确认运行时权限，然后在工作线程调用同步 C++ 接口：

- Android 12+：BLUETOOTH_SCAN、BLUETOOTH_CONNECT。
- Android 6–11：ACCESS_FINE_LOCATION；系统定位开关等条件也可能影响扫描。
- 示例 manifest 的 `neverForLocation` 表示不使用扫描推导物理位置，可能过滤部分信标。
  若 App 的实际用途不符合，应自行调整 manifest 和定位权限。
- 不包含后台定位/后台服务方案；本阶段仅验证前台集成路径。

权限依据：[Android 蓝牙权限](https://developer.android.com/develop/connectivity/bluetooth/bt-permissions)。
扫描依据：[扫描 BLE 设备](https://developer.android.com/develop/connectivity/bluetooth/ble/find-ble-devices)。
线程附加及引用管理依据：[JNI tips](https://developer.android.com/ndk/guides/jni-tips)。

## C++ 行为和生命周期

`get_Adapters()` 返回默认逻辑适配器，名称 Android Bluetooth，本机 MAC 留空。
选中适配器后使用原公共 API 启停扫描；宿主应限制扫描时间，并在离开前台时停止。
设备按 Android 返回地址去重，名称取广播 LocalName；后续无名称广播保留本轮已知名称。
不使用隐藏 API 猜测地址类型，统一 UNSPECIFIED；随机地址轮换可能产生新设备。
RSSI 和厂商 ID/二进制数据原样传递。

每轮扫描有独立 epoch，每个后端使用注册表 handle（不向 Java 暴露裸 this）。
停止、启动失败或 shutdown 后，旧扫描事件被过滤；重复停止和 shutdown 幂等。
回调由既有公共层事件线程派发。权限缺失映射 PermissionDenied，禁用/不可用映射
BackendUnavailable，扫描功能不支持映射 NotSupported。蓝牙电源变化的实时事件监听
尚未实现；当前 enabled/powered 是按次查询。

## 第一阶段历史验证记录

- arm64-v8a、x86_64 共享库及 Java/AAR 构建通过。
- 主机扫描状态测试通过：按地址去重、同名设备分离、名称保留、二进制厂商数据、
  新轮扫描清空、停止/旧 epoch/销毁后的事件过滤。
- 原公共契约及 Windows 专项回归通过，共 3 项 CTest。
- ADB 没有连接设备，尚未验证 APK 启动、ART/JNI 实际调用、授权拒绝、权限撤销、
  蓝牙切换、真机扫描及反复销毁。构建和主机模拟测试不替代这些验收。
- 下一阶段实现 BluetoothGatt 串行异步操作、连接代次、deadline/取消、服务发现、
  Read、两类 Write、Notify/Indicate/CCCD 和断连资源释放，再接入 IM948 统一测试。

6.1–6.3 主任务保持未完成：连接/GATT 和剩余硬件故障场景仍待实施及验收。

## Kotlin 接入与真机验证（2026-11-07）

桥接已改为 `android/kotlin/org/simplecommkit/ble/NativeBle.kt`，移除原 Java 源码。
构建通过 --kotlin-home 使用 Kotlin 2.3.10，并用 javap 检查 JNI 方法签名。
AAR 不内嵌 kotlin-stdlib，由宿主 Kotlin Gradle 插件管理；若禁用默认依赖，添加
`implementation(kotlin("stdlib"))`。本次使用 Kotlin 2.3.10。

真机 CLS-AL00、Android 12 / API 31、arm64-v8a，经无线 ADB 安装测试 APK：

- 未初始化调用正确报告 BackendUnavailable；初始化后枚举到一个适配器。
- 两轮各 4 秒扫描发现 5、6 个设备，累计回调 13、26 次。
- 第一轮发现 CC:A7:75:0D:58:F0（im948-V3.05），RSSI -51，含厂商数据。
- 重复停止、停止后无回调及 20 次创建/枚举/销毁通过。
- 修复首次选择适配器前 scanning() 错误要求已选适配器的问题。

以上实测更新前文“无连接设备”的历史状态；权限拒绝/撤销、蓝牙切换及连接/GATT 未验收。
日志：`build/android-ble-probe/phone-scan-result.log`。

```powershell
python tests/android/ble-probe/build_probe.py --sdk D:/androidsdk --ndk D:/androidsdk/ndk/25.1.8937393 --java-home "D:/app/android studio/jbr" --kotlin-home "D:/app/android studio/plugins/Kotlin/kotlinc"
```

测试构建要求 Build Tools 37.0.0，APK 输出 `build/android-ble-probe/SimpleCommKitBleProbe.apk`。
使用 `adb install -r -g` 授予测试权限，启动 `org.simplecommkit.ble.probe/.MainActivity`。
结果显示在屏幕并保存在 App 私有目录 files/result.txt。仅扫描，不连接或写入设备。

## IM948 统一测试及 GATT 更新（2026-11-07）

测试 APK 直接包含原 `Im948ReadProbe.hpp`，调用 `testIm948()`，没有复制设备协议实现。
不带参数仍执行前台扫描；显式传入 device_address 启用完整 IM948 会话：

```powershell
adb shell am force-stop org.simplecommkit.ble.probe
adb shell am start -n org.simplecommkit.ble.probe/.MainActivity --es device_address CC:A7:75:0D:58:F0
adb shell run-as org.simplecommkit.ble.probe cat files/result.txt
```

该模式扫描 15 秒后选择目标，执行两轮 AE10 Read、AE05 Indicate 配置/退订、AE01 Write
Command、AE02 Notify 传感器收发、原参数恢复核对及断连重连。临时功能位为 0x0046，
不执行校准、归零或持久化设置；保持在线才能恢复参数，不应在运行中强制停止 App。

CLS-AL00 / Android 12 实测 MTU 247、5 个服务，AE10 两轮均返回 4 字节，收到完整
25 字节传感器包并解析加速度/角速度/欧拉角；两轮 RESTORE PASS、IM948 TEST PASS。
Indicate 已验证 CCCD 配置和退订，实际指示载荷为 0，未据此宣称数据传输通过。
AE10 Write Request 未确认设备协议，沿用统一测试的显式 SKIP；不以命令写入代替验证。

后端由 C++ executor 串行发起 GATT 请求，Kotlin 校验能力和 UUID 唯一性，并等待对应
系统回调。API 33+ 使用带独立值的读/通知回调与新版写入接口，旧系统使用旧接口。
连接时请求 MTU 247，写入不自动分包，超出 MTU-3 报 InvalidArgument。
Notify/Indicate 通过系统注册加 CCCD 写入确认；直接写 CCCD 返回 NotSupported。

每个连接有 token/generation；旧 GATT 回调、退订后的通知、已关闭对象事件均隔离。
等待定期检查公共 Operation 的取消与 deadline；失败后关闭连接域，避免迟到回调被
下一请求认作成功，需重新连接。shutdown 取消等待并释放 GATT，不强制关闭系统蓝牙。
服务变化会关闭连接并报告 GattError；电源控制、解除配对、配对/连接设备系统枚举、
TX power 和通用 connectable 查询仍 NotSupported。

测试 APK：`build/android-ble-probe/SimpleCommKitBleProbe.apk`。
首轮原始日志：`build/android-ble-probe/im948-session-initial.log`。
最终复测日志：`build/android-ble-probe/im948-session-final.log`。
主机测试补充连接代次、退订/旧通知、服务变化断连及二进制数据检查。
尚待验证实际 Write Request/描述符业务写入、Indicate 数据、蓝牙/权限故障、取消/超时真机
竞态、后台/长时间稳定性、Android 23/33+ 实机及其它机型。
