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
#include <unistd.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>

#include "qcloud_iot_export.h"
#include "qcloud_iot_import.h"

/*
 * 多线程环境使用SDK注意事项：
 * 1. 不允许多线程调用IOT_MQTT_Yield，IOT_MQTT_Construct以及IOT_MQTT_Destroy
 * 2. 可以多线程调用IOT_MQTT_Publish，IOT_MQTT_Subscribe及IOT_MQTT_Unsubscribe
 * 3. IOT_MQTT_Yield 作为从socket读取并处理MQTT报文的函数，应保证一定的执行时间，避免被长时间挂起或抢占
 */

/*
 * 本示例程序基于Linux pthread环境对MQTT接口函数进行多线程测试，注意该测试设备数据格式应为自定义，非JSON格式
 */

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

#define MAX_SIZE_OF_TOPIC_CONTENT 100
#define MAX_PUB_THREAD_COUNT 5
#define PUBLISH_COUNT 10
#define THREAD_SLEEP_INTERVAL_USEC 500000
#define CONNECT_MAX_ATTEMPT_COUNT 3
#define RX_RECEIVE_PERCENTAGE 99.0f
#define INTEGRATION_TEST_TOPIC ""QCLOUD_IOT_MY_PRODUCT_ID"/"QCLOUD_IOT_MY_DEVICE_NAME"/data"	// 需要创建设备的时候配置权限

static bool sg_terminate_yield_thread;
static bool sg_terminate_subUnsub_thread;

static unsigned int sg_countArray[MAX_PUB_THREAD_COUNT][PUBLISH_COUNT];	// 订阅回调函数中，每次成功收订阅topic返回的数据的时候，记录每个订阅的成功次数
static unsigned int sg_rxMsgBufferTooBigCounter;                        // 记录收到消息长度过长的次数
static unsigned int sg_rxUnexpectedNumberCounter;                       // 记录收到错误消息的次数
static unsigned int sg_rePublishCount;									// 记录重新发布的次数
static unsigned int sg_wrongYieldCount;									// 记录yield失败的次数
static unsigned int sg_threadStatus[MAX_PUB_THREAD_COUNT];				// 记录所有线程的状态

typedef struct ThreadData {
	int threadId;
	void *client;
} ThreadData;


void event_handler(void *pclient, void *handle_context, MQTTEventMsg *msg) {
	MQTTMessage* mqtt_messge = (MQTTMessage*)msg->msg;
	uintptr_t packet_id = (uintptr_t)msg->msg;

	switch(msg->event_type) {
		case MQTT_EVENT_UNDEF:
			Log_i("undefined event occur.");
			break;

		case MQTT_EVENT_DISCONNECT:
			Log_i("MQTT disconnect.");
			break;

		case MQTT_EVENT_RECONNECT:
			Log_i("MQTT reconnect.");
			break;

		case MQTT_EVENT_PUBLISH_RECVEIVED:
			Log_i("topic message arrived but without any related handle: topic=%.*s, topic_msg=%.*s",
					  mqtt_messge->topic_len,
					  mqtt_messge->ptopic,
					  mqtt_messge->payload_len,
					  mqtt_messge->payload);
			break;
		case MQTT_EVENT_SUBCRIBE_SUCCESS:
			Log_i("subscribe success, packet-id=%u", (unsigned int)packet_id);			
			break;

		case MQTT_EVENT_SUBCRIBE_TIMEOUT:
			Log_i("subscribe wait ack timeout, packet-id=%u", (unsigned int)packet_id);
			break;

		case MQTT_EVENT_SUBCRIBE_NACK:
			Log_i("subscribe nack, packet-id=%u", (unsigned int)packet_id);
			break;

		case MQTT_EVENT_UNSUBCRIBE_SUCCESS:
			Log_i("unsubscribe success, packet-id=%u", (unsigned int)packet_id);
			break;

		case MQTT_EVENT_UNSUBCRIBE_TIMEOUT:
			Log_i("unsubscribe timeout, packet-id=%u", (unsigned int)packet_id);
			break;

		case MQTT_EVENT_UNSUBCRIBE_NACK:
			Log_i("unsubscribe nack, packet-id=%u", (unsigned int)packet_id);
			break;

		case MQTT_EVENT_PUBLISH_SUCCESS:
			Log_i("publish success, packet-id=%u", (unsigned int)packet_id);
			break;

		case MQTT_EVENT_PUBLISH_TIMEOUT:
			Log_i("publish timeout, packet-id=%u", (unsigned int)packet_id);
			break;

		case MQTT_EVENT_PUBLISH_NACK:
			Log_i("publish nack, packet-id=%u", (unsigned int)packet_id);
			break;
		default:
			Log_i("Should NOT arrive here.");
			break;
	}
}

static int _setup_connect_init_params(MQTTInitParams* initParams)
{
	initParams->device_name = QCLOUD_IOT_MY_DEVICE_NAME;
	initParams->product_id = QCLOUD_IOT_MY_PRODUCT_ID;

#ifdef AUTH_MODE_CERT
	/* 使用非对称加密*/
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

	initParams->cert_file = sg_cert_file;
	initParams->key_file = sg_key_file;
#else
	initParams->device_secret = QCLOUD_IOT_DEVICE_SECRET;
#endif


	initParams->command_timeout = QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;
	initParams->keep_alive_interval_ms = QCLOUD_IOT_MQTT_KEEP_ALIVE_INTERNAL;

	initParams->auto_connect_enable = 1;
	initParams->event_handle.h_fp = event_handler;

    return QCLOUD_ERR_SUCCESS;
}

/**
 * 订阅topic回调函数，解析payload报文- 自定义格式，非JSON格式
 */
static void _mqtt_message_handler(void *pClient, MQTTMessage *message, void *userData) 
{
	if (message == NULL) {
		return;
	}

	if(MAX_SIZE_OF_TOPIC_CONTENT >= message->payload_len) {
		/* 解析 payload */
		char tempBuf[MAX_SIZE_OF_TOPIC_CONTENT + 1] = {0};
		unsigned int tempRow = 0, tempCol = 0;
        char *temp = NULL;		
        
		HAL_Snprintf(tempBuf, message->payload_len + 1, "%s", (char *)message->payload);
		Log_d("Message received : %s", tempBuf);
		temp = strtok(tempBuf, " ,:");
		if(NULL == temp) {
			return;
		}
		temp = strtok(NULL, " ,:");
		if(NULL == temp) {
			return;
		}
		tempRow = atoi(temp);
		temp = strtok(NULL, " ,:");
		if(NULL == temp) {
			return;
		}
		temp = strtok(NULL, " ,:");
		if(NULL == temp) {
			return;
		}
		tempCol = atoi(temp);

		if(((tempRow - 1) < MAX_PUB_THREAD_COUNT) && (tempCol < PUBLISH_COUNT)) {
			sg_countArray[tempRow - 1][tempCol]++;
		} else {
			Log_e(" Unexpected Thread : %d, Message : %d ", tempRow, tempCol);
			sg_rxUnexpectedNumberCounter++;
		}
	} else {
		sg_rxMsgBufferTooBigCounter++;
	}
}

/**
 * yield 测试线程函数
 */
static void *_mqtt_yield_thread_runner(void *ptr) {
	int rc = QCLOUD_ERR_SUCCESS;
	void *pClient = ptr;
    
	while(false == sg_terminate_yield_thread) {

        rc = IOT_MQTT_Yield(pClient, 200);

        if (rc == QCLOUD_ERR_MQTT_ATTEMPTING_RECONNECT) {
            usleep(THREAD_SLEEP_INTERVAL_USEC);
            continue;
        }
        else if (rc != QCLOUD_ERR_SUCCESS && rc != QCLOUD_ERR_MQTT_RECONNECTED){
            Log_e("Yield thread exit with error: %d", rc);
            break;
        }
        usleep(THREAD_SLEEP_INTERVAL_USEC);
	}
	return NULL;
}

/**
 * subscribe/unsubscribe 测试线程函数
 * 函数中会在一个单独的线程中先订阅相关主题，然后再取消订阅
 */
static void *_mqtt_sub_unsub_thread_runner(void *ptr) {
	int rc = QCLOUD_ERR_SUCCESS;
	void *pClient = ptr;
	char testTopic[128];
	HAL_Snprintf(testTopic, 128, ""QCLOUD_IOT_MY_PRODUCT_ID"/"QCLOUD_IOT_MY_DEVICE_NAME"/control");

	while(QCLOUD_ERR_SUCCESS == rc && false == sg_terminate_subUnsub_thread) {
		do {
			usleep(THREAD_SLEEP_INTERVAL_USEC);
			SubscribeParams sub_params = DEFAULT_SUB_PARAMS;
			sub_params.qos = QOS1;
			sub_params.on_message_handler = _mqtt_message_handler;
			rc = IOT_MQTT_Subscribe(pClient, testTopic, &sub_params);

		} while(QCLOUD_ERR_MQTT_NO_CONN == rc || QCLOUD_ERR_MQTT_REQUEST_TIMEOUT == rc);

		if(rc < 0) {
			Log_e("Subscribe failed. Ret : %d ", rc);
		}

        sleep(1);
        
		do {
			usleep(THREAD_SLEEP_INTERVAL_USEC);
			rc = IOT_MQTT_Unsubscribe(pClient, testTopic);
		} while(QCLOUD_ERR_MQTT_NO_CONN == rc|| QCLOUD_ERR_MQTT_REQUEST_TIMEOUT == rc);

		if(rc < 0) {
			Log_e("Unsubscribe failed. Returned : %d ", rc);
		}
	}
	return NULL;
}

/**
 * 在子线程上进行subscribe操作
 * 这里会记录发布前/后的时间，并保存到pSubscribeTime中
 */
static int _mqtt_subscribe_to_test_topic(void *pClient)
{
	SubscribeParams sub_params = DEFAULT_SUB_PARAMS;
    sub_params.on_message_handler = _mqtt_message_handler;
    sub_params.qos = QOS1;
	return IOT_MQTT_Subscribe(pClient, INTEGRATION_TEST_TOPIC, &sub_params);
}

/**
 * 在子线程上进行publish操作
 * 这里会循环PUBLISH_COUNT次，发布topic
 * 如果第一次publish失败，则进行第二次发布，并且记录失败的次数
 */
static void *_mqtt_publish_thread_runner(void *ptr) {
	int itr = 0;
	char topic_content[MAX_SIZE_OF_TOPIC_CONTENT + 1] = {0};

	PublishParams params;
	int rc = QCLOUD_ERR_SUCCESS;
	ThreadData *threadData = (ThreadData *) ptr;
	void *pClient = threadData->client;
	int threadId = threadData->threadId;

    int sleep_us = THREAD_SLEEP_INTERVAL_USEC;
	for(itr = 0; itr < PUBLISH_COUNT; itr++) {
        int size = HAL_Snprintf(topic_content, sizeof(topic_content), "Thread : %d, Msg : %d", threadId, itr);
    	if (size < 0 || size > sizeof(topic_content) - 1) {
    		Log_e("payload content length not enough! content size:%d  buf size:%d", size, (int)sizeof(topic_content));
    		return 0;
    	}		

		params.payload = (void *) topic_content;
		params.payload_len = strlen(topic_content);
		params.qos = QOS1;
		Log_d("Msg being published: %s", topic_content);

		do {
			rc = IOT_MQTT_Publish(pClient, INTEGRATION_TEST_TOPIC, &params);
			usleep(sleep_us);
		} while(QCLOUD_ERR_MQTT_NO_CONN == rc || QCLOUD_ERR_MQTT_REQUEST_TIMEOUT == rc);
		
		// 发布失败的时候进行一次重新发布，并且记录重新发布的次数
		if(rc < 0) {
			Log_e("Failed attempt 1 Publishing Thread : %d, Msg : %d, cs : %d ", threadId, itr, rc);
			do {
				rc = IOT_MQTT_Publish(pClient, INTEGRATION_TEST_TOPIC, &params);
				usleep(sleep_us);
			} while(QCLOUD_ERR_MQTT_NO_CONN == rc);
			sg_rePublishCount++;
			if(QCLOUD_ERR_SUCCESS != rc) {
				Log_e("Failed attempt 2 Publishing Thread : %d, Msg : %d, cs : %d Second Attempt ", threadId, itr, rc);
			}
		}
	}
	sg_threadStatus[threadId - 1] = 1;
	return 0;
}

/**
 * 线程安全测试函数
 */
static int _mqtt_multi_thread_test(void* client)
{
	pthread_t publish_thread[MAX_PUB_THREAD_COUNT];	// 用来保存所有publish的线程
	pthread_t yield_thread;							// yield的线程
	pthread_t sub_unsub_thread;						// 订阅/取消订阅的线程
	int threadId[MAX_PUB_THREAD_COUNT];				// 记录线程id
	int pubThreadReturn[MAX_PUB_THREAD_COUNT];		// 保存pub线程创建函数返回结果		
	float percentOfRxMsg = 0.0;						// 记录从pub到sub整个流程的成功率
	int finishedThreadCount = 0;					// 记录publish_thread线程数组中, 已经执行完pub操作的线程数
	int rxMsgCount = 0;         					// 记录订阅回调函数成功的次数	
	ThreadData threadData[MAX_PUB_THREAD_COUNT];	// 对client和threadId进行包装，在publish线程函数中进行传递
    
	int rc = QCLOUD_ERR_SUCCESS;
	int test_result = 0;	
    int i = 0 , j = 0;

    // init the global variables    
	sg_terminate_yield_thread = false;
	sg_rxMsgBufferTooBigCounter = 0;
	sg_rxUnexpectedNumberCounter = 0;
	sg_rePublishCount = 0;
	sg_wrongYieldCount = 0;

    if (client == NULL) {
        Log_e("MQTT client is invalid!");
        return QCLOUD_ERR_FAILURE;
    }    

    /* Firstly create the thread for IOT_MQTT_Yield() so we can read and handle MQTT packet */
    rc = pthread_create(&yield_thread, NULL, _mqtt_yield_thread_runner, client);
    if (rc < 0) {
        Log_e("Create Yield thread failed. err: %d - %s", rc, strerror(rc));
        return rc;
    }

    /* create a thread to test subscribe and unsubscribe another topic */
	rc = pthread_create(&sub_unsub_thread, NULL, _mqtt_sub_unsub_thread_runner, client);
    if (rc < 0) {
        Log_e("Create Sub_unsub thread failed. err: %d - %s", rc, strerror(rc));
        return rc;
    }
    
    /* subscribe the same test topic as publish threads */
	rc = _mqtt_subscribe_to_test_topic(client);
    if (rc < 0) {
        Log_e("Client subscribe failed: %d", rc);
        return rc;
    }    
    
    /* setup the thread info for pub-threads */
    for(j = 0; j < MAX_PUB_THREAD_COUNT; j++) {
		threadId[j] = j + 1;	    // self defined thread ID: 1 - MAX_PUB_THREAD_COUNT
		sg_threadStatus[j] = 0;	    // thread status flag
		for(i = 0; i < PUBLISH_COUNT; i++) {
			sg_countArray[j][i] = 0;
		}
	}

	/* create multi threads to test IOT_MQTT_Publish() */
	for(i = 0; i < MAX_PUB_THREAD_COUNT; i++) {
		threadData[i].client = client;
		threadData[i].threadId = threadId[i];
		pubThreadReturn[i] = pthread_create(&publish_thread[i], NULL, _mqtt_publish_thread_runner,
											&threadData[i]);
											
		if (pubThreadReturn[i] < 0) {
            Log_e("Create publish thread(ID: %d) failed. err: %d - %s", 
                        threadId[i], pubThreadReturn[i], strerror(pubThreadReturn[i]));
        }
	}
    
	/* wait for all pub-threads to finish their jobs */
	do {
		finishedThreadCount = 0;
		for(i = 0; i < MAX_PUB_THREAD_COUNT; i++) { 
			finishedThreadCount += sg_threadStatus[i];
		}
		Log_i(">>>>>>>>Finished thread count : %d", finishedThreadCount);
		sleep(1);
	} while(finishedThreadCount < MAX_PUB_THREAD_COUNT);

	Log_i("Publishing is finished");

	sg_terminate_yield_thread = true;
	sg_terminate_subUnsub_thread = true;

	/* Allow time for yield_thread and sub_sunsub thread to exit */
    
    /* Not using pthread_join because all threads should have terminated gracefully at this point. If they haven't,
         * which should not be possible, something below will fail. */
	sleep(1);
	
	/* Calculating Test Results */
	for(i = 0; i < PUBLISH_COUNT; i++) {
		for(j = 0; j < MAX_PUB_THREAD_COUNT; j++) {
			if(sg_countArray[j][i] > 0) {
				rxMsgCount++;
			}
		}
	}
    percentOfRxMsg = (float) rxMsgCount * 100 / (PUBLISH_COUNT * MAX_PUB_THREAD_COUNT);
    
	printf("\n\nMQTT Multi-thread Test Result : \n");	
    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	if(RX_RECEIVE_PERCENTAGE <= percentOfRxMsg && 	// 成功率达标
		0 == sg_rxMsgBufferTooBigCounter && 		// 返回数据buffer没有越界
		0 == sg_rxUnexpectedNumberCounter &&		// 返回预期范围内的数据
	    0 == sg_wrongYieldCount) 					// yield过程中没有发生失败
	{	
		// 测试成功
		printf("Success! PercentOfRxMsg: %f %%\n", percentOfRxMsg);
		printf("Published Messages: %d , Received Messages: %d \n", PUBLISH_COUNT * MAX_PUB_THREAD_COUNT, rxMsgCount);
		printf("QoS 1 re publish count %u\n", sg_rePublishCount);
		printf("Yield count without error during callback %u\n", sg_wrongYieldCount);
		test_result = 0;
	} else {
		// 测试失败
		printf("\nFailure! PercentOfRxMsg: %f %%\n", percentOfRxMsg);
        printf("Published Messages: %d , Received Messages: %d \n", PUBLISH_COUNT * MAX_PUB_THREAD_COUNT, rxMsgCount);
		printf("\"Received message was too big than anything sent\" count: %u\n", sg_rxMsgBufferTooBigCounter);
		printf("\"The number received is out of the range\" count: %u\n", sg_rxUnexpectedNumberCounter);
		printf("Yield count without error during callback %u\n", sg_wrongYieldCount);
		test_result = -1;
	}	
    printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");
    
	return test_result;
}


int main(int argc, char **argv) {
    int rc;
    
    //Init log level
    IOT_Log_Set_Level(DEBUG);

    //Avoid broken pipe crash
    signal(SIGPIPE, SIG_IGN);

    //Init connection
    MQTTInitParams init_params = DEFAULT_MQTTINIT_PARAMS;
    rc = _setup_connect_init_params(&init_params);
	if (rc != QCLOUD_ERR_SUCCESS) {
		return rc;
    }

    //MQTT client connect
	void * client = IOT_MQTT_Construct(&init_params);
    if (client != NULL) {
        Log_i("Cloud Device Construct Success");
    } else {
        Log_e("Cloud Device Construct Failed");
        return QCLOUD_ERR_FAILURE;
    }

    //Start rock & roll
    rc = _mqtt_multi_thread_test(client);
	if(0 != rc) {
        Log_e("MQTT multi-thread test FAILED! RC: %d", rc);
    } else {
        Log_i("MQTT multi-thread test SUCCESS");
    }

    //Finish and destroy
    rc = IOT_MQTT_Destroy(&client);
    return rc;
}
