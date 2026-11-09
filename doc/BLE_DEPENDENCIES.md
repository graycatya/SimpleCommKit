# BLE 后端依赖记录

## Android 扫描阶段更新（2026-11-07）

使用 NDK 25.1.8937393、API 23、compileSdk 35、JDK 21、Kotlin 2.3.10；
自有 Kotlin/JNI 桥接不链接 SimpleBLE。AAR 随附 libc++_shared.so、NDK NOTICE/
NOTICE.toolchain 和项目 LICENSE；Kotlin 标准库由宿主 Gradle 管理，避免重复类。
生产分发核验尚未完成；接入及 Android 12 真机扫描证据见 BLE_ANDROID.md。

## 范围

BLE uses only in-house Linux / Windows / macOS / iOS / Android backends. SimpleBLE is no longer downloaded, built or linked. Existing local source/zip caches are not automatically deleted. Platform validation limits still apply.

| 组件 | 类别与实际版本 | 来源/许可依据 | 授权选择、修改及分发 |
| --- | --- | --- | --- |
| libdbus-1 | 系统头文件与动态链接库；本地 pkg-config 1.16.2，配置接受 >=1.12 | [上游发布目录](https://dbus.freedesktop.org/releases/dbus/)；1.16.2 源包的 COPYING 与 LICENSES；本机 Debian copyright 原文副本见下 | 选择 AFL-2.1（双许可另一选项 GPL-2.0-or-later）；未修改。保留版权、许可、来源和相关 notices；若随产品重分发库，依 AFL 原文核对源码获取、归属与授权接受等义务，不仅附 SPDX |
| BlueZ | 系统服务，经 D-Bus 进程间调用；目标 BlueZ 5.x，最低实机版本待固定 | [BlueZ 上游](https://www.bluez.org/)、[接口文档](https://bluez.readthedocs.io/en/latest/) | 不编译、链接、打包 BlueZ 源码或守护进程；不将其系统服务许可证等同于 libdbus 许可证。若交付镜像含 BlueZ，须按镜像实际包另行核对并保留其源码和 notices |
| Linux SDK / libc / C++ 标准库 | 系统工具链；GNU C++ 15.2 本次编译 | 使用者工具链与发行版 | 不新增打包副本；静态发布时需要另行审核实际链接运行库及其例外条款 |
| dbus-daemon、python3-dbus、PyGObject/GLib | 可选测试工具，不是 BLE 库依赖 | 发行版包，仅用于独立模拟总线测试 | 不加入 BLE 发布产物；测试环境许可证与运行时库清单分开 |
| Windows SDK / C++/WinRT | 系统 SDK，最低 10.0.19041；本机 10.0.22621.0 | 使用安装的 C++/WinRT 头文件和 windowsapp.lib；未复制或分发 SDK，完整 SDK 条款及发布清单待 3.3 核验 | 3.1 x64 静态库/示例已构建，发布许可核验未完成 |
| CoreBluetooth / Foundation | macOS / iOS 系统 frameworks；本机 Xcode 26.6、SDK 26.5、Apple Clang 21；最低 macOS 10.15（arm64 11+）/ iOS 15 | 安装的 Apple SDK 官方头文件与系统动态 frameworks；未复制或修改 SDK / framework | 已接入本机静态/动态消费者构建；发布物不捆绑系统 frameworks。SDK 许可原文、签名/打包与发布清单仍待核验，不据此宣称分发许可已完成；iOS 静态/动态库、开发签名 App 及 iPhone XR 测试通过，生产发布许可核验仍未完成 |
| Android SDK/NDK 与 Kotlin/JNI 组件 | API 23+、NDK 25.1、Kotlin 2.3.10；AAR 集成和 Android 12 真机通过 | 自有桥接与系统 SDK，随附运行库/NOTICE 范围见 BLE_ANDROID.md；完整发布条款待核验 | 已接入；生产分发核验未完成 |

`doc/licenses/dbus-1.16.2-debian-copyright` 是从本机
`/usr/share/doc/libdbus-1-dev/copyright` 原样保留的发行版版权/许可文件，包含 AFL-2.1 原文、
BSD/MIT 类附加条款和归属信息。它证明本次本地依赖的元数据，不取代其它发行版或版本的核验。
源代码位置：https://dbus.freedesktop.org/releases/dbus/dbus-1.16.2.tar.xz 。

当前链接系统动态 libdbus，不把它复制进安装包。原生 BLE 安装时附带本记录和版权原文副本。
未来若嵌入或静态链接 libdbus，必须重新检查实际构建文件及所有传递依赖，保持获取源码和 notices
的交付路径。其余平台依赖没有完成核验，不能据此发布全平台许可合格声明。
