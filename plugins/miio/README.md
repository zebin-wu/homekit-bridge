# Mi Home - miio

## Introduction

This plugin implement the [miio protocol](https://github.com/OpenMiHome/mihome-binary-protocol/blob/master/doc/PROTOCOL.md), and encapsulate the class `MiioDevice`, provide features:
- Create a device object.
- Start a request and receive the response asynchronously.
- Automatically sync properties.
- Get/Set properties.

After the device is created, the plugin will search for the corresponding product module according to the device model to generate the specific accessory.

## Supported devices

Product Name | Model
-|-
Xiaomi Mi Air Conditioner Companion 2 | `lumi.acpartner.mcn02`

## Configure your device

### Get device token

Refer to the following link: https://www.home-assistant.io/integrations/xiaomi_miio#retrieving-the-access-token

### Write configuration

Name | Type | Description | Required | Example
-|-|-|-|-
`addr` | `string` | Device address | Yes | `"192.168.1.10"`
`token` | `string` | Device token | Yes | `"d1abcd1230238cf1g123a142962agdd1"`
`name` | `string` | Accessory name | No | `"My device"`

Example: config.lua
```lua
return {
    bridge = {
        name = "HomeKit Bridge"
    },
    plugins = {},
    accessories = {
        {
            plugin = "miio",
            addr = "192.168.1.10",
            token = "d1abcd1230238cf1g123a142962agdd1",
            name = "My device"
        }
    }
}
```