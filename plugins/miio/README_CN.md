# 米家 - miio

[English Version](README.md) | 中文版

## 介绍

**miio** 实现了 [miio 协议](https://github.com/OpenMiHome/mihome-binary-protocol/blob/master/doc/PROTOCOL.md), 并将协议封装抽象成Lua类 `MiioDevice`, 提供以下功能:

- 创建设备对象
- 发起请求并同步获取响应
- 获取/设置属性

**miio** 还实现了与米家云端通信的协议, 允许在配置好用户名、密码之后, 自动从云端拉取设备信息(包含设备密钥), 以便后续与米家设备建立连接。

当设备创建后，插件就会根据设备的型号`{mfg}.{product}.{submodel}`寻找已经适配的产品脚本，如果能找到，则会生成HomeKit附件。

## 配置

### 配置项

名称 | 类型 | 描述 | 是否必选 | 例子
-|-|-|-|-
`region` | `string` | 地区 | YES | `cn`,`de`,`us`,`ru`,`tw`,`sg`,`in`,`i2`
`username` | `string` | 用户ID或邮箱 | YES | `xxx@xxx.com`,`12345678`
`password` | `string` | 用户密码 | YES | `12345678`
`ssid` | `string` | Wi-FI SSID | YES | `HUAWEI-A1`

参考 ``config.json``:

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
