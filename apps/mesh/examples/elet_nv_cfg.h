#ifndef __ELET_NV_CFG_H__
#define __ELET_NV_CFG_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STRINGIZER(arg)                         #arg
#define STR_VALUE(arg)                          STRINGIZER(arg)

#define USER_VERSION                            10002
#define Bluetooth_Complete_Version              "ET23_MESH_V"STR_VALUE(USER_VERSION)
#define FW_VERSION_OTA                          ET23_MESH_V
#define Bluetooth_Complete_Version_OTA          STR_VALUE(FW_VERSION_OTA)STR_VALUE(USER_VERSION)

#define BT_DEVNAME_MAX_LEN                      29      /* Bluetooth BLE Device Name Max Len */
#define LOCAL_NAME_WITH_LOCAL_MAC  				1

#define BTA_NVRAM_FLASH_DATA_DS1_ADD            0x00080000
#define BTA_NVRAM_FLASH_DATA_DS2_ADD            0x00082000
#define BTA_NVRAM_FlASH_DATA_IS_INIT            0xA55A0008
#define BTA_NVRAM_RESET_OPCODE                  0x90090999
#define DEFAULT_MODULE_NAME                     "ET23_MESH_"

// #define ELET_BLE_ADVT_TYPE_STOP                 0
#define ELET_BLE_ADVT_TYPE_NORMAL               1
#define ELET_BLE_ADVT_TYPE_BEACON               2

#define ELET_BEACON_UUID                        0xFD, 0xA5, 0x06, 0x93, 0xA4, 0xE2, 0x4F, 0xB1, 0xAF, 0xCF, 0xC6, 0xEB, 0x07, 0x64, 0x78, 0x25
#define ELET_MAJOR_VALUE                        0x1227
#define ELET_MINOR_VALUE                        0xE13F
#define ELET_MEASURED_RSSI                      0xC3

/* Conversion macros */
#define BIT16_TO_8( val ) \
    (uint8_t)(  (val)        & 0xff),/* LSB */ \
    (uint8_t)(( (val) >> 8 ) & 0xff) /* MSB */

#define UUID16_LEN                              (2)
#define UUID128_LEN                             (16)

#define UUID16_ELET_SERVICE                     0xFFE1
#define UUID16_ELET_CHARACTERISTIC_NOTIFY       0xFFE2
#define UUID16_ELET_CHARACTERISTIC_WRITE        0xFFE3
// #define ELET_UUID128_ENABLE
// 0000FFE1-0000-1000-8000-00805f9b34fb
/* UUID value of the Hello Sensor Service */
#define UUID128_ELET_SERVICE                    0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, BIT16_TO_8(UUID16_ELET_SERVICE), 0x00, 0x00
/* UUID value of the Hello Sensor Characteristic, Value Notification */
#define UUID128_ELET_CHARACTERISTIC_NOTIFY      0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, BIT16_TO_8(UUID16_ELET_CHARACTERISTIC_NOTIFY), 0x00, 0x00
/* UUID value of the Hello Sensor Characteristic, Configuration */
#define UUID128_ELET_CHARACTERISTIC_WRITE       0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, BIT16_TO_8(UUID16_ELET_CHARACTERISTIC_WRITE), 0x00, 0x00

typedef struct
{
	uint32_t data_init_flag;
	uint32_t data_len;

	uint32_t fw_num;

	char     mesh_name[BT_DEVNAME_MAX_LEN + 1];
	uint16_t mesh_mcid;
	uint16_t mesh_ccid;
	uint32_t mesh_pid;

	uint32_t uart_rate;
	uint8_t  uart_flow_ctrl;
	uint8_t  uart_parity;

	uint32_t crc32; // 此变量一定要放置结构体最后位置，不可改动
} nv_cfg_t;

void 	 elet_nv_cfg_mesh_mcid_set(uint16_t mesh_mcid);
uint16_t elet_nv_cfg_mesh_mcid_get(void);
void 	 elet_nv_cfg_mesh_ccid_set(uint16_t mesh_ccid);
uint16_t elet_nv_cfg_mesh_ccid_get(void);
void 	 elet_nv_cfg_mesh_pid_set(uint32_t mesh_pid);
uint32_t elet_nv_cfg_mesh_pid_get(void);
void 	 elet_nv_cfg_uart_rate_set(uint32_t uart_rate);
uint32_t elet_nv_cfg_uart_rate_get(void);
void 	 elet_nv_cfg_uart_flow_ctrl_set(uint8_t uart_flow_ctrl);
uint8_t  elet_nv_cfg_uart_flow_ctrl_get(void);

void 	 elet_nv_cfg_reset_default(void);
void	 elet_nv_cfg_init(uint32_t operation);

#ifdef __cplusplus
}
#endif

#endif // __ELET_NV_CFG_H__