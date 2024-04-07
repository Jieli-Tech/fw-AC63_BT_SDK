/*********************************************************************************************
    *   Filename        : app_keyboard.c

    *   Description     :

    *   Author          :

    *   Email           :

    *   Last modifiled  : 2019-07-05 10:09

    *   Copyright:(c)JIELI  2011-2019  @ , All Rights Reserved.
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
/* #include "code_switch.h" */
/* #include "omsensor/OMSensor_manage.h" */
#include "le_common.h"
#include <stdlib.h>
#include "standard_hid.h"
#include "rcsp_bluetooth.h"
#include "app_charge.h"
#include "app_power_manage.h"
#include "app_chargestore.h"
#include "app_comm_bt.h"

#if(CONFIG_APP_KEYBOARD)
#define LOG_TAG_CONST       HID_KEY
#define LOG_TAG             "[HID_KEY]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_AUDIO_ENABLE
#include "tone_player.h"
#include "media/includes.h"
#include "key_event_deal.h"
extern void midi_paly_test(u32 key);
#endif/*TCFG_AUDIO_ENABLE*/

//测试每个interval上行发一包数据
#define  HID_TEST_KEEP_SEND_EN        1//just for test keep data

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
static const edr_sniff_par_t hidkey_sniff_param = {
    .sniff_mode = SNIFF_MODE_TYPE,
    .cnt_time = SNIFF_CNT_TIME,
    .max_interval_slots = SNIFF_MAX_INTERVALSLOT,
    .min_interval_slots = SNIFF_MIN_INTERVALSLOT,
    .attempt_slots = SNIFF_ATTEMPT_SLOT,
    .timeout_slots = SNIFF_TIMEOUT_SLOT,
    .check_timer_period = SNIFF_CHECK_TIMER_PERIOD,
};

typedef enum {
    HID_MODE_NULL = 0,
    HID_MODE_EDR,
    HID_MODE_BLE,
    HID_MODE_INIT = 0xff
} bt_mode_e;


static bt_mode_e bt_hid_mode;
static volatile u8 is_hidkey_active = 0;//1-临界点,系统不允许进入低功耗，0-系统可以进入低功耗
static u16 g_auto_shutdown_timer = 0;
static void hidkey_app_select_btmode(u8 mode);
void hidkey_power_event_to_user(u8 event);
//----------------------------------
static const u8 hidkey_report_map[] = {
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

// consumer key
#define CONSUMER_VOLUME_INC             0x0001
#define CONSUMER_VOLUME_DEC             0x0002
#define CONSUMER_PLAY_PAUSE             0x0004
#define CONSUMER_MUTE                   0x0008
#define CONSUMER_SCAN_PREV_TRACK        0x0010
#define CONSUMER_SCAN_NEXT_TRACK        0x0020
#define CONSUMER_SCAN_FRAME_FORWARD     0x0040
#define CONSUMER_SCAN_FRAME_BACK        0x0080

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
static const edr_init_cfg_t hidkey_edr_config = {
    .page_timeout = 8000,
    .super_timeout = 8000,
    .io_capabilities = 3,
    .passkey_enable = 0,
    .authentication_req = 2,
    .oob_data = 0,
    .sniff_param = &hidkey_sniff_param,
    .class_type = BD_CLASS_KEYBOARD,
    .report_map = hidkey_report_map,
    .report_map_size = sizeof(hidkey_report_map),
};

//----------------------------------
static const ble_init_cfg_t hidkey_ble_config = {
    .same_address = 0,
    .appearance = BLE_APPEARANCE_HID_KEYBOARD,
    .report_map = hidkey_report_map,
    .report_map_size = sizeof(hidkey_report_map),
};

extern void p33_soft_reset(void);
extern void ble_hid_key_deal_test(u16 key_msg);
extern void ble_module_enable(u8 en);
static void hidkey_set_soft_poweroff(void);

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
static void hidkey_auto_shutdown_disable(void)
{
    log_info("----%s", __FUNCTION__);
    if (g_auto_shutdown_timer) {
        sys_timeout_del(g_auto_shutdown_timer);
    }
}

/*************************************************************************************************/
/*!
 *  \brief      测试一直发空键
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
#if HID_TEST_KEEP_SEND_EN
static u8 test_keep_send_start = 0;
extern int ble_hid_timer_handle;
extern int edr_hid_timer_handle;
extern int ble_hid_data_send(u8 report_id, u8 *data, u16 len);
void hidkey_test_keep_send_data(void)
{
    static const u8 test_data_000[8] = {0, 0, 0, 0};
    void (*hid_data_send_pt)(u8 report_id, u8 * data, u16 len) = NULL;

    if (!test_keep_send_start) {
        return;
    }

    if (bt_hid_mode == HID_MODE_EDR) {
#if TCFG_USER_EDR_ENABLE
        hid_data_send_pt = edr_hid_data_send;
        bt_comm_edr_sniff_clean();
#endif
    } else {
#if TCFG_USER_BLE_ENABLE
        hid_data_send_pt = ble_hid_data_send;
#endif
    }
    hid_data_send_pt(1, test_data_000, sizeof(test_data_000));
}

/*************************************************************************************************/
/*!
 *  \brief      初始化测试发送
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void hidkey_test_keep_send_init(void)
{
    if (bt_hid_mode == HID_MODE_BLE) {
#if TCFG_USER_BLE_ENABLE
        log_info("###keep test ble\n");
        ble_hid_timer_handle = sys_s_hi_timer_add((void *)0, hidkey_test_keep_send_data, 10);
#endif
    } else {
#if TCFG_USER_EDR_ENABLE
        log_info("###keep test edr\n");
        edr_hid_timer_handle = sys_s_hi_timer_add((void *)0, hidkey_test_keep_send_data, 10);
#endif
    }
}
#endif


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
static void hidkey_app_key_deal_test(u8 key_type, u8 key_value)
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
        /* audio_demo_enc_open(NULL, AUDIO_CODING_USBC, 0); */



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
        /* demo_frame_test(); */
    }

    if (key_type == KEY_EVENT_CLICK && key_value == TCFG_ADKEY_VALUE2) {
        //	printf(">>>key2:play  midi\n");
        //	midi_paly_test(KEY_IR_NUM_2);
    }

#endif/*TCFG_AUDIO_ENABLE*/

#if HID_TEST_KEEP_SEND_EN
    if (key_type == KEY_EVENT_LONG && key_value == TCFG_ADKEY_VALUE0) {
        test_keep_send_start = !test_keep_send_start;
        log_info("test_keep_send_start=%d\n", test_keep_send_start);
        return;
    }
#endif

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
        hidkey_power_event_to_user(POWER_EVENT_POWER_SOFTOFF);
        return;
    }

    if (key_type == KEY_EVENT_DOUBLE_CLICK && key_value == TCFG_ADKEY_VALUE0) {
#if (TCFG_USER_EDR_ENABLE && TCFG_USER_BLE_ENABLE)
        is_hidkey_active = 1;
        if (HID_MODE_BLE == bt_hid_mode) {
            hidkey_app_select_btmode(HID_MODE_EDR);
        } else {
            hidkey_app_select_btmode(HID_MODE_BLE);
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
typedef struct {
    u16 head_tag;
    u8  mode;
} hid_vm_cfg_t;

#define	HID_VM_HEAD_TAG (0x3AA3)
static void hidkey_vm_deal(u8 rw_flag)
{
    hid_vm_cfg_t info;
    int ret;
    int vm_len = sizeof(hid_vm_cfg_t);

    log_info("-hid_info vm_do:%d\n", rw_flag);
    memset(&info, 0, vm_len);

    if (rw_flag == 0) {
        bt_hid_mode = HID_MODE_NULL;
        ret = syscfg_read(CFG_AAP_MODE_INFO, (u8 *)&info, vm_len);
        if (!ret) {
            log_info("-null--\n");
        } else {
            if (HID_VM_HEAD_TAG == info.head_tag) {
                log_info("-exist--\n");
                log_info_hexdump((u8 *)&info, vm_len);
                bt_hid_mode = info.mode;
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

            if (bt_hid_mode != info.mode) {
                log_info("-write00--\n");
                info.mode = bt_hid_mode;
                syscfg_write(CFG_AAP_MODE_INFO, (u8 *)&info, vm_len);
            }
        }
    } else {
        info.mode = bt_hid_mode;
        info.head_tag = HID_VM_HEAD_TAG;
        syscfg_write(CFG_AAP_MODE_INFO, (u8 *)&info, vm_len);
        log_info("-write11--\n");
        log_info_hexdump((u8 *)&info, vm_len);
    }
}

/*************************************************************************************************/
/*!
 *  \brief      软关机消息处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void hidkey_power_event_to_user(u8 event)
{
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg  = (void *)DEVICE_EVENT_FROM_POWER;
    e.u.dev.event = event;
    e.u.dev.value = 0;
    sys_event_notify(&e);
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
static void hidkey_set_soft_poweroff(void)
{
    log_info("hidkey_set_soft_poweroff\n");
    is_hidkey_active = 1;

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

static void hidkey_timer_handle_test(void)
{
    log_info("not_bt");
    //mem_stats();//see memory
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
static void hidkey_app_start()
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
    btstack_edr_start_before_init(&hidkey_edr_config, 0);
#endif

#if TCFG_USER_BLE_ENABLE
    btstack_ble_start_before_init(&hidkey_ble_config, 0);
    /* le_hogp_set_reconnect_adv_cfg(ADV_IND, 5000); */
    le_hogp_set_reconnect_adv_cfg(ADV_DIRECT_IND_LOW, 5000);
#endif

    btstack_init();

#else
    //no bt,to for test
    sys_timer_add(NULL, hidkey_timer_handle_test, 1000);
#endif

    /* 按键消息使能 */
    sys_key_event_enable();
#if TCFG_SOFTOFF_WAKEUP_KEY_DRIVER_ENABLE
    set_key_wakeup_send_flag(1);
#endif

#if (TCFG_HID_AUTO_SHUTDOWN_TIME)
    //无操作定时软关机
    g_auto_shutdown_timer = sys_timeout_add((void *)POWER_EVENT_POWER_SOFTOFF, hidkey_power_event_to_user, TCFG_HID_AUTO_SHUTDOWN_TIME * 1000);
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
static int hidkey_state_machine(struct application *app, enum app_state state, struct intent *it)
{
    switch (state) {
    case APP_STA_CREATE:
        break;
    case APP_STA_START:
        if (!it) {
            break;
        }
        switch (it->action) {
        case ACTION_HID_MAIN:
            hidkey_app_start();
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
 *  \brief      蓝牙HCI事件消息处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int hidkey_bt_hci_event_handler(struct bt_event *bt)
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
static int hidkey_bt_connction_status_event_handler(struct bt_event *bt)
{
    log_info("----%s %d", __FUNCTION__, bt->event);


    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        /*
         * 蓝牙初始化完成
         */
        log_info("BT_STATUS_INIT_OK\n");

#if TCFG_NORMAL_SET_DUT_MODE
#if TCFG_USER_EDR_ENABLE
        log_info("set edr dut mode\n");
        bredr_set_dut_enble(1, 1);
#else
        log_info("set ble dut mode\n");
        ble_standard_dut_test_init();
#endif
        break;
#endif

        hidkey_vm_deal(0);//bt_hid_mode read for VM

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

        hidkey_app_select_btmode(HID_MODE_INIT);//

#if HID_TEST_KEEP_SEND_EN
        hidkey_test_keep_send_init();
#endif
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
static int hidkey_bt_common_event_handler(struct bt_event *bt)
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
        hidkey_auto_shutdown_disable();
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
static void hidkey_key_event_handler(struct sys_event *event)
{
    /* u16 cpi = 0; */
    u8 event_type = 0;
    u8 key_value = 0;

    if (event->arg == (void *)DEVICE_EVENT_FROM_KEY) {
        event_type = event->u.key.event;
        key_value = event->u.key.value;
        log_info("app_key_evnet: %d,%d\n", event_type, key_value);
        hidkey_app_key_deal_test(event_type, key_value);
    }
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
static int hidkey_event_handler(struct application *app, struct sys_event *event)
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
        hidkey_key_event_handler(event);
        return 0;

    case SYS_BT_EVENT:
#if (TCFG_USER_EDR_ENABLE || TCFG_USER_BLE_ENABLE)
        if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
            hidkey_bt_connction_status_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_TYPE_HCI_STATUS) {
            hidkey_bt_hci_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_FORM_COMMON) {
            return hidkey_bt_common_event_handler(&event->u.dev);
        }
#endif
        return 0;

    case SYS_DEVICE_EVENT:
        if ((u32)event->arg == DEVICE_EVENT_FROM_POWER) {
            return app_power_event_handler(&event->u.dev, hidkey_set_soft_poweroff);
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
static void hidkey_app_select_btmode(u8 mode)
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

    hidkey_vm_deal(1);
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
static u8 hidkey_app_idle_query(void)
{
    return !is_hidkey_active;
}

REGISTER_LP_TARGET(app_hidkey_lp_target) = {
    .name = "app_hidkey_deal",
    .is_idle = hidkey_app_idle_query,
};


static const struct application_operation app_hidkey_ops = {
    .state_machine  = hidkey_state_machine,
    .event_handler 	= hidkey_event_handler,
};

/*
 * 注册模式
 */
REGISTER_APPLICATION(app_hidkey) = {
    .name 	= "hid_key",
    .action	= ACTION_HID_MAIN,
    .ops 	= &app_hidkey_ops,
    .state  = APP_STA_DESTROY,
};


#endif

