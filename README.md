# homekit-bridge

English Version | [中文版](README_CN.md)

## Introduction

homekit-bridge designed for embedded devices allows you to connect non-HomeKit devices to Apple HomeKit quickly. It provides the following features:

- Configure the devices you want to connect to HomeKit.
- Write plugins to generate HomeKit bridged accessories.

homekit-bridge is based on [HomeKitADK](https://github.com/apple/HomeKitADK); the main C code is in the application layer of ADK, using PAL interface or HAP interface, and not directly using platform-related interfaces.
> HomekitADK not only implements HomeKit Accessory Protocol(HAP), but also abstracts platform-related interfaces to Platform Adapter Layer(PAL) to make the same application code behave consistently on different platforms.

In order to achieve better scalability and reduce development difficulty, homekit-bridge introduced the dynamic language [Lua](https://www.lua.org), encapsulated C modules into Lua modules, and used Lua to write upper-level applications. homekit-bridge also made the following optimizations to run Lua on devices with compact resources:

- Generate text code to byte code via `luac`.
- Generate directory trees for multiple Lua scripts and embed them in C code.
- Make the Lua VM supports reading the code directly from ROM instead of copying the code to RAM.

## Supported devices

### Mi Home - [miio](plugins/miio/README.md)

Product Name | Model
-|-
Xiaomi Mi Air Conditioner Companion 2 | `lumi.acpartner.mcn02`
Xiaomi Plug Base (1 Socket) | `chuangmi.plug.m3`
Zhimi DC Variable Frequency Fan 2S | `zhimi.fan.za4`
Xiaomi DC Variable Frequency Tower Fan | `dmaker.fan.p9`

## Supported platform

- Linux (Ubuntu/Raspberry Pi OS)
- ESP-IDF (ESP32/ESP32-S2)

## Getting started

### Attention

After clone the repository to the local, please initialize submodules by the follow command:

```bash
$ git submodule update --init
```

### Linux

#### Prepare

```bash
$ sudo apt install cmake ninja-build clang libavahi-compat-libdnssd-dev libssl-dev python3-pip
$ sudo pip3 install cpplint
```

#### Compile and install

```bash
$ mkdir build
$ cd build
$ cmake -G Ninja .. && ninja
$ sudo ninja install
```

#### Run

Run homekit-bridge by default:

```bash
$ homekit-bridge
```

The following options can be specified when running homekit-bridge:

Option | Description
-|-
`-d`, `--dir` | set the scripts directory
`-e`, `--entry` | set the entry script name
`-h`, `--help` | display this help and exit

#### Configure

The configuration script `config.lua` is placed in `/usr/local/lib/homekit-bridge` by default, you can edit it before running homekit-bridge. If you specified the scripts directory, homekit-bridge will find `config.lua` in the specified directory.

### ESP-IDF

#### Prepare

Set up the host environment and ESP-IDF as per the steps given [here](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html).

The currently tested ESP-IDF version is **v4.3.1**, switch to this version with the following command:

```bash
$ git fetch --tag
$ git checkout v4.3.1
$ git submodule update
```

ESP-IDF currently uses MbedTLS 2.16.x, whereas HomeKit ADK requires 2.18. A branch mbedtls-2.16.6-adk is being maintained [here](https://github.com/espressif/mbedtls/tree/mbedtls-2.16.6-adk) which has the required patches from 2.18, on top of 2.16.6. To switch to this, follow these steps:

```bash
$ cd $IDF_PATH/components/mbedtls/mbedtls
$ git pull
$ git checkout -b mbedtls-2.16.6-adk origin/mbedtls-2.16.6-adk
```

#### Compile and flash

You can use homekit-bridge with any ESP32 or ESP32-S2 board. Compile, flash and connect to console as below:

```bash
$ cd /path/to/homekit-bridge/platform/esp
$ export ESPPORT=/dev/ttyUSB0  # Set your board's serial port here
$ idf.py set-target <esp32/esp32s2>
$ idf.py flash
$ idf.py monitor
```

#### Join Wi-Fi

Use `join` command to configure Wi-Fi:

```bash
esp32 > join "<ssid>" "<password>"
```

#### Configure

TODO

## License

[MIT © 2021 KNpTrue and homekit-bridge contributors.](LICENSE)
