
/*********************************************************************************************
 *   Filename        : app_mouse.c

 *   Description     : 双模同时开 + 切换 2.4G

 *   Author          :

 *   Email           :

 *   Last modifiled  : 2020-03-24 09:52:50

 *   Copyright:(c)JIELI  2011-2020  @ , All Rights Reserved.
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
#include "hid_user.h"
/* #include "code_switch.h" */
/* #include "omsensor/OMSensor_manage.h" */
#include "le_common.h"
/* #include <stdlib.h>  */
#include "rcsp_bluetooth.h"
#include "rcsp_user_update.h"
#include "app_charge.h"
#include "app_power_manage.h"
#include "app_config.h"
#include "OMSensor_manage.h"

#if(CONFIG_APP_MOUSE_DUAL)

#define LOG_TAG_CONST       MOUSE
#define LOG_TAG             "[MOUSE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define trace_run_debug_val(x)   //log_info("\n## %s: %d,  0x%04x ##\n",__FUNCTION__,__LINE__,x)

//2.4G模式: 0---ble, 非0---2.4G配对码
/* #define CFG_RF_24G_CODE_ID       (0) //<=24bits */
#define CFG_RF_24G_CODE_ID       (0x23) //<=24bits

//切换控制,可自己修改按键方式
#define MIDDLE_KEY_SWITCH          1   //中键长按数秒切换 edr & ble , or 2.4g & ble, or ble & 2.4g & edr
#define MIDDLE_KEY_HOLD_CNT       (4)  //长按中键计算次数 >= 4s

//使能开配对管理，BLE & 2.4g 鼠标只绑定1个主机,可自己修改按键方式
#define DOUBLE_KEY_HOLD_PAIR         (1)  //左键+右键 长按数秒,进入配对模式
#define DOUBLE_KEY_HOLD_CNT          (4)  //长按中键计算次数 >= 4s
#define CPI_KEY_HOLD_CNT             (3)  //长按切换CPI

#define PAIR_BONDING_DEVICE        1   //配对绑定后,不允许再更新配对

#define EDR_CYCLE_RECONNECT_TIME  (3000)//unit: ms
#define SYS_WAKEUP_CLRDOG_TIME    (3000)//unit: ms


#define LED_MODE_ONOFF_MS          (1000)   //MS
#define LED_LOWPOWER_WARNING_MS    (250)   //MS

#define BLE_WAIT_PAIR_TIME_MS      (5000)

static u8 cpi_key_long_cnt = 0;
static u8 double_key_long_cnt = 0;

static u16 g_auto_shutdown_timer = 0;

int sniff_timer = 0;
void bt_check_exit_sniff();

extern void bt_set_osc_cap(u8 sel_l, u8 sel_r);
extern const u8 *bt_get_mac_addr();
extern void lib_make_ble_address(u8 *ble_address, u8 *edr_address);


/* static const u8 bt_address_default[] = {0x11, 0x22, 0x33, 0x66, 0x77, 0x88}; */
static void app_select_btmode(u8 mode);
void sys_auto_sniff_controle(u8 enable, u8 *addr);
void bt_sniff_ready_clean(void);
static void hid_vm_deal(u8 rw_flag);

extern int app_send_user_data(u16 handle, u8 *data, u16 len, u8 handle_type);

extern int edr_hid_is_connected(void);
extern int ble_hid_is_connected(void);
void ble_set_fix_pwr(u8 fix);//set tx power
void le_hogp_set_pair_config(u8 pair_max, u8 is_allow_cover);
void le_hogp_set_pair_allow(void);
static void bt_wait_phone_connect_control(u8 enable);
int bt_connect_phone_back_start(void);
static void app_timeout_handle(u32 type);
extern void lmp_sniff_t_slot_attemp_reset(u16 slot, u16 attemp);
extern const int sniff_support_reset_anchor_point;   //sniff状态下是否支持reset到最近一次通信点，用于HID

void sm_allow_ltk_reconstruction_without_le_device_db_entry(int allow);

//----------------------------------

#define WAIT_DISCONN_TIME_MS     (300)

typedef enum {
    HID_MODE_NULL = 0,
    HID_MODE_EDR,
    HID_MODE_BLE,//2.4G_BLE
    HID_MODE_BLE_24G,
    HID_MODE_DUAL,
    HID_MODE_CLOSE,
    HID_MODE_INIT = 0xff
} bt_mode_e;

#define PAIR_EDR_BIT   (1<<0)
#define PAIR_BLE_BIT   (1<<1)
#define PAIR_24G_BIT   (1<<2)

static bt_mode_e bt_hid_mode; //当前工作模式
static u8 bt_pair_mode = 0;   //记录 已连接bt 模式 HID_MODE_EDR 、HID_MODE_BLE、HID_MODE_BLE_24G,用于模式切换
static u8 bt_pair_start = 0;  //配对模式状态
static u8 mouse_cpi_mode = 1; //0--800,1--1200,2--1600

static u8 pair_step = 0;


static const u16 cpi_value_table[3] = {800, 1200, 1600};


#define IS_PAIR_MODE(a)      (bt_pair_mode & a)
#define SET_PAIR_MODE(a)      bt_pair_mode |= a
#define CLEAR_PAIR_MODE(a)    bt_pair_mode &= (~a)

#define CPI_800_FLASH_CNT       (1*2)
#define CPI_1200_FLASH_CNT      (2*2)
#define CPI_1600_FLASH_CNT      (3*2)

static  const u8 cpi_led_flash_cnt[3] = {CPI_800_FLASH_CNT, CPI_1200_FLASH_CNT, CPI_1600_FLASH_CNT}; //公用

static volatile u8 is_hid_active = 0;//1-临界点,系统不允许进入低功耗，0-系统可以进入低功耗

//----------------------------------
enum {
    TT_CLR_DOG = 1,
    TT_PAGE_AGAIN,
    TT_LED_24G,
    TT_LED_BLE,
    TT_LED_EDR,
    TT_LED_DUAL,
    TT_LED_LOWPOWER_WARNING,//低电闪灯
    TT_CHECK_PAIR_COMPLETE,
};

//----------------------------------
static u8 led_state[3] = {0, 0, 0};
//ble和edr公用一个灯
#define LED_24G_ONOFF(a)    {led_state[0] = a;gpio_set_output_value(TCFG_IOKEY_MOUSE_24GLED_PORT, a);}
#define LED_BLE_ONOFF(a)    {led_state[1] = a;gpio_set_output_value(TCFG_IOKEY_MOUSE_EDRLED_PORT, a);}//gpio_set_output_value(TCFG_IOKEY_MOUSE_BLELED_PORT, a)
#define LED_EDR_ONOFF(a)    {led_state[2] = a;gpio_set_output_value(TCFG_IOKEY_MOUSE_EDRLED_PORT, a);}

#define LED_24G_STATE()     led_state[0]
#define LED_BLE_STATE()     led_state[1]
#define LED_EDR_STATE()     led_state[2]

#define DETECT_MOUSE_MODE()   gpio_read(IO_PORTA_05)

static u16 led_24g_flash_cnt = 0;
static u16 led_ble_flash_cnt = 0;
static u16 led_edr_flash_cnt = 0;
static u16 led_dual_flash_cnt = 0;

static u16 led_warning_flash_cnt = 0;

//----------------------------------
#define BUTTONS_IDX							0
#define WHEEL_IDX               			(BUTTONS_IDX + 1)
#define SENSOR_XLSB_IDX         			0
#define SENSOR_YLSB_XMSB_IDX    			(SENSOR_XLSB_IDX + 1)
#define SENSOR_YMSB_IDX         			(SENSOR_YLSB_XMSB_IDX +1)

typedef struct {
    u8 data[3];
    u8 event_type;
    u8 *event_arg;
    u8 key_val;
} mouse_packet_data_t;


extern int ble_hid_timer_handle;
extern int edr_hid_timer_handle;
static u8 button_send_flag = 0;
static u8 wheel_send_flag = 0;
static u8 sensor_send_flag = 0;

/* static u16 auto_shutdown_timer = 0; */
static volatile mouse_packet_data_t first_packet = {0};
static volatile mouse_packet_data_t second_packet = {0};

static const u8 hid_report_map[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x09, 0x01,        //   Usage (Pointer)
    0xA1, 0x00,        //   Collection (Physical)
    0x95, 0x05,        //     Report Count (5)
    0x75, 0x01,        //     Report Size (1)
    0x05, 0x09,        //     Usage Page (Button)
    0x19, 0x01,        //     Usage Minimum (0x01)
    0x29, 0x05,        //     Usage Maximum (0x05)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //     Report Count (1)
    0x75, 0x03,        //     Report Size (3)
    0x81, 0x01,        //     Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x01,        //     Report Count (1)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x09, 0x38,        //     Usage (Wheel)
    0x15, 0x81,        //     Logical Minimum (-127)
    0x25, 0x7F,        //     Logical Maximum (127)
    0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x0C,        //     Usage Page (Consumer)
    0x0A, 0x38, 0x02,  //     Usage (AC Pan)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0x85, 0x02,        //   Report ID (2)
    0x09, 0x01,        //   Usage (Consumer Control)
    0xA1, 0x00,        //   Collection (Physical)
    0x75, 0x0C,        //     Report Size (12)
    0x95, 0x02,        //     Report Count (2)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x09, 0x30,        //     Usage (X)
    0x09, 0x31,        //     Usage (Y)
    0x16, 0x01, 0xF8,  //     Logical Minimum (-2047)
    0x26, 0xFF, 0x07,  //     Logical Maximum (2047)
    0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0xC0,              // End Collection
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x03,        //   Report ID (3)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x01,        //   Report Count (1)
    0x09, 0xCD,        //   Usage (Play/Pause)
    0x81, 0x06,        //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x0A, 0x83, 0x01,  //   Usage (AL Consumer Control Configuration)
    0x81, 0x06,        //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xB5,        //   Usage (Scan Next Track)
    0x81, 0x06,        //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xB6,        //   Usage (Scan Previous Track)
    0x81, 0x06,        //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xEA,        //   Usage (Volume Decrement)
    0x81, 0x06,        //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xE9,        //   Usage (Volume Increment)
    0x81, 0x06,        //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x0A, 0x25, 0x02,  //   Usage (AC Forward)
    0x81, 0x06,        //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x0A, 0x24, 0x02,  //   Usage (AC Back)
    0x81, 0x06,        //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x05,        //   Usage (Headphone)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x02,        //   Report Count (2)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection
    // 149 bytes
};

//------------------------------------------------------------------------------
extern void delete_link_key(bd_addr_t *bd_addr, u8 id);
extern u8 *ble_cur_connect_addrinfo(void);
//------------------------------------------------------------------------------
static void led_all_stop(void)
{
    led_24g_flash_cnt = 0;
    led_ble_flash_cnt = 0;
    led_edr_flash_cnt = 0;
    led_dual_flash_cnt = 0;

    LED_24G_ONOFF(0);
    LED_BLE_ONOFF(0);
    LED_EDR_ONOFF(0);
}

//------------------------------------------------------------------------------

void auto_shutdown_disable(void)
{
    if (g_auto_shutdown_timer) {
        sys_timeout_del(g_auto_shutdown_timer);
    }
}

extern int ble_hid_data_send(u8 report_id, u8 *data, u16 len);
static void ble_mouse_timer_handler(void)
{
#if TCFG_USER_BLE_ENABLE

    if (!ble_hid_is_connected()) {
        return;
    }

    if (button_send_flag == 0 || wheel_send_flag == 0) {
        /* log_info_hexdump(&first_packet.data, sizeof(first_packet.data)); */
        ble_hid_data_send(1, &first_packet.data, sizeof(first_packet.data));

        if (button_send_flag == 0) {
            button_send_flag = 1;
        }

        if (wheel_send_flag == 0) {
            wheel_send_flag = 1;
            first_packet.data[WHEEL_IDX] = 0;
        }
    }

    if (sensor_send_flag == 0) {
        ble_hid_data_send(2, &second_packet.data, sizeof(second_packet.data));
        memset(&second_packet.data, 0, sizeof(second_packet.data));
        sensor_send_flag = 1;
    }
#endif
}

extern int  edr_hid_tx_buff_is_ok(void);
extern void edr_hid_data_send(u8 report_id, u8 *data, u16 len);
static void edr_mouse_timer_handler(void)
{
#if TCFG_USER_EDR_ENABLE

    static u8 timer_exit_cnt = 0;

    if ((!edr_hid_is_connected()) || (!edr_hid_tx_buff_is_ok())) {
        return;
    }

    if (!sniff_timer) {
        if (!(button_send_flag == 0 || wheel_send_flag == 0 || sensor_send_flag == 0)) {
            //no data
            return;
        }

        if (timer_exit_cnt) {
            timer_exit_cnt--;
        } else {
            timer_exit_cnt = 15; //150ms
            //send exit sniff
            bt_check_exit_sniff();
        }
        return;
    }

    timer_exit_cnt = 0;

    if (button_send_flag == 0 || wheel_send_flag == 0) {
        /* log_info_hexdump(&first_packet.data, sizeof(first_packet.data)); */
        edr_hid_data_send(1, &first_packet.data, sizeof(first_packet.data));

        if (button_send_flag == 0) {
            button_send_flag = 1;
        }

        if (wheel_send_flag == 0) {
            wheel_send_flag = 1;
            first_packet.data[WHEEL_IDX] = 0;
        }
        bt_sniff_ready_clean();
    }

    if (sensor_send_flag == 0) {
        edr_hid_data_send(2, &second_packet.data, sizeof(second_packet.data));
        memset(&second_packet.data, 0, sizeof(second_packet.data));
        sensor_send_flag = 1;
        bt_sniff_ready_clean();
    }
#endif
}



extern void p33_soft_reset(void);
static void power_set_soft_reset(void)
{
    p33_soft_reset();
    while (1);
}

static int check_update_ble_list(u8 *old_addr_info, u8 *new_addr_info)
{
    u8 tmp_address1[6];
    u8 tmp_address2[6];
    u8 tmp_out_address1[6];
    u8 tmp_out_address2[6];
    int ret = 1;

    swapX(old_addr_info + 1, tmp_address1, 6);
    swapX(new_addr_info + 1, tmp_address2, 6);

    bool ret1 = ble_list_get_id_addr(tmp_address1, old_addr_info[0], tmp_out_address1);
    bool ret2 = ble_list_get_id_addr(tmp_address2, new_addr_info[0], tmp_out_address2);

    log_info("%s: have_id_address? %d, %d", __FUNCTION__, ret1, ret2);
    put_buf(old_addr_info, 7);
    put_buf(new_addr_info, 7);

    put_buf(tmp_address1, 6);
    put_buf(tmp_address2, 6);

    if (ret1) {
        if (ret2 && 0 == memcmp(tmp_out_address1, tmp_out_address2, 6)) {
            //地址一样
            log_info("the same id_address");
            ret = 2;
        } else {
            log_info("ble old id_address");
            put_buf(tmp_out_address1, 6);
            ret1 = ble_list_delete_device(tmp_address1, old_addr_info[0]);
            log_info("del dev result:%d", ret1);
        }
    }
    return ret;
}

static bool check_ble_24g_new_address_is_same(void)
{
    u8 old_addr_info[7];
    bool result = false;
    u16 vm_id;

    if (bt_hid_mode == HID_MODE_BLE_24G) {
        vm_id = CFG_AAP_MODE_24G_ADDR;
    } else {
        vm_id = CFG_AAP_MODE_BLE_ADDR;
    }

    int ret = syscfg_read(vm_id, old_addr_info, 7);
    if (ret > 0) {
        if (2 == check_update_ble_list(old_addr_info, ble_cur_connect_addrinfo())) {
            result = true;
        }
    }
    log_info("%s: %d", __FUNCTION__, result);
    return result;
}

static void pair_info_address_update(u8 pair_mode_bit, u8 *new_addr_info)
{
    u8 old_addr_info[7];
    int ret = 0;

    memset(old_addr_info, 0xff, 7);

    log_info("%s", __FUNCTION__);

    switch (pair_mode_bit) {
    case PAIR_EDR_BIT:
        ret = syscfg_read(CFG_AAP_MODE_EDR_ADDR, old_addr_info, 6);
        if (ret < 0 || memcmp(new_addr_info, old_addr_info, 6)) {
            if (ret > 0) {
                log_info("delete edr old address");
                put_buf(old_addr_info, 6);
                //清EDR回连地址
                delete_link_key(old_addr_info, get_remote_dev_info_index());
            }
            log_info("add edr new address");
            put_buf(old_addr_info, 6);
            syscfg_write(CFG_AAP_MODE_EDR_ADDR, new_addr_info, 6);
        }
        break;

    case PAIR_BLE_BIT:
        ret = syscfg_read(CFG_AAP_MODE_BLE_ADDR, old_addr_info, 7);
        if (ret < 0) {
            syscfg_write(CFG_AAP_MODE_BLE_ADDR, new_addr_info, 7);
        } else if (check_update_ble_list(old_addr_info, new_addr_info)) {
            syscfg_write(CFG_AAP_MODE_BLE_ADDR, new_addr_info, 7);
        }
        break;

    case PAIR_24G_BIT:
        ret = syscfg_read(CFG_AAP_MODE_24G_ADDR, old_addr_info, 7);
        if (ret < 0) {
            syscfg_write(CFG_AAP_MODE_24G_ADDR, new_addr_info, 7);
        } else if (check_update_ble_list(old_addr_info, new_addr_info)) {
            syscfg_write(CFG_AAP_MODE_24G_ADDR, new_addr_info, 7);
        }
        break;

    default:
        break;
    }

}


//使能edr模块 开关
static void bt_edr_mode_enable(u8 enable)
{
#if TCFG_USER_EDR_ENABLE
    static u8 edr_cfg_en = 1;
    if (edr_cfg_en == enable) {
        r_printf("###------repeat %s:%02x\n", __FUNCTION__, enable);
        return;
    }

    edr_cfg_en = enable;

    log_info("%s:%02x", __FUNCTION__, enable);
    if (enable) {
        user_hid_enable(1);
        btctrler_task_init_bredr();

        if (bt_hid_mode == HID_MODE_EDR) {
            bt_connect_phone_back_start();
        } else {
            bt_wait_phone_connect_control(1);
        }

        sys_auto_sniff_controle(1, NULL);
        if (!edr_hid_timer_handle)   {
            log_info("add edr timer");
            edr_hid_timer_handle = sys_s_hi_timer_add((void *)0, edr_mouse_timer_handler, 5);
        }
    } else {
        user_hid_enable(0);
        if (edr_hid_timer_handle) {
            sys_s_hi_timer_del(edr_hid_timer_handle);
            edr_hid_timer_handle = 0;
            log_info("del edr timer");
        }
        bt_wait_phone_connect_control(0);
        sys_auto_sniff_controle(0, NULL);
        btctrler_task_close_bredr();
    }

#endif
}

//使能ble模块 开关
void ble_module_enable(u8 en);
static void bt_ble_mode_enable(u8 enable)
{
#if TCFG_USER_BLE_ENABLE
    static u8 ble_cfg_en = 1;
    if (ble_cfg_en == enable) {
        r_printf("###------repeat %s:%02x\n", __FUNCTION__, enable);
        return;
    }

    ble_cfg_en = enable;

    log_info("%s:%02x", __FUNCTION__, enable);
    ble_module_enable(enable);

    if (enable) {
        if (!ble_hid_timer_handle)   {
            ble_hid_timer_handle = sys_s_hi_timer_add((void *)0, ble_mouse_timer_handler, 10);
            log_info("add ble timer");
        }
    } else {
        if (ble_hid_timer_handle) {
            sys_s_hi_timer_del(ble_hid_timer_handle);
            ble_hid_timer_handle = 0;
            log_info("del ble timer");
        }
    }

#endif
}

static void bt_24g_mode_set(u8 code_id)
{
#if TCFG_USER_BLE_ENABLE
    log_info("%s:%02x", __FUNCTION__, code_id);
    rf_set_24g_hackable_coded(code_id);
#endif
}

static void app_select_btmode(u8 mode)
{
    if (mode != HID_MODE_INIT) {
        if (bt_hid_mode == mode) {
            return;
        }
        bt_hid_mode = mode;
    } else {
        //init start
    }

    is_hid_active = 1;

    log_info("###### %s: %d,%d\n", __FUNCTION__, mode, bt_hid_mode);
    bt_edr_mode_enable(0);
    bt_ble_mode_enable(0);
    led_all_stop();

    //os_time_dly(10);

    switch (bt_hid_mode) {
    case HID_MODE_BLE:
        log_info("---------app select ble--------\n");
        bt_24g_mode_set(0);
        bt_ble_mode_enable(1);
        LED_BLE_ONOFF(1);

        sys_timeout_add((void *)TT_LED_BLE, app_timeout_handle, LED_MODE_ONOFF_MS);
        led_ble_flash_cnt = 2;
        break;

    case HID_MODE_EDR:
        log_info("---------app select edr--------\n");
        bt_edr_mode_enable(1);
        LED_EDR_ONOFF(1);
        sys_timeout_add((void *)TT_LED_EDR, app_timeout_handle, LED_MODE_ONOFF_MS);
        led_edr_flash_cnt = 0;
        break;

    case HID_MODE_BLE_24G:
        log_info("---------app select 24g--------\n");
        bt_24g_mode_set(CFG_RF_24G_CODE_ID);
        bt_ble_mode_enable(1);
        LED_24G_ONOFF(1);

        sys_timeout_add((void *)TT_LED_24G, app_timeout_handle, LED_MODE_ONOFF_MS);

        if (0 == bt_pair_start) {
            led_24g_flash_cnt = 0;
        } else {
            led_24g_flash_cnt = -1;
        }
        break;

    case HID_MODE_DUAL:
        log_info("---------app select dual--------\n");
        bt_24g_mode_set(0);
        bt_edr_mode_enable(1);
        bt_ble_mode_enable(1);
        LED_BLE_ONOFF(1);
        //			LED_EDR_ONOFF(1);
        sys_timeout_add((void *)TT_LED_DUAL, app_timeout_handle, LED_MODE_ONOFF_MS);
        /* bt_edr_mode_enable(1); */
        led_dual_flash_cnt = -1;
        break;

    case HID_MODE_NULL:
        log_info("---------app select null--------\n");
        break;

    default:
        log_info("###unknow mode:%02x", bt_hid_mode);
        break;
    }

    if (HID_MODE_NULL != bt_hid_mode) {
        hid_vm_deal(1);
    }
    is_hid_active = 0;
}

static void mode_switch_handler(void)
{
    log_info("%s: bt_hid_mode= %02x,bt_pair_mode= %02x", __FUNCTION__, bt_hid_mode, bt_pair_mode);

    if (HID_MODE_DUAL == bt_hid_mode || HID_MODE_BLE == bt_hid_mode || HID_MODE_EDR == bt_hid_mode) {
        app_select_btmode(HID_MODE_BLE_24G);
    } else {
        //判断，进入已配对的模式
        if (IS_PAIR_MODE(PAIR_EDR_BIT)) {
            app_select_btmode(HID_MODE_EDR);
        } else if (IS_PAIR_MODE(PAIR_BLE_BIT)) {
            app_select_btmode(HID_MODE_BLE);
        } else {
            app_select_btmode(HID_MODE_DUAL);
        }
    }
    /* sys_timeout_add(NULL, power_set_soft_reset, WAIT_DISCONN_TIME_MS); */
}

static void mode_enter_pair_start(void)
{
    log_info("%s: bt_hid_mode= %02x,bt_pair_mode= %02x", __FUNCTION__, bt_hid_mode, bt_pair_mode);

    u8 cur_mode = bt_hid_mode;

    //disable mode
    app_select_btmode(HID_MODE_NULL);

    //    CLEAR_PAIR_MODE();

#if TCFG_USER_EDR_ENABLE
    //clear pair info
    //user_send_cmd_prepare(USER_CTRL_DEL_ALL_REMOTE_INFO, 0, NULL);
#endif

#if TCFG_USER_BLE_ENABLE
    //clear pair info
    //ble_list_clear_all();
#endif

    bt_pair_start = 1;
    if (HID_MODE_DUAL == cur_mode || HID_MODE_BLE == cur_mode || HID_MODE_EDR == cur_mode) {
        app_select_btmode(HID_MODE_DUAL);
    } else {
        app_select_btmode(HID_MODE_BLE_24G);
    }

}


static void app_code_sw_event_handler(struct sys_event *event)
{
    static s8 sw_val = 0;

    if (wheel_send_flag) {
        sw_val = 0;
    }

    if (event->u.codesw.event == 0) {
        /* log_info("sw_val = %d\n", event->u.codesw.value); */
        sw_val += event->u.codesw.value;
        first_packet.data[WHEEL_IDX] = sw_val;
    }

    wheel_send_flag = 0;
}

static s16 gradient_acceleration(s16 src)
{
#define GRADIENT_1              3
#define GRADIENT_2              10
#define ACCELERATION_1(x)       (x * 3 / 2) // *1.5
#define ACCELERATION_2(x)       (x * 2) // *2
#define ACCELERATION_3(x)       (x * 3) // *3


    return src;
}


static void gradient_acceleration_xy(s16 *x, s16 *y)
{
#define XY_GRADIENT_1              5
#define XY_GRADIENT_2              10
#define XY_ACCELERATION_1(x)       (x * 3 / 2) // *1.5
#define XY_ACCELERATION_2(x)       (x * 2) // *2
#define XY_ACCELERATION_3(x)       (x * 3) // *3


    s16 src_x = *x;
    s16 src_y = *y;



    if ((src_x < XY_GRADIENT_1) && (src_x > -XY_GRADIENT_1) && (src_y < XY_GRADIENT_1) && (src_y > -XY_GRADIENT_1)) {
        //        *x = XY_ACCELERATION_1(src_x);
        //        *y = XY_ACCELERATION_1(src_y);
    } else if ((src_x < XY_GRADIENT_2) && (src_x > -XY_GRADIENT_2) && (src_y < XY_GRADIENT_2) && (src_y > -XY_GRADIENT_2)) {
        *x = XY_ACCELERATION_2(src_x);
        *y = XY_ACCELERATION_2(src_y);
    }
}

static void app_optical_sensor_event_handler(struct sys_event *event)
{
    static s16 delta_x = 0, delta_y = 0;

    if (sensor_send_flag) {
        delta_x = 0;
        delta_y = 0;
    }

    if (event->u.axis.event == 0) {
        if (((delta_x + event->u.axis.x) >= -2047) && ((delta_x + event->u.axis.x) <= 2047)) {
            //event->u.axis.x = gradient_acceleration(event->u.axis.x);
            //delta_x += event->u.axis.x;
        } else {
            event->u.axis.x = 0;
        }

        if (((delta_y + event->u.axis.y) >= -2047) && ((delta_y + event->u.axis.y) <= 2047)) {
            //event->u.axis.y = gradient_acceleration(event->u.axis.y);
            //delta_y += event->u.axis.y;
        } else {
            event->u.axis.y = 0;
        }

        //        gradient_acceleration_xy(&event->u.axis.x,&event->u.axis.y);

#if 0
        delta_x += event->u.axis.x;
        delta_y += event->u.axis.y;
#else
        //坐标调整
        delta_x += event->u.axis.y;
        delta_y += (-event->u.axis.x);
#endif

        second_packet.data[SENSOR_XLSB_IDX] = delta_x & 0xFF;
        second_packet.data[SENSOR_YLSB_XMSB_IDX] = ((delta_y << 4) & 0xF0) | ((delta_x >> 8) & 0x0F);
        second_packet.data[SENSOR_YMSB_IDX] = (delta_y >> 4) & 0xFF;
        second_packet.event_arg = event->arg;

        //log_info("x = %d.\ty = %d.\n", event->u.axis.x, event->u.axis.y);
    }

    sensor_send_flag = 0;
}


//----------------------------------

typedef struct {
    u16  head_tag;
    u8   last_mode;    //记录 最后使用的模式
    u8   pair_mode;    //记录 已连接bt 模式 HID_MODE_EDR or HID_MODE_BLE,用于模式切换
} hid_vm_cfg_t;
#define	HID_VM_HEAD_TAG (0x3AA3)

static void hid_vm_deal(u8 rw_flag)
{
    hid_vm_cfg_t info;
    int ret;
    int vm_len = sizeof(hid_vm_cfg_t);

    log_info("-hid_info vm_do:%d\n", rw_flag);
    memset(&info, 0, vm_len);

    if (rw_flag == 0) {
        //read config
        bt_hid_mode = HID_MODE_NULL; //default set
        ret = syscfg_read(CFG_AAP_MODE_INFO, (u8 *)&info, vm_len);

        if (!ret) {
            log_info("-null--\n");
        } else {
            if (HID_VM_HEAD_TAG == info.head_tag) {
                log_info("-exist--\n");
                log_info_hexdump((u8 *)&info, vm_len);
                bt_hid_mode = info.last_mode;
                bt_pair_mode = info.pair_mode;
            }
        }

        if (HID_MODE_NULL == bt_hid_mode) {
#if TCFG_USER_BLE_ENABLE && TCFG_USER_EDR_ENABLE
            //没有配置信息，默认开双模蓝牙
            bt_hid_mode = HID_MODE_DUAL;
#elif TCFG_USER_BLE_ENABLE //
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

        }

        if (bt_hid_mode != info.last_mode || HID_MODE_NULL == bt_hid_mode) {
            log_info("-write00--\n");
            info.last_mode = bt_hid_mode;
            info.pair_mode = 0;
            syscfg_write(CFG_AAP_MODE_INFO, (u8 *)&info, vm_len);
        }

    } else {
        //write config
        info.last_mode = bt_hid_mode;
        info.head_tag = HID_VM_HEAD_TAG;
        info.pair_mode = bt_pair_mode;

        hid_vm_cfg_t tmp_info;
        syscfg_read(CFG_AAP_MODE_INFO, (u8 *)&tmp_info, vm_len);

        if (memcmp(&tmp_info, &info, vm_len)) {
            syscfg_write(CFG_AAP_MODE_INFO, (u8 *)&info, vm_len);
            log_info("-write11--\n");
            log_info_hexdump((u8 *)&info, vm_len);
        }
    }
}


static void bt_function_select_init()
{
    __set_user_ctrl_conn_num(TCFG_BD_NUM);
    __set_support_msbc_flag(1);

#if BT_SUPPORT_DISPLAY_BAT
    __bt_set_update_battery_time(60);
#else
    __bt_set_update_battery_time(0);
#endif

    __set_page_timeout_value(8000); /*回连搜索时间长度设置,可使用该函数注册使用，ms单位,u16*/
    __set_super_timeout_value(8000); /*回连时超时参数设置。ms单位。做主机有效*/

#if (TCFG_BD_NUM == 2)
    __set_auto_conn_device_num(2);
#endif

    //io_capabilities ; /*0: Display only 1: Display YesNo 2: KeyboardOnly 3: NoInputNoOutput*/
    //authentication_requirements: 0:not protect  1 :protect
    __set_simple_pair_param(3, 0, 2);

#if TCFG_USER_BLE_ENABLE
    {
        u8 tmp_ble_addr[6];
        /* le_controller_set_mac((void*)"012345"); */
        lib_make_ble_address(tmp_ble_addr, (void *)bt_get_mac_addr());
        le_controller_set_mac((void *)tmp_ble_addr);

        printf("\n-----edr + ble 's address-----");
        printf_buf((void *)bt_get_mac_addr(), 6);
        printf_buf((void *)tmp_ble_addr, 6);
    }
#endif

}

static void bredr_handle_register()
{
#if (USER_SUPPORT_PROFILE_HID==1)
    user_hid_set_icon(BD_CLASS_MOUSE);
    user_hid_set_ReportMap(hid_report_map, sizeof(hid_report_map));
    user_hid_init(NULL);
#endif

    /* bt_dut_test_handle_register(bt_dut_api); */
}
void soft_poweroff_wakeup_reset(void);
void hid_set_soft_poweroff(void)
{
    log_info("hid_set_soft_poweroff\n");
    is_hid_active = 1;
    //必须先主动断开蓝牙链路,否则要等链路超时断开
    if (bt_hid_mode == HID_MODE_EDR) {
        user_hid_enable(0);
    } else {
#if TCFG_USER_BLE_ENABLE
        bt_ble_mode_enable(0);
#endif
    }

    soft_poweroff_wakeup_reset();

    //延时300ms，确保BT退出链路断开
    sys_timeout_add(NULL, power_set_soft_poweroff, WAIT_DISCONN_TIME_MS);
}

static void app_timeout_handle(u32 type)
{
    log_info("%s:%02x,warnning=%d", __FUNCTION__, type, led_warning_flash_cnt);

    switch (type) {
    case TT_PAGE_AGAIN:
        bt_connect_phone_back_start();
        break;

    case TT_CLR_DOG:
        if (!edr_hid_is_connected()) {
            sys_timeout_add((void *)TT_CLR_DOG, app_timeout_handle, SYS_WAKEUP_CLRDOG_TIME);
        }
        break;

    case TT_LED_LOWPOWER_WARNING:
        log_info("led_warning_flash_cnt= %d", led_warning_flash_cnt);
        if (led_warning_flash_cnt) {
            led_warning_flash_cnt--;
            if (bt_hid_mode == HID_MODE_BLE_24G) {
                LED_24G_ONOFF(!LED_24G_STATE());
            } else {
                LED_BLE_ONOFF(!LED_BLE_STATE());
            }
            sys_timeout_add((void *)TT_LED_LOWPOWER_WARNING, app_timeout_handle, LED_LOWPOWER_WARNING_MS);
        } else {
            LED_24G_ONOFF(0);
            LED_BLE_ONOFF(0);
        }
        break;

    case TT_LED_24G:
        //有低电显示，不控制LED
        log_info("led_24g_flash_cnt= %d", led_24g_flash_cnt);
        if (led_24g_flash_cnt) {
            led_24g_flash_cnt--;
            if (0 == led_warning_flash_cnt) {
                LED_24G_ONOFF(!LED_24G_STATE());
            }
            sys_timeout_add((void *)TT_LED_24G, app_timeout_handle, LED_MODE_ONOFF_MS);
        } else {
            if (0 == led_warning_flash_cnt) {
                LED_24G_ONOFF(0);
            }
        }
        break;

    case TT_LED_BLE:
        log_info("led_ble_flash_cnt= %d", led_ble_flash_cnt);
        if (led_ble_flash_cnt) {
            led_ble_flash_cnt--;
            if (0 == led_warning_flash_cnt) {
                LED_BLE_ONOFF(!LED_BLE_STATE());
            }
            sys_timeout_add((void *)TT_LED_BLE, app_timeout_handle, LED_MODE_ONOFF_MS);
        } else {
            if (0 == led_warning_flash_cnt) {
                LED_BLE_ONOFF(0);
            }
        }
        break;

    case TT_LED_EDR:
        log_info("led_edr_flash_cnt= %d", led_edr_flash_cnt);
        if (led_edr_flash_cnt) {
            led_edr_flash_cnt--;
            if (0 == led_warning_flash_cnt) {
                LED_EDR_ONOFF(!LED_EDR_STATE());
            }
            sys_timeout_add((void *)TT_LED_EDR, app_timeout_handle, LED_MODE_ONOFF_MS);
        } else {
            if (0 == led_warning_flash_cnt) {
                LED_EDR_ONOFF(0);
            }
        }
        break;

    case TT_LED_DUAL:
        log_info("led_dual_flash_cnt= %d", led_dual_flash_cnt);
        if (led_dual_flash_cnt) {
            led_dual_flash_cnt--;
            if (0 == led_warning_flash_cnt) {
                LED_BLE_ONOFF(!LED_BLE_STATE());

            }
            sys_timeout_add((void *)TT_LED_DUAL, app_timeout_handle, LED_MODE_ONOFF_MS);
        } else {
            if (0 == led_warning_flash_cnt) {
                LED_BLE_ONOFF(0);
            }
        }
        break;

    case TT_CHECK_PAIR_COMPLETE:
        if (pair_step > 0) {
            log_info("timeout to disconnect no pair");
            ble_app_disconnect();
        }
        break;

    default:
        break;
    }
}

//显示低电灯
static void app_lowpower_warning_start(void)
{
    log_info("%s:%d", __FUNCTION__, led_warning_flash_cnt);
    if (led_warning_flash_cnt == 0) {
        //init start

        if (bt_hid_mode == HID_MODE_BLE_24G) {

            LED_24G_ONOFF(1);
        } else {
            LED_BLE_ONOFF(1);
        }
        sys_timeout_add((void *)TT_LED_LOWPOWER_WARNING, app_timeout_handle, LED_LOWPOWER_WARNING_MS);
        led_warning_flash_cnt = 5;
    } else {
        led_warning_flash_cnt = 5;
        //keep to do
    }

}

extern void bt_pll_para(u32 osc, u32 sys, u8 low_power, u8 xosc);
static void app_start()
{
    log_info("=======================================");
    log_info("-------------HID Mouse-----------------");
    log_info("=======================================");

    clk_set("sys", BT_NORMAL_HZ);
    u32 sys_clk =  clk_get("sys");
    bt_pll_para(TCFG_CLOCK_OSC_HZ, sys_clk, 0, 0);


    bt_function_select_init();
    bredr_handle_register();
    btstack_init();

#if TCFG_USER_EDR_ENABLE
    sys_auto_sniff_controle(1, NULL);
#endif
    /* 按键消息使能 */
    sys_key_event_enable();

#if (TCFG_HID_AUTO_SHUTDOWN_TIME)
    //无操作定时软关机
    g_auto_shutdown_timer = sys_timeout_add(NULL, hid_set_soft_poweroff, TCFG_HID_AUTO_SHUTDOWN_TIME * 1000);
#endif
    /* sys_auto_shut_down_enable(); */
    /* sys_auto_sniff_controle(1, NULL); */
}

static int state_machine(struct application *app, enum app_state state, struct intent *it)
{
    switch (state) {
    case APP_STA_CREATE:
        break;
    case APP_STA_START:
        if (!it) {
            break;
        }
        switch (it->action) {
        case ACTION_MOUSE_MAIN:
            app_start();
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

#define  SNIFF_CNT_TIME               1/////<空闲5S之后进入sniff模式

#define SNIFF_MAX_INTERVALSLOT        16
#define SNIFF_MIN_INTERVALSLOT        16
#define SNIFF_ATTEMPT_SLOT            2
#define SNIFF_TIMEOUT_SLOT            1

static u8 sniff_ready_status = 0; //0:sniff_ready 1:sniff_not_ready
static int exit_sniff_timer = 0;


void bt_check_exit_sniff()
{
    if (exit_sniff_timer) {
        sys_timeout_del(exit_sniff_timer);
        exit_sniff_timer = 0;
    }
    user_send_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
}

void bt_sniff_ready_clean(void)
{
    sniff_ready_status = 1;
}

void bt_check_enter_sniff()
{
    struct sniff_ctrl_config_t sniff_ctrl_config;
    u8 addr[12];
    u8 conn_cnt = 0;
    u8 i = 0;

    if (sniff_ready_status) {
        sniff_ready_status = 0;
        return;
    }

    /*putchar('H');*/
    conn_cnt = bt_api_enter_sniff_status_check(SNIFF_CNT_TIME, addr);

    ASSERT(conn_cnt <= 2);

    for (i = 0; i < conn_cnt; i++) {
        log_info("-----USER SEND SNIFF IN %d %d\n", i, conn_cnt);
        sniff_ctrl_config.sniff_max_interval = SNIFF_MAX_INTERVALSLOT;
        sniff_ctrl_config.sniff_mix_interval = SNIFF_MIN_INTERVALSLOT;
        sniff_ctrl_config.sniff_attemp = SNIFF_ATTEMPT_SLOT;
        sniff_ctrl_config.sniff_timeout  = SNIFF_TIMEOUT_SLOT;
        memcpy(sniff_ctrl_config.sniff_addr, addr + i * 6, 6);
        user_send_cmd_prepare(USER_CTRL_SNIFF_IN, sizeof(struct sniff_ctrl_config_t), (u8 *)&sniff_ctrl_config);
    }

}
void sys_auto_sniff_controle(u8 enable, u8 *addr)
{
#if TCFG_USER_EDR_ENABLE
    if (addr) {
        if (bt_api_conn_mode_check(enable, addr) == 0) {
            log_info("sniff ctr not change\n");
            return;
        }
    }

    if (enable) {
        if (addr) {
            log_info("sniff cmd timer init\n");
            user_cmd_timer_init();
        }

        if (sniff_timer == 0) {
            log_info("check_sniff_enable\n");
            sniff_timer = sys_timer_add(NULL, bt_check_enter_sniff, 100);
        }
    } else {

        if (addr) {
            log_info("sniff cmd timer remove\n");
            remove_user_cmd_timer();
        }

        if (sniff_timer) {
            log_info("check_sniff_disable\n");
            sys_timeout_del(sniff_timer);
            sniff_timer = 0;

            if (exit_sniff_timer == 0) {
                /* exit_sniff_timer = sys_timer_add(NULL, bt_check_exit_sniff, 5000); */
            }
        }
    }
#endif
}
/*开关可发现可连接的函数接口*/
static void bt_wait_phone_connect_control(u8 enable)
{
    if (enable) {
        if (bt_hid_mode != HID_MODE_EDR && bt_hid_mode != HID_MODE_DUAL) {
            return;
        }
        log_info("is_1t2_connection:%d \t total_conn_dev:%d, enable:%d\n", is_1t2_connection(), get_total_connect_dev(), enable);
        if (is_1t2_connection()) {
            /*达到最大连接数，可发现(0)可连接(0)*/
            user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
            user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
        } else {
            if (get_total_connect_dev() == 1) {
                /*支持连接2台，只连接一台的情况下，可发现(0)可连接(1)*/
                user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
                user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
            } else {
                /*可发现(1)可连接(1)*/
                user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
                user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
            }
        }
    } else {
        user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
    }
    log_info("bt_end\n");
}


void bt_send_pair(u8 en)
{
    log_info("%s:%d", __FUNCTION__, en);
    user_send_cmd_prepare(USER_CTRL_PAIR, 1, &en);
}


u8 connect_last_device_from_vm();
int bt_connect_phone_back_start(void)
{
    if (bt_hid_mode != HID_MODE_EDR) {
        return 0;
    }

    if (connect_last_device_from_vm()) {
        log_info("------bt_connect_phone_start------\n");
        return 1 ;
    }
    return 0;
}






#define HCI_EVENT_INQUIRY_COMPLETE                            0x01
#define HCI_EVENT_CONNECTION_COMPLETE                         0x03
#define HCI_EVENT_DISCONNECTION_COMPLETE                      0x05
#define HCI_EVENT_PIN_CODE_REQUEST                            0x16
#define HCI_EVENT_IO_CAPABILITY_REQUEST                       0x31
#define HCI_EVENT_USER_CONFIRMATION_REQUEST                   0x33
#define HCI_EVENT_USER_PASSKEY_REQUEST                        0x34
#define HCI_EVENT_USER_PRESSKEY_NOTIFICATION			      0x3B
#define HCI_EVENT_VENDOR_NO_RECONN_ADDR                       0xF8
#define HCI_EVENT_VENDOR_REMOTE_TEST                          0xFE
#define BTSTACK_EVENT_HCI_CONNECTIONS_DELETE                  0x6D


#define ERROR_CODE_SUCCESS                                    0x00
#define ERROR_CODE_PAGE_TIMEOUT                               0x04
#define ERROR_CODE_AUTHENTICATION_FAILURE                     0x05
#define ERROR_CODE_PIN_OR_KEY_MISSING                         0x06
#define ERROR_CODE_CONNECTION_TIMEOUT                         0x08
#define ERROR_CODE_SYNCHRONOUS_CONNECTION_LIMIT_TO_A_DEVICE_EXCEEDED  0x0A
#define ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS                      0x0B
#define ERROR_CODE_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES       0x0D
#define ERROR_CODE_CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR    0x0F
#define ERROR_CODE_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED         0x10
#define ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION          0x13
#define ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST        0x16

#define CUSTOM_BB_AUTO_CANCEL_PAGE                            0xFD  //// app cancle page
#define BB_CANCEL_PAGE                                        0xFE  //// bb cancle page



static void bt_hci_event_connection(struct bt_event *bt)
{
    u8 record_addr[6];
    bt_wait_phone_connect_control(0);
    log_info("bt_hci_event_connection");
    put_buf(bt->args, 6);
}

static void bt_hci_event_disconnect(struct bt_event *bt)
{
    bt_wait_phone_connect_control(1);
}

static void bt_hci_event_linkkey_missing(struct bt_event *bt)
{
}

static void bt_edr_reconnect_do(void)
{
    if (bt_hid_mode == HID_MODE_DUAL) {
        //配对模式下
        bt_wait_phone_connect_control(1);
    } else {
        sys_timeout_add((void *)TT_PAGE_AGAIN, app_timeout_handle, EDR_CYCLE_RECONNECT_TIME);
        sys_timeout_add((void *)TT_CLR_DOG, app_timeout_handle, SYS_WAKEUP_CLRDOG_TIME);
    }
}

static void bt_hci_event_page_timeout(struct bt_event *bt)
{
    if (bt_pair_start) {
        bt_wait_phone_connect_control(1);
    } else {
        bt_edr_reconnect_do();
    }
}

static void bt_hci_event_connection_timeout(struct bt_event *bt)
{
    if (bt_pair_start) {
        bt_wait_phone_connect_control(1);
    } else {
        bt_edr_reconnect_do();
    }
}

static void bt_hci_event_connection_exist(struct bt_event *bt)
{
    if (bt_pair_start) {
        bt_wait_phone_connect_control(1);
    } else {
        bt_edr_reconnect_do();
    }
}

extern void set_remote_test_flag(u8 own_remote_test);
static int bt_hci_event_handler(struct bt_event *bt)
{
    //对应原来的蓝牙连接上断开处理函数  ,bt->value=reason
    log_info("------------------------bt_hci_event_handler reason %x %x", bt->event, bt->value);

    if (bt->event == HCI_EVENT_VENDOR_REMOTE_TEST) {
        if (0 == bt->value) {
            set_remote_test_flag(0);
            log_info("clear_test_box_flag");
            return 0;
        } else {

#if TCFG_USER_BLE_ENABLE
            if (1 == bt->value) {
                extern void bt_ble_adv_enable(u8 enable);
                bt_ble_adv_enable(0);
            }
#endif
        }
    }

    if ((bt->event != HCI_EVENT_CONNECTION_COMPLETE) ||
        ((bt->event == HCI_EVENT_CONNECTION_COMPLETE) && (bt->value != ERROR_CODE_SUCCESS))) {
#if TCFG_TEST_BOX_ENABLE
        if (chargestore_get_testbox_status()) {
            if (get_remote_test_flag()) {
                chargestore_clear_connect_status();
            }
            //return 0;
        }
#endif
        if (get_remote_test_flag() \
            && !(HCI_EVENT_DISCONNECTION_COMPLETE == bt->event) \
            && !(HCI_EVENT_VENDOR_REMOTE_TEST == bt->event)) {
            log_info("cpu reset\n");
            cpu_reset();
        }
    }

    switch (bt->event) {
    case HCI_EVENT_INQUIRY_COMPLETE:
        log_info(" HCI_EVENT_INQUIRY_COMPLETE \n");
        /* bt_hci_event_inquiry(bt); */
        break;
    case HCI_EVENT_USER_CONFIRMATION_REQUEST:
        log_info(" HCI_EVENT_USER_CONFIRMATION_REQUEST \n");
        ///<可通过按键来确认是否配对 1：配对   0：取消

#if PAIR_BONDING_DEVICE
        bt_send_pair(bt_pair_start);
#else
        bt_send_pair(1);
#endif
        break;

    case HCI_EVENT_USER_PASSKEY_REQUEST:
        log_info(" HCI_EVENT_USER_PASSKEY_REQUEST \n");
        ///<可以开始输入6位passkey
        break;
    case HCI_EVENT_USER_PRESSKEY_NOTIFICATION:
        log_info(" HCI_EVENT_USER_PRESSKEY_NOTIFICATION %x\n", bt->value);
        ///<可用于显示输入passkey位置 value 0:start  1:enrer  2:earse   3:clear  4:complete
        break;
    case HCI_EVENT_PIN_CODE_REQUEST :
        log_info("HCI_EVENT_PIN_CODE_REQUEST  \n");

#if PAIR_BONDING_DEVICE
        bt_send_pair(bt_pair_start);
#else
        bt_send_pair(1);
#endif
        break;

    case HCI_EVENT_VENDOR_NO_RECONN_ADDR :
        log_info("HCI_EVENT_VENDOR_NO_RECONN_ADDR \n");
        bt_hci_event_disconnect(bt) ;
        break;

    case HCI_EVENT_DISCONNECTION_COMPLETE :
        log_info("HCI_EVENT_DISCONNECTION_COMPLETE \n");
        bt_hci_event_disconnect(bt) ;
        break;

    case BTSTACK_EVENT_HCI_CONNECTIONS_DELETE:
    case HCI_EVENT_CONNECTION_COMPLETE:
        log_info(" HCI_EVENT_CONNECTION_COMPLETE \n");
        switch (bt->value) {
        case ERROR_CODE_SUCCESS :
            log_info("ERROR_CODE_SUCCESS  \n");
            bt_hci_event_connection(bt);
            break;
        case ERROR_CODE_PIN_OR_KEY_MISSING:
            log_info(" ERROR_CODE_PIN_OR_KEY_MISSING \n");
            bt_hci_event_linkkey_missing(bt);

        case ERROR_CODE_SYNCHRONOUS_CONNECTION_LIMIT_TO_A_DEVICE_EXCEEDED :
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES:
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR:
        case ERROR_CODE_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED  :
        case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION   :
        case ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST :
        case ERROR_CODE_AUTHENTICATION_FAILURE :
        case CUSTOM_BB_AUTO_CANCEL_PAGE:
            bt_hci_event_disconnect(bt) ;
            break;

        case ERROR_CODE_PAGE_TIMEOUT:
            log_info(" ERROR_CODE_PAGE_TIMEOUT \n");
            bt_hci_event_page_timeout(bt);
            break;

        case ERROR_CODE_CONNECTION_TIMEOUT:
            log_info(" ERROR_CODE_CONNECTION_TIMEOUT \n");
            bt_hci_event_connection_timeout(bt);
            break;

        case ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS  :
            log_info("ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS   \n");
            bt_hci_event_connection_exist(bt);
            break;
        default:
            break;

        }
        break;
    default:
        break;

    }
    return 0;
}

extern void mouse_board_devices_init(void);
static int bt_connction_status_event_handler(struct bt_event *bt)
{

    log_info("-----------------------bt_connction_status_event_handler %d", bt->event);

    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        /*
         * 蓝牙初始化完成
         */
        log_info("BT_STATUS_INIT_OK\n");
        mouse_board_devices_init();

        u16 init_set_cpi = optical_mouse_sensor_set_cpi(cpi_value_table[mouse_cpi_mode]);
        log_info("init_reset_cpi= %d", init_set_cpi);

        //read vm first
        hid_vm_deal(0);//bt_hid_mode read for VM

#if TCFG_USER_BLE_ENABLE
        ble_set_fix_pwr(6);//range:0~9
        le_hogp_set_icon(BLE_APPEARANCE_HID_MOUSE);//mouse
        le_hogp_set_ReportMap(hid_report_map, sizeof(hid_report_map));

        log_info("##init_24g_code: %04x", CFG_RF_24G_CODE_ID);

        bt_ble_init();
#endif

        if (sniff_support_reset_anchor_point) {
            lmp_sniff_t_slot_attemp_reset(SNIFF_MAX_INTERVALSLOT, SNIFF_ATTEMPT_SLOT);
        }

#if MIDDLE_KEY_SWITCH
        int mode_sel = 0;
        int i =  0;
        for (i = 0; i < 5; i++) {
            mode_sel = DETECT_MOUSE_MODE();
        }
        bt_hid_mode = HID_MODE_NULL;
        log_info("mode_detect: %d", mode_sel);

        /* mode_sel = 0; */

        if (1 == mode_sel) {
            if (!IS_PAIR_MODE(PAIR_EDR_BIT)) {
                bt_pair_start = 1;
            }
            app_select_btmode(HID_MODE_BLE_24G);
        } else {
            if (IS_PAIR_MODE(PAIR_EDR_BIT)) {
                app_select_btmode(HID_MODE_EDR);
            } else if (IS_PAIR_MODE(PAIR_BLE_BIT)) {
                app_select_btmode(HID_MODE_BLE);
            } else {
                bt_pair_start = 1;
                app_select_btmode(HID_MODE_DUAL);
            }

        }
#else
        app_select_btmode(HID_MODE_INIT);
#endif
        break;

    case BT_STATUS_SECOND_CONNECTED:
    case BT_STATUS_FIRST_CONNECTED:
        log_info("BT_STATUS_CONNECTED\n");
        put_buf(bt->args, 6);

        if (bt_hid_mode == HID_MODE_DUAL) {
            //配对模式下
            bt_ble_mode_enable(0);
            bt_hid_mode = HID_MODE_EDR; //保存当前HID模式
            CLEAR_PAIR_MODE(PAIR_BLE_BIT);
            SET_PAIR_MODE(PAIR_EDR_BIT);
            hid_vm_deal(1);
            led_all_stop();
            pair_info_address_update(PAIR_EDR_BIT, bt->args);
            bt_pair_start = 0;
        }
        break;

    case BT_STATUS_FIRST_DISCONNECT:
    case BT_STATUS_SECOND_DISCONNECT:
        log_info("BT_STATUS_DISCONNECT\n");
        break;

    case BT_STATUS_PHONE_INCOME:
        log_info("BT_STATUS_PHONE_INCOME\n");
        break;

    case BT_STATUS_PHONE_OUT:
        log_info("BT_STATUS_PHONE_OUT\n");
        break;

    case BT_STATUS_PHONE_ACTIVE:
        log_info("BT_STATUS_PHONE_ACTIVE\n");
        break;

    case BT_STATUS_PHONE_HANGUP:
        log_info("BT_STATUS_PHONE_HANGUP\n");
        break;

    case BT_STATUS_PHONE_NUMBER:
        log_info("BT_STATUS_PHONE_NUMBER\n");
        break;

    case BT_STATUS_INBAND_RINGTONE:
        log_info("BT_STATUS_INBAND_RINGTONE\n");
        break;

    case BT_STATUS_BEGIN_AUTO_CON:
        log_info("BT_STATUS_BEGIN_AUTO_CON\n");
        break;

    case BT_STATUS_A2DP_MEDIA_START:
        log_info(" BT_STATUS_A2DP_MEDIA_START");
        break;

    case BT_STATUS_SNIFF_STATE_UPDATE:
        log_info(" BT_STATUS_SNIFF_STATE_UPDATE %d\n", bt->value);    //0退出SNIFF
        if (bt->value == 0) {
            sys_auto_sniff_controle(1, bt->args);
        } else {
            sys_auto_sniff_controle(0, bt->args);
        }
        break;
    case  BT_STATUS_TRIM_OVER:
        log_info("BT STATUS TRIM OVER\n");
        break;
    default:
        log_info(" BT STATUS DEFAULT\n");
        break;
    }
    return 0;
}

u16 optical_mouse_sensor_set_cpi(u16 cpi_sel);
u16 hal_pixart_set_cpi_ext(u8 cpi_id);
static void app_key_event_handler(struct sys_event *event)
{
    u16 cpi = 0;
    u8 event_type = 0;

    if (event->arg == (void *)DEVICE_EVENT_FROM_KEY) {
        log_info("key_value = %d.\tevent_type = %d.\n", event->u.key.value, event->u.key.event);
        event_type = event->u.key.event;

        first_packet.data[BUTTONS_IDX] = 0;

        if (event_type == KEY_EVENT_CLICK || \
            event_type == KEY_EVENT_LONG || \
            event_type == KEY_EVENT_HOLD) {
            first_packet.data[BUTTONS_IDX] |= (event->u.key.value & 0x1f);
        }

        //设置CPI
        if (event->u.key.value == KEY_CPI_VAL && event_type == KEY_EVENT_LONG) {
            log_info("cpi key hold:%d", cpi_key_long_cnt);
            if (++cpi_key_long_cnt >= CPI_KEY_HOLD_CNT) {

                u8 flash_count;
                mouse_cpi_mode++;
                cpi_key_long_cnt = 0;
                if (mouse_cpi_mode > 2) {
                    mouse_cpi_mode = 0;
                }

                flash_count = cpi_led_flash_cnt[mouse_cpi_mode];
                u16 cur_cpi = optical_mouse_sensor_set_cpi(cpi_value_table[mouse_cpi_mode]);

                //delay ms 开始点LED
                u16 delay_time_on = 10;

                switch (bt_hid_mode) {
                case HID_MODE_BLE:
                    if (ble_hid_is_connected()) {
                        LED_BLE_ONOFF(0);
                        led_ble_flash_cnt = flash_count;
                        sys_timeout_add((void *)TT_LED_BLE, app_timeout_handle, delay_time_on);
                    }
                    break;

                case HID_MODE_EDR:
                    if (edr_hid_is_connected()) {
                        LED_EDR_ONOFF(0);
                        led_edr_flash_cnt = flash_count;
                        sys_timeout_add((void *)TT_LED_EDR, app_timeout_handle, delay_time_on);
                    }
                    break;

                case HID_MODE_BLE_24G:
                    if (ble_hid_is_connected()) {
                        LED_24G_ONOFF(0);
                        led_24g_flash_cnt = flash_count;
                        sys_timeout_add((void *)TT_LED_24G, app_timeout_handle, delay_time_on);
                    }
                    break;
                }

                log_info("CPI= %d, flash_count= %d", cur_cpi, flash_count);
            }
        }

        else {

            cpi_key_long_cnt = 0 ;
        }


        //左键+右键 长按数秒进入
#if (DOUBLE_KEY_HOLD_PAIR)
        if (event->u.key.value < KEY_HK_VAL && event_type == KEY_EVENT_LONG) {
            if (ONE_PORT_TO_LOW == gpio_read(TCFG_IOKEY_MOUSE_LK_PORT) && ONE_PORT_TO_LOW == gpio_read(TCFG_IOKEY_MOUSE_RK_PORT)) {
                log_info("double key hold1:%d", double_key_long_cnt);
                if (++double_key_long_cnt >= DOUBLE_KEY_HOLD_CNT) {
                    mode_enter_pair_start();
                    double_key_long_cnt = 0;
                }

            }
        } else {
            double_key_long_cnt = 0;
        }
#endif
        //中键长按数秒切换 edr + ble, or 2.4g


    }


    if (button_send_flag) {
        button_send_flag = 0;
    }
}

static bool check_ble_connect_dev_is_allow(u8 *conn_old_addr_info)
{
    u8 tmp_address1[6];
    u8 tmp_out_address1[6];

    swapX(conn_old_addr_info + 1, tmp_address1, 6);
    return ble_list_check_addr_is_exist(tmp_address1, conn_old_addr_info[0]);
}

static void hogp_ble_status_callback(ble_state_e status, u8 reason)
{

    log_info("hogp_ble_status_callback============== %02x   reason:0x%x\n", status, reason);
    switch (status) {
    case BLE_ST_IDLE:
        break;

    case BLE_ST_ADV:
        break;

    case BLE_ST_CONNECT:
        log_info("BLE_ST_CONNECT\n");
        pair_step = 1;
        sys_timeout_add((void *)TT_CHECK_PAIR_COMPLETE, app_timeout_handle, BLE_WAIT_PAIR_TIME_MS);
        break;

    case BLE_PRIV_MSG_PAIR_CONFIRM:
        log_info("BLE_PRIV_MSG_PAIR_CONFIRM\n");

#if PAIR_BONDING_DEVICE
        if (0 == bt_pair_start && !check_ble_24g_new_address_is_same()) {
            log_info("not allow to connect!!!");
            ble_app_disconnect();
            break;
        }
#endif
        pair_step++;
        break;

    case BLE_PRIV_PAIR_ENCRYPTION_CHANGE:
        log_info("BLE_PRIV_PAIR_ENCRYPTION_CHANGE\n");

        pair_step++;

        if (bt_hid_mode == HID_MODE_DUAL) {
            //配对模式下
            bt_edr_mode_enable(0);
            bt_hid_mode = HID_MODE_BLE; //保存当前HID模式
            CLEAR_PAIR_MODE(PAIR_EDR_BIT);
            SET_PAIR_MODE(PAIR_BLE_BIT);
            hid_vm_deal(1);
            led_all_stop();
            if (pair_step == 3) {
                pair_info_address_update(PAIR_BLE_BIT, ble_cur_connect_addrinfo());
            }

        } else if (bt_hid_mode == HID_MODE_BLE_24G && bt_pair_start) {
            SET_PAIR_MODE(PAIR_24G_BIT);
            hid_vm_deal(1);
            led_all_stop();
            if (pair_step == 3) {
                pair_info_address_update(PAIR_24G_BIT, ble_cur_connect_addrinfo());
            }
        }
        bt_pair_start = 0;
        pair_step = 0;
        break;

    case BLE_ST_SEND_DISCONN:
        pair_step = 0;
        break;

    case BLE_ST_DISCONN:
        if (reason == ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST) {
            log_info("BLE_ST_DISCONN BY LOCAL\n");
        }
        break;

    case BLE_ST_NOTIFY_IDICATE:
        break;

    default:
        break;
    }
}

static int event_handler(struct application *app, struct sys_event *event)
{
#if (TCFG_HID_AUTO_SHUTDOWN_TIME)
    //重置无操作定时计数
    if (event->type != SYS_DEVICE_EVENT || DEVICE_EVENT_FROM_POWER != event->arg) { //过滤电源消息
        sys_timer_modify(g_auto_shutdown_timer, TCFG_HID_AUTO_SHUTDOWN_TIME * 1000);
    }
#endif

    bt_sniff_ready_clean();


    switch (event->type) {
    case SYS_KEY_EVENT:
        /* log_info("Sys Key : %s", event->arg); */
        app_key_event_handler(event);
        return 0;

    case SYS_BT_EVENT:
        if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
            bt_connction_status_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_TYPE_HCI_STATUS) {
            bt_hci_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_BLE_STATUS) {
            hogp_ble_status_callback(event->u.bt.event, event->u.bt.value);
        }

        return 0;

    case SYS_DEVICE_EVENT:
        if (event->arg == "code_switch") {
            app_code_sw_event_handler(event);
        } else if (event->arg == "omsensor_axis") {
            app_optical_sensor_event_handler(event);
        } else if ((u32)event->arg == DEVICE_EVENT_FROM_POWER) {
            if (event->u.dev.event == POWER_EVENT_POWER_WARNING) {
                if (bt_hid_mode != NULL) {
                    app_lowpower_warning_start();
                }

            }
            return app_power_event_handler(&event->u.dev);
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

static const struct application_operation app_mouse_ops = {
    .state_machine  = state_machine,
    .event_handler 	= event_handler,
};

/*
 * 注册AT Module模式
 */
REGISTER_APPLICATION(app_mouse) = {
    .name 	= "mouse",
    .action	= ACTION_MOUSE_MAIN,
    .ops 	= &app_mouse_ops,
    .state  = APP_STA_DESTROY,
};

//-----------------------
//system check go sleep is ok
static u8 app_hid_idle_query(void)
{
    return !is_hid_active;
}

REGISTER_LP_TARGET(app_hid_lp_target) = {
    .name = "app_hid_deal",
    .is_idle = app_hid_idle_query,
};


#endif


