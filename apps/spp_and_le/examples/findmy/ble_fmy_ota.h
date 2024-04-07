#ifndef __BLE_FMY_OTA_H__
#define __BLE_FMY_OTA_H__

#include "typedef.h"
#include "third_party/fmna/fmna_api.h"

//配置支持UARP功能需要开3个SDK配置: 双备份，OTA ENABLE, OTA_BIN file
#define FMY_OTA_SUPPORT_CONFIG  (CONFIG_DOUBLE_BANK_ENABLE && CONFIG_APP_OTA_ENABLE && CONFIG_DB_UPDATE_DATA_GENERATE_EN)

typedef enum {
    FMY_OTA_STATE_IDLE = 0,
    FMY_OTA_STATE_REQ,
    FMY_OTA_STATE_CHECK_FILESIZE,
    FMY_OTA_STATE_WRITE_DATA,
    FMY_OTA_STATE_COMPLETE,
    FMY_OTA_STATE_WRITE_ERROR,
} fmy_ota_st_e;

uint8_t dual_bank_update_verify_without_crc(int (*verify_result_hdl)(int calc_crc));
int fmy_ota_process(uarp_cmd_type_t cmd_type, uint8_t *recv_data, uint32_t recv_len);

#endif
