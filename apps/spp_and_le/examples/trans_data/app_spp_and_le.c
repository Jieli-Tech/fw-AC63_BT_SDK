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
#include "rcsp_bluetooth.h"
#include "rcsp_user_update.h"
#include "app_charge.h"
#include "app_chargestore.h"
#include "app_power_manage.h"
#include "app_comm_bt.h"
#include "usb/device/cdc.h"

#define LOG_TAG_CONST       SPP_AND_LE
#define LOG_TAG             "[SPP_AND_LE]"
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

extern void rtc_alarm_set_timer(u32 seconds);
extern void trans_disconnect(void);
extern void midi_paly_test(u32 key);
#endif/*TCFG_AUDIO_ENABLE*/


#if CONFIG_APP_SPP_LE

static u8 is_app_spple_active = 0;
static u8 enter_btstack_num = 0;
extern void app_switch(const char *name, int action);
//---------------------------------------------------------------------
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
void spple_power_event_to_user(u8 event)
{
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg  = (void *)DEVICE_EVENT_FROM_POWER;
    e.u.dev.event = event;
    e.u.dev.value = 0;
    sys_event_notify(&e);
}

static void spple_set_soft_poweroff(void)
{
    log_info("set_soft_poweroff\n");
    is_app_spple_active = 1;
    //必须先主动断开蓝牙链路,否则要等链路超时断开

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

/* #define LED_GPIO_PIN     IO_PORTB_05//IO_PORTB_01//IO_PORTA_00 */
/* static void led_io_init(void) */
/* { */
/* gpio_set_die(LED_GPIO_PIN, 1); */
/* gpio_set_pull_down(LED_GPIO_PIN, 0); */
/* gpio_set_pull_up(LED_GPIO_PIN, 0); */
/* gpio_direction_output(LED_GPIO_PIN, 0); */
/* } */

// cdc send test
static void usb_cdc_send_test()
{
#if TCFG_USB_SLAVE_CDC_ENABLE
    log_info("-send test cdc data-");
    u8 cdc_test_buf[3] = {0x11, 0x22, 0x33};
    cdc_write_data(USB0, cdc_test_buf, 3);
    /* char test_char[] = "cdc test"; */
    /* cdc_write_data(USB0, test_char, sizeof(test_char)-1); */
#endif
}

extern void mem_stats(void);
static void spple_timer_handle_test(void)
{
    log_info("not_bt");
    //	mem_stats();//see memory
    //sys_timer_dump_time();
}

static const ble_init_cfg_t trans_data_ble_config = {
#if DOUBLE_BT_SAME_MAC
    .same_address = 1,
#else
    .same_address = 0,
#endif
    .appearance = 0,
};

/*************************************************************************************************/
/*!
 *  \brief      app start
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void spple_app_start()
{
    log_info("=======================================");
    log_info("-----------spp_and_le demo-------------");
    log_info("=======================================");
    log_info("app_file: %s", __FILE__);

    if (enter_btstack_num == 0) {
        enter_btstack_num = 1;
        clk_set("sys", BT_NORMAL_HZ);

//有蓝牙
#if (TCFG_USER_EDR_ENABLE || TCFG_USER_BLE_ENABLE)
        u32 sys_clk =  clk_get("sys");
        bt_pll_para(TCFG_CLOCK_OSC_HZ, sys_clk, 0, 0);

#if TCFG_USER_EDR_ENABLE
        btstack_edr_start_before_init(NULL, 0);
#if USER_SUPPORT_PROFILE_HCRP
        __change_hci_class_type(BD_CLASS_PRINTING);
#endif
#if DOUBLE_BT_SAME_MAC
        //手机自带搜索界面，默认搜索到EDR
        __change_hci_class_type(BD_CLASS_TRANSFER_HEALTH);//
#endif
#endif

#if TCFG_USER_BLE_ENABLE
        btstack_ble_start_before_init(&trans_data_ble_config, 0);
#endif

        btstack_init();

#else
//no bt,to for test
        sys_timer_add(NULL, spple_timer_handle_test, 1000);
#endif
    }
    /* 按键消息使能 */
    sys_key_event_enable();
#if TCFG_SOFTOFF_WAKEUP_KEY_DRIVER_ENABLE
    set_key_wakeup_send_flag(1);
#endif

#if TCFG_USB_SLAVE_CDC_ENABLE
    extern void usb_start();
    usb_start();
#endif
}
/*************************************************************************************************/
/*!
 *  \brief      app 状态机处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int spple_state_machine(struct application *app, enum app_state state, struct intent *it)
{
    switch (state) {
    case APP_STA_CREATE:
        break;
    case APP_STA_START:
        if (!it) {
            break;
        }
        switch (it->action) {
        case ACTION_SPPLE_MAIN:
            spple_app_start();
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
 *  \brief      hci 事件处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int spple_bt_hci_event_handler(struct bt_event *bt)
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
 *  \brief      bt 连接状态处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int spple_bt_connction_status_event_handler(struct bt_event *bt)
{
    log_info("----%s %d", __FUNCTION__, bt->event);

#if TCFG_USER_EDR_ENABLE
    bt_comm_edr_status_event_handler(bt);
#endif

#if TCFG_USER_BLE_ENABLE
    bt_comm_ble_status_event_handler(bt);
#endif
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
static void spple_key_event_handler(struct sys_event *event)
{
    /* u16 cpi = 0; */
    u8 event_type = 0;
    u8 key_value = 0;

    if (event->arg == (void *)DEVICE_EVENT_FROM_KEY) {
        event_type = event->u.key.event;
        key_value = event->u.key.value;
        log_info("app_key_evnet: %d,%d\n", event_type, key_value);
        /*Change Case To Idle Demo*/
#if CONFIG_APP_SPP_LE_TO_IDLE
        if (event_type == KEY_EVENT_CLICK && key_value == TCFG_ADKEY_VALUE1) {
            log_info(">>>hold key1:chage case to idle\n");
            //change case to idle test
#if TCFG_USER_BLE_ENABLE
            btstack_ble_exit(0);
#endif

#if TCFG_USER_EDR_ENABLE
            btstack_edr_exit(0);
#endif
            app_switch("idle", ACTION_IDLE_MAIN);//切换case
            return;
        }
#endif  /*Btctrler Task Del Enable */

        /*Audio Test Demo*/
#if TCFG_AUDIO_ENABLE
        if (event_type == KEY_EVENT_CLICK && key_value == TCFG_ADKEY_VALUE0) {
            log_info(">>>key0:mic/encode test\n");
            //AC695N/AC696N mic test
            /* extern int audio_adc_open_demo(void); */
            /* audio_adc_open_demo(); */
            //AD697N/AC897N/AC698N mic test
            /* extern void audio_adc_mic_demo(u8 mic_idx, u8 gain, u8 mic_2_dac); */
            /* audio_adc_mic_demo(1, 1, 1); */


            /*encode test*/
            /* extern int audio_mic_enc_open(int (*mic_output)(void *priv, void *buf, int len), u32 code_type); */
            /* audio_mic_enc_open(NULL, AUDIO_CODING_OPUS);//opus encode test */
            /* audio_mic_enc_open(NULL, AUDIO_CODING_SPEEX);//speex encode test */


            /*
            //AC632N
            编码测试类型：
            AUDIO_CODING_LC3
            AUDIO_CODING_USBC
            */

            /* extern int audio_demo_enc_open(int (*demo_output)(void *priv, void *buf, int len), u32 code_type, u8 ai_type); */
            /* audio_demo_enc_open(NULL, AUDIO_CODING_USBC, 0); */



            /*midi test*/

            /* log_info(">>>key0:open midi\n"); */
            // midi_paly_test(KEY_IR_NUM_0);


        }
        if (event_type == KEY_EVENT_CLICK && key_value == TCFG_ADKEY_VALUE1) {
            log_info(">>>key1:tone/decode test\n");
            //AC695N/AC696N tone play test
            /* tone_play_by_path(TONE_NORMAL, 1); */
            /* tone_play_by_path(TONE_BT_CONN, 1); */
            //AD697N/AC897N/AC698N tone play test
            /* tone_play(TONE_NUM_8, 1); */
            /* tone_play(TONE_SIN_NORMAL, 1); */
            /* log_info(">>>key0:set  midi\n"); */
            // midi_paly_test(KEY_IR_NUM_1);



            /*
            //AC632N
            解码测试类型：(需要在audio_decode.c中配置)
            AUDIO_CODING_LC3
            AUDIO_CODING_USBC
             */

            /* extern void demo_frame_test(void); */
            /* demo_frame_test(); */
        }


        if (event_type == KEY_EVENT_CLICK && key_value == TCFG_ADKEY_VALUE2) {
            /* log_info(">>>key2:play  midi\n"); */
            // midi_paly_test(KEY_IR_NUM_2);
        }

#endif/*TCFG_AUDIO_ENABLE*/

        if (event_type == KEY_EVENT_TRIPLE_CLICK
            && (key_value == TCFG_ADKEY_VALUE3 || key_value == TCFG_ADKEY_VALUE0)) {
            //for test
            spple_power_event_to_user(POWER_EVENT_POWER_SOFTOFF);
            return;
        }

        if (event_type == KEY_EVENT_DOUBLE_CLICK && key_value == TCFG_ADKEY_VALUE0) {
#if TCFG_USER_EDR_ENABLE
            //for test
            static u8 edr_en = 1;
            edr_en = !edr_en;
            bt_comm_edr_mode_enable(edr_en);
#endif
        }

        if (event_type == KEY_EVENT_CLICK && key_value == TCFG_ADKEY_VALUE0) {
#if TCFG_USER_BLE_ENABLE
            log_info(">>>test to disconnect\n");
            trans_disconnect();
#endif
        }

        if (event_type == KEY_EVENT_DOUBLE_CLICK && key_value == TCFG_ADKEY_VALUE1) {
#if TCFG_USB_SLAVE_CDC_ENABLE
            log_info(">>>test to cdc send\n");
            usb_cdc_send_test();
#endif

#if TCFG_RTC_ALARM_ENABLE
            log_info(">>>test to rtc_test\n");
            rtc_alarm_set_timer(60);
            spple_power_event_to_user(POWER_EVENT_POWER_SOFTOFF);
#endif
        }

    }
}

/*************************************************************************************************/
/*!
 *  \brief      app 事件处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int spple_event_handler(struct application *app, struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        spple_key_event_handler(event);
        return 0;

    case SYS_BT_EVENT:
#if (TCFG_USER_EDR_ENABLE || TCFG_USER_BLE_ENABLE)
        if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
            spple_bt_connction_status_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_TYPE_HCI_STATUS) {
            spple_bt_hci_event_handler(&event->u.bt);
        }
#endif
        return 0;

    case SYS_DEVICE_EVENT:
        if ((u32)event->arg == DEVICE_EVENT_FROM_POWER) {
            return app_power_event_handler(&event->u.dev, spple_set_soft_poweroff);
        }

#if TCFG_CHARGE_ENABLE
        else if ((u32)event->arg == DEVICE_EVENT_FROM_CHARGE) {
            app_charge_event_handler(&event->u.dev);
        }
#endif
        return 0;

    default:
        return FALSE;
    }
    return FALSE;
}

static const struct application_operation app_spple_ops = {
    .state_machine  = spple_state_machine,
    .event_handler 	= spple_event_handler,
};

/*
 * 注册AT Module模式
 */
REGISTER_APPLICATION(app_spple) = {
    .name 	= "spp_le",
    .action	= ACTION_SPPLE_MAIN,
    .ops 	= &app_spple_ops,
    .state  = APP_STA_DESTROY,
};

/*************************************************************************************************/
/*!
 *  \brief      注册控制是否允许系统进入sleep状态
 *
 *  \param      [in]
 *
 *  \return     1--可以进入sleep  0--不允许
 *
 *  \note
 */
/*************************************************************************************************/
static u8 spple_state_idle_query(void)
{
    return !is_app_spple_active;
}

REGISTER_LP_TARGET(spple_state_lp_target) = {
    .name = "spple_state_deal",
    .is_idle = spple_state_idle_query,
};

#endif


