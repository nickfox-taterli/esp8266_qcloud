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

#ifndef QCLOUD_IOT_LOG_UPLOAD_H_
#define QCLOUD_IOT_LOG_UPLOAD_H_

#ifdef __cplusplus
extern "C" {
#endif
    
#include "qcloud_iot_export_log.h"


/**
 * @brief init the log upload functions
 *
 * @param init_params
 */
void init_log_uploader(LogUploadInitParams *init_params);

/**
 * @brief append one log item to upload buffer
 *
 * @param log_content
 * @param log_size
 * @return 0 when success, -1 when fail
 */
int append_to_upload_buffer(const char *log_content, size_t log_size);

/**
 * @brief clear current upload buffer
 *
 * @return 
 */
void clear_upload_buffer();

/**
 * @brief do one upload to server
 *
 * @param force_upload if true, it will do upload right away, otherwise it will check log_level, buffer left and upload interval
 * @return QCLOUD_ERR_SUCCESS when success or no log to upload or timer is not expired
 */
int do_log_upload(bool force_upload);

/**
 * @brief set the log mqtt client to get system time
 *
 * @param client
 */
void set_log_mqtt_client(void *client);

/**
 * @brief set if only do log upload when communication error with IoT Hub
 *
 * @param value if true, only do log upload when communication error with IoT Hub
 */
void set_log_upload_in_comm_err(bool value);

/**
 * @brief get current upload log_level from IoT Hub
 *
 * @param client
 * @param log_level
 * @return QCLOUD_ERR_SUCCESS when success
 */
int qcloud_get_log_level(void* client, int *log_level);

/**
 * @brief subscribe to upload log_level topic
 *
 * @param client
 * @return QCLOUD_ERR_SUCCESS when success 
 */
int qcloud_log_topic_subscribe(void *client);

    
#ifdef __cplusplus
}
#endif

#endif //QCLOUD_IOT_LOG_UPLOAD_H_

