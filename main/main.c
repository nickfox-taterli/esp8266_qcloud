#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "qcloud_iot_export.h"
#include "qcloud_iot_import.h"
#include "lite-utils.h"
#include "shadow_client.h"

/* 影子操作所需MQTT的缓冲区也较大,QCLOUD_IOT_MQTT_TX_BUF_LEN和QCLOUD_IOT_MQTT_RX_BUF_LEN需要大一些. */

/* QCLOUD总钥匙ID */
#define QCLOUD_SECRETID				"AKIDb092f45G7kBPPCu8GAqIWWJsDKDsrWWI"
/* QCLOUD总钥匙密码 */
#define QCLOUD_SECRETKEY			"g9OQEvOefT2NUN2SbU6RWKr76oGRM0OW"
/* 测试路由器帐号 */
#define TEST_WIFI_SSID				"TaterLiEnt"
/* 测试路由器密码 */
#define TEST_WIFI_PASSWORD			"xxx"
/* 产品名称 */
#define QCLOUD_IOT_MY_PRODUCT_ID		"VUCGL7FQ04"
/* 设备名称 */
#define QCLOUD_IOT_MY_DEVICE_NAME		"ESP8266"
/* 设备订阅主题 */
#define QCLOUD_IOT_TOPIC_SUB			"control"
/* 设备发布主题 */
#define QCLOUD_IOT_TOPIC_PUB			"event"
/* 设备密码 */
#define QCLOUD_IOT_DEVICE_SECRET		"cCzadEUBvpWL3p7mEK+Nog=="
/* JSON缓冲区大小 */
#define MAX_LENGTH_OF_UPDATE_JSON_BUFFER	200
/* 接收缓冲区大小 */
#define MAX_RECV_LEN 				(512 + 1)
/* 发送缓冲区大小 */
#define MAX_SIZE_OF_TOPIC_CONTENT 		100

static int Shadow_Temperature_Desire = 20; /* 远程设定期望温度 */
static int Shadow_Temperature_Actual = 20; /* 采样本地实际温度(没有实际做采样) */

static DeviceProperty Shadow_Temperature_Actual_Prop; /* 实际温度描述符 */
static DeviceProperty Shadow_Temperature_Desire_Prop; /* 期望温度描述符 */

static bool Temperature_Desire_Set_Signal = false; /* 是否改变了温度,如果是,则需要读出. */

static MQTTEventType Shadow_Subscribe_Event_Result = MQTT_EVENT_UNDEF; /* 当前状态 */
static bool Shadow_Sync_Finish = false; /* 开机同步结果 */

char Shadow_Document_Buffer[MAX_LENGTH_OF_UPDATE_JSON_BUFFER]; /* 影子缓冲区 */
size_t Shadow_Document_Buffersize = sizeof(Shadow_Document_Buffer) / sizeof(Shadow_Document_Buffer[0]); /* 影子缓冲区大小 */

/* WIFI连接位 */
static const int CONNECTED_BIT = BIT0;
static EventGroupHandle_t wifi_event_group;

nvs_handle nvs;

/**
 * 发布数据
 */
static int MQTT_Publish_Topic(void *client, char *topic_content)
{
    char topicName[128] = {0};

    sprintf(topicName, "%s/%s/%s", QCLOUD_IOT_MY_PRODUCT_ID, QCLOUD_IOT_MY_DEVICE_NAME, QCLOUD_IOT_TOPIC_PUB);

    PublishParams pub_params = DEFAULT_PUB_PARAMS;
    pub_params.qos = QOS1;
    pub_params.payload = topic_content;
    pub_params.payload_len = strlen(topic_content);

    return IOT_MQTT_Publish(client, topicName, &pub_params);
}

/**
 * 每一次Shadow调用成功与否都回调此函数.
 */
static void Shadow_Request_Handler(void *pClient, Method method, RequestAck status, const char *jsonDoc, void *userData)
{
    if (status == ACK_ACCEPTED && method == UPDATE)
    {
        if (Temperature_Desire_Set_Signal)  /* 需要更新期望数据,更新后就要取消标志. */
            Temperature_Desire_Set_Signal = false;
    }
    else if (status == ACK_ACCEPTED && method == GET)
    {
        Shadow_Sync_Finish = true;
    }
    else if (status == ACK_TIMEOUT && method == UPDATE)
    {
        esp_restart(); /* 更新失败,通常是因为Socket意外关闭,或者WIFI不稳定,不如重启. */
    }
    else if(status == ACK_TIMEOUT && method == GET && Shadow_Sync_Finish == false)
    {
        esp_restart(); /* 开机同步失败,通常网络实在太差了. */
    }
}

/**
 * MQTT消息接收处理函数
 */
static void IOT_Subscribe_Callback(void *pClient, MQTTMessage *message, void *userData)
{
    if (message == NULL)
        return;

    Log_i("Receive Message With topicName:%.*s, payload:%.*s", (int) message->topic_len, message->ptopic, (int) message->payload_len, (char *) message->payload);

}

/**
 * 期望温度数据收到
 */
static void Temperature_Desire_Set_Callback(void *pClient, const char *jsonResponse, uint32_t responseLen, DeviceProperty *context)
{
    esp_err_t err = nvs_flash_init();

    if (context != NULL)
    {
        Log_i("Desire Temperature : %d", *(int *) context->data);
        err = nvs_open("nvs", NVS_READWRITE, &nvs);
        if (err == ESP_OK)
        {
            nvs_set_i32(nvs, "temperature", *(int *) context->data);
            nvs_commit(nvs);
        }
        Temperature_Desire_Set_Signal = true;
    }
}

/**
 * MQTT包到达回调
 */
static void Mqtt_Event_Handler(void *pclient, void *handle_context, MQTTEventMsg *msg)
{
    /* uintptr_t packet_id = (uintptr_t)msg->msg; */

    switch(msg->event_type)
    {
    case MQTT_EVENT_UNDEF:
        break;

    case MQTT_EVENT_DISCONNECT:
        break;

    case MQTT_EVENT_RECONNECT:
        break;

    case MQTT_EVENT_SUBCRIBE_SUCCESS:
        Shadow_Subscribe_Event_Result = msg->event_type;
        break;

    case MQTT_EVENT_SUBCRIBE_TIMEOUT:
        Shadow_Subscribe_Event_Result = msg->event_type;
        break;

    case MQTT_EVENT_SUBCRIBE_NACK:
        Shadow_Subscribe_Event_Result = msg->event_type;
        break;

    case MQTT_EVENT_PUBLISH_SUCCESS:
        break;

    case MQTT_EVENT_PUBLISH_TIMEOUT:
        break;

    case MQTT_EVENT_PUBLISH_NACK:
        break;
    default:
        break;
    }
}

/**
 * 上报影子数据.
 */
static int Report_Temperature_Actual(void *client)
{
    /* Shadow_Temperature_Actual_Prop 就是实际温度,在主函数内应该修改.*/
    int rc = IOT_Shadow_JSON_ConstructReport(client, Shadow_Document_Buffer, Shadow_Document_Buffersize, 1, &Shadow_Temperature_Actual_Prop);

    if (rc != QCLOUD_ERR_SUCCESS)
    {
        return rc;
    }

    rc = IOT_Shadow_Update(client, Shadow_Document_Buffer, Shadow_Document_Buffersize, Shadow_Request_Handler, NULL, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);

    if (rc != QCLOUD_ERR_SUCCESS)
    {
        return rc;
    }

    return rc;
}

/**
 * 收到影子调用,然后取出数据.
 */
static int Read_Temperature_Disire(void *client)
{
    /* 收到信息,需要设置完属性,然后清空期望,否则一直存在,直到主动清除. */

    int rc;
    rc = IOT_Shadow_JSON_ConstructReportAndDesireAllNull(client, Shadow_Document_Buffer, Shadow_Document_Buffersize, 1, &Shadow_Temperature_Desire_Prop);

    if (rc != QCLOUD_ERR_SUCCESS)
    {
        return rc;
    }

    rc = IOT_Shadow_Update(client, Shadow_Document_Buffer, Shadow_Document_Buffersize, Shadow_Request_Handler, NULL, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);

    if (rc != QCLOUD_ERR_SUCCESS)
    {
        return rc;
    }

    return rc;
}

/**
 * 订阅主题
 */
static int IOT_Subscribe_Topics(void *client)
{
    static char topic_name[128] = {0};

    HAL_Snprintf(topic_name, sizeof(topic_name), "%s/%s/%s", QCLOUD_IOT_MY_PRODUCT_ID, QCLOUD_IOT_MY_DEVICE_NAME, QCLOUD_IOT_TOPIC_SUB);

    SubscribeParams sub_params = DEFAULT_SUB_PARAMS;
    sub_params.qos = QOS1;
    sub_params.on_message_handler = IOT_Subscribe_Callback;
    return IOT_Shadow_Subscribe(client, topic_name, &sub_params);
}

int mqtt_loop(void)
{
    int rc = 0;
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    err = nvs_open("nvs", NVS_READWRITE, &nvs);
    if (err == ESP_OK)
    {
        nvs_get_i32(nvs, "temperature", &Shadow_Temperature_Desire);
        Log_i("Desire Temperature : %d", Shadow_Temperature_Desire);
        nvs_close(nvs);
    }

    IOT_Log_Set_Level(DEBUG);

    ShadowInitParams IOT_Init_Params = DEFAULT_SHAWDOW_INIT_PARAMS;

    IOT_Init_Params.device_name = QCLOUD_IOT_MY_DEVICE_NAME;
    IOT_Init_Params.product_id = QCLOUD_IOT_MY_PRODUCT_ID;

    IOT_Init_Params.device_secret = QCLOUD_IOT_DEVICE_SECRET;

    IOT_Init_Params.auto_connect_enable = 1;
    IOT_Init_Params.event_handle.h_fp = Mqtt_Event_Handler;


    void *client = IOT_Shadow_Construct(&IOT_Init_Params);
    if (client != NULL)
    {
        /* 链接OK */
        Log_i("Cloud Device Construct Success");
    }
    else
    {
        /* 链接失败 */
        Log_e("Cloud Device Construct Failed");
        esp_restart();
    }

    /* 由于资源限制,类型只能是JINT/JUNIT.宽度是8/16/32. */

    /* 实际温度: 设备 ---> 影子 <---> API(远程) */
    Shadow_Temperature_Actual_Prop.key = "temperatureActual";
    Shadow_Temperature_Actual_Prop.data = &Shadow_Temperature_Actual;
    Shadow_Temperature_Actual_Prop.type = JINT8;
    /* 预期温度: API(远程) ---> 影子 <---> 设备 */
    Shadow_Temperature_Desire_Prop.key = "temperatureDesire";
    Shadow_Temperature_Desire_Prop.data = &Shadow_Temperature_Desire;
    Shadow_Temperature_Desire_Prop.type = JINT8;
    /* 当远程预期温度被设置时,访问回调. */
    IOT_Shadow_Register_Property(client, &Shadow_Temperature_Desire_Prop, Temperature_Desire_Set_Callback);


    /* 先同步一下远程版本. */
    IOT_Shadow_Get(client, Shadow_Request_Handler, NULL, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);
    while(Shadow_Sync_Finish == false)
    {
        if(rc++ > 5) esp_restart(); /* 初始化太久,一般网络出毛病了. */
        Log_i("Wait for Shadow Sync");
        IOT_Shadow_Yield(client, 200);
        vTaskDelay(100);
    }
    /* 订阅默认的主题 */
    rc = IOT_Subscribe_Topics(client);
    if (rc < 0)
    {
        Log_e("Client Subscribe Topic Failed: %d", rc);
        vTaskDelay(5000);
        esp_restart();
    }

    while (IOT_Shadow_IsConnected(client) || rc == QCLOUD_ERR_MQTT_ATTEMPTING_RECONNECT || rc == QCLOUD_ERR_MQTT_RECONNECTED)
    {

        rc = IOT_Shadow_Yield(client, 200);

        if (rc == QCLOUD_ERR_MQTT_ATTEMPTING_RECONNECT)
        {
            vTaskDelay(1);
            continue;
        }
        else if (rc != QCLOUD_ERR_SUCCESS)
        {
            esp_restart();
        }

        /* 收到需要设置的数据 */
        if (Temperature_Desire_Set_Signal)
            Read_Temperature_Disire(client);

        if (Temperature_Desire_Set_Signal == false)
        {
            if(Shadow_Temperature_Desire > Shadow_Temperature_Actual) Shadow_Temperature_Actual++;
            if(Shadow_Temperature_Desire < Shadow_Temperature_Actual) Shadow_Temperature_Actual--;
            Report_Temperature_Actual(client);
            /* 因为是异步调用,调用下一个发送前,先延迟一下. */
            vTaskDelay(1000);
            MQTT_Publish_Topic(((Qcloud_IoT_Shadow *)client)->mqtt, "{\"action\":\"Hello\"}");
        }
        vTaskDelay(1000);
    }

    rc = IOT_Shadow_Destroy(client);

    return rc;
}

void mqtt_main(void *parm)
{
    EventBits_t uxBits;

    while (1)
    {
        uxBits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, true, false, portMAX_DELAY);

        if (uxBits & CONNECTED_BIT)
        {
            mqtt_loop();
        }
    }
}


static void wifi_connection(void)
{
    wifi_config_t wifi_config =
    {
        .sta = {
            .ssid = TEST_WIFI_SSID,
            .password = TEST_WIFI_PASSWORD,
        },
    };

    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_connect();
}


static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        /* 系统启动 */
        xTaskCreate(mqtt_main, "mqtt_main", 5120, NULL, 3, NULL);
        wifi_connection();
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
        /* 已经链接,IP字符串:ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip) */
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);

        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* 断开 */
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        esp_wifi_connect();
        break;

    default:
        break;
    }

    return ESP_OK;
}

static void wifi_initialise(void)
{
    tcpip_adapter_init();

    wifi_event_group = xEventGroupCreate();
    esp_event_loop_init(wifi_event_handler, NULL);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
}

void app_main()
{
    nvs_flash_init();
    wifi_initialise();
}
