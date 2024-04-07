/*********************************************************************************************
    *   Filename        : ble_fmy_fmna.c

    *   Description     :

    *   Author          : JM

    *   Email           : zh-jieli.com

    *   Last modifiled  : 2024-03-05 15:14

    *   Copyright:(c)JIELI  2011-2026  @ , All Rights Reserved.
*********************************************************************************************/
#include "system/app_core.h"
#include "system/includes.h"

#include "app_config.h"
#include "app_action.h"

#include "btstack/btstack_task.h"
#include "btstack/bluetooth.h"
#include "user_cfg.h"
#include "vm.h"
#include "btcontroller_modules.h"
#include "bt_common.h"
#include "3th_profile_api.h"
#include "le_common.h"
#include "rcsp_bluetooth.h"
#include "JL_rcsp_api.h"
#include "custom_cfg.h"
#include "btstack/btstack_event.h"
#include "gatt_common/le_gatt_common.h"
#include "gSensor/fmy/gsensor_api.h"
#include "ble_fmy.h"
#include "ble_fmy_profile.h"
#include "system/malloc.h"
#include "ble_fmy_cfg.h"
#include "ble_fmy_ota.h"
#include "ble_fmy_sensor_uart.h"
#include "ble_fmy_fmna.h"

#if CONFIG_APP_FINDMY

#if LE_DEBUG_PRINT_EN
#define log_info(x, ...)  printf("[BLE_FMY_FMNA]" x "\r\n", ## __VA_ARGS__)
/* #define log_info(x, ...)  r_printf("[BLE_FMY_FMNA]" x "\r\n", ## __VA_ARGS__) */
#define log_info_hexdump  put_buf

#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

//==================================================================================
// !!!!!重要说明事项!!!!!!
// 下列是测试findmy功能需要填入自己的token信息格式示例
// 只能被搜索，不能配对绑定连接; 如要正常使用功能需要向苹果申请自己的token信息替换写入测试

//用户申请的UID,需要修改填入
static const uint8_t fmy_serial_number[16] = "86f9c81176d140f3";
//用户申请的product_data,需要修改填入
static const uint8_t fmy_product_data[8] = {0xF8, 0x6A, 0xE4, 0x3A, 0xC6, 0x7A, 0xCC, 0x6A};//JL

//默认使用的base64的公钥key,一般不需要修改
static const char fmy_Server_Encryption_Key[] = "BJzFrd3QKbdTXTDm5dFtt6jSGxtItVsZ1bEQ6VvzFUXndM9Rjeu+PHFoM+RD8RRHblpLBU42dQcFbjmVzGuWkJY=";
static const char fmy_Signature_Verification_Key[] = "BDNMWnP9Yd82Qz+8aZI245jklBLzwP3E5doLQRh3lRcIcSCIjpeSN3a6SNxRfA+oe5xiqf7paw84QD9mnh5nVWA=";

//用户申请的token,base64格式或者hex格式,需要修改填入
#if UUID_TOKEN_IS_BASE64_MODE
static const char fmy_token_uuid_char[] = "0d9290f5-2e7d-49b1-9aab-259b93f17e0e";
static const char fmy_token_auth_char[] = "MYG/ME8CAQECAQEERzBFAiEA3hgTtFg2nTHkX4iuG1Mxk/KNUCfKv8XIe85voCqFY3YCIH/hZVR/JanIF3LmEnZrUTDb7CVWew8E+setdWaW5oVxMGwCAQICAQEEZDFiMAkCAWYCAQEEAQIwEAIBZQIBAQQIYWnfhIYBAAAwEQICAMoCAQEECAAAAAAAAAAIMBYCAgDJAgEBBA0yNjI5ODMtMjYzMTg0MBgCAWcCAQEEENzutdJeT04ym3Vw0QXUzZA=";

#else

static const uint8_t fmy_token_uuid_hex[16] = {
    0x7E, 0x23, 0x2A, 0x81, 0x13, 0xE7, 0x4F, 0xA6, 0x99, 0x2C, 0x27, 0x23, 0x94, 0x80, 0x72, 0x7A,
};

static const uint8_t fmy_token_auth_hex[] = {
    0x31, 0x81, 0xBF, 0x30, 0x4F, 0x02, 0x01, 0x01, 0x02, 0x01, 0x01, 0x04, 0x47, 0x30, 0x45, 0x02,
    0x21, 0x00, 0xAC, 0xA8, 0xA6, 0x76, 0x03, 0x90, 0x51, 0x7A, 0xE3, 0x82, 0x5A, 0xF0, 0x65, 0x87,
    0x41, 0x6A, 0x83, 0x62, 0xDF, 0x1A, 0x93, 0xA4, 0x90, 0x27, 0x80, 0xA4, 0xF0, 0x2B, 0x9A, 0x72,
    0xD7, 0x56, 0x02, 0x20, 0x43, 0x51, 0xCC, 0x12, 0xA4, 0xB0, 0x21, 0xB5, 0x87, 0x9D, 0x76, 0x2C,
    0x62, 0x3E, 0xB6, 0xBA, 0x8D, 0x09, 0x21, 0x65, 0x9A, 0x91, 0x43, 0x52, 0xE5, 0x26, 0x5D, 0x0E,
    0xA3, 0x92, 0xBA, 0x26, 0x30, 0x6C, 0x02, 0x01, 0x02, 0x02, 0x01, 0x01, 0x04, 0x64, 0x31, 0x62,
    0x30, 0x09, 0x02, 0x01, 0x66, 0x02, 0x01, 0x01, 0x04, 0x01, 0x02, 0x30, 0x10, 0x02, 0x01, 0x65,
    0x02, 0x01, 0x01, 0x04, 0x08, 0x0F, 0x30, 0x3B, 0x23, 0x8C, 0x01, 0x00, 0x00, 0x30, 0x11, 0x02,
    0x02, 0x00, 0xCA, 0x02, 0x01, 0x01, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
    0x30, 0x16, 0x02, 0x02, 0x00, 0xC9, 0x02, 0x01, 0x01, 0x04, 0x0D, 0x32, 0x36, 0x32, 0x39, 0x38,
    0x33, 0x2D, 0x32, 0x36, 0x33, 0x31, 0x38, 0x34, 0x30, 0x18, 0x02, 0x01, 0x67, 0x02, 0x01, 0x01,
    0x04, 0x10, 0x90, 0x08, 0x8F, 0x05, 0x6E, 0xCE, 0x49, 0x08, 0x96, 0xDD, 0x5E, 0xE0, 0x7F, 0x70,
    0x13, 0xB8, 0x31, 0x81, 0xBE, 0x30, 0x4E, 0x02, 0x01, 0x01, 0x02, 0x01, 0x01, 0x04, 0x46, 0x30,
    0x44, 0x02, 0x20, 0x07, 0x29, 0xA0, 0x08, 0xDE, 0x65, 0xB1, 0xBB, 0xC3, 0x13, 0x3F, 0x87, 0x00,
    0xB3, 0x43, 0x44, 0x22, 0x1D, 0xEF, 0x3B, 0xD6, 0x2C, 0x9E, 0x85, 0x6B, 0x9B, 0x81, 0xE3, 0xF5,
    0x6C, 0x45, 0x8B, 0x02, 0x20, 0x3C, 0xDD, 0x99, 0xB0, 0x97, 0x33, 0xDF, 0x18, 0xD0, 0xB0, 0xDB,
    0x8F, 0xB9, 0xBC, 0xE7, 0xF7, 0xE5, 0x42, 0x72, 0xD7, 0x91, 0x64, 0x28, 0xE7, 0xD4, 0x7E, 0xEC,
    0x9B, 0xB6, 0x5D, 0x3F, 0x55, 0x30, 0x6C, 0x02, 0x01, 0x02, 0x02, 0x01, 0x01, 0x04, 0x64, 0x31,
    0x62, 0x30, 0x09, 0x02, 0x01, 0x66, 0x02, 0x01, 0x01, 0x04, 0x01, 0x02, 0x30, 0x10, 0x02, 0x01,
    0x65, 0x02, 0x01, 0x01, 0x04, 0x08, 0xE8, 0x65, 0xEA, 0x22, 0x8C, 0x01, 0x00, 0x00, 0x30, 0x11,
    0x02, 0x02, 0x00, 0xCA, 0x02, 0x01, 0x01, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x30, 0x16, 0x02, 0x02, 0x00, 0xC9, 0x02, 0x01, 0x01, 0x04, 0x0D, 0x32, 0x36, 0x32, 0x39,
    0x38, 0x33, 0x2D, 0x32, 0x36, 0x33, 0x31, 0x38, 0x34, 0x30, 0x18, 0x02, 0x01, 0x67, 0x02, 0x01,
    0x01, 0x04, 0x10, 0x73, 0x8E, 0x17, 0x4D, 0x87, 0xA7, 0x4A, 0xF1, 0x89, 0x4F, 0x1D, 0x4B, 0xC5,
    0x45, 0x9B, 0xF3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

#endif
//==================================================================================
//******************************************************************************************

static const uint16_t fmna_separated_adv_interval[] = {FMY_FMNA_SEPARATED_ADV_FAST_INTERVAL, FMY_FMNA_SEPARATED_ADV_SLOW_INTERVAL};
static const uint16_t fmna_nearby_adv_interval[] =    {FMY_FMNA_NEARBY_ADV_FAST_INTERVAL, FMY_FMNA_NEARBY_ADV_SLOW_INTERVAL};
static const uint16_t fmna_pairing_adv_interval[] =   {FMY_FMNA_PAIRING_ADV_FAST_INTERVAL, FMY_FMNA_PAIRING_ADV_SLOW_INTERVAL};

//******************************************************************************************
static adv_cfg_t  fmy_server_adv_config;
static uint8_t    fmy_adv_data[ADV_RSP_PACKET_MAX];//max is 31
static uint8_t    fmy_scan_rsp_data[ADV_RSP_PACKET_MAX];//max is 31

#if FMY_DEBUG_TEST_MOTION_DETETION//for test
static uint16_t motion_test_id;
static bool motion_test_flag = 1;
#endif

//增加一个flash配置区用于记录绑定和更新token,防止token丢失,
//已存有token，升级时不要擦除,如要替换直接写入新的token信息即可
//配置区在xxx_build_cfg.h定义
static const char fmy_user_config_path[] = "mnt/sdfile/EXT_RESERVED/FINDMY";//配置区路径(isd_config_rule.c)
//***********************************************************************************************
static int fmy_set_adv_enable(uint8_t enable);
static int fmy_set_adv_mode(uint8_t mode);
static int fmy_get_static_mac(uint8_t *mac);
static int fmy_set_static_mac(uint8_t *mac);
static int fmy_get_battery_level(void);
static int fmy_disconnect(uint16_t conn_handle, uint8_t reason);
static void fmy_test_get_send_sensor_data(void);

//***********************************************************************************************
static const fmna_att_handle_t fmna_att_handle_table = {
    .pairing_control_point_handle = ATT_CHARACTERISTIC_4F860001_943B_49EF_BED4_2F730304427A_01_VALUE_HANDLE,
    .owner_cfg_control_point_handle = ATT_CHARACTERISTIC_4F860002_943B_49EF_BED4_2F730304427A_01_VALUE_HANDLE,
    .owner_info_porint_handle = ATT_CHARACTERISTIC_4F860004_943B_49EF_BED4_2F730304427A_01_VALUE_HANDLE,
    .non_owner_control_point_handle = ATT_CHARACTERISTIC_4F860003_943B_49EF_BED4_2F730304427A_01_VALUE_HANDLE,
    .debug_control_point_handle = ATT_CHARACTERISTIC_4F860005_943B_49EF_BED4_2F730304427A_01_VALUE_HANDLE,
    .firmware_update_handle = ATT_CHARACTERISTIC_94110001_6D9B_4225_A4F1_6A4A7F01B0DE_01_VALUE_HANDLE,
};

//============================================================================================
static void port_io_init(uint32_t port_io, uint32_t dir)
{
    gpio_set_die(port_io, 1);
    gpio_set_pull_down(port_io, 0);
    gpio_set_pull_up(port_io, 0);
    gpio_set_direction(port_io, dir);
}

/*************************************************************************************************/
/*!
 *  \brief      reset system
 *
 *  \param      [in] delay_ticks
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void fmy_systerm_reset(uint32_t delay_ticks)
{
    if (delay_ticks) {
        log_info("delay_reset: %u ticks", delay_ticks);
        os_time_dly(delay_ticks);
    }

    log_info("reset start......");
    cpu_reset();
    while (1);
}

/*************************************************************************************************/
/*!
 *  \brief      推送消息到app_core 任务，app处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int fmy_event_post_msg(uint8_t priv_event, void *evt_data, uint32_t value, void *handler)
{
    struct sys_event e;
    e.type = SYS_FMNA_EVENT;
    e.u.fmna.event = priv_event;
    e.u.fmna.value = value;
    e.u.fmna.event_data = evt_data;
    e.u.fmna.handler = handler;

    /* log_info("fmna_app_evnet_post: event=%d，evt_data= %x,value=%02x,handler= %08x,", priv_event, evt_data, value, handler); */
    sys_event_notify(&e);
    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      提供接口reset static 地址
 *
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int fmy_reset_static_mac(uint8_t *mac)
{
    log_info("%s(%d)", __FUNCTION__, __LINE__);
    log_info("make new address");
    mac[0] = rand32() | STATIC_ADV_ADDR_TYPE_MASK;
    mac[1] = rand32();
    mac[2] = rand32();
    mac[3] = rand32();
    mac[4] = rand32();
    mac[5] = rand32();
    log_info_hexdump(mac, 6);
    syscfg_write(CFG_FMNA_BLE_ADDRESS_INFO, mac, 6);
    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      提供接口获取static 地址
 *
 *  \param      [in]mac,--static address
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int fmy_get_static_mac(uint8_t *mac)
{
    log_info("%s(%d)", __FUNCTION__, __LINE__);

#if FMY_MAC_CHANGE_LOCK
    int ret;
    ret = syscfg_read(CFG_FMNA_BLE_ADDRESS_INFO, mac, 6);
    if (ret == 6) {
        log_info("get vm address");
        log_info_hexdump(mac, 6);
        return 0;
    }
#endif

    log_info("get new address");
    fmy_reset_static_mac(mac);
    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      提供接口配置static 地址
 *
 *  \param      [in]mac,--static address
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int fmy_set_static_mac(uint8_t *mac)
{
    log_info("%s(%d)", __FUNCTION__, __LINE__);
    uint8_t new_swap_addr[6];
    ble_set_make_random_address(1);
    swapX(mac, new_swap_addr, 6);
    le_controller_set_random_mac(new_swap_addr);
    log_info_hexdump(new_swap_addr, 6);
    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      提供接口获取电量
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int fmy_get_battery_level(void)
{
    log_info("%s,bat_lev= %d", __FUNCTION__, __fydata->battery_level);
    return __fydata->battery_level;
}

/*************************************************************************************************/
/*!
 *  \brief      提供接口断开链路
 *
 *  \param      [in]conn_handle,reason
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int fmy_disconnect(uint16_t conn_handle, uint8_t reason)
{
    log_info("%s(%d): %04x,%02x", __FUNCTION__, __LINE__, conn_handle, reason);
    return ble_comm_disconnect_extend(conn_handle, reason);
}

#if SOUND_PASSIVE_BUZZER
// 无源蜂鸣器需要交流信号去驱动，用pwm来推信号
static struct pwm_platform_data p_buzzer_pwm_data;
static void fmy_sound_passive_buzzer_init(uint32_t gpio, pwm_ch_num_type pwm_ch)
{
    p_buzzer_pwm_data.pwm_aligned_mode = pwm_edge_aligned;         //边沿对齐
    p_buzzer_pwm_data.frequency = 5000;                            // 5KHz

    p_buzzer_pwm_data.pwm_ch_num = pwm_ch;                        //通道选择
    p_buzzer_pwm_data.duty = 5000;                                 //占空比50%
    // 指定PWM(AC635)硬件管脚的芯片需要注意高低引脚的选择
    p_buzzer_pwm_data.h_pin = -1;                                //没有则填 -1。h_pin_output_ch_num无效，可不配置
    p_buzzer_pwm_data.l_pin = gpio;                               //硬件引脚，l_pin_output_ch_num无效，可不配置
    p_buzzer_pwm_data.complementary_en = 0;                        //两个引脚的波形, 1: 互补, 0: 同步;
    mcpwm_init(&p_buzzer_pwm_data);
}

/*************************************************************************************************/
/*!
 *  \brief     提供开关sound
 *
 *  \param      [in] pwm_ch, on_off: 1 or 0
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void fmy_sound_passive_onoff(pwm_ch_num_type pwm_ch, bool on_off)
{
    // 开关低功耗，pwm模块会在低功耗时被关闭
    fmy_state_idle_set_active(on_off);
    // 开、关pwm输出
    port_io_init(SOUND_GPIO_PORT, !on_off);
    mcpwm_ch_open_or_close(pwm_ch, on_off);

}
#endif

/*************************************************************************************************/
/*!
 *  \brief      定时处理蜂鸣器切换开关状态
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void fmy_sound_timer_control(void)
{
    DEV_SOUND_STATE(__fydata->sound_onoff);
    __fydata->sound_onoff = !__fydata->sound_onoff;
}

/*************************************************************************************************/
/*!
 *  \brief      提供蜂鸣器开关操作操作处理
 *
 *  \param      [in] op
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int fmy_sound_control(FMNA_SOUND_OP_t op)
{
    log_info("func:%s,op= %d", __FUNCTION__, op);
    switch (op) {
    case FMNA_SOUND_INIT:
        DEV_SOUND_INIT();
        DEV_SOUND_STATE(PORT_VALUE_LOW);
        break;

    case FMNA_SOUND_START:
#if TCFG_PWMLED_ENABLE
        fmy_state_idle_set_active(true);
#endif
        __fydata->sound_onoff = 1;
        if (!__fydata->sound_ctrl_timer_id) {
            __fydata->sound_ctrl_timer_id = sys_timer_add(NULL, fmy_sound_timer_control, SOUND_TICKS_MS);
        }
        break;

    case FMNA_SOUND_STOP:
#if TCFG_PWMLED_ENABLE
        fmy_state_idle_set_active(false);
#endif

        if (__fydata->sound_ctrl_timer_id) {
            sys_timer_del(__fydata->sound_ctrl_timer_id);
            __fydata->sound_ctrl_timer_id = 0;
        }
        DEV_SOUND_STATE(PORT_VALUE_LOW);
        break;

    default:
        break;
    }
}

/*************************************************************************************************/
/*!
 *  \brief      配对超时处理
 *
 *  \param      [in] priv --no used
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void fmy_pairing_timeout_handle(void *priv)
{
    __fydata->pairing_mode_timer_id = 0;
    __fydata->pairing_mode_enable = 0;
    if (__fydata->adv_fmna_state == FMNA_SM_PAIR) {
        log_info("%s,%d", __FUNCTION__, __LINE__);
        fmy_set_adv_enable(0);
    }
}

/*************************************************************************************************/
/*!
 *  \brief      配对超时定时器停止
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void fmy_pairing_timeout_stop(void)
{
    if (__fydata->pairing_mode_timer_id) {
        log_info("%s,%d", __FUNCTION__, __LINE__);
        sys_timeout_del(__fydata->pairing_mode_timer_id);
        __fydata->pairing_mode_timer_id = 0;
    }
}

/*************************************************************************************************/
/*!
 *  \brief      配对超时定时器开始
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void fmy_pairing_timeout_start(void)
{
    log_info("%s,%d", __FUNCTION__, __LINE__);
    if (__fydata->pairing_mode_timer_id) {
        fmy_pairing_timeout_stop();
    }
    __fydata->pairing_mode_timer_id = sys_timeout_add(0, fmy_pairing_timeout_handle, FMY_ADV_PAIR_TIMEOUT_MS);
}

/*************************************************************************************************/
/*!
 *  \brief      提供reset 广播adv操作
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void fmy_pairing_restart_adv(void)
{
    if (__fydata->adv_fmna_state == FMNA_SM_PAIR && BLE_ST_ADV != ble_gatt_server_get_work_state()) {
        log_info("%s,%d", __FUNCTION__, __LINE__);
        __fydata->pairing_mode_enable = 1;
        fmy_set_adv_enable(1);
        fmy_pairing_timeout_start();
    }
}

/*************************************************************************************************/
/*!
 *  \brief      提供reset配置地址操作
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void fmy_pairing_reset_address(void)
{
    uint8_t address[6];
    ble_state_e cur_state;

    if (__fydata->adv_fmna_state == FMNA_SM_PAIR) {
        log_info("%s,%d", __FUNCTION__, __LINE__);
        cur_state = ble_gatt_server_get_work_state();
        fmy_set_adv_enable(0);
        fmy_reset_static_mac(address);
        fmy_set_static_mac(address);
        if (BLE_ST_ADV == cur_state) {
            fmy_set_adv_enable(1);
        }
    }
}

/*************************************************************************************************/
/*!
 *  \brief      组织adv包数据，放入buff
 *
 *  \param      [in] fmna_state,fmna_payload,size
 *
 *  \return     0
 *
 *  \note
 */
/*************************************************************************************************/
static int fmy_set_adv_data(uint8_t fmna_state, uint8_t *fmna_payload, uint8_t size)
{
    uint8_t cnt = 0;
    uint8_t *buf = fmy_adv_data;

    switch (fmna_state) {
    case FMNA_SM_PAIR:
        buf[cnt++] = 3;
        buf[cnt++] = 3;
        buf[cnt++] = FMY_UUID_SERVICE & 0xff;
        buf[cnt++] = (FMY_UUID_SERVICE >> 8);
        buf[cnt++] = 3 + size;//3 + size -1;
        buf[cnt++] = 0x16;
        buf[cnt++] = FMY_UUID_SERVICE & 0xff;
        buf[cnt++] = (FMY_UUID_SERVICE >> 8);
        break;

    case FMNA_SM_NEARBY:
        buf[cnt++] = 7;
        buf[cnt++] = 0xff;
        buf[cnt++] = APPLE_VENDOR_ID & 0xff;
        buf[cnt++] = (APPLE_VENDOR_ID >> 8);
        break;

    case FMNA_SM_SEPARATED:
        buf[cnt++] = 0x1E;
        buf[cnt++] = 0xff;
        buf[cnt++] = APPLE_VENDOR_ID & 0xff;
        buf[cnt++] = (APPLE_VENDOR_ID >> 8);
        break;

    default:
        log_info("err_state: fmna_state= %d", fmna_state);
        break;

    }

    __fydata->adv_fmna_state = fmna_state;
    memcpy(&buf[cnt], fmna_payload, size);
    cnt += size;
    log_info("fmy state(%d), adv_data(%d)", fmna_state, cnt);
    log_info_hexdump(buf, cnt);

    fmy_server_adv_config.adv_data_len = cnt;
    fmy_server_adv_config.adv_data = fmy_adv_data;
    fmy_server_adv_config.rsp_data_len = 0;
    fmy_server_adv_config.rsp_data = fmy_scan_rsp_data;
    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      配置广播参数
 *
 *  \param      [in] mode
 *
 *  \return
 *
 *  \note      开广播前配置都有效
 */
/*************************************************************************************************/
static int fmy_set_adv_mode(uint8_t mode)
{
    if (mode > FMNA_ADV_MODE_SLOW) {
        return -1;
    }

    fmy_server_adv_config.adv_auto_do = 0;
    fmy_server_adv_config.adv_type = ADV_IND;
    fmy_server_adv_config.adv_channel = ADV_CHANNEL_ALL;

    switch (__fydata->adv_fmna_state) {
    case FMNA_SM_PAIR:
        log_info("FMNA_SM_PAIR");
        fmy_server_adv_config.adv_interval = fmna_pairing_adv_interval[mode];
        break;
    case FMNA_SM_SEPARATED:
        log_info("FMNA_SM_SEPARATED");
        fmy_server_adv_config.adv_interval = fmna_separated_adv_interval[mode];
        break;
    case FMNA_SM_NEARBY:
        log_info("FMNA_SM_NEARBY");
        fmy_server_adv_config.adv_interval = fmna_nearby_adv_interval[mode];
        if (ble_comm_dev_get_connected_nums(GATT_ROLE_SERVER)) {
            log_info("===set_adv second devices===");
            fmy_server_adv_config.adv_interval = FMY_FMNA_NEARBY_ADV_SLOW_INTERVAL;
        } else {
        }
    default:
        break;

    }

    log_info("set_adv_mode(%d), adv_fmna_state(%d), adv_interval= %04x, adv_type= %d", mode, __fydata->adv_fmna_state, fmy_server_adv_config.adv_interval, fmy_server_adv_config.adv_type);
    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief     开启广播使能
 *
 *  \param      [in] enable
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int fmy_set_adv_enable(uint8_t enable)
{
    log_info("%s(%d): %d", __FUNCTION__, __LINE__, enable);

    if (enable) {

        if (__fydata->adv_fmna_state == FMNA_SM_PAIR) {
            if (0 == __fydata->pairing_mode_enable) {
                log_info("hold Find My network pairing mode!!!");
                return 0;
            } else {
                if (__fydata->pairing_mode_timer_id) {
                    log_info("reset pairing timer");
                    fmy_pairing_timeout_stop();
                    fmy_pairing_timeout_start();
                } else {
                    log_info("start pairing timer");
                    fmy_pairing_timeout_start();
                }
            }
        } else {
            //other state,bonded, default pairing unlock
            __fydata->pairing_mode_enable = 1;
        }

        if (ble_comm_dev_get_connected_nums(GATT_ROLE_SERVER)) {
            log_info("===second dev adv_enable===");
        }

        ble_gatt_server_set_adv_config(&fmy_server_adv_config);
    }

    if (enable && BLE_ST_ADV == ble_gatt_server_get_work_state()) {
        log_info("already adv open!!!");
        return 0;
    }

    ble_gatt_server_adv_enable(enable);
    log_info("set adv done,adv_fmna_state= %d", __fydata->adv_fmna_state);
    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      提供att发送数据接口
 *
 *  \param      [in]conn_handle,att_handle,data,len,op_type
 *
 *  \return     0-success,非0-fail
 *
 *  \note
 */
/*************************************************************************************************/
static int fmy_att_send_data(uint16_t conn_handle, uint16_t att_handle, uint8_t *data, uint16_t len, att_op_type_e op_type)
{
    log_info("fmy_att_send:con_hdl= %04x,att_hdl= %04x,le= %d,op_type= %d", conn_handle, att_handle, len, op_type);
    log_info_hexdump(data, len);
    return ble_comm_att_send_data(conn_handle, att_handle, data, len, op_type);
}

/*************************************************************************************************/
/*!
 *  \brief      认证测试电量变化接口
 *
 *  \param      [in] bat_val--fmna_bat_state_level_t
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void fmy_test_switch_battery_level(uint8_t bat_val)
{
    __fydata->battery_level = bat_val;
    log_info("%s, bat= %d", __FUNCTION__, __fydata->battery_level);
}

/*************************************************************************************************/
/*!
 *  \brief      state callback，状态类别参考FMNA_SM_State_t枚举体
 *
 *  \param      [in]fmy_state, state_string
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static uint16_t fmy_state_callback(FMNA_SM_State_t fmy_state, const char *state_string)
{
    log_info("fmy state change->fmy_state:%s", state_string);
    switch (fmy_state) {
    case FMNA_SM_FMNA_PAIR_COMPLETE:
        log_info(">>>findmy pair success!!");
        break;
    case FMNA_SM_UNPAIR:
        log_info(">>>findmy unpair success!!");
        break;
    default:
        break;
    }
    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      事件定时器处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
#define RED_ON_STATE        1
#define BLUE_ON_STATE       0
static void fmy_event_timer_handle(void)
{
#ifdef LED_PAIR_GAPIO_POR

    static uint8_t cnt = 0;
    putchar('^');
    if (cnt++ % 15 == 0) {
        //    see memory's status
        /* mem_stats(); */
    }

    static uint8_t led_state = RED_ON_STATE; //1-red,0-blue

    //ADV 指示灯状态
    if (ble_gatt_server_get_work_state() == BLE_ST_ADV) {
        putchar('0' + __fydata->adv_fmna_state);
        switch (__fydata->adv_fmna_state) {
        case FMNA_SM_PAIR:
            if (__fydata->pairing_mode_enable) {
                led_state = !led_state;
            } else {
                //hold pair adv
                led_state = BLUE_ON_STATE;
            }
            break;

        case FMNA_SM_NEARBY:
            if (cnt % 3 == 0) {
                led_state = BLUE_ON_STATE;
            } else {
                led_state = RED_ON_STATE;
            }
            break;

        case FMNA_SM_SEPARATED:
            if (cnt % 7 == 0) {
                led_state = BLUE_ON_STATE;
            } else {
                led_state = RED_ON_STATE;
            }
            break;

        default:
            led_state = BLUE_ON_STATE;
            break;

        }
    } else {
        putchar('0');
        led_state = BLUE_ON_STATE;
    }

    if (led_state) {
        DEV_OPEN_LED_STATE();
    } else {
        DEV_CLOSE_LED_STATE();
    }

#endif

}

/*************************************************************************************************/
/*!
 *  \brief      定义fmna库调用接口，给库调用
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static const fmna_app_api_t fmna_app_api_table = {
    .evnet_post_msg = fmy_event_post_msg,
    .set_adv_data = fmy_set_adv_data,
    .set_adv_mode = fmy_set_adv_mode,
    .set_adv_enable = fmy_set_adv_enable,
    .get_mac = fmy_get_static_mac,
    .set_mac = fmy_set_static_mac,
    .att_send_data = fmy_att_send_data,
    .get_battery_level = fmy_get_battery_level,
    .call_disconnect = fmy_disconnect,
    .sound_control = fmy_sound_control,

#if TCFG_GSENSOR_ENABLE
    .sensor_init = sensor_init,
    .sensor_deinit = sensor_deinit,
    .motion_deteted = sensor_motion_detection,
#endif

#if FMY_OTA_SUPPORT_CONFIG
    .uarp_ota_process = fmy_ota_process,
#else
    .uarp_ota_process = NULL,
#endif

    .state_callback = fmy_state_callback,
    .check_capability = fmy_check_capabilities_is_enalbe,
};

/*************************************************************************************************/
/*!
 *  \brief      获取配置去最后的用户信息
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static bool fmy_read_last_user_config(void)
{
    bool result = false;
    uint8_t *ram_buf = zalloc(16 + 16 + 1024);
    fmna_input_cfg_t ouput_cfg;
    log_info("=>read user_cfg info");
    if (ram_buf) {
        ouput_cfg.serial_number = &ram_buf[0];
        ouput_cfg.uuid = &ram_buf[16];
        ouput_cfg.token = &ram_buf[32];
        ret_code_t ret = fmna_user_cfg_read(&ouput_cfg);
        if (ret == FMY_SUCCESS) {
            log_info("serial_number:");
            log_info_hexdump(ouput_cfg.serial_number, 16);
            log_info("uuid:");
            log_info_hexdump(ouput_cfg.uuid, 16);
            log_info("token:");
            log_info_hexdump(ouput_cfg.token, 1024);
            result = true;
        } else {
            log_info("user_cfg is NULL,%d", ret);
        }
        free(ram_buf);
    } else {
        log_info("read malloc fail!");
    }
    return result;
}

/*************************************************************************************************/
/*!
 *  \brief      配置用户认证信息,product_data,uid,uuid,token
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note  token信息写入，慎重修改！！！！！！
 */
/*************************************************************************************************/
static int fmy_set_user_infomation(void)
{
    ret_code_t ret = FMY_SUCCESS;
    fmna_input_cfg_t input_cfg;

    mem_stats();
    //设置配置区的路径,配置区在xxx_build_cfg.h定义
    //patch, product, crypto_enc_key,每次上电都要配置一下
    fmna_user_cfg_set_patch(fmy_user_config_path);
    fmna_set_product_data(fmy_product_data);
    fmna_set_crypto_enc_key_config(fmy_Server_Encryption_Key, fmy_Signature_Verification_Key);

    //!!!配置区写入有效的token,为空时默认写入一次，使用后token会被手机更新
    //注意不要重复写入,以免token被手机更新后,丢失失效
    uint8_t *data_ram = zalloc(16 + 1024);

    if (data_ram) {
        fmna_user_cfg_set_patch(fmy_user_config_path);
        ret = fmna_user_cfg_open();
        if (ret != FMY_SUCCESS) {
            log_info("open user_cfg fail,%d", ret);
            goto w_end_free;
        }


        //先检查配置区是否已存入token
        if (fmna_user_cfg_is_exist()) {
            //用户可确定保存的token是否有效，如果无效可修改流程，写入新的token替换
            //注意不要重复写入,以免token被手机更新后,丢失失效
            log_info("===exist fmy_cfg info!");
            fmy_read_last_user_config();
            fmna_user_cfg_close();
            goto w_end_free;
        }

        //配置区存在,但token为空才写入
        input_cfg.serial_number = fmy_serial_number;
        input_cfg.uuid = data_ram;
        input_cfg.token = &data_ram[16];

#if UUID_TOKEN_IS_BASE64_MODE
        //base64格式转换
        int len = uuid_str_to_hex(fmy_token_uuid_char, input_cfg.uuid);
        if (len != 16) {
            log_info("err uuid char");
            ret = FMY_ERROR_INVALID_DATA;
        } else {
            len = fmna_Base64Decode(fmy_token_auth_char, input_cfg.token, 1024);
            if (len == 0) {
                log_info("err token char");
                ret = FMY_ERROR_INVALID_DATA;
            }
        }
#else
        //hex格式copy
        memcpy(input_cfg.uuid, fmy_token_uuid_hex, 16);
        memcpy(input_cfg.token, fmy_token_auth_hex, sizeof(fmy_token_auth_hex));
#endif

        if (ret == FMY_SUCCESS) {
            //写入完需要判断返回值,确定是否成功
            ret = fmna_user_cfg_write(&input_cfg);
            if (ret == FMY_SUCCESS) {
                log_info("===update fmy_cfg success!");
                //read and display
                fmy_read_last_user_config();
            }
            fmna_user_cfg_close();
        }

w_end_free:
        free(data_ram);
    } else {
        ret = FMY_ERROR_NO_MEM;
    }

    if (ret == FMY_SUCCESS) {
        log_info("set user_cfg succ");
    } else {
        log_info("set user_cfg fail,%d", ret);
    }

    return ret;
}

/*************************************************************************************************/
/*!
 *  \brief      设备初始化信息
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void fmy_devices_init(void)
{
    log_info("%s", __FUNCTION__);

#ifdef LED_KEY_GAPIO_PORT
    port_io_init(LED_KEY_GAPIO_PORT, PORT_DIR_OUPUT);
    gpio_write(LED_KEY_GAPIO_PORT, PORT_VALUE_HIGH);
#endif

    DEV_SOUND_INIT();
    DEV_SOUND_STATE(PORT_VALUE_HIGH);

#ifdef LED_PAIR_GAPIO_POR
    port_io_init(LED_PAIR_GAPIO_POR, PORT_DIR_OUPUT);
    gpio_write(LED_PAIR_GAPIO_POR, PORT_VALUE_LOW);
#endif

    os_time_dly(100);

#ifdef LED_KEY_GAPIO_PORT
    gpio_write(LED_KEY_GAPIO_PORT, PORT_VALUE_LOW);
#endif

//    DEV_SOUND_STATE(PORT_VALUE_LOW);

#ifdef LED_PAIR_GAPIO_POR
    sys_timer_add(0, fmy_event_timer_handle, 1000);
#endif

}

void fmy_fmna_adv_cofig_init(void)
{
    fmy_server_adv_config.adv_auto_do = 0;
    ble_gatt_server_set_adv_config(&fmy_server_adv_config);
}

/*************************************************************************************************/
/*!
 *  \brief      fmna模块初始化
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void fmy_fmna_init(void)
{
    log_info("fmna_init, uarp_en %d, %s,%s", FMY_OTA_SUPPORT_CONFIG, __DATE__, __TIME__);
    log_info("production mode= %d, fmy_version= %d", TOKEN_ID_PRODUCTION_MODE,
             FMY_FW_VERSION_MAJOR_NUMBER * 100 + FMY_FW_VERSION_MINOR_NUMBER * 10 + FMY_FW_VERSION_REVISION_NUMBER);
    ble_rf_vendor_set_config(ble_rf_vendor_get_config() | BIT(2));
    fmy_devices_init();
    //用户信息配置
    fmy_set_user_infomation();
    //配置库可用VM id
    fmna_config_user_vm_rang(CFG_FMNA_SOFTWARE_AUTH_START, CFG_FMNA_SOFTWARE_AUTH_END);
    //配置库可用api
    fmna_set_app_api(&fmna_app_api_table);
    //配置库可用att handle 表
    fmna_gatt_set_att_handle_table(&fmna_att_handle_table);
    //配置库的上报的 firmware version
    fmna_version_init(FMY_FW_VERSION_MAJOR_NUMBER, FMY_FW_VERSION_MINOR_NUMBER, FMY_FW_VERSION_REVISION_NUMBER);
    //启动库初始化运行
    fmna_main_start();
    log_info("reboot fmna state: %d", fmna_get_current_state());

}

/*************************************************************************************************/
/*!
 *  \brief      led超时状态处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/

static void fmy_led_timeout_handle(uint32_t priv)
{
#ifdef LED_KEY_GAPIO_PORT
    gpio_write(LED_KEY_GAPIO_PORT, PORT_VALUE_LOW);
#endif
}

#if FMY_DEBUG_TEST_MOTION_DETETION
//测试sensor数据获取
static void fmy_test_get_send_sensor_data(void)
{

#if FMY_DEBUG_SENSOR_TO_UART_ENBALE
    log_info("sensor to uart");
    sensor_uart_send_data();
#else
    if (sensor_motion_detection()) {
        log_info("sensor is moving!!");
    } else {
        log_info("sensor is static!!");
    }
#endif

}
#endif

/*************************************************************************************************/
/*!
 *  \brief      支持进入测试盒可连接状态，用于频偏校准等
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
#if FMY_SUPPORT_TEST_BOX_MODE
//控制是否可以广播给测试盒连接(广播包中要带名字)
static const char fmy_test_box_name[] = "fmy_testbox_mode";
static bool fmy_enter_testbox_mode(uint8_t enable)
{
    log_info(">>>test to enable testbox:%d", enable);
    __fydata->testbox_mode_enable = enable;
    fmy_set_adv_enable(0);

    if (!enable) {
        fmy_systerm_reset(0);
        return true;
    }

    __fydata->pairing_mode_enable = enable;

    uint8_t offset = 0;
    uint8_t *buf = fmy_adv_data;

    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, FLAGS_GENERAL_DISCOVERABLE_MODE | FLAGS_EDR_NOT_SUPPORTED, 1);

    ble_comm_set_config_name(fmy_test_box_name, 0);
    char *gap_name = fmy_test_box_name;
    uint8_t name_len = strlen(gap_name);
    uint8_t vaild_len = ADV_RSP_PACKET_MAX - (offset + 2);
    if (name_len > vaild_len) {
        name_len = vaild_len;
    }

    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)gap_name, name_len);

    if (offset > ADV_RSP_PACKET_MAX) {
        log_info("***adv_data overflow!!!!!!\n");
        return false;
    }
    log_info("testbox_adv_data(%d):", offset);
    log_info_hexdump(buf, offset);

    uint8_t tmp_ble_addr[6];
    ble_op_set_own_address_type(0);//use public
    memcpy(tmp_ble_addr, bt_get_mac_addr(), 6);
    le_controller_set_mac((void *)tmp_ble_addr);
    log_info("public mac addr:", offset);
    log_info_hexdump(tmp_ble_addr, 6);

    fmy_server_adv_config.adv_data_len = offset;
    fmy_server_adv_config.adv_data = fmy_adv_data;
    fmy_server_adv_config.rsp_data_len = 0;
    fmy_server_adv_config.rsp_data = fmy_scan_rsp_data;

    fmy_server_adv_config.adv_auto_do = 0;
    fmy_server_adv_config.adv_type = ADV_IND;
    fmy_server_adv_config.adv_channel = ADV_CHANNEL_ALL;
    fmy_server_adv_config.adv_interval = 0x30;
    ble_gatt_server_set_adv_config(&fmy_server_adv_config);

    log_info(">>>enable testbox adv");
    fmy_set_adv_enable(1);
    return true;

}
#endif

/*************************************************************************************************/
/*!
 *  \brief      按键事件消息处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void fmy_ble_key_event_handler(uint8_t event_type, uint8_t key_value)
{
    log_info("%s: event= %d,key= %d", __FUNCTION__, event_type, key_value);

    if (event_type == KEY_EVENT_DOUBLE_CLICK) {
        switch (key_value) {
        case TCFG_ADKEY_VALUE0:
#if FMY_DEBUG_TEST_MOTION_DETETION
            if (motion_test_flag) {
                sensor_init();
                motion_test_id = sys_timer_add(NULL, fmy_test_get_send_sensor_data, 10000);
            } else {
                sys_timer_del(motion_test_id);
                motion_test_id = 0;
                sensor_deinit();
            }
            motion_test_flag = !motion_test_flag;
#endif

        case TCFG_ADKEY_VALUE1:
#if FMY_SUPPORT_TEST_BOX_MODE
            if (__fydata->adv_fmna_state == FMNA_SM_PAIR) {
                fmy_enter_testbox_mode(1);
            }
#endif
            break;

        case TCFG_ADKEY_VALUE2:
#if FMY_SUPPORT_TEST_BOX_MODE
            if (__fydata->testbox_mode_enable) {
                fmy_enter_testbox_mode(0);
            }
#endif
            break;

        }
    }

    if (event_type == KEY_EVENT_CLICK) {
        switch (key_value) {
        case TCFG_ADKEY_VALUE0:
            log_info(">>>test to enable start pairing adv");
#if FMY_MAC_CHANGE_LOCK
            fmy_pairing_reset_address();
#endif
            fmy_pairing_restart_adv();
            break;

        case TCFG_ADKEY_VALUE1:
        case TCFG_ADKEY_VALUE7:
#if FMY_FMCA_TEST_MODE
            log_info(">>>test to low battery");
            fmy_test_switch_battery_level(BAT_STATE_CRITICALLY_LOW);
#endif
            break;

        case TCFG_ADKEY_VALUE3:
        case TCFG_ADKEY_VALUE8:
#if FMY_FMCA_TEST_MODE
            log_info(">>>test to enable serialnumber_lookup read");
            fmna_paired_serialnumber_lookup_enable(1);
#endif
            break;

        default:
            break;
        }
    }

    if (event_type == KEY_EVENT_LONG) {
        switch (key_value) {
        case TCFG_ADKEY_VALUE0:
            log_info(">>>test to reset accessory");
            __fy_vm->reset_config = 1;
            fmy_vm_deal(__fy_vm, 1);
            fmna_plaform_reset_config();
            fmy_systerm_reset(30);

            break;

        case TCFG_ADKEY_VALUE1:
        case TCFG_ADKEY_VALUE7:
            log_info(">>>test to full battery");
            fmy_test_switch_battery_level(BAT_STATE_FULL);
            break;

        case TCFG_ADKEY_VALUE3:
        case TCFG_ADKEY_VALUE8:
            log_info(">>>test to disable serialnumber_lookup read");
            fmna_paired_serialnumber_lookup_enable(0);
            break;

        default:
            break;
        }
    }

    if (event_type == KEY_EVENT_CLICK || event_type == KEY_EVENT_LONG) {
#ifdef LED_KEY_GAPIO_PORT
        gpio_write(LED_KEY_GAPIO_PORT, PORT_VALUE_HIGH);
        sys_timeout_add(0, fmy_led_timeout_handle, 500);
#endif
    }

}

#endif



