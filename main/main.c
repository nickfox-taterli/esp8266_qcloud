/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS chips only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "qcloud_iot_export.h"
#include "qcloud_iot_import.h"

/* 测试路由器帐号  */
#define TEST_WIFI_SSID                 "xxxxx"
/* 测试路由器密码  */
#define TEST_WIFI_PASSWORD             "xxxxx"
/* 产品名称, 与云端同步设备状态时需要  */
#define QCLOUD_IOT_MY_PRODUCT_ID       "VUCGL7FQ04"
/* 设备名称, 与云端同步设备状态时需要 */
#define QCLOUD_IOT_MY_DEVICE_NAME      "ESP8266"

#define MAX_SIZE_OF_TOPIC_CONTENT 100

#define QCLOUD_IOT_PSK                  "cCzadEUBvpWL3p7mEK+Nog=="

static const int CONNECTED_BIT = BIT0;
static const char* TAG = "esp32-qcloud-mqtt-demo";
static int sg_count = 0;
static int sg_sub_packet_id = -1;
static EventGroupHandle_t wifi_event_group;

bool log_handler(const char* message)
{
    //实现日志回调的写方法
    //实现内容后请返回true
    return false;
}

void mqtt_demo_event_handler(void* pclient, void* handle_context, MQTTEventMsg* msg)
{
    MQTTMessage* mqtt_messge = (MQTTMessage*)msg->msg;
    uintptr_t packet_id = (uintptr_t)msg->msg;

    switch (msg->event_type) {
        case MQTT_EVENT_UNDEF:
            ESP_LOGI(TAG, "undefined event occur.");
            break;

        case MQTT_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "MQTT disconnect.");
            break;

        case MQTT_EVENT_RECONNECT:
            ESP_LOGI(TAG, "MQTT reconnect.");
            break;

        case MQTT_EVENT_PUBLISH_RECVEIVED:
            ESP_LOGI(TAG, "topic message arrived but without any related handle: topic=%d-%s, topic_msg=%d-%s",
                     mqtt_messge->topic_len,
                     (char*)mqtt_messge->ptopic,
                     mqtt_messge->payload_len,
                     (char*)mqtt_messge->payload);
            break;

        case MQTT_EVENT_SUBCRIBE_SUCCESS:
            ESP_LOGI(TAG, "subscribe success, packet-id=%u", (unsigned int)packet_id);
            sg_sub_packet_id = packet_id;
            break;

        case MQTT_EVENT_SUBCRIBE_TIMEOUT:
            ESP_LOGI(TAG, "subscribe wait ack timeout, packet-id=%u", (unsigned int)packet_id);
            sg_sub_packet_id = packet_id;
            break;

        case MQTT_EVENT_SUBCRIBE_NACK:
            ESP_LOGI(TAG, "subscribe nack, packet-id=%u", (unsigned int)packet_id);
            sg_sub_packet_id = packet_id;
            break;

        case MQTT_EVENT_UNSUBCRIBE_SUCCESS:
            ESP_LOGI(TAG, "unsubscribe success, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_UNSUBCRIBE_TIMEOUT:
            ESP_LOGI(TAG, "unsubscribe timeout, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_UNSUBCRIBE_NACK:
            ESP_LOGI(TAG, "unsubscribe nack, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_PUBLISH_SUCCESS:
            ESP_LOGI(TAG, "publish success, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_PUBLISH_TIMEOUT:
            ESP_LOGI(TAG, "publish timeout, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_PUBLISH_NACK:
            ESP_LOGI(TAG, "publish nack, packet-id=%u", (unsigned int)packet_id);
            break;

        default:
            ESP_LOGI(TAG, "Should NOT arrive here.");
            break;
    }
}

/**
 * MQTT消息接收处理函数
 *
 * @param topicName         topic主题
 * @param topicNameLen      topic长度
 * @param message           已订阅消息的结构
 * @param userData         消息负载
 */
static void on_message_callback(void* pClient, MQTTMessage* message, void* userData)
{
    if (message == NULL) {
        return;
    }

    ESP_LOGI(TAG, "Receive Message With topicName:%.*s, payload:%.*s",
             (int) message->topic_len, message->ptopic, (int) message->payload_len, (char*) message->payload);
}

/**
 * 设置MQTT connet初始化参数
 *
 * @param initParams MQTT connet初始化参数
 *
 * @return 0: 参数初始化成功  非0: 失败
 */

static int setup_connect_init_params(MQTTInitParams* initParams)
{
    initParams->device_name = QCLOUD_IOT_MY_DEVICE_NAME;
    initParams->product_id = QCLOUD_IOT_MY_PRODUCT_ID;

#ifndef NOTLS_ENABLED
#ifdef ASYMC_ENCRYPTION_ENABLED
    /* 使用非对称加密*/
    sprintf(sg_cert_file, "%s/%s", base_path, QCLOUD_IOT_CERT_FILENAME);
    sprintf(sg_key_file, "%s/%s", base_path, QCLOUD_IOT_KEY_FILENAME);

    initParams->cert_file = sg_cert_file;
    initParams->key_file = sg_key_file;
#else
    initParams->device_secret = QCLOUD_IOT_PSK;
#endif
#endif

    initParams->command_timeout = QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;
    initParams->keep_alive_interval_ms = QCLOUD_IOT_MQTT_KEEP_ALIVE_INTERNAL;

    initParams->auto_connect_enable = 1;
    initParams->event_handle.h_fp = mqtt_demo_event_handler;
    initParams->event_handle.context = NULL;

    return QCLOUD_ERR_SUCCESS;
}

/**
 * 发送topic消息
 *
 */
static int _publish_msg(void* client)
{
    char topicName[128] = {0};
    sprintf(topicName, "%s/%s/%s", QCLOUD_IOT_MY_PRODUCT_ID, QCLOUD_IOT_MY_DEVICE_NAME, "event");

    PublishParams pub_params = DEFAULT_PUB_PARAMS;
    pub_params.qos = QOS1;

    char topic_content[MAX_SIZE_OF_TOPIC_CONTENT + 1] = {0};

    int size = HAL_Snprintf(topic_content, sizeof(topic_content), "{\"action\": \"publish_test\", \"count\": \"%d\"}", sg_count++);

    if (size < 0 || size > sizeof(topic_content) - 1) {
        ESP_LOGE(TAG, "payload content length not enough! content size:%d  buf size:%d", size, (int)sizeof(topic_content));
        return -3;
    }

    pub_params.payload = topic_content;
    pub_params.payload_len = strlen(topic_content);

    return IOT_MQTT_Publish(client, topicName, &pub_params);
}

/**
 * 订阅关注topic和注册相应回调处理
 *
 */
static int _register_subscribe_topics(void* client)
{
    static char topic_name[128] = {0};
    static char user_data[128] = {0};
    int size = HAL_Snprintf(topic_name, sizeof(topic_name), "%s/%s/%s", QCLOUD_IOT_MY_PRODUCT_ID, QCLOUD_IOT_MY_DEVICE_NAME, "control");

    if (size < 0 || size > sizeof(topic_name) - 1) {
        Log_e("topic content length not enough! content size:%d  buf size:%d", size, (int)sizeof(topic_name));
        return QCLOUD_ERR_FAILURE;
    }

    SubscribeParams sub_params = DEFAULT_SUB_PARAMS;
    sub_params.on_message_handler = on_message_callback;
    sub_params.user_data = user_data;
    return IOT_MQTT_Subscribe(client, topic_name, &sub_params);
}

void qcloud_mqtt_demo(void)
{
    IOT_Log_Set_Level(DEBUG);
    IOT_Log_Set_MessageHandler(log_handler);

    int rc;

    MQTTInitParams init_params = DEFAULT_MQTTINIT_PARAMS;

    rc = setup_connect_init_params(&init_params);

    if (rc != QCLOUD_ERR_SUCCESS) {
        ESP_LOGE(TAG, "setup_connect_init_params Failed");
        return;
    }

    void* client = IOT_MQTT_Construct(&init_params);

    if (client != NULL) {
        ESP_LOGI(TAG, "Cloud Device Construct Success");
    } else {
        ESP_LOGE(TAG, "Cloud Device Construct Failed");
        return;
    }

    rc = _register_subscribe_topics(client);

    if (rc < 0) {
        ESP_LOGE(TAG, "Client Subscribe Topic Failed: %d", rc);
        return;
    }

    do {
        if (sg_sub_packet_id > 0) {
            if (_publish_msg(client) < 0) {
                ESP_LOGE(TAG, "client publish topic failed :%d.", rc);
            }
        }

        rc = IOT_MQTT_Yield(client, 5000);

        if (rc == QCLOUD_ERR_MQTT_ATTEMPTING_RECONNECT) {
            vTaskDelay(1000);
            continue;
        } else if (rc != QCLOUD_ERR_SUCCESS && rc != QCLOUD_ERR_MQTT_RECONNECTED) {
            ESP_LOGE(TAG, "exit with error: %d", rc);
            break;
        }

    } while (1);

    IOT_MQTT_Destroy(&client);
}

void qcloud_example_task(void* parm)
{
    EventBits_t uxBits;

    while (1) {
        uxBits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, true, false, portMAX_DELAY);

        if (uxBits & CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi Connected to ap");
            qcloud_mqtt_demo();
        }
    }
}

static void wifi_connection(void)
{
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = TEST_WIFI_SSID,
            .password = TEST_WIFI_PASSWORD,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

    esp_wifi_connect();
}

static esp_err_t event_handler(void* ctx, system_event_t* event)
{
    ESP_LOGI(TAG, "event = %d", event->event_id);

    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
            xTaskCreate(qcloud_example_task, "qcloud_example_task", 10240, NULL, 3, NULL);
            wifi_connection();
            break;

        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG, "Got IPv4[%s]", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);

            break;

        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            esp_wifi_connect();
            break;

        default:
            break;
    }

    return ESP_OK;
}

static void esp32_wifi_initialise(void)
{
    tcpip_adapter_init();

    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    esp32_wifi_initialise();
}
