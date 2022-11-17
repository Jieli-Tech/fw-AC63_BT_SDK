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
#include "rcsp_bluetooth.h"
#include "rcsp_user_update.h"
#include "app_charge.h"
#include "app_chargestore.h"
#include "app_power_manage.h"
#include "app_comm_bt.h"
#include "led_api.h"

#if CONFIG_APP_ELECTROCAR

#include "electrocar.h"

#define LOG_TAG_CONST       ELECTROCAR
#define LOG_TAG             "[ELECTROCAR]"
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

extern void trans_disconnect(void);
extern void midi_paly_test(u32 key);
#endif/*TCFG_AUDIO_ENABLE*/

static u8 is_electrocar_active = 0;
static u8 enter_btstack_num = 0;
static u8 blue_state, work_state, work_state1;
extern void app_switch(const char *name, int action);
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

/* #define LED_GPIO_PIN     IO_PORTB_05//IO_PORTB_01//IO_PORTA_00 */
/* static void led_io_init(void) */
/* { */
/* gpio_set_die(LED_GPIO_PIN, 1); */
/* gpio_set_pull_down(LED_GPIO_PIN, 0); */
/* gpio_set_pull_up(LED_GPIO_PIN, 0); */
/* gpio_direction_output(LED_GPIO_PIN, 0); */
/* } */

extern void mem_stats(void);
static void spple_timer_handle_test(void)
{
    log_info("not_bt");
    //	mem_stats();//see memory
}

static const ble_init_cfg_t electrocar_ble_config = {
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
extern void bt_pll_para(u32 osc, u32 sys, u8 low_power, u8 xosc);
static void electrocar_app_start()
{
    log_info("=======================================");
    log_info("-------------ELECTROCAR DEMO-----------");
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
#if DOUBLE_BT_SAME_MAC
        //手机自带搜索界面，默认搜索到EDR
        __change_hci_class_type(BD_CLASS_TRANSFER_HEALTH);//
#endif
#endif

#if TCFG_USER_BLE_ENABLE
        btstack_ble_start_before_init(&electrocar_ble_config, 0);
#endif

        btstack_init();

#else
//no bt,to for test
        sys_timer_add(NULL, spple_timer_handle_test, 1000);
#endif
    }
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


#if (TCFG_HID_AUTO_SHUTDOWN_TIME)
    //无操作定时软关机
    g_auto_shutdown_timer = sys_timeout_add(NULL, electrocar_set_soft_poweroff, TCFG_HID_AUTO_SHUTDOWN_TIME * 1000);
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
void ble_transfer_channel_recieve(u8 *packet, u16 size)
{
#if TCFG_USER_BLE_ENABLE
    u8 return_data[1];

    log_info("--- %s", __FUNCTION__);
    put_buf(packet, size);

    switch (packet[0]) {
    case APP_TO_DEVICE_IGNITION:
        log_info("APP_TO_DEVICE_IGNITION");

        /* if (work_state == APP_TO_DEVICE_IGNITION) { */
        /*     log_info("last work_state is ignition"); */
        /*     break; */
        /* } */

        electrocar_control_io_out(LOCK_LED_PIN, 1);
        log_info("IGNITION succ");
        work_state = APP_TO_DEVICE_IGNITION;
        break;

    case APP_TO_DEVICE_SHUTDOWM:
        log_info("APP_TO_DEVICE_SHUTDOWM");
        /* if (work_state == APP_TO_DEVICE_SHUTDOWM) { */
        /*     log_info("last work_state is shutdown"); */
        /*     break; */
        /* } */

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

        /* if (work_state1 == APP_TO_DEVICE_LOCK) { */
        /*     log_info("last work_state is lock"); */
        /*     break; */
        /* } */

        electrocar_control_io_out(LOCK_MOTOR_PIN, 0);
        log_info("LOCK succ");
        work_state1 = APP_TO_DEVICE_LOCK;
        break;

    case APP_TO_DEVICE_UNLOCK:
        log_info("APP_TO_DEVICE_UNLOCK");

        /* if (work_state1 == APP_TO_DEVICE_UNLOCK) { */
        /*     log_info("last work_state is unlock"); */
        /*     break; */
        /* } */

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
 *  \brief      bt 连接状态处理
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
static void electrocar_key_event_handler(struct sys_event *event)
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
            electrocar_set_soft_poweroff();
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
static int electrocar_event_handler(struct application *app, struct sys_event *event)
{
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
            /* return electrocar_bt_common_event_handler(&event->u.dev); */
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
        return FALSE;
    }
    return FALSE;
}



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



