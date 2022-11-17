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
#ifndef QCLOUD_BLE_QIOT_MESH_CFG_H
#define QCLOUD_BLE_QIOT_MESH_CFG_H

#ifdef __cplusplus
extern "C" {
#endif


// qiot data storage address
#define LLSYNC_MESH_RECORD_FLASH_ADDR         (0xFE000)

// flash page size, see chip datasheet
#define LLSYNC_MESH_RECORD_FLASH_PAGESIZE     (4096)

// Total broadcast hours of unallocated broadcasts(ms)
#define LLSYNC_MESH_UNNET_ADV_TOTAL_TIME       (10 * 60 * 1000)

// If the unmatching timeout expires and the network is still not matched,
// does it enter the silent broadcast state, which is only used to let Provisioner discover the device
#define LLSYNC_MESH_SILENCE_ADV_ENABLE         (1)

// Unallocated Broadcast Single Broadcast Duration(ms)
#define LLSYNC_MESH_UNNET_ADV_DURATION         (150)

// Silent Broadcast Single Broadcast Duration(ms)
#define LLSYNC_MESH_SILENCE_ADV_DURATION       (150)

// Unallocated broadcast interval(ms)
#define LLSYNC_MESH_UNNET_ADV_INTERVAL         (500)

// Silent broadcast interval(ms)
#define LLSYNC_MESH_SILENCE_ADV_INTERVAL       (60 * 1000)

#define __ORDER_LITTLE_ENDIAN__ 1234
#define __ORDER_BIG_ENDIAN__    3412
#define __BYTE_ORDER__          __ORDER_LITTLE_ENDIAN__

#define MESH_LOG_PRINT(...)                 printf(__VA_ARGS__)

#define BLE_QIOT_USER_DEFINE_HEXDUMP        (0)

#ifdef __cplusplus
}
#endif

#endif  // QCLOUD_BLE_QIOT_MESH_CFG_H
