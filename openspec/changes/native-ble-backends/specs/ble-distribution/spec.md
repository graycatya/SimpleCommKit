# Spec Delta

## Purpose

定义 BLE 原生平台构建与分发约束，使使用者可以独立启用或关闭蓝牙支持，并追溯每项依赖的版本、来源、许可证及实际验证范围，避免隐式引入受限依赖。

## ADDED Requirements

### Requirement: 平台构建和模块隔离
系统 SHALL 提供 Windows、Linux、macOS、iOS、Android 原生 BLE 构建路径，仅检测目标平台依赖；关闭 BLE 时不得要求蓝牙依赖，不支持的目标启用 BLE 时 SHALL 明确配置失败。

#### Scenario: 关闭 BLE
- **WHEN** 未安装蓝牙专用依赖的环境关闭 BLE
- **THEN** 其他模块可配置构建，不下载 SimpleBLE 或检测 D-Bus/Java 蓝牙组件

#### Scenario: 交付平台后端
- **WHEN** 某平台后端声明可用
- **THEN** 提供最低系统和工具链版本、系统链接项、权限及打包说明，示例可构建

### Requirement: 依赖许可与移除 SimpleBLE
交付版本 SHALL 不再下载、编译、链接或打包 SimpleBLE。新增依赖 SHALL 逐项核验实际版本的官方许可及分发义务，不得带非商业或用途限制。

#### Scenario: 发布审查
- **WHEN** 审查依赖清单和构建产物
- **THEN** 每个新增 SDK、头文件、库和打包组件均有来源、版本、许可原文依据、修改及分发义务记录，产物无 SimpleBLE

### Requirement: 验收证据
项目 SHALL 记录每个公共方法的平台能力和错误行为，并区分编译、自动化、硬件及未验证项目。

#### Scenario: 无硬件可测
- **WHEN** 仅通过编译与假后端测试
- **THEN** 文档标记硬件未验证，不声称完整验收
