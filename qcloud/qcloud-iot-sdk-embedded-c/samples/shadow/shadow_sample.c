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
#include <signal.h>
#include "qcloud_iot_export.h"

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

static char sg_shadow_update_buffer[200];
size_t sg_shadow_update_buffersize = sizeof(sg_shadow_update_buffer) / sizeof(sg_shadow_update_buffer[0]);

static DeviceProperty sg_shadow_property;
static int sg_current_update_count = 0;
static bool sg_delta_arrived = false;

void OnDeltaCallback(void *pClient, const char *pJsonValueBuffer, uint32_t valueLength, DeviceProperty *pProperty) {
	int rc = IOT_Shadow_JSON_ConstructDesireAllNull(pClient, sg_shadow_update_buffer, sg_shadow_update_buffersize);

	if (rc == QCLOUD_ERR_SUCCESS) {
		sg_delta_arrived = true;
	}
	else {
		Log_e("construct desire failed, err: %d", rc);
	}
}

void OnShadowUpdateCallback(void *pClient, Method method, RequestAck requestAck, const char *pJsonDocument, void *pUserdata) {
	Log_i("recv shadow update response, response ack: %d", requestAck);
}

int demo_device_shadow()
{
	int rc = QCLOUD_ERR_FAILURE;

	void* shadow_client = NULL;

	ShadowInitParams init_params = DEFAULT_SHAWDOW_INIT_PARAMS;
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
	init_params.auto_connect_enable = 1;

	shadow_client = IOT_Shadow_Construct(&init_params);
	if (shadow_client == NULL) {
		Log_e("shadow client constructed failed.");
		return QCLOUD_ERR_FAILURE;
	}

	//注册delta属性
	sg_shadow_property.key = "updateCount";
	sg_shadow_property.data = &sg_current_update_count;
	sg_shadow_property.type = JINT32;
	rc = IOT_Shadow_Register_Property(shadow_client, &sg_shadow_property, OnDeltaCallback);
	if (rc != QCLOUD_ERR_SUCCESS) {
		rc = IOT_Shadow_Destroy(shadow_client);
		Log_e("register device shadow property failed, err: %d", rc);
		return rc;
	}

	//进行Shdaow Update操作的之前，最后进行一次同步操作，否则可能本机上shadow version和云上不一致导致Shadow Update操作失败
	rc = IOT_Shadow_Get_Sync(shadow_client, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);
	if (rc != QCLOUD_ERR_SUCCESS) {
		Log_e("get device shadow failed, err: %d", rc);
		return rc;
	}

	while (IOT_Shadow_IsConnected(shadow_client) || QCLOUD_ERR_MQTT_ATTEMPTING_RECONNECT == rc ||
			QCLOUD_ERR_MQTT_RECONNECTED == rc || QCLOUD_ERR_SUCCESS == rc) {

		rc = IOT_Shadow_Yield(shadow_client, 200);

		if (QCLOUD_ERR_MQTT_ATTEMPTING_RECONNECT == rc) {
			sleep(1);
			continue;
		}
		else if (rc != QCLOUD_ERR_SUCCESS && rc != QCLOUD_ERR_MQTT_RECONNECTED) {
			Log_e("exit with error: %d", rc);
			return rc;
		}

		if (sg_delta_arrived) {
			rc = IOT_Shadow_Update_Sync(shadow_client, sg_shadow_update_buffer, sg_shadow_update_buffersize, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);
			sg_delta_arrived = false;
			if (rc == QCLOUD_ERR_SUCCESS) 
				Log_i("shadow update success");
		}

		IOT_Shadow_JSON_ConstructReport(shadow_client, sg_shadow_update_buffer, sg_shadow_update_buffersize, 1, &sg_shadow_property);
		rc = IOT_Shadow_Update(shadow_client, sg_shadow_update_buffer, sg_shadow_update_buffersize, OnShadowUpdateCallback, NULL, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);
		sg_current_update_count++;

		// sleep for some time in seconds
		sleep(1);
	}

	Log_e("loop exit with error: %d", rc);

	rc = IOT_Shadow_Destroy(shadow_client);

	return rc;
}

int main()
{
	IOT_Log_Set_Level(DEBUG);
    
    // to avoid process crash when writing to a broken socket
    signal(SIGPIPE, SIG_IGN);

	demo_device_shadow();

	return 0;
}
