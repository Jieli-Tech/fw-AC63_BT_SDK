/*
 * Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
 * Licensed under the MIT License (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT
 * Unless required by applicable law or agreed to in writing, software distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#ifdef __cplusplus
extern "C" {
#endif
#include <string.h>
#include "ble_qiot_hmac256.h"
#include "ble_qiot_sha256.h"
#include "ble_qiot_utils_base64.h"
#include "ble_qiot_export.h"
#include "ble_qiot_import.h"
#include "ble_qiot_common.h"
#include "ble_qiot_utils_mesh.h"
#include "ble_qiot_mesh_cfg.h"
#include "ble_qiot_template.h"
#include "ble_qiot_log.h"

static int llsync_mesh_hex(const void *buf, size_t len, void *out)
{
    const char hex[] = "0123456789abcdef";
    char hexbufs[137];
    const uint8_t *b = buf;
    int i;

    len = LLSYNC_MESH_MIN(len, (sizeof(hexbufs) - 1) / 2);
    for (i = 0; i < len; i++) {
        hexbufs[i * 2] = hex[b[i] >> 4];
        hexbufs[i * 2 + 1] = hex[b[i] & 0xf];
    }
    hexbufs[i * 2] = '\0';

    memcpy(out, hexbufs, i * 2);

    return i * 2;
}

// mesh接收数据处理
ble_qiot_ret_status_t llsync_mesh_recv_data_handle(uint32_t Opcode, uint8_t *data, uint16_t data_len)
{
    LLSYNC_MESH_POINTER_CHECK(data, LLSYNC_MESH_RS_ERR_PARA);

    switch (Opcode) {
    case LLSYNC_MESH_VND_MODEL_OP_SET: {
        llsync_mesh_vendor_data_set(data, data_len);
        break;
    }

    case LLSYNC_MESH_VND_MODEL_OP_GET: {
        llsync_mesh_vendor_data_get(data, data_len);
        break;
    }

    case LLSYNC_MESH_VND_MODEL_OP_SET_UNACK: {
        llsync_mesh_vendor_data_set_unack(data, data_len);
        break;
    }

    case LLSYNC_MESH_VND_MODEL_OP_CONFIRMATION: {
        llsync_mesh_vendor_op_confirmation(data, data_len);
        break;
    }

    default:
        break;
    }

    return LLSYNC_MESH_RS_OK;
}

// 扫描响应数据获取
ble_qiot_ret_status_t llsync_mesh_scan_data_get(uint8_t *data, uint8_t *data_len)
{
    LLSYNC_MESH_POINTER_CHECK(data, LLSYNC_MESH_RS_ERR_PARA);

    uint8_t scan_resp_data[LLSYNC_MESH_ADV_MAX_LEN] = {0x02, 0x01, 0x06, 0x0d, 0x09};
    uint8_t offset                           = 5;
    ble_device_info_t dev_info;

    memset(&dev_info, 0, sizeof(ble_device_info_t));
    llsync_mesh_dev_info_get(&dev_info);

    memcpy(&scan_resp_data[offset], dev_info.device_name, strlen(dev_info.device_name));
    offset += strlen(dev_info.device_name);

    memcpy(data, scan_resp_data, offset);
    *data_len = offset;

    return LLSYNC_MESH_RS_OK;
}

// UUID获取接口
int llsync_mesh_dev_uuid_get(uint8_t type, uint8_t *data, uint8_t data_len)
{
    LLSYNC_MESH_POINTER_CHECK(data, LLSYNC_MESH_RS_ERR_PARA);

    uint8_t index = 0;
    ble_device_info_t dev_info;

    if (LLSYNC_MESH_DEVICE_UUID_LEN > data_len) {
        ble_qiot_log_e("The buf len too short.");
        return LLSYNC_MESH_RS_ERR_PARA;
    }

    data[index++] = LLSYNC_MESH_CID_VENDOR_LOW;
    data[index++] = LLSYNC_MESH_CID_VENDOR_HIGH;
    if (type == LLSYNC_MESH_UNNET_ADV_BIT) {
        data[index++] = LLSYNC_MESH_PROTOCOL_VERSION | LLSYNC_MESH_UNNET_ADV_BIT;
    } else {
        data[index++] = LLSYNC_MESH_PROTOCOL_VERSION | LLSYNC_MESH_SILENCE_ADV_BIT;
    }
    data[index++] = LLSYNC_MESH_ADV_RFU_FLAG2;

    memset(&dev_info, 0, sizeof(ble_device_info_t));
    llsync_mesh_dev_info_get(&dev_info);
    memcpy(&data[index], dev_info.product_id, LLSYNC_MESH_PRODUCT_ID_LEN);
    index += LLSYNC_MESH_PRODUCT_ID_LEN;
    data[index++] = 0;  //RFU
    data[index++] = 0;  //RFU
    data[index] = '\0';

    return index;
}

// AuthValue计算
ble_qiot_ret_status_t llsync_mesh_auth_clac(uint8_t *random, uint8_t *auth_data)
{
    LLSYNC_MESH_POINTER_CHECK(auth_data, LLSYNC_MESH_RS_ERR_PARA);
    LLSYNC_MESH_POINTER_CHECK(random, LLSYNC_MESH_RS_ERR_PARA);

    uint8_t temp_data[LLSYNC_MESH_RAMDOM_DATA_LEN] = {0};
    uint8_t sign[HMAC_SHA256_DIGEST_SIZE]       = {0};
    uint8_t buf[128]                            = {0};
    uint8_t buf_len                             = 0;
    uint8_t i                                   = 0;
    ble_device_info_t dev_info;

    memset(&dev_info, 0, sizeof(ble_device_info_t));
    llsync_mesh_dev_info_get(&dev_info);
    buf_len += llsync_mesh_hex(random, LLSYNC_MESH_RAMDOM_DATA_LEN, buf);
    memcpy(buf + buf_len, dev_info.product_id, LLSYNC_MESH_PRODUCT_ID_LEN);
    buf_len += LLSYNC_MESH_PRODUCT_ID_LEN;
    memcpy(buf + buf_len, dev_info.device_name, strlen(dev_info.device_name));
    buf_len += strlen(dev_info.device_name);

    llsync_hmac_sha256(sign, (const uint8_t *)buf, buf_len,
                       (const uint8_t *)dev_info.psk, sizeof(dev_info.psk));

    for (i = 0; i < HMAC_SHA256_DIGEST_SIZE / 2; i++) {
        temp_data[i] = sign[i] ^ sign[i + HMAC_SHA256_DIGEST_SIZE / 2];
    }

    memcpy(auth_data, temp_data, LLSYNC_MESH_RAMDOM_DATA_LEN);

    return LLSYNC_MESH_RS_OK;
}

#ifdef __cplusplus
}
#endif
