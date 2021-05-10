# homekit-bridge

![](https://img.shields.io/badge/language-c|lua-orange.svg)
![](https://img.shields.io/badge/platform-linux-lightgrey.svg)
![](https://img.shields.io/badge/platform-esp-lightgrey.svg)
[![license](https://img.shields.io/github/license/KNpTrue/lua-homekit-bridge)](LICENSE)

## Introduction

A HomeKit gateway specially designed for embedded devices, it allows you to connect non-HomeKit devices to HomeKit through simple configuration.

## Code style

HomeKit bridge supports code style checking, the checker is [cpplint](https://github.com/google/styleguide), a command line tool that checks for style issues in C/C++ files according to the [Google C++ Style Guide](http://google.github.io/styleguide/cppguide.html).

## Build

### Clone the repo
> Add `--recursive` to initialize submodules in the clone.
```
    git clone --recursive https://github.com/KNpTrue/lua-homekit-bridge.git
```

### Platform Linux (Ubuntu)
1. Install dependencies:
    ```bash
    $ sudo apt install cmake ninja-build clang libavahi-compat-libdnssd-dev libssl-dev pip3
    $ pip3 install cpplint
    ```

2. Compile and run:
    ```bash
    $ mkdir build
    $ cd build
    $ cmake -G Ninja .. && ninja
    $ ./platform/linux/homekit-bridge
    ```

3. Run the example
    ```bash
    $ ./platform/linux/homekit-bridge -d example_scripts
    ```

### Platform ESP
1. Set up the host environment and ESP-IDF (**v4.3-beta3**) as per the steps given [here](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html).

2. ESP-IDF currently uses MbedTLS 2.16.x, whereas HomeKit ADK requires 2.18. A branch mbedtls-2.16.6-adk is being maintained [here](https://github.com/espressif/mbedtls/tree/mbedtls-2.16.6-adk) which has the required patches from 2.18, on top of 2.16.6. To switch to this, follow these steps:
    ```bash
    $ cd $IDF_PATH/components/mbedtls/mbedtls
    $ git pull
    $ git checkout -b mbedtls-2.16.6-adk origin/mbedtls-2.16.6-adk
    ```

3. You can use homekit-bridge with any ESP32 or ESP32-S2 board. Compile, flash and connect to console as below:
    ```bash
    $ cd /path/to/homekit-bridge/platform/esp
    $ export ESPPORT=/dev/tty.SLAB_USBtoUART  # Set your board's serial port here
    $ idf.py set-target <esp32/esp32s2>
    $ idf.py flash
    $ idf.py monitor
    ```

4. Use `join` command to configure Wi-Fi:
    ```bash
    esp32 > join "<ssid>" "<password>"
    ```

## Usage

TODO

## License

[MIT © 2021 KNpTrue and homekit-bridge contributors.](LICENSE)
