#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "app_config.h"
#include "rcsp_bluetooth.h"
#include "string.h"
#include "JL_rcsp_api.h"
#include "JL_rcsp_protocol.h"
#include "JL_rcsp_packet.h"
#include "btstack/avctp_user.h"
#include "system/timer.h"
#include "app_core.h"
#include "rcsp_user_update.h"
#include "spp_user.h"
#include "rcsp_msg.h"
#include "app_config.h"
#include "app_action.h"
#include "custom_cfg.h"
#include "btstack_3th_protocol_user.h"
#include "update.h"



#if RCSP_KEY_OPT
#include "key_event_deal.h"
#endif

#if TCFG_APP_FM_EMITTER_EN
#include "fm_emitter/fm_emitter_manage.h"
//#include "fm_emitter/fm_emitter_digvol_ctrl.h"
#endif

#if TCFG_UI_ENABLE
#include "ui/ui_api.h"
#endif

#define  TULING_AI_EN         0
#define  DEEPBRAN_AI_EN       1

#if (RCSP_BTMATE_EN)

#define RCSP_DEBUG_EN
#ifdef RCSP_DEBUG_EN
#define rcsp_putchar(x)                putchar(x)
#define rcsp_printf                    printf
#define rcsp_printf_buf(x,len)         put_buf(x,len)
#else
#define rcsp_putchar(...)
#define rcsp_printf(...)
#define rcsp_printf_buf(...)
#endif

struct JL_AI_VAR jl_ai_var = {
    .rcsp_run_flag = 0,
};

#define __this (&jl_ai_var)

#define RCSP_USE_BLE      0
#define RCSP_USE_SPP      1
#define RCSP_CHANNEL_SEL  RCSP_USE_BLE


#define BT_FUNCTION        0
#define MUSIC_FUNCTION     1
#define RTC_FUNCTION       2
#define LINEIN_FUNCTION    3
#define FM_FUNCTION        4
#define LIGHT_FUNCTION     5
#define FMTX_FUNCTION      6


#define BT_FUNCTION_MASK        0
#define MUSIC_FUNCTION_MASK     1
#define RTC_FUNCTION_MASK       2
#define LINEIN_FUNCTION_MASK    3
#define FM_FUNCTION_MASK        4
#define FMTX_FUNCTION_MASK      6
#define COMMON_FUNCTION    		0xFF
#define FUNCTION_INFO  			(BIT(MUSIC_FUNCTION_MASK) | BIT(LINEIN_FUNCTION_MASK) | BIT(RTC_FUNCTION_MASK) | BIT(FMTX_FUNCTION_MASK))


#define TEMP_SHIELD_EQ_OPERATING	0


#pragma pack(1)

struct _SYS_info {
    u8 bat_lev;
    u8 sys_vol;
    u8 max_vol;
    u8 reserve;
};

struct _EDR_info {
    u8 addr_buf[6];
    u8 profile;
    u8 state;
};

struct _DEV_info {
    u8 status;
    u32 usb_handle;
    u32 sd0_handle;
    u32 sd1_handle;
    u32 flash_handle;
};

struct _EQ_INFO {
    u8 mode;
    s8 gain_val[10];
};

struct _MUSIC_STATUS_info {
    u8 status;
    u32 cur_time;
    u32 total_time;
    u8 cur_dev;
};

struct _dev_version {
    u16 _sw_ver2: 4; //software l version
    u16 _sw_ver1: 4; //software m version
    u16 _sw_ver0: 4; //software h version
    u16 _hw_ver:  4; //hardware version
};


#pragma pack()

/* #pragma pack(1) */
/* #pragma pack() */


#if TEMP_SHIELD_EQ_OPERATING
#include "audio_eq.h"
extern s8 eq_mode_get_gain(u8 mode, u16 index);
extern int eq_mode_set_custom_param(u16 index, int gain);
extern int eq_mode_set(u8 mode);
extern int eq_mode_get_cur(void);
#endif


static u32 JL_auto_update_sys_info(u8 fun_type, u32 mask);
extern void rcsp_update_data_api_unregister(void);

#if RCSP_UPDATE_EN
static volatile u8 JL_bt_chl = 0;
u8 JL_get_cur_bt_channel_sel(void);
void JL_ble_disconnect(void);
#endif

#if RCSP_KEY_OPT
void rcsp_app_common_key_event_handler(u8 key_event)
{
    switch (key_event) {
    case KEY_FM_EMITTER_NEXT_FREQ:
    case KEY_FM_EMITTER_PERV_FREQ:
        JL_auto_update_sys_info(COMMON_FUNCTION, BIT(COMMON_INFO_ATTR_FMTX));
        break;
    default:
        break;
    }
}
#endif

u8 get_rcsp_support_new_reconn_flag(void)
{
    return __this->new_reconn_flag;
}

u8 get_rcsp_connect_status(void)
{
#if RCSP_UPDATE_EN
    if (RCSP_BLE == JL_get_cur_bt_channel_sel()) {
        if (__this->JL_ble_status == BLE_ST_CONNECT || __this->JL_ble_status == BLE_ST_NOTIFY_IDICATE) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return __this->JL_spp_status;
    }
#else
    return 0;
#endif
}

extern const int support_dual_bank_update_en;
static u32 JL_opcode_get_target_info(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    u8 buf[256];
    u8 offset = 0;

    __this->phone_platform = data[4];
    if (__this->phone_platform == ANDROID) {
        rcsp_printf("phone_platform == ANDROID\n");
    } else if (__this->phone_platform == APPLE_IOS) {
        rcsp_printf("phone_platform == APPLE_IOS\n");
    } else {
        rcsp_printf("phone_platform ERR\n");
    }

    u32 mask = READ_BIG_U32(data);
    rcsp_printf("FEATURE MASK : %x\n", mask);

    u32 ret = 0;

    //get version
    if (mask & BIT(ATTR_TYPE_PROTOCOL_VERSION)) {
        rcsp_printf("ATTR_TYPE_PROTOCOL_VERSION\n");
        u8 ver = get_rcsp_version();
        offset += add_one_attr(buf, sizeof(buf), offset, ATTR_TYPE_PROTOCOL_VERSION, &ver, 1);
    }

    //get powerup sys info
    if (mask & BIT(ATTR_TYPE_SYS_INFO)) {
        rcsp_printf("ATTR_TYPE_SYS_INFO\n");

        struct _SYS_info sys_info;
        //extern u16 get_battery_level(void);
        sys_info.bat_lev = 0; //get_battery_level() / 10;
        sys_info.sys_vol = 0; //sound.vol.sys_vol_l;
        sys_info.max_vol = 0; //MAX_SYS_VOL_L;

        offset += add_one_attr(buf, sizeof(buf), offset, ATTR_TYPE_SYS_INFO, \
                               (u8 *)&sys_info, sizeof(sys_info));
    }

    //get EDR info
    if (mask & BIT(ATTR_TYPE_EDR_ADDR)) {
        rcsp_printf("ATTR_TYPE_EDR_ADDR\n");
        struct _EDR_info edr_info;
        /* extern void hook_get_mac_addr(u8 * btaddr); */
        /* hook_get_mac_addr(edr_info.addr_buf); */

        extern const u8 *bt_get_mac_addr();
        u8 taddr_buf[6];
        memcpy(taddr_buf, bt_get_mac_addr(), 6);
        edr_info.addr_buf[0] =  taddr_buf[5];
        edr_info.addr_buf[1] =  taddr_buf[4];
        edr_info.addr_buf[2] =  taddr_buf[3];
        edr_info.addr_buf[3] =  taddr_buf[2];
        edr_info.addr_buf[4] =  taddr_buf[1];
        edr_info.addr_buf[5] =  taddr_buf[0];
        /* extern u8 get_edr_suppor_profile(void); */
        /* edr_info.profile = get_edr_suppor_profile(); */
        edr_info.profile = 0x0E;
#if (RCSP_CHANNEL_SEL == RCSP_USE_BLE)
        edr_info.profile &= ~BIT(7);
#else
        edr_info.profile |= BIT(7);
#endif
        extern u8 get_bt_connect_status(void);
        if (get_bt_connect_status() ==  BT_STATUS_WAITINT_CONN) {
            edr_info.state = 0;
        } else {
            edr_info.state = 1;
        }
        offset += add_one_attr(buf, sizeof(buf), offset, ATTR_TYPE_EDR_ADDR, (u8 *)&edr_info, sizeof(struct _EDR_info));
    }

    //get platform info
#if 0
    if (mask & BIT(ATTR_TYPE_PLATFORM)) {
        rcsp_printf("ATTR_TYPE_PLATFORM\n");

        u8 lic_val = 0;
        offset += add_one_attr(buf, sizeof(buf), offset, ATTR_TYPE_PLATFORM, &lic_val, 1);
    }
#endif

    //get function info
    if (mask & BIT(ATTR_TYPE_FUNCTION_INFO)) {
        rcsp_printf("ATTR_TYPE_FUNCTION_INFO\n");

        u32 function_info_mask = 0;
        function_info_mask |= FUNCTION_INFO;
        printf("FUN config = %x\n", function_info_mask);
        function_info_mask = app_htonl(function_info_mask);

        u8 tmp_buf[5];
        u8 fun = BT_FUNCTION;//get_cur_function();
        memcpy(tmp_buf, (u8 *)&function_info_mask, 4);
        memcpy(tmp_buf + 4, &fun, 1);
        offset += add_one_attr(buf, sizeof(buf), offset, ATTR_TYPE_FUNCTION_INFO, tmp_buf, sizeof(tmp_buf));
    }



    //get dev info
    if (mask & BIT(ATTR_TYPE_DEV_VERSION)) {
        /* rcsp_printf("ATTR_TYPE_DEV_VERSION\n"); */
        u8 tmp_ver = 0;
        offset += add_one_attr(buf, sizeof(buf), offset, ATTR_TYPE_DEV_VERSION, (u8 *)&tmp_ver, 1);
    }

    if (mask & BIT(ATTR_TYPE_UBOOT_VERSION)) {
        /* rcsp_printf("ATTR_TYPE_UBOOT_VERSION\n"); */
        /* u8 *uboot_ver_flag = (u8 *)(0x1C000 - 0x8); */
        /* u8 uboot_version[2] = {uboot_ver_flag[0], uboot_ver_flag[1]}; */
        /* offset += add_one_attr(buf, sizeof(buf), offset, ATTR_TYPE_UBOOT_VERSION, uboot_version, sizeof(uboot_version)); */
    }

    if (mask & BIT(ATTR_TYPE_DOUBLE_PARITION)) {
        /* rcsp_printf("ATTR_TYPE_DOUBLE_PARITION:%x\n",support_dual_bank_update_en); */
        u8 double_partition_value;
        u8 ota_loader_need_download_flag;
        if (support_dual_bank_update_en) {
            double_partition_value = 0x1;
            ota_loader_need_download_flag = 0x00;
        } else {
            double_partition_value = 0x0;
            ota_loader_need_download_flag = 0x01;
        }
        u8 update_param[2] = {
            double_partition_value,
            ota_loader_need_download_flag,
        };
        offset += add_one_attr(buf, sizeof(buf), offset, ATTR_TYPE_DOUBLE_PARITION, (u8 *)update_param, sizeof(update_param));
    }

    if (mask & BIT(ATTR_TYPE_UPDATE_STATUS)) {
        /* rcsp_printf("ATTR_TYPE_UPDATE_STATUS\n"); */
        u8 update_status_value = 0x0;
        offset += add_one_attr(buf, sizeof(buf), offset, ATTR_TYPE_UPDATE_STATUS, (u8 *)&update_status_value, sizeof(update_status_value));
    }

    if (mask & BIT(ATTR_TYPE_DEV_VID_PID)) {
        /* rcsp_printf("ATTR_TYPE_DEV_VID_PID\n"); */

        u8 temp_dev_vid_pid = 0;
        offset += add_one_attr(buf, sizeof(buf), offset, ATTR_TYPE_DEV_VID_PID, (u8 *)&temp_dev_vid_pid, sizeof(temp_dev_vid_pid));
    }

    if (mask & BIT(ATTR_TYPE_SDK_TYPE)) {
        /* rcsp_printf("ATTR_TYPE_SDK_TYPE\n"); */
        u8 sdk_type = 1;//0：AI SDK， 1：BT_MATE
        offset += add_one_attr(buf, sizeof(buf), offset, ATTR_TYPE_SDK_TYPE, &sdk_type, 1);
    }


#if VER_INFO_EXT_COUNT
    // get AuthKey
    if (mask & BIT(ATTR_TYPE_DEV_AUTHKEY)) {
        rcsp_printf("ATTR_TYPE_DEV_AUTHKEY\n");
        u8 authkey_len = 0;
        u8 *local_authkey_data = NULL;
        get_authkey_procode_from_cfg_file(&local_authkey_data, &authkey_len, GET_AUTH_KEY_FROM_EX_CFG);
        if (local_authkey_data && authkey_len) {
            putbuf(local_authkey_data, authkey_len);
            offset += add_one_attr(buf, sizeof(buf), offset, ATTR_TYPE_DEV_AUTHKEY, local_authkey_data, authkey_len);
        }
    }

    if (mask & BIT(ATTR_TYPE_DEV_PROCODE)) {
        rcsp_printf("ATTR_TYPE_DEV_PROCODE\n");
        u8 procode_len = 0;
        u8 *local_procode_data = NULL;
        get_authkey_procode_from_cfg_file(&local_procode_data, &procode_len, GET_PRO_CODE_FROM_EX_CFG);
        if (local_procode_data && procode_len) {
            put_buf(local_procode_data, procode_len);
            offset += add_one_attr(buf, sizeof(buf), offset, ATTR_TYPE_DEV_PROCODE, local_procode_data, procode_len);
        }
    }
#endif

    if (mask & BIT(ATTR_TYPE_DEV_MAX_MTU)) {
        /* rcsp_printf(" ATTR_TYPE_DEV_MTU_SIZE\n"); */
        u16 rx_max_mtu = JL_packet_get_rx_max_mtu();
        u16 tx_max_mtu = JL_packet_get_tx_max_mtu();
        u8 t_buf[4];
        t_buf[0] = (tx_max_mtu >> 8) & 0xFF;
        t_buf[1] = tx_max_mtu & 0xFF;
        t_buf[2] = (rx_max_mtu >> 8) & 0xFF;
        t_buf[3] = rx_max_mtu & 0xFF;
        offset += add_one_attr(buf, sizeof(buf), offset,  ATTR_TYPE_DEV_MAX_MTU, t_buf, 4);
    }

    if (mask & BIT(ATTR_TEYP_BLE_ADDR)) {
        rcsp_printf(" ATTR_TEYP_BLE_ADDR\n");
        /* extern void lib_make_ble_address(u8 * ble_address, u8 * edr_address); */
        extern int le_controller_get_mac(void *addr);
        extern const u8 *bt_get_mac_addr();
        u8 taddr_buf[7];
        taddr_buf[0] = 0;
        /* lib_make_ble_address(taddr_buf + 1, (void *)bt_get_mac_addr()); */
        le_controller_get_mac(taddr_buf + 1);
        for (u8 i = 0; i < (6 / 2); i++) {
            taddr_buf[i + 1] ^= taddr_buf[7 - i - 1];
            taddr_buf[7 - i - 1] ^= taddr_buf[i + 1];
            taddr_buf[i + 1] ^= taddr_buf[7 - i - 1];
        }
        offset += add_one_attr(buf, sizeof(buf), offset,  ATTR_TEYP_BLE_ADDR, taddr_buf, sizeof(taddr_buf));
    }

    if (mask & BIT(ATTR_TYPE_MD5_GAME_SUPPORT)) {
        rcsp_printf("ATTR_TYPE_MD5_GAME_SUPPORT\n");
        u8 md5_support = UPDATE_MD5_ENABLE;
        offset += add_one_attr(buf, sizeof(buf), offset, ATTR_TYPE_MD5_GAME_SUPPORT, &md5_support, 1);
    }

    rcsp_printf_buf(buf, offset);

    ret = JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, buf, offset);

    return ret;
}


static u16 common_function_info(u32 mask, u8 *buf, u16 len)
{
    u16 offset = 0;
#if 1
    if (mask & BIT(COMMON_INFO_ATTR_BATTERY)) {
        /* rcsp_printf("COMMON_INFO_ATTR_BATTERY\n"); */
        /* extern u16 get_battery_level(void); */
        /* u8 battery = get_battery_level() / 10; */
        /* offset += add_one_attr(buf, len, offset, COMMON_INFO_ATTR_BATTERY, &battery, 1); */
    }

    if (mask & BIT(COMMON_INFO_ATTR_VOL)) {
        /* rcsp_printf("COMMON_INFO_ATTR_VOL\n"); */
        /* offset += add_one_attr(buf, len, offset, COMMON_INFO_ATTR_VOL, &sound.vol.sys_vol_l, 1); */
    }

    if (mask & BIT(COMMON_INFO_ATTR_ERR_REPORT)) {
        /* rcsp_printf("COMMON_INFO_ATTR_ERR_REPORT\n"); */
        /* offset += add_one_attr(buf, len, offset, COMMON_INFO_ATTR_ERR_REPORT, &__this->err_report, 1); */
    }
#endif

#if TEMP_SHIELD_EQ_OPERATING
    if (mask & BIT(COMMON_INFO_ATTR_EQ)) {
        /* rcsp_printf("COMMON_INFO_ATTR_EQ\n"); */

        /* extern u8 eq_mode_get(void); */
        /* extern int eq_custom_val_get(u8 mode, u8 index); */
        struct _EQ_INFO eq_info;

        eq_info.mode = eq_mode_get_cur();

        u16 i;
        for (i = 0; i < 10; i++) {
            eq_info.gain_val[i] = eq_mode_get_gain(eq_info.mode, i);
        }

        offset += add_one_attr(buf, len, offset, COMMON_INFO_ATTR_EQ, (u8 *)(&eq_info), sizeof(struct _EQ_INFO));
    }
#endif

    if (mask & BIT(COMMON_INFO_ATTR_FUN_TYPE)) {
        /* rcsp_printf("COMMON_INFO_ATTR_FUN_TYPE\n"); */
        /* u8 fun = get_cur_function(); */
        /* offset += add_one_attr(buf, len, offset, COMMON_INFO_ATTR_FUN_TYPE, (u8 *)&fun, 1); */
    }
    if (mask & BIT(COMMON_INFO_ATTR_FMTX)) {
        rcsp_printf("COMMON_INFO_ATTR_EQ\n");
#if TCFG_APP_FM_EMITTER_EN
        u16 FMTXPoint = fm_emitter_manage_get_fre();
        printf("FMTXPoint = %d\n", FMTXPoint);
        FMTXPoint =	app_htons(FMTXPoint);
        offset += add_one_attr(buf, len, offset, COMMON_INFO_ATTR_FMTX, (u8 *)&FMTXPoint, sizeof(FMTXPoint));
#endif
    }

    return offset;
}






static u32 JL_opcode_get_sys_info(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    u8 buf[512];
    u32 ret = 0;
    u8 offset = 0;
    u8 fun_type = data[0];
    u32 mask = READ_BIG_U32(data + 1);
    rcsp_printf("SYS MASK : %x\n", mask);

    /* printf_buf(data, len); */

    buf[0] = fun_type;

    u8 *p = buf + 1;
    u16 size = sizeof(buf) - 1;

    switch (fun_type) {
    case COMMON_FUNCTION:
        rcsp_printf("COMMON_FUNCTION\n");
        offset = common_function_info(mask, p, size);
        break;
    case BT_FUNCTION:
        /* rcsp_printf("BT_FUNCTION\n"); */
        /* offset = bt_function_info(mask, p, size); */
        break;
    case MUSIC_FUNCTION:
        /* rcsp_printf("MUSIC_FUNCTION\n"); */
#if 0
        if (get_cur_function() != MUSIC_FUNCTION) {
            ret = JL_CMD_response_send(OpCode, JL_PRO_STATUS_FAIL, OpCode_SN, NULL, 0);
            return ret;
        }
        MUSIC_PLAYER *obj = (MUSIC_PLAYER *)&music_player_hdl;
        if ((music_player_get_status(obj) != MUSIC_DECODER_ST_STOP) && (get_tone_status() == 0)) {
            printf("music play ready\n");
            offset = music_function_info(mask, p, size);
        } else {
            printf("music not play ok\n");
            ret = JL_CMD_response_send(OpCode, JL_PRO_STATUS_BUSY, OpCode_SN, NULL, 0);
            return ret;
        }
        if (0 == offset) {
            printf("****************busy*************\n");
            ret = JL_CMD_response_send(OpCode, JL_PRO_STATUS_BUSY, OpCode_SN, NULL, 0);
            return ret;
        }
#endif
        break;
    case RTC_FUNCTION:
        /* rcsp_printf("RTC_FUNCTION\n"); */
        /* offset = rtc_function_info(mask, p, size); */
        break;
    case LINEIN_FUNCTION:
        /* rcsp_printf("LINEIN_FUNCTION\n"); */
        /* offset = linein_function_info(mask, p, size); */
        break;
    case FM_FUNCTION:
        /* rcsp_printf("FM_FUNCTION\n"); */
        /* offset = fm_function_info(mask, p, size); */
        break;


    default:
        break;

    }

    rcsp_printf_buf(buf, offset + 1);

    ret = JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, buf, offset + 1);

    return ret;
}
#if TEMP_SHIELD_EQ_OPERATING
void rcsp_custom_eq_set(void *cmd_param)
{
    u8 data;
    u8 status;
    data = *((u8 *)cmd_param);
    status = *(((u8 *)cmd_param) + 1);
    if (EQ_MODE_CUSTOM == data) {
        if (status != 0x7F) {//是调节进度条导致自定义修改
            u8 i;
            for (i = 0; i < EQ_SECTION_MAX; i++) {//app只支持10段
                eq_mode_set_custom_param(i, ((s8 *)cmd_param)[i + 1]);
            }
        }
        eq_mode_set(EQ_MODE_CUSTOM);
    }
}
#endif
static u32 JL_auto_update_sys_info(u8 fun_type, u32 mask)
{
    u8 buf[512];
    u8 offset = 0;
    u32 ret = 0;

    rcsp_printf("JL_auto_update_sys_info\n");

    buf[0] = fun_type;

    u8 *p = buf + 1;
    u8 size = sizeof(buf) - 1;

    switch (fun_type) {
    case COMMON_FUNCTION:
        rcsp_printf("COMMON_FUNCTION\n");
        offset = common_function_info(mask, p, size);
        break;
    case BT_FUNCTION:
        rcsp_printf("BT_FUNCTION\n");
        /* offset = bt_function_info(mask, p, size); */
        break;
    case MUSIC_FUNCTION:
        rcsp_printf("MUSIC_FUNCTION\n");
        /* offset = music_function_info(mask, p, size); */
        break;
    case RTC_FUNCTION:
        rcsp_printf("RTC_FUNCTION\n");
        /* offset = rtc_function_info(mask, p, size); */
        break;
    case LINEIN_FUNCTION:
        rcsp_printf("LINEIN_FUNCTION\n");
        /* offset = linein_function_info(mask, p, size); */
        break;
    case FM_FUNCTION:
        rcsp_printf("FM_FUNCTION\n");
        /* offset = fm_function_info(mask, p, size); */
        break;

    case FMTX_FUNCTION:
        rcsp_printf("FMTX_FUNCTION\n");

        break;

    default:
        break;
    }

    rcsp_printf_buf(buf, offset + 1);

    ret = JL_CMD_send(JL_OPCODE_SYS_INFO_AUTO_UPDATE, buf, offset + 1, JL_NOT_NEED_RESPOND);
    return ret;
}



static void set_common_function_info(u8 *data, u16 len)
{
    u8 offset = 0;
    //analysis sys info
    while (offset < len) {
        u8 len_tmp = data[offset];
        u8 type = data[offset + 1];
        rcsp_printf("common info:\n");
        rcsp_printf_buf(&data[offset], len_tmp + 1);

        switch (type) {
        case COMMON_INFO_ATTR_BATTERY:
            break;
        case COMMON_INFO_ATTR_VOL:

            /* sound.vol.sys_vol_l = data[offset + 2]; */
            /* sound.vol.sys_vol_r = sound.vol.sys_vol_l; */
            /* printf("vol set : %d\n", sound.vol.sys_vol_l); */
            /* set_sys_vol(sound.vol.sys_vol_l, sound.vol.sys_vol_r, FADE_OFF); */

            /* if (0 == sound.vol.sys_vol_l) { */
            /* dac_mute(1, 1); */
            /* if (task_get_cur() == TASK_ID_LINEIN) { */
            /* linein_mute(1); */
            /* } */
            /* } else { */
            /* extern u16 get_aux_mute_flag(void); */
            /* if (task_get_cur() == TASK_ID_LINEIN) { */
            /* if (!get_aux_mute_flag()) { */
            /* if (linein_mute_status()) { */
            /* linein_mute(0); */
            /* } */
            /* } */
            /* } */

            /* dac_mute(0, 1); */

            /* #if (SYS_DEFAULT_VOL == 0) */
            /* vm_cache_write(VM_SYS_VOL, &sound.vol.sys_vol_l, 2); */
            /* #endif */
            /* } */

            break;
        case COMMON_INFO_ATTR_DEV:
            break;
        case COMMON_INFO_ATTR_ERR_REPORT:
            break;
#if TEMP_SHIELD_EQ_OPERATING
        case COMMON_INFO_ATTR_EQ :
            rcsp_printf("COMMON_INFO_ATTR_EQ\n");
            u8 cmd_param = data[offset + 2];
            rcsp_printf("rcsp eq mode : %d\n", cmd_param);

            if (cmd_param < EQ_MODE_CUSTOM) {
                rcsp_printf("set eq mode\n");
                eq_mode_set(cmd_param);
            } else {
                /*自定义修改EQ参数*/
                rcsp_printf("set eq param\n");
                rcsp_custom_eq_set((void *)(&(data[offset + 2])));
            }
            rcsp_msg_post(RCSP_MSG_UPDATE_EQ, 1, NULL);
            break;
#endif
        case COMMON_INFO_ATTR_FMTX: {
            rcsp_printf("COMMON_INFO_ATTR_FMTX\n");
            u16 FMTX_point;
            memcpy(&FMTX_point, data + offset + 2, sizeof(FMTX_point));
            FMTX_point = app_ntohs(FMTX_point);
            printf("FMTXPoint = %d\n", FMTX_point);
            rcsp_msg_post(RCSP_MSG_SET_FMTX_POINT, 1, FMTX_point);
        }
        break;

        default:
            break;
        }

        offset += len_tmp + 1;
    }
}

static u32 JL_opcode_set_sys_info(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    u32 ret = 0;
    u8 fun_type = data[0];
    u8 *p = data + 1;

    len = len - 1;

    switch (fun_type) {
    case COMMON_FUNCTION:
        rcsp_printf("COMMON_FUNCTION\n");
        set_common_function_info(p, len);
        break;
    case BT_FUNCTION:
        rcsp_printf("BT_FUNCTION\n");
        /* set_bt_function_info(p, len); */
        break;
    case MUSIC_FUNCTION:
        rcsp_printf("MUSIC_FUNCTION\n");
        /* set_music_function_info(p, len); */
        break;
    case RTC_FUNCTION:
        rcsp_printf("RTC_FUNCTION\n");
        /* ret = set_rtc_function_info(p, len); */
        break;
    case LINEIN_FUNCTION:
        rcsp_printf("LINEIN_FUNCTION\n");
        /* set_linein_function_info(p, len); */
        break;
    case FM_FUNCTION:
        rcsp_printf("FM_FUNCTION\n");
        /* set_fm_function_info(p, len); */
        break;

    /* case FMTX_FUNCTION: */
    /* rcsp_printf("FMTX_FUNCTION\n"); */
    /* set_fmtx_function_info(p, len); */
    /* break; */
    default:
        break;
    }

    if (ret) {
        ret = JL_CMD_response_send(OpCode, JL_PRO_STATUS_FAIL, OpCode_SN, NULL, 0);
    } else {
        ret = JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0);
    }

    return ret;
}

#define ADDR_RECORD_MAX	10
static u8 bt_addr_record[ADDR_RECORD_MAX][6];
static u8 bt_addr_idx = 0;
static void JL_rcsp_update_bt_state(int argc, int *argv)
{
    printf("JL_rcsp_update_bt_state\n");
    u32 dev_class = (u32) argv[0];
    u8 *addr = (u8 *) argv[1];
    char rssi = (char) argv[2];
    u8 *bt_name = (u8 *) argv[3];
    u8 name_len = (u8) argv[4];
    u8 *tmp_data = NULL;
    u8 i = 0;
    for (; bt_addr_idx != ADDR_RECORD_MAX && i < bt_addr_idx; i++) {
        if (bt_addr_idx > 1 && 0 == memcmp(bt_addr_record[i], addr, 6)) {
            return;
        }
    }

    if (i) {
        bt_addr_idx = i;
        i = sizeof(dev_class) + 6 + sizeof(rssi) + sizeof(name_len) + name_len;
        tmp_data = malloc(i);
        if (!tmp_data) {
            free(tmp_data);
            return;
        }
        i = 0;
        memcpy(tmp_data + i, (u8 *) dev_class, sizeof(dev_class));
        i += sizeof(dev_class);
        memcpy(bt_addr_record[bt_addr_idx - 1], addr, 6);
        memcpy(tmp_data + i, addr, 6);
        i += 6;
        tmp_data[i++] = rssi;
        tmp_data[i++] = name_len;
        memcpy(tmp_data + i, bt_name, name_len);
        i += name_len;
        JL_CMD_send(JL_OPCODE_SYS_UPDATE_BT_STATUS, tmp_data, i, JL_NOT_NEED_RESPOND);
        free(tmp_data);
    }
}

static void JL_rcsp_stop_bt_scan(u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    rcsp_printf("JL_rcsp_stop_bt_scan\n");
#if TCFG_USER_EMITTER_ENABLE
    extern void emitter_search_stop(u8 result);
    if (bt_addr_idx) {
        bt_addr_idx = 0;
        emitter_search_stop(0);
    }
    u8 result = 0x00;
#else
    u8 result = 0x01;
#endif
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, &result, sizeof(result));
}

static void JL_rcsp_open_bt_scan(u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    rcsp_printf("JL_rcsp_open_bt_scan\n");
#if TCFG_USER_EMITTER_ENABLE
    bt_addr_idx = 1;
    extern void bt_search_device(void);
    bt_search_device();
    u8 result = 0x00;
#else
    u8 result = 0x01;
#endif
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, &result, sizeof(result));
}

static void rcsp_process_timer();
static void JL_rcsp_resend_timer_opt(u8 flag, u32 usec)
{
    if (flag) {
        if (0 == __this->rcsp_timer_hdl) {
            __this->rcsp_timer_hdl = sys_timer_add(NULL, rcsp_process_timer, usec);
        }
    } else {
        if (__this->rcsp_timer_hdl)	{
            sys_timer_del(__this->rcsp_timer_hdl);
            __this->rcsp_timer_hdl = 0;
        }
    }
}

//phone send cmd to firmware need respond
static void JL_rcsp_cmd_resp(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    rcsp_printf("JL_ble_cmd_resp op = %d\n", OpCode);

    if (__this->JL_ble_status != BLE_ST_NOTIFY_IDICATE) {
        /*从机多机处理部分,同步深圳办的修改，2022-12-29*/
        if (__this->rcsp_ble && __this->rcsp_ble->adv_enable) {
            __this->rcsp_ble->adv_enable(NULL, 0);
        }
        rcsp_printf("%s[__this->JL_ble_status:%d]", __func__, __this->JL_ble_status);
        __this->JL_ble_status = BLE_ST_NOTIFY_IDICATE;
    }

    switch (OpCode) {
    case JL_OPCODE_GET_TARGET_FEATURE:
        rcsp_printf("JL_OPCODE_GET_TARGET_INFO\n");
        JL_rcsp_resend_timer_opt(1, 500);
        JL_opcode_get_target_info(priv, OpCode, OpCode_SN, data, len);
        break;

    case JL_OPCODE_SYS_INFO_GET:
        rcsp_printf("JL_OPCODE_SYS_INFO_GET\n");
        JL_opcode_get_sys_info(priv, OpCode, OpCode_SN, data, len);
        break;

    case JL_OPCODE_SYS_INFO_SET:
        rcsp_printf("JL_OPCODE_SYS_INFO_SET\n");
        JL_opcode_set_sys_info(priv, OpCode, OpCode_SN, data, len);
        break;


    case JL_OPCODE_SWITCH_DEVICE:
        __this->device_type = data[0];
        if (len > 1) {          //新增一个Byte用于标识是否支持新的回连方式, 新版本APP会发多一个BYTE用于标识是否支持新的回连方式
            __this->new_reconn_flag = data[1];
            JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, &__this->new_reconn_flag, 1);
        } else {
            JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0);
        }
#if RCSP_UPDATE_EN
        os_time_dly(10);
        if (get_jl_update_flag()) {
#if 0//CONFIG_HID_CASE_ENABLE
            void rcsp_update_jump_for_hid_device();
            sys_timeout_add(NULL, rcsp_update_jump_for_hid_device, 200);
            /* os_time_dly(10);        //延时让回复命令发完 */
            /* rcsp_update_jump_for_hid_device();      //这里处理hid设备主动断开手机,系统不会回连的问题 */
#else
            if (RCSP_BLE == JL_get_cur_bt_channel_sel()) {
                rcsp_printf("BLE_ CON START DISCON\n");
                rcsp_msg_post(MSG_JL_DEV_DISCONNECT, 0);
            } else if (RCSP_SPP == JL_get_cur_bt_channel_sel()) {
                rcsp_printf("WAIT_FOR_SPP_DISCON\n");
#if CONFIG_APP_OTA_ENABLE
            } else {
                rcsp_printf("RCSP HID DISCON\n");
                void set_curr_update_type(u8 type);
                set_curr_update_type(RCSP_HID);
                void rcsp_update_jump_for_hid_device();
                sys_timeout_add(NULL, rcsp_update_jump_for_hid_device, 200);
#endif
            }
#endif
        }
#endif
        break;
    case JL_OPCODE_SYS_OPEN_BT_SCAN:
        JL_rcsp_open_bt_scan(OpCode, OpCode_SN, data, len);
        break;
    case JL_OPCODE_SYS_STOP_BT_SCAN:
        JL_rcsp_stop_bt_scan(OpCode, OpCode_SN, data, len);
        break;


    default:
#if RCSP_UPDATE_EN
        if ((OpCode >= JL_OPCODE_GET_DEVICE_UPDATE_FILE_INFO_OFFSET) && \
            (OpCode <= JL_OPCODE_SET_DEVICE_REBOOT)) {
            JL_rcsp_update_cmd_resp(priv, OpCode, OpCode_SN, data, len);
        } else
#endif
        {
            JL_CMD_response_send(OpCode, JL_ERR_NONE, OpCode_SN, data, len);
        }
        break;
    }

//  JL_ERR JL_CMD_response_send(u8 OpCode, u8 status, u8 sn, u8 *data, u16 len)
}

//phone send cmd to firmware not need respond
static void JL_rcsp_cmd_no_resp(void *priv, u8 OpCode, u8 *data, u16 len)
{
    rcsp_printf("JL_ble_cmd_no_resp\n");
}

//phone send data to firmware need respond
static void JL_rcsp_data_resp(void *priv, u8 OpCode_SN, u8 CMD_OpCode, u8 *data, u16 len)
{
    rcsp_printf("JL_ble_data_resp\n");
    switch (CMD_OpCode) {
    case 0:
        break;

    default:
        break;
    }

}
//phone send data to firmware not need respond
static void JL_rcsp_data_no_resp(void *priv, u8 CMD_OpCode, u8 *data, u16 len)
{
    rcsp_printf("JL_ble_data_no_resp\n");
    switch (CMD_OpCode) {
    case 0:
        break;

    default:
        break;
    }

}

//phone respone firmware cmd
static void JL_rcsp_cmd_recieve_resp(void *priv, u8 OpCode, u8 status, u8 *data, u16 len)
{
    rcsp_printf("rec resp:%x\n", OpCode);

    switch (OpCode) {
    default:
#if RCSP_UPDATE_EN
        if ((OpCode >= JL_OPCODE_GET_DEVICE_UPDATE_FILE_INFO_OFFSET) && \
            (OpCode <= JL_OPCODE_SET_DEVICE_REBOOT)) {
            JL_rcsp_update_cmd_receive_resp(priv, OpCode, status, data, len);
        }
#endif
        break;
    }
}
//phone respone firmware data
static void JL_rcsp_data_recieve_resp(void *priv, u8 status, u8 CMD_OpCode, u8 *data, u16 len)
{
    rcsp_printf("JL_ble_data_recieve_resp\n");
    switch (CMD_OpCode) {

    case 0:
        break;

    default:
        break;
    }

}
//wait resp timout
static u8 JL_rcsp_wait_resp_timeout(void *priv, u8 OpCode, u8 counter)
{
    rcsp_printf("JL_rcsp_wait_resp_timeout\n");

    return 0;
}


///**************************************************************************************///
///************     rcsp ble                                                   **********///
///**************************************************************************************///
void bt_ble_adv_enable(u8 enable);
static void JL_ble_status_callback(void *priv, ble_state_e status)
{
    rcsp_printf("JL_ble_status_callback==================== %d\n", status);
    __this->JL_ble_status = status;
    switch (status) {
    case BLE_ST_IDLE:
#if RCSP_UPDATE_EN
        if (get_jl_update_flag()) {
            rcsp_msg_post(MSG_JL_UPDATE_START, 0);
        }
#endif
        break;
    case BLE_ST_ADV:
        break;
    case BLE_ST_CONNECT:
        break;
    case BLE_ST_SEND_DISCONN:
        JL_rcsp_resend_timer_opt(0, 0);
        break;
    case BLE_ST_DISCONN:
#if RCSP_UPDATE_EN
        if (get_jl_update_flag()) {
            if (__this->rcsp_ble->adv_enable) {
                __this->rcsp_ble->adv_enable(NULL, 0);
            }
            //bt_ble_adv_enable(NULL, 0);
        }
#endif
        break;
    case BLE_ST_NOTIFY_IDICATE:
        break;
    default:
        break;
    }
}

static bool JL_ble_fw_ready(void *priv)
{
    return ((__this->JL_ble_status == BLE_ST_NOTIFY_IDICATE) ? true : false);
}

static s32 JL_ble_send(void *priv, void *data, u16 len)
{
    if ((__this->rcsp_ble != NULL) && (__this->JL_ble_status == BLE_ST_NOTIFY_IDICATE)) {
        int err = __this->rcsp_ble->send_data(NULL, (u8 *)data, len);
        /* rcsp_printf("send :%d\n", len); */
        if (len < 128) {
            /* rcsp_printf_buf(data, len); */
        } else {
            /* rcsp_printf_buf(data, 128); */
        }

        if (err == 0) {
            return 0;
        } else if (err == APP_BLE_BUFF_FULL) {
            return 1;
        }
    } else {
        rcsp_printf("send err -1 !!\n");
    }

    return -1;
}

static const JL_PRO_CB JL_pro_BLE_callback = {
    .priv              = NULL,
    .fw_ready          = JL_ble_fw_ready,
    .fw_send           = JL_ble_send,
    .CMD_resp          = JL_rcsp_cmd_resp,
    .CMD_no_resp       = JL_rcsp_cmd_no_resp,
    .DATA_resp         = JL_rcsp_data_resp,
    .DATA_no_resp      = JL_rcsp_data_no_resp,
    .CMD_recieve_resp  = JL_rcsp_cmd_recieve_resp,
    .DATA_recieve_resp = JL_rcsp_data_recieve_resp,
    .wait_resp_timeout = JL_rcsp_wait_resp_timeout,
};

static void rcsp_ble_callback_set(void (*resume)(void), void (*recieve)(void *, void *, u16), void (*status)(void *, ble_state_e))
{
    printf("----0x%x   0x%x   0x%x  0x%x\n", __this->rcsp_ble, __this->rcsp_ble->regist_wakeup_send, __this->rcsp_ble->regist_recieve_cbk, __this->rcsp_ble->regist_state_cbk);
    __this->rcsp_ble->regist_wakeup_send(NULL, resume);
    __this->rcsp_ble->regist_recieve_cbk(NULL, recieve);
    __this->rcsp_ble->regist_state_cbk(NULL, status);
}

#if CONFIG_APP_OTA_ENABLE

static void JL_rcsp_hid_status_callback(u8 status)
{

}

static int JL_rcsp_hid_data_send(void *priv, u8 *data, u16 len)
{
    rcsp_printf("### rcsp_hid_data_send %d\n", len);
    int err = 0;
    if (__this->rcsp_hid != NULL) {
        err = __this->rcsp_hid->send_data(NULL, data, len);
    }
    return err;
}

static s32 JL_rcsp_hid_send(void *priv, void *buf, u16 len)
{
    if (len < 128) {
        rcsp_printf("send: \n");
        rcsp_printf_buf(buf, (u32)len);
    }
    if ((__this->rcsp_hid != NULL) && (JL_rcsp_hid_fw_ready(NULL))) {
        return __this->rcsp_hid->send_data(NULL, buf, len);
    } else {
        rcsp_printf("send err -1 !!\n");
    }
    return -1;
}

static const JL_PRO_CB JL_pro_HID_callback = {
    .priv              = NULL,
    .fw_ready          = JL_rcsp_hid_fw_ready,
    .fw_send           = JL_rcsp_hid_send,
    .CMD_resp          = JL_rcsp_cmd_resp,
    .DATA_resp         = JL_rcsp_data_resp,
    .CMD_no_resp       = JL_rcsp_cmd_no_resp,
    .DATA_no_resp      = JL_rcsp_data_no_resp,
    .CMD_recieve_resp  = JL_rcsp_cmd_recieve_resp,
    .DATA_recieve_resp = JL_rcsp_data_recieve_resp,
    .wait_resp_timeout = JL_rcsp_wait_resp_timeout,
};

static void rcsp_hid_callback_set(void (*recieve)(void *, void *, u16), void (*status)(u8))
{
    printf("----0x%x   0x%x   0x%x\n", __this->rcsp_hid, __this->rcsp_hid->regist_recieve_cbk, __this->rcsp_hid->regist_state_cbk);
    __this->rcsp_hid->regist_recieve_cbk(NULL, recieve);
    __this->rcsp_hid->regist_state_cbk(NULL, status);
}

#endif

#if RCSP_UPDATE_EN
u8 JL_get_cur_bt_channel_sel(void)
{
    return JL_bt_chl;
}

void JL_ble_disconnect(void)
{
    __this->rcsp_ble->disconnect(NULL);
}

u8 get_curr_device_type(void)
{
    return __this->device_type;
}

void set_curr_update_type(u8 type)
{
    __this->device_type = type;
}
#endif

///**************************************************************************************///
///************     rcsp spp                                                   **********///
///**************************************************************************************///
static void JL_spp_status_callback(u8 status)
{
    rcsp_printf("JL_spp_status_callback==================== %d\n", status);
    switch (status) {
    case SPP_USER_ST_NULL:
    case SPP_USER_ST_DISCONN:
        __this->JL_spp_status = 0;
        JL_rcsp_resend_timer_opt(0, 0);
        break;
    case SPP_USER_ST_CONNECT:
        __this->JL_spp_status = 1;
        break;
    default:
        __this->JL_spp_status = 0;
        break;
    }
}

static bool JL_spp_fw_ready(void *priv)
{
    return (__this->JL_spp_status ? true : false);
}

static s32 JL_spp_send(void *priv, void *data, u16 len)
{
    if (len < 128) {
        rcsp_printf("send: \n");
        rcsp_printf_buf(data, (u32)len);
    }
    if ((__this->rcsp_spp != NULL) && (__this->JL_spp_status == 1)) {
        u32 err = __this->rcsp_spp->send_data(NULL, (u8 *)data, len);
        if (err == 0) {
            return 0;
        } else if (err == SPP_USER_ERR_SEND_BUFF_BUSY) {
            return 1;
        }
    } else {
        rcsp_printf("send err -1 !!\n");
    }

    return -1;
}

static const JL_PRO_CB JL_pro_SPP_callback = {
    .priv              = NULL,
    .fw_ready          = JL_spp_fw_ready,
    .fw_send           = JL_spp_send,
    .CMD_resp          = JL_rcsp_cmd_resp,
    .DATA_resp         = JL_rcsp_data_resp,
    .CMD_no_resp       = JL_rcsp_cmd_no_resp,
    .DATA_no_resp      = JL_rcsp_data_no_resp,
    .CMD_recieve_resp  = JL_rcsp_cmd_recieve_resp,
    .DATA_recieve_resp = JL_rcsp_data_recieve_resp,
    .wait_resp_timeout = JL_rcsp_wait_resp_timeout,
};

static void rcsp_spp_callback_set(void (*resume)(void), void (*recieve)(void *, void *, u16), void (*status)(u8))
{
    __this->rcsp_spp->regist_wakeup_send(NULL, resume);
    __this->rcsp_spp->regist_recieve_cbk(NULL, recieve);
    __this->rcsp_spp->regist_state_cbk(NULL, status);
}

static int rcsp_spp_data_send(void *priv, u8 *buf, u16 len)
{
    int err = 0;
    if (__this->rcsp_spp != NULL) {
        err = __this->rcsp_spp->send_data(NULL, (u8 *)buf, len);
    }

    return err;

}

static int rcsp_ble_data_send(void *priv, u8 *buf, u16 len)
{
    rcsp_printf("### rcsp_ble_data_send %d\n", len);
    int err = 0;
    if (__this->rcsp_ble != NULL) {
        err = __this->rcsp_ble->send_data(NULL, (u8 *)buf, len);
    }

    return err;

}

static const u8 link_key_data[16] = {0x06, 0x77, 0x5f, 0x87, 0x91, 0x8d, 0xd4, 0x23, 0x00, 0x5d, 0xf1, 0xd8, 0xcf, 0x0c, 0x14, 0x2b};
void JL_rcsp_recieve_handle(void *priv, void *buf, u16 len)
{
    //put_buf(buf, len > 0x20 ? 0x20 : len);
    if (0 == BT_CONNECTION_VERIFY) {
        if (!JL_rcsp_get_auth_flag()) {
            JL_rcsp_auth_recieve(buf, len);
            return;
        }
    }

    JL_protocol_data_recieve(priv, buf, len);
}
void rcsp_dev_select_v1(u8 type)
{
#if RCSP_UPDATE_EN
    set_jl_update_flag(0);
#endif
    switch (type) {
    case RCSP_BLE:
#if RCSP_UPDATE_EN
        JL_bt_chl = RCSP_BLE;
#endif
        rcsp_printf("------RCSP_BLE-----\n");
        //rcsp_spp_callback_set(NULL, NULL, NULL);
        rcsp_ble_callback_set(JL_protocol_resume, JL_rcsp_recieve_handle, JL_ble_status_callback);
        JL_protocol_dev_switch(&JL_pro_BLE_callback);
        JL_rcsp_auth_init(rcsp_ble_data_send, (u8 *)link_key_data, NULL);
        break;
    case RCSP_SPP:
#if RCSP_UPDATE_EN
        JL_bt_chl = RCSP_SPP;
#endif
        rcsp_printf("------RCSP_SPP-----\n");
        rcsp_spp_callback_set(JL_protocol_resume, JL_rcsp_recieve_handle, JL_spp_status_callback);
        rcsp_ble_callback_set(NULL, NULL, NULL);
        JL_protocol_dev_switch(&JL_pro_SPP_callback);
        JL_rcsp_auth_init(rcsp_spp_data_send, (u8 *)link_key_data, NULL);
        break;

#if CONFIG_APP_OTA_ENABLE
    case RCSP_HID:
#if RCSP_UPDATE_EN
        JL_bt_chl = RCSP_HID;
        set_curr_update_type(RCSP_HID);
#endif
        rcsp_printf("------RCSP_HID-----\n");
        rcsp_ble_callback_set(NULL, NULL, NULL);
        rcsp_hid_callback_set(JL_rcsp_recieve_handle, JL_rcsp_hid_status_callback);
        JL_protocol_dev_switch(&JL_pro_HID_callback);
        JL_rcsp_auth_init(JL_rcsp_hid_data_send, NULL, NULL);
        break;
#endif

    }
}

/* u8 rcsp_buf[1611] __attribute__((aligned(4))); */

////////////////////// RCSP process ///////////////////////////////

OS_SEM rcsp_sem;

void JL_rcsp_resume_do(void)
{
    //rcsp_printf("######## resume_do\n");
    os_sem_post(&rcsp_sem);
}

static u32 rcsp_protocol_tick = 0;
static void rcsp_process_timer()
{
    JL_set_cur_tick(rcsp_protocol_tick++);
    os_sem_post(&rcsp_sem);
    //rcsp_printf("# 500\n");
}
static void rcsp_process_task(void *p)
{
    while (1) {
        os_sem_pend(&rcsp_sem, 0);
        JL_protocol_process();
    }
}

/////////////////////////////////////////////////////
static u8 *rcsp_buffer = NULL;
extern void ble_get_server_operation_table(struct ble_server_operation_t **interface_pt);
void rcsp_init()
{
    if (__this->rcsp_run_flag) {
        return;
    }

    memset((u8 *)__this, 0, sizeof(struct JL_AI_VAR));

    /* __this->start_speech = start_speech; */
    /* __this->stop_speech = stop_speech; */
    //__this->rcsp_user = (struct __rcsp_user_var *)get_user_rcsp_opt();

    rcsp_hid_get_operation_table(&__this->rcsp_hid);
    ble_get_server_operation_table(&__this->rcsp_ble);
    //spp_get_operation_table(&__this->rcsp_spp);

    /* rcsp_printf("rcsp need buf size:%x\n", rcsp_protocol_need_buf_size()); */
    /* rcsp_printf("set buf:%x %x\n", rcsp_buf, sizeof(rcsp_buf)); */
    /* JL_protocol_init(rcsp_buf, sizeof(rcsp_buf)); */

    u32 size = rcsp_protocol_need_buf_size();
    rcsp_printf("rcsp need buf size:%x\n", size);

    rcsp_buffer = zalloc(size);
    ASSERT(rcsp_buffer, "no, memory for rcsp_init\n");
    JL_protocol_init(rcsp_buffer, size);

    os_sem_create(&rcsp_sem, 0);
    /* __this->rcsp_timer_hdl = sys_timer_add(NULL, rcsp_process_timer, 500); */
    int err = task_create(rcsp_process_task, NULL, "rcsp_task");

    //default use ble , can switch spp anytime
#if CONFIG_APP_DONGLE
    rcsp_dev_select_v1(RCSP_HID);
#else
    rcsp_dev_select_v1(RCSP_BLE);
#endif

    __this->rcsp_run_flag = 1;
#if (0 != BT_CONNECTION_VERIFY)
    JL_rcsp_set_auth_flag(1);
#endif
}

#if CONFIG_APP_DONGLE
#include "system/init.h"
int rcsp_hid_late_init(void)
{
    rcsp_init();
    return 0;
}
late_initcall(rcsp_hid_late_init);
#endif

void rcsp_exit(void)
{
#if ((AI_SOUNDBOX_EN==1) && (RCSP_BTMATE_EN==1))
    if (speech_status() == true) {
        rcsp_cancel_speech();
        speech_stop();
    }
    __set_a2dp_sound_detect_counter(50, 250); /*第一个参数是后台检测返回蓝牙的包数目，第二个参数是退出回到原来模式静音的包数目*/
    __this->wait_asr_end = 0;
#endif

    void rcsp_resume(void);
    rcsp_resume();
    rcsp_printf("####  rcsp_exit_cb\n");
    /* if (__this->rcsp_timer_hdl) { */
    /*     sys_timer_del(__this->rcsp_timer_hdl); */
    /* } */
    if (rcsp_buffer) {
        free(rcsp_buffer);
        rcsp_buffer = 0;
    }
    rcsp_update_data_api_unregister();
    task_kill("rcsp_task");
    __this->rcsp_run_flag = 0;
    return;
}






static int JL_rcsp_event_handler(RCSP_MSG msg, int argc, int *argv);
bool rcsp_msg_post(RCSP_MSG msg, int argc, ...)
{
#define  RCSP_MSG_VAL_MAX	8
    int argv[RCSP_MSG_VAL_MAX] = {0};
    bool ret = true;
    va_list argptr;
    va_start(argptr, argc);

    if (argc > (RCSP_MSG_VAL_MAX - 3)) {
        printf("rcsp post msg argc err\n");
        ret = false;
    } else {
        argv[0] = (int)JL_rcsp_event_handler;
        argv[2] = msg;
        for (int i = 0; i < argc; i++) {
            argv[i + 3] = va_arg(argptr, int);
        }

        if (argc >= 2) {
            argv[1] = argc + 1;
        } else {
            argv[1] = 3;
            argc = 3;//不够的， 发够三个参数
        }

        if (os_taskq_post_type("app_core", Q_CALLBACK, argc + 3, argv)) {
            printf("rcsp post msg err\n");
            ret = false;
        }

    }

    printf("rcsp msg send end\n");
    va_end(argptr);

    return ret;
}
typedef struct __RCSP_SCLUST_INFO {
    int dev_logo;
    u32 sclust;
    volatile u8 flag;
} RCSP_SCLUST_INFO;
static RCSP_SCLUST_INFO rcsp_sclust_ply = {
    .flag = 0,
};

//extern int music_play_by_dev_filesclut(int dev_param, u32 sclust);
bool rcsp_dev_sclust_file_play_check(void)
{
    if (rcsp_sclust_ply.flag) {
        rcsp_sclust_ply.flag = 0;
        printf("%s, %d\n", __FUNCTION__, __LINE__);
        //music_play_by_dev_filesclut((int)rcsp_sclust_ply.dev_logo, rcsp_sclust_ply.sclust);
        return true;
    } else {
        return false;
    }
}


extern void JL_rcsp_msg_deal(RCSP_MSG msg, int argc, int *argv);
static int JL_rcsp_event_handler(RCSP_MSG msg, int argc, int *argv)
{
    printf("JL_rcsp_event_handler = %d\n", msg);
    int ret = 0;

    switch (msg) {
    case RCSP_MSG_UPDATE_EQ:
        printf("RCSP_MSG_UPDATE_EQ\n");
        JL_auto_update_sys_info(COMMON_FUNCTION, BIT(COMMON_INFO_ATTR_EQ));
        break;
    case RCSP_MSG_SET_FMTX_POINT:
#if TCFG_APP_FM_EMITTER_EN
    {
        u16 point = (u16)argv[0];
        printf("RCSP_MSG_SET_FMTX_POINT %d\n", point);
        fm_emitter_manage_set_fre(point);

#if TCFG_UI_ENABLE
        ui_set_menu_ioctl(MENU_FM_DISP_FRE, point);
#endif
    }
#endif
    break;

    case RCSP_MSG_BT_SCAN:
        if (argc != 5) {
            printf("RCSP_MSG_BT_SCAN err\n");
            return -1;
        }
        JL_rcsp_update_bt_state(argc, argv);
        break;

    default:
#if RCSP_UPDATE_EN
        JL_rcsp_msg_deal(msg, argc, argv);
#endif
        break;
    }

    return ret;
}
#endif

