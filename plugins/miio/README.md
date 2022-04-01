# Mi Home - miio

## Introduction

**miio** implement the [miio protocol](https://github.com/OpenMiHome/mihome-binary-protocol/blob/master/doc/PROTOCOL.md), and encapsulate the class `MiioDevice`, provide features:

- Create a device object.
- Start a request and get the response synchronously.
- Get/Set properties.

After the device is created, the plugin will search for the corresponding product module according to the device model to generate the specific accessory.

**miio** also can get the devices information from xiaomi cloud `cloudapi`. As long as you provide your xiaomi account information and your home SSID, you can get all the device information in your home.

## Configure miio

### Configuration field description

Name | Type | Description | Required | Example
-|-|-|-|-
`region` | `string` | Server region | Yes | `"cn"`,`"de"`,`"us"`,`"ru"`,`"tw"`,`"sg"`,`"in"`,`"i2"`
`username` | `string` | User ID or email | YES | `"xxx@xxx.com"`,`"12345678"`
`password` | `string` | User password | YES | `12345678`
`ssid` | `string` | The SSID of the Wi-Fi | Yes | `"HUAWEI-A1"`

Example: config.lua

```lua
return {
    bridge = {
        name = "HomeKit Bridge"
    },
    plugins = {
        miio = {
            ssid = "HUAWEI-A1",
            region = "cn",
            username = "xxxxxx@xxxx.com",
            password = "12345678",
        }
    }
}
```
