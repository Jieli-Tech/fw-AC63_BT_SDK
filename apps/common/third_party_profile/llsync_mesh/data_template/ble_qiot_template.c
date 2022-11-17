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
#include "ble_qiot_template.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "ble_qiot_common.h"
#include "ble_qiot_log.h"

llsync_mesh_onoff mesh_onoff;
llsync_mesh_unix_time mesh_unix_time;
llsync_mesh_power_percent mesh_power_percent;
llsync_mesh_HSL mesh_HSL;
llsync_mesh_lightness_level mesh_lightness_level;
llsync_mesh_color_temperature mesh_color_temperature;
llsync_mesh_struct_test mesh_struct_test;

static int llsync_mesh_onoff_set(const char *data, uint16_t len)
{
    memcpy(&mesh_onoff.onoff, data, len);
    ble_qiot_log_i("Mesh set data is OnOff:%d", mesh_onoff.onoff);
    llsync_led_set(mesh_onoff.onoff);
    return 0;
}

static int llsync_mesh_onoff_get(char *data, uint16_t len)
{
    memcpy(data, &mesh_onoff.onoff, sizeof(mesh_onoff.onoff));
    return sizeof(mesh_onoff.onoff);
}

static int llsync_mesh_unix_time_set(const char *data, uint16_t len)
{
    uint8_t i = 0;

    memcpy(mesh_unix_time.unix_time, data, len);

    for (i = 0; i < 6; i++) {
        ble_qiot_log_i("Mesh set data is unix_time:%d-0x%02x", i, mesh_unix_time.unix_time[i]);
    }
    return 0;
}

static int llsync_mesh_unix_time_get(char *data, uint16_t len)
{
    memcpy(data, mesh_unix_time.unix_time, sizeof(mesh_unix_time.unix_time));
    return sizeof(mesh_unix_time.unix_time);
}

static int llsync_mesh_power_percent_set(const char *data, uint16_t len)
{
    memcpy(&mesh_power_percent.power_percent, data, len);
    ble_qiot_log_i("Mesh set data is power_percent:%d", mesh_power_percent.power_percent);
    return 0;
}

static int llsync_mesh_power_percent_get(char *data, uint16_t len)
{
    memcpy(data, &mesh_power_percent.power_percent, sizeof(mesh_power_percent.power_percent));
    return sizeof(mesh_power_percent.power_percent);
}

static int llsync_mesh_HSL_set(const char *data, uint16_t len)
{
    uint8_t i = 0;
    memcpy(mesh_HSL.HSL, data, len);
    for (i = 0; i < (sizeof(mesh_HSL.HSL) / sizeof(mesh_HSL.HSL[0])); i++) {
        mesh_HSL.HSL[i] = HTONS(mesh_HSL.HSL[i]);
    }
    return 0;
}

static int llsync_mesh_HSL_get(char *data, uint16_t len)
{
    uint8_t i = 0;
    for (i = 0; i < (sizeof(mesh_HSL.HSL) / sizeof(mesh_HSL.HSL[0])); i++) {
        mesh_HSL.HSL[i] = HTONS(mesh_HSL.HSL[i]);
    }
    memcpy(data, mesh_HSL.HSL, sizeof(mesh_HSL.HSL));
    return sizeof(mesh_HSL.HSL);
}

static int llsync_mesh_lightness_level_set(const char *data, uint16_t len)
{
    memcpy(&mesh_lightness_level.lightness_level, data, len);
    mesh_lightness_level.lightness_level = HTONS(mesh_lightness_level.lightness_level);
    ble_qiot_log_i("Mesh set data is lightness_level:%d", mesh_lightness_level.lightness_level);
    return 0;
}

static int llsync_mesh_lightness_level_get(char *data, uint16_t len)
{
    mesh_lightness_level.lightness_level = HTONS(mesh_lightness_level.lightness_level);
    memcpy(data, &mesh_lightness_level.lightness_level, sizeof(mesh_lightness_level.lightness_level));
    return sizeof(mesh_lightness_level.lightness_level);
}

static int llsync_mesh_color_temperature_set(const char *data, uint16_t len)
{
    memcpy(&mesh_color_temperature.color_temperature, data, len);
    mesh_color_temperature.color_temperature = HTONS(mesh_color_temperature.color_temperature);
    return 0;
}

static int llsync_mesh_color_temperature_get(char *data, uint16_t len)
{
    mesh_color_temperature.color_temperature = HTONS(mesh_color_temperature.color_temperature);
    memcpy(data, &mesh_color_temperature.color_temperature, sizeof(mesh_color_temperature.color_temperature));
    return sizeof(mesh_color_temperature.color_temperature);
}

static int llsync_mesh_struct_test_set(const char *data, uint16_t len)
{
    memcpy(&mesh_struct_test, data, len);
    mesh_struct_test.testa = NTOHL(mesh_struct_test.testa);
    mesh_struct_test.testb = NTOHL(mesh_struct_test.testb);
    ble_qiot_log_i("llsync_mesh_struct_test_set:0x%x", mesh_struct_test.testa);
    ble_qiot_log_i("llsync_mesh_struct_test_set:0x%x", mesh_struct_test.testb);
    return 0;
}

static int llsync_mesh_struct_test_get(char *data, uint16_t len)
{
    mesh_struct_test.testa = NTOHL(mesh_struct_test.testa);
    mesh_struct_test.testb = NTOHL(mesh_struct_test.testb);
    memcpy(data, &mesh_struct_test, sizeof(mesh_struct_test));
    return sizeof(mesh_struct_test);
}

ble_property_t sg_ble_property_array[1] = {
    {llsync_mesh_onoff_set, llsync_mesh_onoff_get, LLSYNC_MESH_VENDOR_ONOFF_TYPE, sizeof(llsync_mesh_onoff)},
    /* {llsync_mesh_unix_time_set, llsync_mesh_unix_time_get, LLSYNC_MESH_VENDOR_UNIX_TIME_TYPE, sizeof(llsync_mesh_unix_time)}, */
    /* {llsync_mesh_power_percent_set, llsync_mesh_power_percent_get, LLSYNC_MESH_VENDOR_POWER_PERCENT_TYPE, sizeof(llsync_mesh_power_percent)}, */
    /* {llsync_mesh_HSL_set, llsync_mesh_HSL_get, LLSYNC_MESH_VENDOR_HSL_TYPE, sizeof(llsync_mesh_HSL)}, */
    /* {llsync_mesh_lightness_level_set, llsync_mesh_lightness_level_get, LLSYNC_MESH_VENDOR_LIGHTNESS_LEVEL_TYPE, sizeof(llsync_mesh_lightness_level)}, */
    /* {llsync_mesh_color_temperature_set, llsync_mesh_color_temperature_get, LLYNSC_MESH_VENDOR_COLOR_TEMPRETURE_TYPE, sizeof(llsync_mesh_color_temperature)}, */
    /* {llsync_mesh_struct_test_set, llsync_mesh_struct_test_get, LLSYNC_MESH_VENDOR_STRUCT_TEST_TYPE, sizeof(llsync_mesh_struct_test)}, */
};

uint16_t llsync_mesh_vendor_size_get(void)
{
    return sizeof(sg_ble_property_array) / sizeof(sg_ble_property_array[0]);
}

ble_property_t *llsync_mesh_property_array_get(void)
{
    return sg_ble_property_array;
}

#ifdef __cplusplus
}
#endif
