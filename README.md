## 0. 实验结果：

在ESP-01模块上,工作正常,主要修改有两个文件,需要自行打补丁完成,不会打补丁?那我也没办法了.

## 1. 概述

ESP8266 适配了[腾讯云](https://cloud.tencent.com) 设备端 C-SDK v2.0.0 版本, 用户可以参考 Espressif 提供的设备端 MQTT Demo 进行二次开发，快速接入腾讯云平台。

MQTT Demo 参考腾讯官方 [qcloud-iot-sdk-embedded-c v2.1.0](https://github.com/tencentyun/qcloud-iot-sdk-embedded-c) 里的 demo，添加了基于 esp32 的 WiFi 连接、File 存储等相关功能，用户可根据产品需求自行添加或删减相关功能

## 2. Demo 使用

用户拿到乐鑫的 esp8266-qcloud 后，编译下载固件到 ESP8266 开发板。设备侧首先连接路由器, 然后连上腾讯云, 在设备侧以及腾讯云端可以看到相应的调试 log。

由于使用了多线程,而ESP8266默认没使用PTHREAD功能,需要在menuconfig中配置,同时,也由于我比较懒,请自行修改源码里面的各种配置,而且由于ESP-01只有512K的Flash,实在太小了,只能密钥认证了.

ESP_IDF请自行配置,新版的ESP8266_RTOS_SDK也是ESP_IDF配置的.

### 2.1. 环境搭建

* 硬件准备

*  **开发板**：ESP8266 开发板
*  **路由器**：可以连接外网

### 2.1 工程编译和测试

```
make && make flash && make monitor

```

* Espressif 官网： [http://espressif.com](http://espressif.com)
* TaterLi 个人博客： [http://www.lijingquan.net](http://www.lijingquan.net)
* 腾讯云官网：[tencent cloud](https://cloud.tencent.com)
* 腾讯云设备端 SDK： [qcloud-iot-sdk](https://github.com/tencentyun/qcloud-iot-sdk-embedded-c)


