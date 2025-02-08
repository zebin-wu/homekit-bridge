# homekit-bridge

## Introduction

homekit-bridge designed for embedded devices allows you to connect non-HomeKit devices to Apple HomeKit quickly. It provides the following features:

- Configure the devices you want to connect to HomeKit.
- Write plugins to generate HomeKit bridged accessories.

homekit-bridge is based on [HomeKitADK](https://github.com/apple/HomeKitADK); the main C code is in the application layer of ADK.
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
Xiaomi Smart Plug (Wi-Fi) | `chuangmi.plug.m3`
Xiaomi Smart Plug 2 (Bluetooth Gateway) | `chuangmi.plug.212a01`
Mijia Smart Plug 3 | `cuco.plug.v3`
Zhimi DC Variable Frequency Fan 2S | `zhimi.fan.za4`
Xiaomi DC Variable Frequency Fan 1X | `dmaker.fan.p5`
Xiaomi DC Variable Frequency Tower Fan | `dmaker.fan.p9`
Xiaomi Smart Dehumidifier 22L | `dmaker.derh.22l`
Xiaomi Smart Heater | `xiaomi.heater.ma4`

## Supported platform

- Linux (Ubuntu/Raspberry Pi OS)
- ESP-IDF (ESP32/ESP32-S2/ESP32-C3/ESP32-S3)

## Getting started

### Attention

After clone the repository to the local, please initialize submodules by the follow command:

```bash
git submodule update --init
```

### Linux (Ubuntu)

#### Prepare

```bash
sudo apt install cmake ninja-build clang libavahi-compat-libdnssd-dev libssl-dev python3-pip
sudo pip3 install cpplint
```

#### Compile and install

```bash
mkdir build
cd build
cmake -G Ninja .. && ninja
sudo ninja install
```

#### Run

Run homekit-bridge by default:

```bash
homekit-bridge
```

The following options can be specified when running homekit-bridge:

Option | Description
-|-
`-d`, `--dir` | set the working directory
`-h`, `--help` | display this help and exit

### ESP-IDF

#### Prepare

Set up the host environment and ESP-IDF as per the steps given [here](https://docs.espressif.com/projects/esp-idf/en/v5.1.2/get-started/index.html).

The currently tested ESP-IDF version is **v5.1.2**, switch to this version with the following command:

```bash
git fetch --tag
git checkout v5.1.2
git submodule update
```

#### Compile and flash

Compile, flash and connect to console as below:

```bash
idf.py set-target [esp32|esp32s2|esp32c3|esp32s3|esp32c6]
idf.py flash monitor
```

#### Join Wi-Fi

Use `join` command to configure Wi-Fi:

```bash
join "<ssid>" "<password>"
```

### Configure

Using the follow commands to operate configuration items:

#### Get a item

```
homekit-bridge config <key>
```

#### Set a item

```
homekit-bridge config <key> <value>
```

#### Add a value to the item

```
homekit-bridge config --add <key> <value>
```

#### Configruation field

Name | Type | Description | Required | Example
-|-|-|-|-
`bridge.name` | `string` | Name of the bridge accessory | YES | `HomeKit Bridge`
`bridge.plugins` | `string[]` | Plugin names | NO | `miio`

Each plugin has its own specific configuration, see the plugin readme for details.

### Add the bridge to Home APP
homekit-bridge will automatically generate a **setup code**, which can be entered when adding accessory. The **setup code** can be obtained through the following command:
```
homekit-bridge setupcode
```

## License

[Apache-2.0 © 2021-2022 Zebin Wu and homekit-bridge contributors.](LICENSE)
