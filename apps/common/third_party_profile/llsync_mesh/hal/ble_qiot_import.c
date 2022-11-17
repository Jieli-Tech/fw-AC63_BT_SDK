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
#include "ble_qiot_import.h"
#include "ble_qiot_common.h"
#include "ble_qiot_utils_mesh.h"
#include "ble_qiot_mesh_cfg.h"
#include "ble_qiot_log.h"

// esp32 code for example

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/event_groups.h"
// #include "esp_bt.h"
// #include "esp_spi_flash.h"

#if 1
#define PRODUCT_ID  "PGKCRQIHE9"
#define DEVICE_NAME "001122334455"   //注意此处的dev name使用设备的mac地址来命名
#define DEVICE_SECRET "LhxVvs7uaLKSHOJclfSbWA=="
#else
#define PRODUCT_ID  "PGKCRQIHE9"
#define DEVICE_NAME "112233445566"   //注意此处的dev name使用设备的mac地址来命名
#define DEVICE_SECRET "dYjvwn+RGw51NI4rhZf7hA=="
#endif


char *DevNameGet(void)
{
    return DEVICE_NAME;
}

ble_qiot_ret_status_t llsync_mesh_dev_info_get(ble_device_info_t *dev_info)
{
    // add your code here

    memcpy(dev_info->product_id, PRODUCT_ID, LLSYNC_MESH_PRODUCT_ID_LEN);
    memcpy(dev_info->device_name, DEVICE_NAME, strlen(DEVICE_NAME));
    memcpy(dev_info->psk, DEVICE_SECRET, LLSYNC_MESH_PSK_LEN);

    return LLSYNC_MESH_RS_OK;
}

ble_qiot_ret_status_t llsync_mesh_adv_handle(e_llsync_mesh_user adv_status, uint8_t *data_buf, uint8_t data_len)
{
    if (LLSYNC_MESH_ADV_START == adv_status) {
        // add your code here
    } else if (LLSYNC_MESH_ADV_STOP == adv_status) {
        // add your code here
    }

    return LLSYNC_MESH_RS_OK;
}

typedef struct __ble_timer_id {
    uint8_t       type;
    ble_timer_cb  handle;
    int timer;
} ble_timer_id;

ble_timer_t llsync_mesh_timer_create(uint8_t type, ble_timer_cb timeout_handle)
{
    // add your code here

    ble_timer_id *p_timer = malloc(sizeof(ble_timer_id));
    if (NULL == p_timer) {
        return NULL;
    }

    p_timer->type   = type;
    p_timer->handle = timeout_handle;
    p_timer->timer  = -1;

    return (ble_timer_t)p_timer;
}

ble_qiot_ret_status_t llsync_mesh_timer_start(ble_timer_t timer_id, uint32_t period)
{
    // add your code here

    ble_timer_id *p_timer = (ble_timer_id *)timer_id;

    if (-1 == p_timer->timer) {
        p_timer->timer = sys_timer_add(NULL, p_timer->handle, period);
    }
    sys_timer_change_period(p_timer->timer, period);

    return LLSYNC_MESH_RS_OK;
}

ble_qiot_ret_status_t llsync_mesh_timer_stop(ble_timer_t timer_id)
{
    // add your code here

    ble_timer_id *p_timer = (ble_timer_id *)timer_id;
    sys_timer_remove(p_timer->timer);
    p_timer->timer  = -1;

    return LLSYNC_MESH_RS_OK;
}

int llsync_mesh_flash_handle(e_llsync_flash_user type, uint32_t flash_addr, char *buf, uint16_t len)
{
    // add your code here

    // int ret = 0;
    // if (LLSYNC_MESH_WRITE_FLASH == type) {
    //     ret = spi_flash_erase_range(flash_addr, LLSYNC_MESH_RECORD_FLASH_PAGESIZE);
    //     ret = spi_flash_write(flash_addr, buf, len);
    // } else if (LLSYNC_MESH_READ_FLASH == type){
    //     ret = spi_flash_read(flash_addr, buf, len);
    // }

    return len;
}

// This data is sent out according to the opcode, both status and indication
void llsync_mesh_vendor_data_send(uint32_t opcode, uint8_t *data, uint16_t data_len)
{
    llsync_vendor_attr_send(opcode, data, data_len);
    return;
}

#ifdef __cplusplus
}
#endif
