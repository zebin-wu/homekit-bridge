# homekit-bridge

[English Version](README.md) | 中文版

## 介绍

**homekit-bridge**专门为嵌入式设备设计，可以将非HomeKit的设备快速地接入到HomeKit。它提供了以下功能：

- 配置你想要连接到HomeKit的设备
- 编写插件来生成HomeKit桥接配件(Bridged Accessory)

**homekit-bridge**在[HomeKitADK](https://github.com/apple/HomeKitADK)的基础上实现，主要的C代码位于ADK的应用层。
> HomeKitADK不仅实现了HomeKit协议(HAP)，还将平台相关的接口抽象到了平台适配层(PAL)，使得应用代码在不同平台上表现一致。

为了更好的扩展性以及降低开发难度，**homekit-bridge**引入了动态语言[Lua](https://www.lua.org)，将C模块封装成Lua模块，使用Lua来编写上层应用代码。**homekit-bridge**还做了以下的优化，使得Lua能够在资源紧凑的设备上运行：

- 通过`luac`将文本代码转换成字节码
- 将多个Lua脚本生成目录树，嵌入到C代码中
- 使得Lua虚拟机支持直接从ROM中读取代码，而不是拷贝到RAM中读取

## 支持的设备

### 米家 - [miio](plugins/miio/README_CN.md)

产品名称 | 型号
-|-
小米空调伴侣2代 | `lumi.acpartner.mcn02`
小米米家智能插座Wi-Fi版 | `chuangmi.plug.m3`
米家智能插座2 蓝牙网关版 | `chuangmi.plug.212a01`
米家智能插座3 | `cuco.plug.v3`
智米直流变频落地扇2S | `zhimi.fan.za4`
小米直流变频落地扇1X | `dmaker.fan.p5`
小米直流变频塔扇 | `dmaker.fan.p9`
米家智能除湿机 22L | `dmaker.derh.22l`

## 支持的平台

目前针对以下平台做了适配：

- Linux (Ubuntu/Raspberry Pi OS)
- ESP-IDF (ESP32/ESP32-S2/ESP32-C3/ESP32-S3)

## 入门

### 注意

在克隆完代码后需要执行以下命令来初始化子模块代码：

```
git submodule update --init
```

### Linux (Ubuntu)

#### 准备

```bash
sudo apt install cmake ninja-build clang libavahi-compat-libdnssd-dev libssl-dev python3-pip
sudo pip3 install cpplint
```

#### 编译和安装

```bash
mkdir build
cd build
cmake -G Ninja .. && ninja
sudo ninja install
```

#### 运行

默认运行homekit-bridge:

```bash
homekit-bridge
```

以下选项可以在运行homekit-bridge的时候指定：

选项 | 描述
-|-
`-d`, `--dir` | 设置工作目录
`-h`, `--help` | 显示帮助并退出

#### 配置

配置文件`config.json`默认位于`/usr/local/lib/homekit-bridge`，可以在运行homekit-bridge之前修改它。如果你指定了工作目录，homekit-bridge将会到指定目录中寻找`config.json`。

### ESP-IDF

#### 准备

根据ESP-IDF官方文档[快速入门](https://docs.espressif.com/projects/esp-idf/zh_CN/v5.1/get-started/index.html)准备环境。

目前经过测试的ESP-IDF版本为**v5.1**，通过以下命令切换到该版本：

```bash
git fetch --tag
git checkout v5.1
git submodule update
```

#### 编译、烧录和运行

使用以下命令来编译、烧录和运行：

```bash
idf.py set-target [esp32|esp32s2|esp32c3|esp32s3|esp32c6]
idf.py flash monitor
```

#### 连接Wi-Fi

使用 `join` 命令来连接Wi-Fi:

```bash
join "<ssid>" "<password>"
```

### 配置

使用以下命令来操作配置项:

#### 获取一个配置项

```
homekit-bridge conf <key>
```

#### 设置一个配置项

```
homekit-bridge conf <key> <value>
```

### 添加一个值到指定配置项

```
homekit-bridge conf --add <key> <value>
```

#### 配置项

名称 | 类型 | 描述 | 是否必选 | 例子
-|-|-|-|-
`bridge.name` | `string` | 桥接附件的名称 | YES | `HomeKit Bridge`
`bridge.plugins` | `string[]` | 插件名称列表 | NO | `miio`

每个插件都有自己特定的配置，可以从插件的readme来获取详情。

## 许可证

[Apache-2.0 © 2021-2022 Zebin Wu and homekit-bridge contributors.](LICENSE)
