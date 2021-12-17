# homekit-bridge

[English Version](README.md) | 中文版

## 介绍

**homekit-bridge**专门为嵌入式设备设计，可以将非HomeKit的设备快速地接入到HomeKit。它提供了以下功能：

- 配置你想要连接到HomeKit的设备
- 编写插件来生成HomeKit桥接配件(Bridged Accessory)

**homekit-bridge**在[HomeKitADK](https://github.com/apple/HomeKitADK)的基础上实现，主要的C代码位于ADK的应用层，只调用平台适配层(PAL)接口或者HomeKit协议(HAP)接口，不直接调用适配平台相关的接口。
> HomeKitADK不仅实现了HomeKit协议(HAP)，还将平台相关的接口抽象到了平台适配层(PAL)，使得应用代码在不同平台上表现一致。

为了更好的扩展性以及降低开发难度，**homekit-bridge**引入了动态语言[Lua](https://www.lua.org)，将C模块封装成Lua模块，使用Lua来编写上层应用代码。**homekit-bridge**还做了以下的优化，使得Lua能够在资源紧凑的设备上运行：

- 通过`luac`将文本代码转换成字节码
- 将多个Lua脚本生成目录树，嵌入到C代码中
- 使得Lua虚拟机支持直接从ROM中读取代码，而不是拷贝到RAM中读取

## 支持的设备

### 米家 - [miio](plugins/miio/README.md)

产品名称 | 型号
-|-
小米空调伴侣2代 | `lumi.acpartner.mcn02`
小米智能插座基础版(1个插座) | `chuangmi.plug.m3`
智米直流变频落地扇2S | `zhimi.fan.za4`
小米直流变频塔扇 | `dmaker.fan.p9`

## 支持的平台

目前针对以下平台做了适配：

- Linux (Ubuntu/Raspberry Pi OS)
- ESP-IDF (ESP32/ESP32-S2)

## 入门

### 注意

在克隆完代码后需要执行以下命令来初始化子模块代码：

```
$ git submodule update --init
```

### Linux (Ubuntu)

#### 准备

```bash
$ sudo apt install cmake ninja-build clang libavahi-compat-libdnssd-dev libssl-dev python3-pip
$ sudo pip3 install cpplint
```

#### 编译和安装

```bash
$ mkdir build
$ cd build
$ cmake -G Ninja .. && ninja
$ sudo ninja install
```

#### 运行

默认运行homekit-bridge:

```bash
$ homekit-bridge
```

以下选项可以在运行homekit-bridge的时候指定：

选项 | 描述
-|-
`-d`, `--dir` | 设置脚本目录
`-e`, `--entry` | 设置入口脚本的名称
`-h`, `--help` | 显示帮助并退出

#### 配置

配置文件`config.lua`默认位于`/usr/local/lib/homekit-bridge`，可以在运行homekit-bridge之前修改它。如果你指定了脚本目录，homekit-bridge将会到指定脚本目录中寻找`config.lua`。

### ESP-IDF

#### 准备

根据ESP-IDF官方文档[快速入门](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/index.html)准备环境。

目前经过测试的ESP-IDF版本为**v4.3.1**，通过以下命令切换到该版本：

```bash
$ git fetch --tag
$ git checkout v4.3.1
$ git submodule update
```

ESP-IDF当前使用的MbedTLS版本为2.16.x，然而HomeKit ADK最低需要2.18版本的MbedTLS。目前维护了一个分支[mbedtls-2.16.6-adk](https://github.com/espressif/mbedtls/tree/mbedtls-2.16.6-adk)，在2.16之上包含了2.18的补丁，可以根据以下步骤切换到该分支：

```bash
$ cd $IDF_PATH/components/mbedtls/mbedtls
$ git pull
$ git checkout -b mbedtls-2.16.6-adk origin/mbedtls-2.16.6-adk
```

#### 编译、烧录和运行

你可以在ESP32或者ESP32-S2上使用homekit-bridge，使用以下命令来编译、烧录和运行：

```bash
$ cd /path/to/homekit-bridge/platform/esp
$ export ESPPORT=/dev/ttyUSB0  # 设置开发板的串口
$ idf.py set-target <esp32/esp32s2>
$ idf.py flash
$ idf.py monitor
```

#### 连接Wi-Fi

使用 `join` 命令来连接Wi-Fi:

```bash
esp32 > join "<ssid>" "<password>"
```

#### 配置

TODO

## 许可证

[MIT © 2021 KNpTrue and homekit-bridge contributors.](LICENSE)
