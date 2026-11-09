# iOS CoreBluetooth 接入与验收

## 当前能力（2026-11-09）

iOS 与 macOS 复用 `CoreBluetoothBackend.mm`，公共 C++ 接口不变。最低部署目标 iOS 15.0，
支持真机 arm64、模拟器 arm64 / x86_64；最低系统运行情况仍需额外验收。
仅链接系统 CoreBluetooth / Foundation。UIKit / CoreGraphics 仅测试 App 使用，不成为 BLE 库依赖。
不依赖 SimpleBLE、Qt、Java/JNI 或第三方蓝牙框架。Mac Catalyst 尚未验收。

设备 `identifier` 显示名称，`address` 是当前 Apple 平台分配的 UUID 虚拟地址，类型 UNSPECIFIED。
**同一设备在 iPhone 和 Mac 上的 UUID 可以不同**，应使用当前手机扫描结果，不能复制 Mac 地址来选设备。
名称不是唯一标识；同名设备在列表中按 UUID 分开。

扫描、连接、发现、特征/描述符读写、Notify / Indicate 及取消订阅复用 Apple 后端。
权限、MTU、配对、电源开关、Notify / Indicate 双能力限制与 macOS 一致，详见 [能力矩阵](BLE_BACKENDS.md)。
初始化及同步操作必须放在工作线程，不能阻塞 UIKit 主线程或 CoreBluetooth delegate 队列。

## 构建

需要 macOS、完整 Xcode 与 iOS SDK。本机使用 Xcode 26.6、SDK 26.5、Apple Clang 21、CMake 4.3.0。

```sh
# Apple Silicon 模拟器 App（无开发证书要求）
python3 scripts/build_ios_ble.py --platform simulator

# 真机 arm64 App（先构建未签名产物）
python3 scripts/build_ios_ble.py --platform device

# 使用本机已有开发团队签名；替换为自己的 Team ID
python3 scripts/build_ios_ble.py --platform device --team YOUR_TEAM_ID

# 只构建 BLE 动态库；测试 App 默认链接静态库
python3 scripts/build_ios_ble.py --platform device --library-only --shared \
  --build-dir build/ios-ble-shared

# Intel 模拟器库
python3 scripts/build_ios_ble.py --platform simulator --arch x86_64 --library-only \
  --build-dir build/ios-ble-simulator-x64
```

脚本使用 CMake 原生 iOS 平台和 Xcode 生成器。`--dry-run` 打印命令；`--config Release`
构建发布配置。`--profile` 仅适用于自己已有的**手动管理** provisioning profile，须同时提供 `--team`；
Xcode 管理的通配 profile 应使用自动签名，不要通过 `--profile` 强制手动签名。
脚本不创建开发者账号、证书或 profile，也不自动联网申请 provisioning。

等价的关键 CMake 参数：

```sh
cmake -S . -B build/ios-ble-device -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
  -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO \
  -DENABLE_SIMPLECOMMKIT_BLE=ON -DSIMPLECOMMKIT_IOS_BLE_PROBE=ON \
  -DENABLE_SIMPLECOMMKIT_TCP=OFF -DENABLE_SIMPLECOMMKIT_UDP=OFF \
  -DENABLE_SIMPLECOMMKIT_WEBSOCKET=OFF
cmake --build build/ios-ble-device --config Debug --target SimpleCommKitBleIosExample
```

`SIMPLECOMMKIT_IOS_BLE_PROBE` 只适用于启用 BLE 的 iOS 构建。普通 iOS 构建开启
`SIMPLECOMMKIT_EXAMPLES` 时，BLE 示例也会构建此 UIKit App，避免生成无法交互使用的控制台示例。
现有 `cmake/ios.toolchain.cmake` 的 BLE 开关仍可显式启用；本次验收使用上面的原生 CMake 参数。

App 路径：

```
build/ios-ble-device/examples/SimpleCommKitBle/SimpleCommKitBleIosExample/Debug-iphoneos/SimpleCommKitBleIosExample.app
build/ios-ble-simulator/examples/SimpleCommKitBle/SimpleCommKitBleIosExample/Debug-iphonesimulator/SimpleCommKitBleIosExample.app
```

## 真机使用

1. 连接并解锁 iPhone，在系统中开启开发者模式，使用覆盖该设备的开发签名构建。
2. 安装 App。可以在 Xcode 中运行生成的工程，或执行：

   ```sh
   xcrun devicectl list devices
   xcrun devicectl device install app --device DEVICE_ID \
     build/ios-ble-device/examples/SimpleCommKitBle/SimpleCommKitBleIosExample/Debug-iphoneos/SimpleCommKitBleIosExample.app
   xcrun devicectl device process launch --device DEVICE_ID com.catgray.SimpleCommKit.BleProbe
   ```

3. 在手机打开“BLE 测试”，点“扫描 25 秒”，允许蓝牙访问。
4. 在列表中选择 IM948，再点“运行 IM948 两轮测试”。设备不能同时被其它客户端占用。
5. 测试复用 `Im948ReadProbe.hpp::testIm948()`：两轮连接、AE10 Read、AE05 Indicate 配置/取消、
   AE01 Write Command / AE02 Notify、临时传感器订阅、原参数恢复、退订及重连。
6. 看到 `[IOS IM948 PASS]` 和两次 `[IM948 RESTORE PASS]` 才表示完整会话通过。
   AE05 的 payloads=0 只证明配置成功，不证明实际 Indicate 载荷传递。

可通过启动参数 `--scan-only` 自动扫描；`--im948-test` 在扫描结果中只有一个名称包含 im948 的设备时
自动运行测试，同名多设备时不会自动选择。App 不会在普通启动时初始化蓝牙或主动弹出授权。

日志显示在 App 内，同时写入它自己的 `Documents/BleProbe.log`，每次进程启动重新建立日志。

```sh
xcrun devicectl device copy from --device DEVICE_ID \
  --domain-type appDataContainer --domain-identifier com.catgray.SimpleCommKit.BleProbe \
  --source Documents/BleProbe.log --destination build/ios-ble-device/iphone-BleProbe.log
```

## 权限与前后台边界

宿主 App 必须在自己的 Info.plist 添加 `NSBluetoothAlwaysUsageDescription`；本测试 App 已提供。
本阶段不声明持续后台蓝牙模式，也不提供状态恢复或后台采集承诺。
官方依据：[Core Bluetooth](https://developer.apple.com/documentation/corebluetooth)、
[用途描述](https://developer.apple.com/documentation/bundleresources/information-property-list/nsbluetoothalwaysusagedescription)、
[前后台行为](https://developer.apple.com/library/archive/documentation/NetworkingInternetWeb/Conceptual/CoreBluetooth_concepts/CoreBluetoothBackgroundProcessingForIOSApps/PerformingTasksWhileYourAppIsInTheBackground.html)。

点停止或进入后台时，扫描请求停止；正在进行的测试先完成当前有限时长的数据会话和参数恢复，
再跳过后续轮次。App 使用一次有限的系统 background task 仅争取清理时间；时间耗尽时取消连接并报告
可能未完成恢复。物理断连、强制退出和系统终止无法保证在线恢复，不能当作成功测试。
后台中断/超时恢复的真机覆盖仍未完成。

## 实际验收记录

- 真机 arm64：BLE/Util 静态库、UIKit App 构建、现有开发签名和安装通过；独立动态库构建通过。
- 模拟器 arm64：App 构建、iOS 18.6 启动、界面检查通过；扫描返回 NotSupported，UI 显示失败而不伪造设备。
  x86_64 模拟器库交叉编译通过，不声称在 Intel 主机运行通过。
- iPhone XR / iOS 18.7.9：用户允许权限并实际操作 App，IM948-V3.05 两轮统一测试通过。
- iPhone 本地 UUID 为 `C3B1EFD0-A05E-1DF2-0542-8F2932E748FB`，与 Mac 上 UUID 不同。
- AE10 每轮 Read 返回 4 字节；AE05 每轮订阅/退订通过，实际 indication 载荷数为 0。
- 共 **48 个完整的 25 字节传感器包**（`11 46 00`）。样本加速度
  `[-0.8948, -0.4833, 9.6995] m/s²`，角速度 `[0,0,0] °/s`，
  欧拉角 `[-0.7141,1.7688,-0.0275]°`；非精度或校准验收。
- 两轮分别等待参数设置 ACK 再读回；前后原参数字节完全一致，订阅标识恢复为 0，正常退订/断连。
- 首次手机授权尚未完成时出现 Timeout；随后用户允许权限、重新扫描后会话通过。未把首次超时当作权限拒绝验收。
- 本机 macOS/公共契约/Android 扫描模拟回归 3 项通过。

原始证据（本机构建目录，不提交包含邻近设备的扫描日志）：
`build/ios-ble-device/iphone-console.log`、`iphone-BleProbe.log`、`iphone-im948-session.log`。

尚未验收：实际 Write Request / Indicate 载荷、描述符写入、真实权限拒绝/撤销、蓝牙关闭/恢复、
连接超时/离线、测试期间前后台切换和强制销毁、长时间/压力、iOS 15 实机、App Store/生产分发及 SDK 条款清单。
测试通过不等同于上述全项或全平台迁移验收完成。
