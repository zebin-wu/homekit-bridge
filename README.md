# homekit-bridge

![](https://img.shields.io/badge/language-c|lua-orange.svg)
![](https://img.shields.io/badge/platform-linux|esp-lightgrey.svg)
[![license](https://img.shields.io/github/license/KNpTrue/lua-homekit-bridge)](LICENSE)

## Contents

- [Introduction](#introduction)
- [Code style](#Code-style)
- [Build](#Build)
- [Usage](#usage)
- [License](#license)

## Introduction

TODO

## Code style

TODO

## Build

### Clone the repo
> Add `--recursive` to initialize submodules in the clone.
```
    git clone --recursive https://github.com/KNpTrue/lua-homekit-bridge.git
```

### Platform Linux (Ubuntu)
1. Install dependencies
    ```text
    $ sudo apt install cmake ninja-build clang libavahi-compat-libdnssd-dev libssl-dev
    ```

2. Compile and run
    ```text
    $ mkdir build
    $ cd build
    $ cmake -G Ninja .. && ninja
    $ ./platform/linux/homekit-bridge
    ```

### Platform ESP
1. Set up the host environment and ESP-IDF (**v4.3-beta3**) as per the steps given [here](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html).

2. ESP-IDF currently uses MbedTLS 2.16.x, whereas HomeKit ADK requires 2.18. A branch mbedtls-2.16.6-adk is being maintained [here](https://github.com/espressif/mbedtls/tree/mbedtls-2.16.6-adk) which has the required patches from 2.18, on top of 2.16.6. To switch to this, follow these steps:
    ```text
    $ cd $IDF_PATH/components/mbedtls/mbedtls
    $ git pull
    $ git checkout -b mbedtls-2.16.6-adk origin/mbedtls-2.16.6-adk
    ```

3. You can use homekit-bridge with any ESP32 or ESP32-S2 board. Compile and flash as below:
    ```text
    $ cd /path/to/homekit-bridge/platform/esp
    $ export ESPPORT=/dev/tty.SLAB_USBtoUART #Set your board's serial port here
    $ idf.py set-target <esp32/esp32s2>
    $ idf.py flash
    $ idf.py monitor
    ```

## Usage

TODO

## License

[MIT © 2021 KNpTrue and homekit-bridge contributors.](LICENSE)
