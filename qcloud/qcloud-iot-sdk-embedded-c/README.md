# 腾讯物联网通信 SDK
腾讯物联网通信 SDK 依靠安全且性能强大的数据通道，为物联网领域开发人员提供终端(如传感器, 执行器, 嵌入式设备或智能家电等等)和云端的双向通信能力。

# 快速开始
本节将讲述如何在腾讯物联网通信控制台申请设备, 并结合本 SDK 快速体验设备通过 MQTT+TLS/SSL 协议连接到腾讯云, 发送和接收消息；通过 CoAP+DTLS 协议连接到腾讯云，上报数据。

## 一. 控制台创建设备

#### 1. 注册/登录腾讯云账号
访问[腾讯云登录页面](https://cloud.tencent.com/login?s_url=https%3A%2F%2Fcloud.tencent.com%2F), 点击[立即注册](https://cloud.tencent.com/register?s_url=https%3A%2F%2Fcloud.tencent.com%2F), 免费获取腾讯云账号，若您已有账号，可直接登录。

#### 2. 访问物联网通信控制台
登录后点击右上角控制台，进入控制台后，鼠标悬停在云产品上，弹出层叠菜单，点击物联网通信。

![](https://main.qcloudimg.com/raw/1a0c668f382a13a6ec33a6b4b1dbd8b5.png)

或者直接访问[物联网通信控制台](https://console.qcloud.com/iotcloud)

#### 3. 创建产品和设备
点击页面**创建新产品**按钮, 创建一个产品。

![](https://main.qcloudimg.com/raw/a0da21dc6ac9a9e1dede0077d40cfb22.png)

在弹出的产品窗口中，选择节点类型和产品类型，输入产品名称，选择认证方式和数据格式，输入产品描述，然后点击确定创建。如果是普通直连设备，可按下图选择。

![](https://main.qcloudimg.com/raw/7ee90122a01f5f277785885669a56aec.png)

如果是普通网关设备，可按下图选择。

![](https://main.qcloudimg.com/raw/d3f5de3bd07a779f9b1306085fa4d1f1.png)

在生成的产品页面下，点击**设备列表**页面添加新设备。

![](https://main.qcloudimg.com/raw/0530e0da724cd36baefc7011ebce4775.png)

如果产品认证方式为证书认证，输入设备名称成功后，切记点击弹窗中的**下载**按钮，下载所得包中的设备密钥文件和设备证书用于设备连接物联网通信的鉴权。

![](https://main.qcloudimg.com/raw/6592056f1b55fa9262e4b2ab31d0b218.png)

如果产品认证方式为密钥认证，输入设备名称成功后，会在弹窗中显示新添加设备的密钥

![](https://main.qcloudimg.com/raw/fe7a013b1d8c29c477d0ed6d00643751.png)

如果是网关产品，还需要按照普通产品的方式先创建子产品和子设备，并在网关产品页面添加子产品和子设备。需要注意的是子设备是无法直连物联网平台的产品，由网关设备代理连接，所以子设备的认证方式不影响连接，由网关设备来负责认证接入。
创建好子产品之后，先到网关产品页面的子产品栏目添加子产品

![](https://main.qcloudimg.com/raw/00da59942515b1d772323c7087f627e3.png)

再到网关设备页面的子设备栏目添加子设备

![](https://main.qcloudimg.com/raw/c24938ac8ed3aa3e0834cb40598740ca.png)

#### 4. 创建可订阅可发布的Topic

按照**第三步**中进入产品设置页面的方法进入页面后, 点击权限列表，再点击**添加Topic权限**。

![](https://main.qcloudimg.com/raw/65a2d1b7251de37ce1ca2ba334733c57.png)

在弹窗中输入 data, 并设置操作权限为**发布和订阅**，点击创建。

![](https://main.qcloudimg.com/raw/f429b32b12e3cb0cf319b1efe11ccceb.png)

随后将会创建出 productID/\${deviceName}/data 的 Topic，在产品页面的权限列表中可以查看该产品的所有权限。

对于网关产品，除了添加本产品的Topic权限，还需要为子产品添加Topic权限

![](https://main.qcloudimg.com/raw/3de74cfd5b235fe942fe18c359ad08af.png)

## 二. 编译示例程序

#### 1. 下载SDK
登录 Linux, 运行如下命令从 github 克隆代码, 或者访问最新[下载](https://github.com/tencentyun/qcloud-iot-sdk-embedded-c/releases)地址, 将下载到的压缩包在 Linux 上解压缩

`git clone https://github.com/tencentyun/qcloud-iot-sdk-embedded-c.git`

#### 2. 填入设备信息
编辑 samples/mqtt/mqtt_sample.c 文件和 samples/coap/coap_sample.c 文件中如下代码段, 填入之前创建产品和设备步骤中得到的 **产品ID**，**设备名称**：

1. 若使用**证书认证**加密方式，填写 **QCLOUD_IOT_CERT_FILENAME** 和 **QCLOUD_IOT_CERT_FILENAME** 并将文件放置在根目录下 certs 目录中。将根目录下 make.settings 文件中的配置项 FEATURE_AUTH_MODE 设置为 CERT，FEATURE_AUTH_WITH_NOTLS 设置为 n。

```
FEATURE_AUTH_MODE        = CERT   # MQTT/CoAP接入认证方式，使用证书认证：CERT；使用密钥认证：KEY
FEATURE_AUTH_WITH_NOTLS  = n      # 接入认证是否不使用TLS，证书方式必须选择使用TLS，密钥认证可选择不使用TLS
```

2. 若使用**秘钥认证**加密方式，填写 **QCLOUD_IOT_DEVICE_SECRET**。将根目录下 make.settings 文件中的配置项 FEATURE_AUTH_MODE 设置为 KEY，FEATURE_AUTH_WITH_NOTLS 设置为 n 时通过 TLS 密钥认证方式连接，设置为 y 时，则通过 HMAC-SHA1 加密算法连接。

```
/* 产品名称, 与云端同步设备状态时需要  */
#define QCLOUD_IOT_MY_PRODUCT_ID            "YOUR_PRODUCT_ID"
/* 设备名称, 与云端同步设备状态时需要 */
#define QCLOUD_IOT_MY_DEVICE_NAME           "YOUR_DEVICE_NAME"

#ifdef AUTH_MODE_CERT
    /* 客户端证书文件名  非对称加密使用*/
    #define QCLOUD_IOT_CERT_FILENAME          "YOUR_DEVICE_NAME_cert.crt"
    /* 客户端私钥文件名 非对称加密使用*/
    #define QCLOUD_IOT_KEY_FILENAME           "YOUR_DEVICE_NAME_private.key"

    static char sg_cert_file[PATH_MAX + 1];      //客户端证书全路径
    static char sg_key_file[PATH_MAX + 1];       //客户端密钥全路径

#else
    #define QCLOUD_IOT_DEVICE_SECRET                  "YOUR_IOT_PSK"
#endif
```

3. 若为**网关产品**，需要将根目录下 make.settings 文件中的配置项 FEATURE_GATEWAY_ENABLED 设置为 y。

```
FEATURE_GATEWAY_ENABLED  = y      # 是否打开网关功能
```

并编辑samples/gateway/gateway_sample.c 文件中如下代码段, 填入之前创建网关产品和子产品及设备步骤中得到的**网关产品ID**，**网关设备名称**，**子产品ID**，**子设备名称**：

```
/* 网关产品名称, 与云端同步设备状态时需要  */
#define QCLOUD_IOT_MY_PRODUCT_ID            "YOUR_GW_PRODUCT_ID"
/* 网关设备名称, 与云端同步设备状态时需要 */
#define QCLOUD_IOT_MY_DEVICE_NAME           "YOUR_GW_DEVICE_NAME"

#ifdef AUTH_MODE_CERT
    /* 网关客户端证书文件名 非对称加密使用*/
    #define QCLOUD_IOT_CERT_FILENAME          "YOUR_GW_DEVICE_NAME_cert.crt"
    /* 网关客户端私钥文件名 非对称加密使用*/
    #define QCLOUD_IOT_KEY_FILENAME           "YOUR_GW_DEVICE_NAME_private.key"

    static char sg_cert_file[PATH_MAX + 1];      //客户端证书全路径
    static char sg_key_file[PATH_MAX + 1];       //客户端密钥全路径

#else
    #define QCLOUD_IOT_DEVICE_SECRET                  "YOUR_GW_IOT_PSK"
#endif

/* 子产品名称, 与云端同步设备状态时需要  */
#define QCLOUD_IOT_SUBDEV_PRODUCT_ID            "YOUR_SUBDEV_PRODUCT_ID"
/* 子设备名称, 与云端同步设备状态时需要 */
#define QCLOUD_IOT_SUBDEV_DEVICE_NAME           "YOUR_SUBDEV_DEVICE_NAME"
```

#### 3. 编译 SDK 产生示例程序
在根目录执行

```
$make clean
$make
```

编译成功完成后, 生成的样例程序在当前目录的 output/release/bin 目录下:

```
output/release/bin/
├── aircond_shadow_sample
├── aircond_shadow_sample_v2
├── certs
│   ├── README.md
│   ├── TEST_CLIENT_cert.crt
│   └── TEST_CLIENT_private.key
├── coap_sample
├── door_coap_sample
├── door_mqtt_sample
├── gateway_sample
├── mqtt_sample
├── ota_mqtt_sample
└── shadow_sample
```

## 三. 运行示例程序

#### 1. 执行 MQTT 示例程序
```
./mqtt_sample
INF|2019-03-06 17:20:17|device.c|iot_device_info_set(65): SDK_Ver: 2.3.1, Product_ID: S3EUVBRJLB, Device_Name: demo-device
DBG|2019-03-06 17:20:17|HAL_TLS_mbedtls.c|HAL_TLS_Connect(204): Connecting to /S3EUVBRJLB.iotcloud.tencentdevices.com/8883...
DBG|2019-03-06 17:20:17|HAL_TLS_mbedtls.c|HAL_TLS_Connect(209): Setting up the SSL/TLS structure...
DBG|2019-03-06 17:20:17|HAL_TLS_mbedtls.c|HAL_TLS_Connect(251): Performing the SSL/TLS handshake...
INF|2019-03-06 17:20:17|HAL_TLS_mbedtls.c|HAL_TLS_Connect(269): connected with /S3EUVBRJLB.iotcloud.tencentdevices.com/8883...
INF|2019-03-06 17:20:17|mqtt_client.c|IOT_MQTT_Construct(115): mqtt connect with id: jwPv6 success
INF|2019-03-06 17:20:17|mqtt_sample.c|main(340): Cloud Device Construct Success
DBG|2019-03-06 17:20:17|mqtt_client_subscribe.c|qcloud_iot_mqtt_subscribe(129): topicName=S3EUVBRJLB/demo-device/data|packet_id=21872|pUserdata=(null)
INF|2019-03-06 17:20:17|mqtt_sample.c|event_handler(82): subscribe success, packet-id=21872
DBG|2019-03-06 17:20:17|mqtt_client_publish.c|qcloud_iot_mqtt_publish(329): publish topic seq=21873|topicName=S3EUVBRJLB/demo-device/data|payload={"action": "publish_test", "count": "0"}
INF|2019-03-06 17:20:17|mqtt_sample.c|on_message_callback(139): Receive Message With topicName:S3EUVBRJLB/demo-device/data, payload:{"action": "publish_test", "count": "0"}
INF|2019-03-06 17:20:18|mqtt_sample.c|event_handler(109): publish success, packet-id=21873
INF|2019-03-06 17:20:18|mqtt_client_connect.c|qcloud_iot_mqtt_disconnect(441): mqtt disconnect!
INF|2019-03-06 17:20:18|mqtt_client.c|IOT_MQTT_Destroy(159): mqtt release!

```

#### 2. 观察消息发送
如下日志信息显示示例程序通过 MQTT 的 Publish 类型消息, 上报数据到 /{productID}/{deviceName}/data, 服务器已经收到并成功完成了该消息的处理 
```
INF|2019-03-06 17:20:18|mqtt_sample.c|event_handler(109): publish success, packet-id=21873
```

#### 3. 观察消息接收
如下日志信息显示该消息因为是到达已被订阅的 Topic, 所以又被服务器原样推送到示例程序, 并进入相应的回调函数
```
INF|2019-03-06 17:20:17|mqtt_sample.c|on_message_callback(139): Receive Message With topicName:S3EUVBRJLB/demo-device/data, payload:{"action": "publish_test", "count": "0"}
```

#### 4. 观察控制台日志
可以登录物联网通信控制台, 点击左边导航栏中的**云日志**, 查看刚才上报的消息
![](https://main.qcloudimg.com/raw/09d3ec0ec827eaa274dad8ca8e1bf04c.png)

#### 5. 执行 CoAP 示例程序
```
./coap_sample
INF|2019-03-06 17:34:28|device.c|iot_device_info_set(65): SDK_Ver: 2.3.1, Product_ID: S3EUVBRJLB, Device_Name: demo-device
INF|2019-03-06 17:34:30|coap_client.c|IOT_COAP_Construct(83): coap connect success
INF|2019-03-06 17:34:30|coap_client_message.c|coap_message_send(404): add coap message id: 53915 into wait list ret: 0
DBG|2019-03-06 17:34:30|coap_client_message.c|_coap_message_handle(297): receive coap piggy ACK message, id 53915
INF|2019-03-06 17:34:30|coap_client_auth.c|_coap_client_auth_callback(42): auth token message success, code_class: 2 code_detail: 5
DBG|2019-03-06 17:34:30|coap_client_auth.c|_coap_client_auth_callback(52): auth_token_len = 10, auth_token = SJGRYBBEGM
DBG|2019-03-06 17:34:30|coap_client_message.c|_coap_message_list_proc(148): remove the message id 53915 from list
INF|2019-03-06 17:34:30|coap_client_message.c|_coap_message_list_proc(87): remove node
INF|2019-03-06 17:34:30|coap_client.c|IOT_COAP_Construct(92): device auth successfully, connid: Vu501
INF|2019-03-06 17:34:30|coap_sample.c|main(163): topic name is S3EUVBRJLB/demo-device/data
INF|2019-03-06 17:34:30|coap_client_message.c|coap_message_send(404): add coap message id: 53916 into wait list ret: 0
DBG|2019-03-06 17:34:30|coap_sample.c|main(170): client topic has been sent, msg_id: 53916
DBG|2019-03-06 17:34:31|coap_client_message.c|_coap_message_handle(297): receive coap piggy ACK message, id 53916
INF|2019-03-06 17:34:31|coap_sample.c|event_handler(90): message received ACK, msgid: 53916
DBG|2019-03-06 17:34:31|coap_client_message.c|_coap_message_list_proc(148): remove the message id 53916 from list
INF|2019-03-06 17:34:31|coap_client_message.c|_coap_message_list_proc(87): remove node
INF|2019-03-06 17:34:31|coap_client.c|IOT_COAP_Destroy(126): coap release!
```

#### 6. 观察消息发送
如下日志信息显示示例程序通过 CoAP 上报数据到 /{productID}/{deviceName}/data 成功。
```
INF|2019-03-06 17:34:31|coap_sample.c|event_handler(90): message received ACK, msgid: 53916
```

#### 7. 执行网关实例程序
如下日志信息显示示例程序通过MQTT网关代理子设备上下线状态变化，发布和订阅消息成功。
```
./gateway_sample
INF|2019-03-06 20:18:57|device.c|iot_device_info_set(65): SDK_Ver: 2.3.1, Product_ID: NINEPLMEB6, Device_Name: Gateway-demo
DBG|2019-03-06 20:18:57|HAL_TLS_mbedtls.c|HAL_TLS_Connect(204): Connecting to /NINEPLMEB6.iotcloud.tencentdevices.com/8883...
DBG|2019-03-06 20:18:57|HAL_TLS_mbedtls.c|HAL_TLS_Connect(209): Setting up the SSL/TLS structure...
DBG|2019-03-06 20:18:57|HAL_TLS_mbedtls.c|HAL_TLS_Connect(251): Performing the SSL/TLS handshake...
INF|2019-03-06 20:18:58|HAL_TLS_mbedtls.c|HAL_TLS_Connect(269): connected with /NINEPLMEB6.iotcloud.tencentdevices.com/8883...
INF|2019-03-06 20:18:58|mqtt_client.c|IOT_MQTT_Construct(115): mqtt connect with id: 35L53 success
DBG|2019-03-06 20:18:58|mqtt_client_subscribe.c|qcloud_iot_mqtt_subscribe(129): topicName=$gateway/operation/result/NINEPLMEB6/Gateway-demo|packet_id=56164|pUserdata=(null)
DBG|2019-03-06 20:18:58|gateway_api.c|_gateway_event_handler(23): gateway sub|unsub(3) success, packet-id=56164
DBG|2019-03-06 20:18:58|gateway_api.c|IOT_Gateway_Subdev_Online(125): there is no session, create a new session
DBG|2019-03-06 20:18:58|mqtt_client_publish.c|qcloud_iot_mqtt_publish(337): publish packetID=0|topicName=$gateway/operation/NINEPLMEB6/Gateway-demo|payload={"type":"online","payload":{"devices":[{"product_id":"S3EUVBRJLB","device_name":"demo-device"}]}
INF|2019-03-06 20:18:58|gateway_common.c|_gateway_message_handler(135): client_id(S3EUVBRJLB/demo-device), online success. result 0
DBG|2019-03-06 20:18:59|mqtt_client_subscribe.c|qcloud_iot_mqtt_subscribe(129): topicName=S3EUVBRJLB/demo-device/data|packet_id=56165|pUserdata=(null)
DBG|2019-03-06 20:18:59|gateway_api.c|_gateway_event_handler(23): gateway sub|unsub(3) success, packet-id=56165
INF|2019-03-06 20:18:59|gateway_sample.c|_event_handler(101): subscribe success, packet-id=56165
DBG|2019-03-06 20:18:59|mqtt_client_publish.c|qcloud_iot_mqtt_publish(329): publish topic seq=56166|topicName=S3EUVBRJLB/demo-device/data|payload={"data":"test gateway"}
INF|2019-03-06 20:18:59|gateway_sample.c|_message_handler(152): Receive Message With topicName:S3EUVBRJLB/demo-device/data, payload:{"data":"test gateway"}
DBG|2019-03-06 20:19:00|mqtt_client_publish.c|qcloud_iot_mqtt_publish(329): publish topic seq=56167|topicName=S3EUVBRJLB/demo-device/data|payload={"data":"test gateway"}
INF|2019-03-06 20:19:00|gateway_sample.c|_event_handler(128): publish success, packet-id=56166
INF|2019-03-06 20:19:00|gateway_sample.c|_message_handler(152): Receive Message With topicName:S3EUVBRJLB/demo-device/data, payload:{"data":"test gateway"}
DBG|2019-03-06 20:19:01|mqtt_client_publish.c|qcloud_iot_mqtt_publish(337): publish packetID=0|topicName=$gateway/operation/NINEPLMEB6/Gateway-demo|payload={"type":"offline","payload":{"devices":[{"product_id":"S3EUVBRJLB","device_name":"demo-device"}]
INF|2019-03-06 20:19:01|gateway_sample.c|_event_handler(128): publish success, packet-id=56167
INF|2019-03-06 20:19:02|gateway_common.c|_gateway_message_handler(140): client_id(S3EUVBRJLB/demo-device), offline success. result 0
INF|2019-03-06 20:19:02|mqtt_client_connect.c|qcloud_iot_mqtt_disconnect(441): mqtt disconnect!
INF|2019-03-06 20:19:02|mqtt_client.c|IOT_MQTT_Destroy(159): mqtt release!
```

#### 8. 多线程环境实例程序
SDK对于MQTT接口在多线程环境下的使用有如下注意事项，详细代码用例可以参考samples/mqtt/multi_thread_mqtt_sample.c
```
1. 不允许多线程调用IOT_MQTT_Yield，IOT_MQTT_Construct以及IOT_MQTT_Destroy
2. 可以多线程调用IOT_MQTT_Publish，IOT_MQTT_Subscribe及IOT_MQTT_Unsubscribe
3. IOT_MQTT_Yield 作为从socket读取并处理MQTT报文的函数，应保证一定的执行时间，避免被长时间挂起或抢占
```

#### 9. 设备日志上报功能
从版本v2.3.1开始，SDK增加设备端日志上报功能，可将设备端的Log通过HTTP上报到云端，并可在控制台展示，方便用户远程调试、诊断及监控设备运行状况。目前该功能仅支持MQTT模式。
只要将SDK的编译宏FEATURE_LOG_UPLOAD_ENABLED置为y（默认为y），并在控制台设置上报级别，则在代码中调用Log_e/w/i/d接口的日志除了会在终端打印出来，还会上报云端并在控制台展示，如下图。
![](https://main.qcloudimg.com/raw/cae7f9e7cf1e354cfc1e3578eb6746bc.png)

上报级别设置可参见下图，Level级别越大则上报的日志越多，比如Level3(信息)会将ERROR/WARN/INFO级别的日志都上报而DEBUG级别则不上报。控制台默认为关闭状态，则表示设备端仅在MQTT连接失败的时候才会上报ERROR级别日志。
![](https://main.qcloudimg.com/raw/826b648993a267b1cc2f082148d8d073.png)

代码具体用例可以参考mqtt_sample以及qcloud_iot_export_log.h注释说明，用户除了打开编译宏开关，还需要调用IOT_Log_Init_Uploader函数进行初始化。SDK在IOT_MQTT_Yield函数中会定时进行上报，此外，用户可根据自身需要，在程序出错退出的时候调用IOT_Log_Upload(true)强制上报。同时SDK提供在HTTP通讯出错无法上报日志时的缓存和恢复正常后重新上报机制，但需要用户根据设备具体情况提供相关回调函数，如不提供回调或回调函数提供不全则该缓存机制不生效，HTTP通讯失败时日志会被丢掉。

## 四. 可变接入参数配置

可变接入参数配置：SDK 的使用可以根据具体场景需求，配置相应的参数，满足实际业务的运行。可变接入参数包括：
1. MQTT 心跳消息发送周期, 单位: ms 
2. MQTT 阻塞调用(包括连接, 订阅, 发布等)的超时时间, 单位:ms。 建议 5000 ms
3. TLS 连接握手超时时间, 单位: ms
4. MQTT 协议发送消息和接受消息的 buffer 大小默认是 512 字节，最大支持 256 KB
5. CoAP 协议发送消息和接受消息的 buffer 大小默认是 512 字节，最大支持 64 KB
6. 重连最大等待时间
修改 qcloud_iot_export.h 文件如下宏定义可以改变对应接入参数的配置。
```
/* MQTT心跳消息发送周期, 单位:ms */
#define QCLOUD_IOT_MQTT_KEEP_ALIVE_INTERNAL                         (240 * 1000)

/* MQTT 阻塞调用(包括连接, 订阅, 发布等)的超时时间, 单位:ms 建议5000ms */
#define QCLOUD_IOT_MQTT_COMMAND_TIMEOUT                             (5000)

/* TLS连接握手超时时间, 单位:ms */
#define QCLOUD_IOT_TLS_HANDSHAKE_TIMEOUT                            (5000)

/* MQTT消息发送buffer大小, 支持最大256*1024 */
#define QCLOUD_IOT_MQTT_TX_BUF_LEN                                  (512)

/* MQTT消息接收buffer大小, 支持最大256*1024 */
#define QCLOUD_IOT_MQTT_RX_BUF_LEN                                  (512)

/* COAP 发送消息buffer大小，最大支持64*1024字节 */
#define COAP_SENDMSG_MAX_BUFLEN                                     (512)

/* COAP 接收消息buffer大小，最大支持64*1024字节 */
#define COAP_RECVMSG_MAX_BUFLEN                                     (512)

/* 重连最大等待时间 */
#define MAX_RECONNECT_WAIT_INTERVAL                                 (60000)
```

##关于 SDK 的更多使用方式及接口了解, 请访问[官方 WiKi](https://github.com/tencentyun/qcloud-iot-sdk-embedded-c/wiki)