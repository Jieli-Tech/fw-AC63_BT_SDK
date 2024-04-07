// binary representation
// attribute size in bytes (16), flags(16), handle (16), uuid (16/128), value(...)

#ifndef _BLE_FMY_H
#define _BLE_FMY_H

#include <stdint.h>
#include "app_config.h"
#include "gatt_common/le_gatt_common.h"
#include "third_party/fmna/fmna_api.h"

enum {
    PROFILE_MODE_UNPAIR = 0,
    PROFILE_MODE_OWNER,
    PROFILE_MODE_NON_OWNER,
    PROFILE_MODE_NEARBY,
    PROFILE_MODE_SEPARATED,
};

typedef struct {
    uint16_t cur_con_handle;//记录最新的连接handle
    uint16_t sound_ctrl_timer_id;//蜂鸣器定时器
    uint16_t pairing_mode_timer_id;//配对模式定时器
    uint8_t  is_app_fmy_active;//控制是否进入低功耗，0-yes,1-no
    uint8_t  enter_btstack_num;//是否重复进入应用
    uint8_t  battery_level;// init BAT_STATE_FULL
    uint8_t  adv_fmna_state;//fmna adv 状态
    uint8_t  profile_mode; //profile 配置模式
    uint8_t  sound_onoff;  //蜂鸣器状态
    uint8_t  pairing_mode_enable; //配对模式使能
    uint8_t  testbox_mode_enable; //测试频偏校准模式
    //uint8_t  cur_remote_address_info[7];//连接对方的地址
} fmy_glb_t;

typedef struct {
    uint8_t  head_tag;
    uint8_t  reset_config;//flag
} fmy_vm_t;

extern fmy_glb_t  fmy_global_data;
#define __fydata  (&fmy_global_data)

extern fmy_vm_t fmy_vm_info;
#define __fy_vm  (&fmy_vm_info)

//------------------------------------------------------------------
//------------------------------------------------------------------
bool att_set_handle_enable(hci_con_handle_t con_handle, uint16_t start_handle, uint16_t end_handle, uint8_t enable);
bool fmy_check_capabilities_is_enalbe(uint8_t cap);
void fmy_state_idle_set_active(uint8_t active);
bool fmy_vm_deal(fmy_vm_t *info, uint8_t rw_flag);
//--------------------------------------------------------------------------------

#endif
