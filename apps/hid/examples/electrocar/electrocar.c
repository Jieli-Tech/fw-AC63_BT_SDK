/*********************************************************************************************
    *   Filename        : electrocar.c

    *   Description     :

    *   Author          : ZQ

    *   Email           : zh-jieli.com

    *   Last modifiled  : 2022-08-12 11:14

*********************************************************************************************/
#include "system/app_core.h"
#include "system/includes.h"
#include "server/server_core.h"
#include "app_config.h"
#include "app_action.h"
#include "os/os_api.h"
#include "btcontroller_config.h"
#include "btctrler/btctrler_task.h"
#include "config/config_transport.h"
#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "bt_common.h"
#include "edr_hid_user.h"
#include "le_common.h"
#include <stdlib.h>
#include "standard_hid.h"
#include "rcsp_bluetooth.h"
#include "app_charge.h"
#include "app_power_manage.h"
#include "app_chargestore.h"
#include "app_comm_bt.h"
#include "led_api.h"

#if CONFIG_APP_ELECTROCAR

#include "electrocar.h"

#define LOG_TAG_CONST       ELECTROCAR
#define LOG_TAG             "[ELECTROCAR]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_AUDIO_ENABLE
#include "tone_player.h"
#include "media/includes.h"
#include "key_event_deal.h"
extern void midi_paly_test(u32 key);
#endif/*TCFG_AUDIO_ENABLE*/

#define trace_run_debug_val(x)   //log_info("\n## %s: %d,  0x%04x ##\n",__FUNCTION__,__LINE__,x)

//---------------------------------------------------------------------
#if SNIFF_MODE_RESET_ANCHOR
#define SNIFF_MODE_TYPE               SNIFF_MODE_ANCHOR
#define SNIFF_CNT_TIME                1/////<空闲?S之后进入sniff模式

#define SNIFF_MAX_INTERVALSLOT        16
#define SNIFF_MIN_INTERVALSLOT        16
#define SNIFF_ATTEMPT_SLOT            2
#define SNIFF_TIMEOUT_SLOT            1
#define SNIFF_CHECK_TIMER_PERIOD      200
#else

#define SNIFF_MODE_TYPE               SNIFF_MODE_DEF
#define SNIFF_CNT_TIME                5/////<空闲?S之后进入sniff模式

#define SNIFF_MAX_INTERVALSLOT        800
#define SNIFF_MIN_INTERVALSLOT        100
#define SNIFF_ATTEMPT_SLOT            4
#define SNIFF_TIMEOUT_SLOT            1
#define SNIFF_CHECK_TIMER_PERIOD      1000
#endif

//默认配置
static const edr_sniff_par_t electrocar_sniff_param = {
    .sniff_mode = SNIFF_MODE_TYPE,
    .cnt_time = SNIFF_CNT_TIME,
    .max_interval_slots = SNIFF_MAX_INTERVALSLOT,
    .min_interval_slots = SNIFF_MIN_INTERVALSLOT,
    .attempt_slots = SNIFF_ATTEMPT_SLOT,
    .timeout_slots = SNIFF_TIMEOUT_SLOT,
    .check_timer_period = SNIFF_CHECK_TIMER_PERIOD,
};


static bt_mode_e bt_hid_mode;
static hid_vm_cfg_t cur_info;//当前使用iden通道信息
static u8 blue_state, work_state, work_state1;
static volatile u8 is_electrocar_active = 0;//1-临界点,系统不允许进入低功耗，0-系统可以进入低功耗
static u16 g_auto_shutdown_timer = 0;

/* #define __this  (bb_hdl) */
//----------------------------------
static const u8 electrocar_report_map[] = {
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x09, 0xE9,        //   Usage (Volume Increment)
    0x09, 0xEA,        //   Usage (Volume Decrement)
    0x09, 0xCD,        //   Usage (Play/Pause)
    0x09, 0xE2,        //   Usage (Mute)
    0x09, 0xB6,        //   Usage (Scan Previous Track)
    0x09, 0xB5,        //   Usage (Scan Next Track)
    0x09, 0xB3,        //   Usage (Fast Forward)
    0x09, 0xB4,        //   Usage (Rewind)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x10,        //   Report Count (16)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
    // 35 bytes
};



//----------------------------------
static const u16 hid_key_click_table[8] = {
    CONSUMER_PLAY_PAUSE,
    CONSUMER_SCAN_PREV_TRACK,
    CONSUMER_VOLUME_DEC,
    CONSUMER_SCAN_NEXT_TRACK,
    CONSUMER_VOLUME_INC,
    CONSUMER_MUTE,
    0,
    0,
};

static const u16 hid_key_hold_table[8] = {
    0,
    CONSUMER_SCAN_FRAME_BACK,
    CONSUMER_VOLUME_DEC,
    CONSUMER_SCAN_FRAME_FORWARD,
    CONSUMER_VOLUME_INC,
    0,
    0,
    0,
};

//----------------------------------
static const edr_init_cfg_t electrocar_edr_config = {
    .page_timeout = 8000,
    .super_timeout = 8000,
    .io_capabilities = 3,
    .passkey_enable = 0,
    .authentication_req = 2,
    .oob_data = 0,
    .sniff_param = &electrocar_sniff_param,
    .class_type = BD_CLASS_KEYBOARD,
    .report_map = electrocar_report_map,
    .report_map_size = sizeof(electrocar_report_map),
};

//----------------------------------
static const ble_init_cfg_t electrocar_ble_config = {
    .same_address = 0,
    .appearance = BLE_APPEARANCE_HID_KEYBOARD,
    .report_map = electrocar_report_map,
    .report_map_size = sizeof(electrocar_report_map),
};

#if TCFG_USER_BLE_ENABLE
extern void le_hogp_disconnect(void);
#endif
extern void p33_soft_reset(void);
extern void ble_hid_key_deal_test(u16 key_msg);
extern void ble_module_enable(u8 en);
static void all_param_scan(void);
static void electrocar_set_soft_poweroff(void);
static void electrocar_app_select_btmode(u8 mode);
static void electrocar_vm_deal(hid_vm_cfg_t *info, u8 rw_flag);
static int electrocar_send_data(u8 channel, u8 *data, u16 len);
extern u8 *ble_cur_connect_addrinfo(void);
/* void electrocar_reject_callback(u8 *buffer, u16 buffer_size); */

/*************************************************************************************************/
/*!
 *  \brief      初始化引脚
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void electrocar_control_io_init(void)
{
    //set output
    gpio_set_direction(LOCK_MOTOR_PIN, 0);
    gpio_set_direction(LOCK_LED_PIN, 0);

    //set input
    /* gpio_set_direction(CHECK_OTHER_LAMP_PIN, 1);//set input direction */
    /* gpio_set_die(CHECK_OTHER_LAMP_PIN, 1);      //set digital input */
    /* gpio_set_pull_down(CHECK_OTHER_LAMP_PIN, 0);//close pulldown */
    /* gpio_set_pull_up(CHECK_OTHER_LAMP_PIN, 0);  //close pullup */

    //set ad input
    /* adc_add_sample_ch(AD_CH_PB6);//pb6 */
}

/*************************************************************************************************/
/*!
 *  \brief      输出引脚调用
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void electrocar_control_io_out(u32 gpio, u32 value)
{
    gpio_write(gpio, value);
}

/*************************************************************************************************/
/*!
 *  \brief      引脚读取调用
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int electrocar_control_io_in(u32 gpio)
{
    return gpio_read(gpio);
}

/*************************************************************************************************/
/*!
 *  \brief      UI启动
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void electrocar_ui_start_up(void)
{
    log_info("electrocar_ui_start_up");
    /* ui初始化 UI实时扫描*/
    led_api_start_ui();
}

/*************************************************************************************************/
/*!
 *  \brief      UI熄火
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void electrocar_ui_shutdown(void)
{
#if LED_UI_EN
    led_api_main_entrance(UI_SHUTDOWM, 1, 0, 0);
#endif
}

/*************************************************************************************************/
/*!
 *  \brief      删除auto关机
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void electrocar_auto_shutdown_disable(void)
{
    log_info("----%s", __FUNCTION__);
    if (g_auto_shutdown_timer) {
        sys_timeout_del(g_auto_shutdown_timer);
    }
}

/*************************************************************************************************/
/*!
 *  \brief      按键处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void electrocar_app_key_deal_test(u8 key_type, u8 key_value)
{
    u16 key_msg = 0;
    u16 key_msg_up = 0;

    /*Audio Test Demo*/
#if TCFG_AUDIO_ENABLE
    if (key_type == KEY_EVENT_CLICK && key_value == TCFG_ADKEY_VALUE0) {
        printf(">>>key0:mic/encode test\n");
        //AC695N/AC696N mic test
        /* extern int audio_adc_open_demo(void); */
        /* audio_adc_open_demo(); */
        //AD697N/AC897N/AC698N mic test
        /* extern void audio_adc_mic_demo(u8 mic_idx, u8 gain, u8 mic_2_dac); */
        /* audio_adc_mic_demo(1, 1, 1); */

        /*encode test*/
        /* extern int audio_mic_enc_open(int (*mic_output)(void *priv, void *buf, int len), u32 code_type); */
        /* audio_mic_enc_open(NULL, AUDIO_CODING_OPUS);//opus encode test */
        /* audio_mic_enc_open(NULL, AUDIO_CODING_SPEEX);//speex encode test  */


        /*
        //AC632N
        编码测试类型：
        AUDIO_CODING_LC3
        AUDIO_CODING_USBC
        */

        extern int audio_demo_enc_open(int (*demo_output)(void *priv, void *buf, int len), u32 code_type, u8 ai_type);
        audio_demo_enc_open(NULL, AUDIO_CODING_USBC, 0);



        /*midi test*/

        //printf(">>>key0:open midi\n");
        //	midi_paly_test(KEY_IR_NUM_0);

    }
    if (key_type == KEY_EVENT_CLICK && key_value == TCFG_ADKEY_VALUE1) {
        printf(">>>key1:tone/decode test\n");
        //AC695N/AC696N tone play test
        /* tone_play_by_path(TONE_NORMAL, 1); */
        /* tone_play_by_path(TONE_BT_CONN, 1); */
        //AD697N/AC897N/AC698N tone play test
        /* tone_play(TONE_NUM_8, 1); */
        /* tone_play(TONE_SIN_NORMAL, 1); */

        //	printf(">>>key0:set  midi\n");
        //	midi_paly_test(KEY_IR_NUM_1);


        /*
        //AC632N
        解码测试类型：(需要在audio_decode.c中配置)
        AUDIO_CODING_LC3
        AUDIO_CODING_USBC
        */

        extern void demo_frame_test(void);
        demo_frame_test();
    }

    if (key_type == KEY_EVENT_CLICK && key_value == TCFG_ADKEY_VALUE2) {
        //	printf(">>>key2:play  midi\n");
        //	midi_paly_test(KEY_IR_NUM_2);
    }

#endif/*TCFG_AUDIO_ENABLE*/

    if (key_type == KEY_EVENT_CLICK) {
        key_msg = hid_key_click_table[key_value];
    } else if (key_type == KEY_EVENT_HOLD) {
        key_msg = hid_key_hold_table[key_value];
    } else if (key_type == KEY_EVENT_UP) {
        log_info("key_up_val = %02x\n", key_value);
        if (bt_hid_mode == HID_MODE_EDR) {
#if TCFG_USER_EDR_ENABLE
            bt_comm_edr_sniff_clean();
            edr_hid_data_send(1, (u8 *)&key_msg_up, 2);
#endif
        } else {
#if TCFG_USER_BLE_ENABLE
            ble_hid_data_send(1, &key_msg_up, 2);
#endif
        }
        return;
    }

    if (key_msg) {
        log_info("key_msg = %02x\n", key_msg);
        if (bt_hid_mode == HID_MODE_EDR) {
#if TCFG_USER_EDR_ENABLE
            bt_comm_edr_sniff_clean();
            edr_hid_data_send(1, (u8 *)&key_msg, 2);
            if (KEY_EVENT_HOLD != key_type) {
                edr_hid_data_send(1, (u8 *)&key_msg_up, 2);
            }
#endif
        } else {
#if TCFG_USER_BLE_ENABLE
            ble_hid_data_send(1, &key_msg, 2);
            if (KEY_EVENT_HOLD != key_type) {
                ble_hid_data_send(1, &key_msg_up, 2);
            }
#endif
        }
        return;
    }

    if (key_type == KEY_EVENT_TRIPLE_CLICK
        && (key_value == TCFG_ADKEY_VALUE3 || key_value == TCFG_ADKEY_VALUE0)) {
        electrocar_set_soft_poweroff();
        return;
    }

    if (key_type == KEY_EVENT_DOUBLE_CLICK && key_value == TCFG_ADKEY_VALUE0) {
#if (TCFG_USER_EDR_ENABLE && TCFG_USER_BLE_ENABLE)
        is_electrocar_active = 1;
        if (HID_MODE_BLE == bt_hid_mode) {
            electrocar_app_select_btmode(HID_MODE_EDR);
        } else {
            electrocar_app_select_btmode(HID_MODE_BLE);
        }
        os_time_dly(WAIT_DISCONN_TIME_MS / 10); //for disconnect ok
        p33_soft_reset();
        while (1);
#endif
    }
}

/*************************************************************************************************/
/*!
 *  \brief      绑定信息 VM读写操作
 *
 *  \param      [in]rw_flag: 0-read vm,1--write
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
#define	HID_VM_HEAD_TAG (0x3AA3)
static void electrocar_vm_deal(hid_vm_cfg_t *info, u8 rw_flag)
{
    /* hid_vm_cfg_t info; */
    int ret;
    int vm_len = sizeof(hid_vm_cfg_t);

    log_info("-hid_info vm_do:%d\n", rw_flag);
    /* memset(&info, 0, vm_len); */

    if (rw_flag == 0) {
        bt_hid_mode = HID_MODE_NULL;
        ret = syscfg_read(CFG_AAP_MODE_INFO, (u8 *)info, vm_len);

        if (!ret) {
            log_info("-null--\n");
        } else {
            if (HID_VM_HEAD_TAG == info->head_tag) {
                log_info("-exist--\n");
                log_info_hexdump(info, vm_len);
                bt_hid_mode = info->mode;
            }
        }

        if (HID_MODE_NULL == bt_hid_mode) {
#if TCFG_USER_BLE_ENABLE
            bt_hid_mode = HID_MODE_BLE;
#else
            bt_hid_mode = HID_MODE_EDR;
#endif
        } else {
            if (!TCFG_USER_BLE_ENABLE) {
                bt_hid_mode = HID_MODE_EDR;
            }

            if (!TCFG_USER_EDR_ENABLE) {
                bt_hid_mode = HID_MODE_BLE;
            }

            if (bt_hid_mode != info->mode) {
                log_info("-write00--\n");
                info->mode = bt_hid_mode;
                syscfg_write(CFG_AAP_MODE_INFO, (u8 *)info, vm_len);
            }
        }
    } else {
        info->mode = bt_hid_mode;
        info->head_tag = HID_VM_HEAD_TAG;
        syscfg_write(CFG_AAP_MODE_INFO, (u8 *)info, vm_len);
        log_info("-write11--\n");
        log_info_hexdump((u8 *)info, vm_len);
    }
}

/*************************************************************************************************/
/*!
 *  \brief      数据发送接口
 *
 *  \param      [in] data1~data16
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int electrocar_send_data(u8 channel, u8 *data, u16 len)
{
#if TCFG_USER_BLE_ENABLE
    return ble_hid_transfer_channel_send1(data, len);
#endif
}

/*************************************************************************************************/
/*!
 *  \brief      APP 获取状态
 *
 *  \param      [in] data1~data4
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
int get_electrocar_state(u8 *buffer)
{
    if (work_state == APP_TO_DEVICE_IGNITION) {
        buffer[0] = 1;
    } else {
        buffer[0] = 0;
    }

    if (work_state1 == APP_TO_DEVICE_UNLOCK) {
        buffer[1] = 1;
    } else {
        buffer[1] = 0;
    }

    buffer[2] = 0;
    buffer[3] = 0;
    return ELECTROCAR_DATA_LONG;
}

/*************************************************************************************************/
/*!
 *  \brief      进入软关机
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void electrocar_set_soft_poweroff(void)
{
    log_info("electrocar_set_soft_poweroff\n");
    is_electrocar_active = 1;

#if TCFG_USER_BLE_ENABLE
    btstack_ble_exit(0);
#endif

#if TCFG_USER_EDR_ENABLE
    btstack_edr_exit(0);
#endif

#if (TCFG_USER_EDR_ENABLE || TCFG_USER_BLE_ENABLE)
    //延时300ms，确保BT退出链路断开
    sys_timeout_add(NULL, power_set_soft_poweroff, WAIT_DISCONN_TIME_MS);
#else
    power_set_soft_poweroff();
#endif
}

static void electrocar_timer_handle_test(void)
{
    log_info("not_bt");
}

/*************************************************************************************************/
/*!
 *  \brief      app 入口
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
extern void bt_pll_para(u32 osc, u32 sys, u8 low_power, u8 xosc);
static void electrocar_app_start()
{
    log_info("=======================================");
    log_info("-------------HID DEMO-----------------");
    log_info("=======================================");
    log_info("app_file: %s", __FILE__);

    clk_set("sys", BT_NORMAL_HZ);

    //有蓝牙
#if (TCFG_USER_EDR_ENABLE || TCFG_USER_BLE_ENABLE)
    u32 sys_clk =  clk_get("sys");
    bt_pll_para(TCFG_CLOCK_OSC_HZ, sys_clk, 0, 0);

#if TCFG_USER_EDR_ENABLE
    btstack_edr_start_before_init(&electrocar_edr_config, 0);
#endif

#if TCFG_USER_BLE_ENABLE
    btstack_ble_start_before_init(&electrocar_ble_config, 0);
    /* le_hogp_set_reconnect_adv_cfg(ADV_IND, 5000); */
    le_hogp_set_reconnect_adv_cfg(ADV_DIRECT_IND_LOW, 5000);
#endif

    btstack_init();

#else
    //no bt,to for test
    /* sys_timer_add(NULL, electrocar_timer_handle_test, 1000); */
#endif

    /* 按键消息使能 */
    sys_key_event_enable();

    //io控制
    electrocar_control_io_init();

    //ui init
#if LED_UI_EN
    led_api_init();
#endif

#if TCFG_AUDIO_ENABLE
    //tone palyer test
    log_info("tone_playr");
    tone_play(TONE_NUM_0, 1);
#endif

    //one_line init
#if TCFG_ONE_PARSE_ENABLE
    timer_cap_one_parse_test();
#endif

    //nrf init
#if TCFG_NFC_ENABLE
    timer_cap_nrf_test();
#endif

    //433 init
#if TCFG_433_ENABLE
    timer_cap_433_test();
#endif

    //init state
    work_state = SHUTDOWM_APP_STATE;
    work_state1 = DEVICE_LOCK;
    electrocar_control_io_out(LOCK_MOTOR_PIN, 0);
    electrocar_control_io_out(LOCK_LED_PIN, 0);


#if (TCFG_HID_AUTO_SHUTDOWN_TIME)
    //无操作定时软关机
    g_auto_shutdown_timer = sys_timeout_add(NULL, electrocar_set_soft_poweroff, TCFG_HID_AUTO_SHUTDOWN_TIME * 1000);
#endif

}


/*************************************************************************************************/
/*!
 *  \brief      app  状态处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int electrocar_state_machine(struct application *app, enum app_state state, struct intent *it)
{
    switch (state) {
    case APP_STA_CREATE:
        break;
    case APP_STA_START:
        if (!it) {
            break;
        }
        switch (it->action) {
        case ACTION_ELECTROCAR:
            electrocar_app_start();
            break;
        }
        break;
    case APP_STA_PAUSE:
        break;
    case APP_STA_RESUME:
        break;
    case APP_STA_STOP:
        break;
    case APP_STA_DESTROY:
        log_info("APP_STA_DESTROY\n");
        break;
    }

    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      433消息处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void return_433_notify(u8 *buffer, u8 size)
{
    /* memcpy(cur_433_buffer, buffer, size); */

    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg  = DEVICE_433_MASSAGE_NOTIFY;
    e.u.nf4.event = 0;
    e.u.nf4.packet = buffer;
    e.u.nf4.size = size;
    sys_event_notify(&e);
}

static int app_433_massage_notify(struct notify_433 *nf4)
{
    log_info("reject 433 data");
    put_buf(nf4->packet, nf4->size);
}

/*************************************************************************************************/
/*!
 *  \brief      nfc消息处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void return_nfc_notify(u8 *buffer, u8 size)
{
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg  = DEVICE_NFC_MASSAGE_NOTIFY;
    e.u.nf4.event = 0;
    e.u.nf4.packet = buffer;
    e.u.nf4.size = size;
    sys_event_notify(&e);
}

static int app_nfc_massage_notify(struct notify_nfc *nfn)
{
    log_info("reject nfc data");
    put_buf(nfn->packet, nfn->size);
}

/*************************************************************************************************/
/*!
 *  \brief      oneparse消息处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void return_onepa_notify(u8 *buffer, u8 size)
{
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg  = DEVICE_ONEPA_MASSAGE_NOTIFY;
    e.u.nf4.event = 0;
    e.u.nf4.packet = buffer;
    e.u.nf4.size = size;
    sys_event_notify(&e);
}

static int app_onepa_massage_notify(struct notify_onepa *nfo)
{
    log_info("reject onepa data");
    put_buf(nfo->packet, nfo->size);
}

/*************************************************************************************************/
/*!
 *  \brief      公共消息处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int app_device_massage_notify(struct device_event *dev)
{

    //IO 触发IO去做什么?
    /* #if LED_UI_EN */
    /*     if (dev->value & UI_LEFT_STEER) { */
    /*         #<{(| r_printf("open left"); |)}># */
    /*         led_api_main_entrance(UI_LEFT_STEER, 1, 0, 0);//显示左转 */
    /*     } else { */
    /*         led_api_main_entrance(UI_LEFT_STEER, 0, 0, 0); */
    /*     } */
    /* #endif */

    //一线通去做什么
    //TODO

    //.....

    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      蓝牙HCI事件消息处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int electrocar_bt_hci_event_handler(struct bt_event *bt)
{
    //对应原来的蓝牙连接上断开处理函数  ,bt->value=reason
    log_info("----%s reason %x %x", __FUNCTION__, bt->event, bt->value);

#if TCFG_USER_EDR_ENABLE
    bt_comm_edr_hci_event_handler(bt);
#endif

#if TCFG_USER_BLE_ENABLE
    bt_comm_ble_hci_event_handler(bt);
#endif

    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      蓝牙连接状态事件消息处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int electrocar_bt_connction_status_event_handler(struct bt_event *bt)
{
    log_info("----%s %d", __FUNCTION__, bt->event);


    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        /*
         * 蓝牙初始化完成
         */
        log_info("BT_STATUS_INIT_OK\n");

        hid_vm_cfg_t info;
        electrocar_vm_deal(&info, 0);//bt_hid_mode read for VM

        //根据模式执行对应蓝牙的初始化
        if (bt_hid_mode == HID_MODE_BLE) {
#if TCFG_USER_BLE_ENABLE
            btstack_ble_start_after_init(0);
#endif
        } else {
#if TCFG_USER_EDR_ENABLE
            btstack_edr_start_after_init(0);
#endif
        }

        electrocar_app_select_btmode(HID_MODE_INIT);//

        break;

    default:
#if TCFG_USER_EDR_ENABLE
        bt_comm_edr_status_event_handler(bt);
#endif

#if TCFG_USER_BLE_ENABLE
        bt_comm_ble_status_event_handler(bt);
#endif
        break;
    }
    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      蓝牙公共消息处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int electrocar_bt_common_event_handler(struct bt_event *bt)
{
    log_info("----%s reason %x %x", __FUNCTION__, bt->event, bt->value);

    switch (bt->event) {
    case COMMON_EVENT_EDR_REMOTE_TYPE:
        log_info(" COMMON_EVENT_EDR_REMOTE_TYPE,%d \n", bt->value);
        break;

    case COMMON_EVENT_BLE_REMOTE_TYPE:
        log_info(" COMMON_EVENT_BLE_REMOTE_TYPE,%d \n", bt->value);
        break;

    case COMMON_EVENT_SHUTDOWN_DISABLE:
        electrocar_auto_shutdown_disable();
        break;

    default:
        break;

    }
    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      按键事件处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void electrocar_key_event_handler(struct sys_event *event)
{
    /* u16 cpi = 0; */
    u8 event_type = 0;
    u8 key_value = 0;

    if (event->arg == (void *)DEVICE_EVENT_FROM_KEY) {
        event_type = event->u.key.event;
        key_value = event->u.key.value;
        log_info("app_key_evnet: %d,%d\n", event_type, key_value);
        electrocar_app_key_deal_test(event_type, key_value);
    }
}

/*************************************************************************************************/
/*!
 *  \brief      定时检测需要信息:io获取、一线通获取
 *
 *  \param      [in]
 *
 *  \return     [out]回调给app core处理
 *
 *  \note
 */
/*************************************************************************************************/
static void all_param_scan(void)
{
    u32 event_massage = 0;

    //IO 获取
    /* if (electrocar_control_io_in(CHECK_LIFT_HAND_PIN)) { */
    /*     event_massage |= UI_LEFT_STEER; */
    /* } else { */
    /*     event_massage &= ~UI_LEFT_STEER; */
    /* } */

    //一线通获取
    //TODO

    //.....

    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg  = DEVICE_GET_MASSAGE_NOTIFY;
    e.u.dev.event = 0;
    e.u.dev.value = event_massage;
    sys_event_notify(&e);

}

/*************************************************************************************************/
/*!
 *  \brief      数据接收处理事件
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
/* void electrocar_reject_callback(u8 *packet, u16 size) */
void ble_hid_transfer_channel_recieve1(u8 *packet, u16 size)
{
#if TCFG_USER_BLE_ENABLE
    u8 return_data[1];

    log_info("--- %s", __FUNCTION__);
    put_buf(packet, size);

    switch (packet[0]) {
    case APP_TO_DEVICE_IGNITION:
        log_info("APP_TO_DEVICE_IGNITION");

        //点火来自解锁状态 && 先解锁再点火
        if (work_state == APP_TO_DEVICE_IGNITION) {
            log_info("last work_state is ignition");
            break;
        }

        electrocar_control_io_out(LOCK_LED_PIN, 1);
        log_info("IGNITION succ");
        work_state = APP_TO_DEVICE_IGNITION;
        break;

    case APP_TO_DEVICE_SHUTDOWM:
        log_info("APP_TO_DEVICE_SHUTDOWM");
        //熄火状态来自点火--未加上钥匙熄火判断是因为app优先级高于key
        if (work_state == APP_TO_DEVICE_SHUTDOWM) {
            log_info("last work_state is shutdown");
            break;
        }

        electrocar_control_io_out(LOCK_LED_PIN, 0);
        log_info("SHUTDOWM succ");
        work_state = APP_TO_DEVICE_SHUTDOWM;
        break;

    default:
        log_info("CMD undeteremined!!");
        break;
    }

    switch (packet[1]) {
    case APP_TO_DEVICE_LOCK:

        log_info("APP_TO_DEVICE_LOCK");

        //先熄火再关锁
        if (work_state1 == APP_TO_DEVICE_LOCK) {
            log_info("last work_state is lock");
            break;
        }

        electrocar_control_io_out(LOCK_MOTOR_PIN, 0);
        log_info("LOCK succ");
        work_state1 = APP_TO_DEVICE_LOCK;
        break;

    case APP_TO_DEVICE_UNLOCK:
        log_info("APP_TO_DEVICE_UNLOCK");

        if (work_state1 == APP_TO_DEVICE_UNLOCK) {
            log_info("last work_state is unlock");
            break;
        }

        electrocar_control_io_out(LOCK_MOTOR_PIN, 1);
        log_info("UNLOCK succ");
        work_state1 = APP_TO_DEVICE_UNLOCK;
        break;

    default:
        log_info("CMD undeteremined!!");
        break;
    }
#endif
}

/*************************************************************************************************/
/*!
 *  \brief      app 线程事件处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int electrocar_event_handler(struct application *app, struct sys_event *event)
{
#if (TCFG_HID_AUTO_SHUTDOWN_TIME)
    //重置无操作定时计数
    if (event->type != SYS_DEVICE_EVENT || DEVICE_EVENT_FROM_POWER != event->arg) { //过滤电源消息
        sys_timer_modify(g_auto_shutdown_timer, TCFG_HID_AUTO_SHUTDOWN_TIME * 1000);
    }
#endif

#if TCFG_USER_EDR_ENABLE
    bt_comm_edr_sniff_clean();
#endif

    switch (event->type) {
    case SYS_KEY_EVENT:
        electrocar_key_event_handler(event);
        return 0;

    case SYS_BT_EVENT:
#if (TCFG_USER_EDR_ENABLE || TCFG_USER_BLE_ENABLE)
        if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
            electrocar_bt_connction_status_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_TYPE_HCI_STATUS) {
            electrocar_bt_hci_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_FORM_COMMON) {
            return electrocar_bt_common_event_handler(&event->u.dev);
        }
#endif
        return 0;

    case SYS_DEVICE_EVENT:
        if ((u32)event->arg == DEVICE_EVENT_FROM_POWER) {
            return app_power_event_handler(&event->u.dev, electrocar_set_soft_poweroff);
        } else if ((u32)event->arg == DEVICE_GET_MASSAGE_NOTIFY) {
            return app_device_massage_notify(&event->u.dev);
        } else if ((u32)event->arg == DEVICE_433_MASSAGE_NOTIFY) {
            return app_433_massage_notify(&event->u.nf4);
        } else if ((u32)event->arg == DEVICE_NFC_MASSAGE_NOTIFY) {
            return app_nfc_massage_notify(&event->u.nfn);
        } else if ((u32)event->arg == DEVICE_ONEPA_MASSAGE_NOTIFY) {
            return app_onepa_massage_notify(&event->u.nfo);
        }
#if TCFG_CHARGE_ENABLE
        else if ((u32)event->arg == DEVICE_EVENT_FROM_CHARGE) {
            app_charge_event_handler(&event->u.dev);
        }
#endif
        return 0;

    default:
        return 0;
    }

    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      切换蓝牙模式
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note      切换模式,重启
 */
/*************************************************************************************************/
static void electrocar_app_select_btmode(u8 mode)
{
    if (mode != HID_MODE_INIT) {
        if (bt_hid_mode == mode) {
            return;
        }
        bt_hid_mode = mode;
    } else {
        //init start
    }

    log_info("###### %s: %d,%d\n", __FUNCTION__, mode, bt_hid_mode);

    if (bt_hid_mode == HID_MODE_BLE) {
        //ble
        log_info("---------app select ble--------\n");
        if (!STACK_MODULES_IS_SUPPORT(BT_BTSTACK_LE) || !BT_MODULES_IS_SUPPORT(BT_MODULE_LE)) {
            log_info("not surpport ble,make sure config !!!\n");
            ASSERT(0);
        }

#if TCFG_USER_EDR_ENABLE
        //close edr
        bt_comm_edr_mode_enable(0);
#endif

#if TCFG_USER_BLE_ENABLE
        if (mode == HID_MODE_INIT) {
            ble_module_enable(1);
        }
#endif

    } else {
        //edr
        log_info("---------app select edr--------\n");
        if (!STACK_MODULES_IS_SUPPORT(BT_BTSTACK_CLASSIC) || !BT_MODULES_IS_SUPPORT(BT_MODULE_CLASSIC)) {
            log_info("not surpport edr,make sure config !!!\n");
            ASSERT(0);
        }

#if TCFG_USER_BLE_ENABLE
        //close ble
        ble_module_enable(0);
#endif

#if TCFG_USER_EDR_ENABLE
        if (mode == HID_MODE_INIT) {
            if (!bt_connect_phone_back_start()) {
                bt_wait_phone_connect_control(1);
            }
        }
#endif

    }

    /* hid_vm_cfg_t info; */
    /* electrocar_vm_deal(&info, 1); */
}

/*************************************************************************************************/
/*!
 *  \brief      注册控制是否进入sleep
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
//-----------------------
//system check go sleep is ok
static u8 electrocar_app_idle_query(void)
{
    return !is_electrocar_active;
}

REGISTER_LP_TARGET(electrocar_lp_target) = {
    .name = "electrocar_deal",
    .is_idle = electrocar_app_idle_query,
};


static const struct application_operation app_electrocar_ops = {
    .state_machine  = electrocar_state_machine,
    .event_handler 	= electrocar_event_handler,
};

/*
 * 注册模式
 */
REGISTER_APPLICATION(app_electrocar) = {
    .name 	= "electrocar",
    .action	= ACTION_ELECTROCAR,
    .ops 	= &app_electrocar_ops,
    .state  = APP_STA_DESTROY,
};


#endif




