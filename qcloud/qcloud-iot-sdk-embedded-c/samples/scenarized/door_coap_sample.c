/*
 * Tencent is pleased to support the open source community by making IoT Hub available.
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

 * Licensed under the MIT License (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT

 * Unless required by applicable law or agreed to in writing, software distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#include "qcloud_iot_export.h"
#include "qcloud_iot_import.h"

#define MAX_SIZE_OF_TOPIC_CONTENT 100

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

void printUsage()
{
    printf("1. ./door_coap come_home [targetDeviceName]\n");
    printf("2. ./door_coap leave_home [targetDeviceName]\n");
}


void event_handler(void *pcontext, CoAPEventMessage *message)
{
	switch (message->event_type) {
		case COAP_EVENT_RECEIVE_ACK:
			Log_i("message received ACK, msgid: %d", (unsigned)(uintptr_t)message->message);
			break;
		case COAP_EVENT_RECEIVE_RESPCONTENT:	/* 暂不支持消息交互，仅支持数据上报 */
			break;
		case COAP_EVENT_UNAUTHORIZED:
			Log_i("coap client auth token expired or invalid, msgid: %d", (unsigned)(uintptr_t)message->message);
			break;
		case COAP_EVENT_FORBIDDEN:
			Log_i("coap URI is invalid for this device, msgid: %d", (unsigned)(uintptr_t)message->message);
			break;
		case COAP_EVENT_INTERNAL_SERVER_ERROR:
			Log_i("coap server internal error, msgid: %d", (unsigned)(uintptr_t)message->message);
			break;
		case COAP_EVENT_ACK_TIMEOUT:
			Log_i("message receive ACK timeout, msgid: %d", (unsigned)(uintptr_t)message->message);
			break;
		case COAP_EVENT_SEPRESP_TIMEOUT:		/* 暂不支持消息交互，仅支持数据上报 */
			break;
		default:
			Log_e("unrecogonized event type: %d", message->event_type);
			break;
	}
}

int main(int argc, char **argv)
{
    if(argc != 3)
    {
        printUsage();
        return -1;
    }

	IOT_Log_Set_Level(DEBUG);

	CoAPInitParams init_params = DEFAULT_COAPINIT_PARAMS;
	init_params.product_id = QCLOUD_IOT_MY_PRODUCT_ID;
	init_params.device_name = QCLOUD_IOT_MY_DEVICE_NAME;

#ifdef AUTH_MODE_CERT
	// 获取CA证书、客户端证书以及私钥文件的路径
	char certs_dir[PATH_MAX + 1] = "certs";
	char current_path[PATH_MAX + 1];
	char *cwd = getcwd(current_path, sizeof(current_path));
	if (cwd == NULL)
	{
		Log_e("getcwd return NULL");
		return QCLOUD_ERR_FAILURE;
	}
	sprintf(sg_cert_file, "%s/%s/%s", current_path, certs_dir, QCLOUD_IOT_CERT_FILENAME);
	sprintf(sg_key_file, "%s/%s/%s", current_path, certs_dir, QCLOUD_IOT_KEY_FILENAME);

	init_params.cert_file = sg_cert_file;
	init_params.key_file = sg_key_file;
#else
	init_params.device_secret = QCLOUD_IOT_DEVICE_SECRET;
#endif

	init_params.command_timeout = QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;
	init_params.event_handle.h_fp = event_handler;
	init_params.max_retry_count = 3;

	void *coap_client = IOT_COAP_Construct(&init_params);
	if (coap_client == NULL) {
		Log_e("COAP Client construct failed.");
		return QCLOUD_ERR_FAILURE;
	}
	else {
		Log_e("%p", coap_client);
	}

	int   rc = QCLOUD_ERR_SUCCESS;
    char* action = argv[1];
    char* target_device_name = argv[2];

    if(strcmp(action, "come_home") == 0 || strcmp(action, "leave_home") == 0)
    {
        char topic_content[MAX_SIZE_OF_TOPIC_CONTENT + 1] = {0};
    	SendMsgParams send_params = DEFAULT_SENDMSG_PARAMS;

        int size = HAL_Snprintf(topic_content, sizeof(topic_content), "{\"action\": \"%s\", \"targetDevice\": \"%s\"}", action, target_device_name);
        if (size < 0 || size > sizeof(topic_content) - 1)
        {
            Log_e("payload content length not enough! content size:%d  buf size:%d", size, (int)sizeof(topic_content));
            return -3;
        }

        send_params.pay_load = topic_content;
    	send_params.pay_load_len = strlen(topic_content);

        char topicName[128] = "";
        sprintf(topicName, "%s/%s/event", QCLOUD_IOT_MY_PRODUCT_ID, QCLOUD_IOT_MY_DEVICE_NAME);
        Log_i("topic name is %s", topicName);

        rc = IOT_COAP_SendMessage(coap_client, topicName, &send_params);
	    if (rc < 0) {
	        Log_e("client publish topic failed :%d.", rc);
	    }
	    else {
	    	Log_d("client topic has been sent, msg_id: %d", rc);
	    }

    	rc = IOT_COAP_Yield(coap_client, 200);

		if (rc != QCLOUD_ERR_SUCCESS){
			Log_e("exit with error: %d", rc);
		}
    }
    else
    {
        printUsage();
        return -2;
    }

    IOT_COAP_Destroy(&coap_client);

    return QCLOUD_ERR_SUCCESS;
}
