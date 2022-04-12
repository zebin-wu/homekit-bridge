# Mi Home - miio

## Introduction

**miio** implement the [miio protocol](https://github.com/OpenMiHome/mihome-binary-protocol/blob/master/doc/PROTOCOL.md), and encapsulate the class `MiioDevice`, provide features:

- Create a device object.
- Start a request and get the response synchronously.
- Get/Set properties.

**miio** also implements a protocol for communicating with the xiaomi cloud, allowing to automatically get device information (including device token) from the cloud after configuring the user name and password, so as to establish a connection with the miio device later.

When the device is created, the plugin will look for the adapted product script based on the device's model `{mfg}.{product}.{submodel}`, and if it can find it, it will generate a HomeKit accessory.

## Configure

### Configuration field

Name | Type | Description | Required | Example
-|-|-|-|-
`region` | `string` | Server region | YES | `cn`,`de`,`us`,`ru`,`tw`,`sg`,`in`,`i2`
`username` | `string` | User ID or email | YES | `xxx@xxx.com`,`12345678`
`password` | `string` | User password | YES | `12345678`
`ssid` | `string` | The SSID of the Wi-Fi | YES | `HUAWEI-A1`

Example: config.json

```json
{
    "bridge": {
        "name": "HomeKit Bridge"
    },
    "plugins": {
        "miio": {
            "region": "cn",
            "username": "xxx@xxx.com",
            "password": "12345678",
            "ssid": "HUAWEI-A1"
        }
    }
}
```
