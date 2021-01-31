
#include "typedef.h"
#include "app_config.h"
#include "task.h"

#define LOG_TAG             "[BT_DUT]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

//bredr test api
extern void ble_enter_dut_tx_mode(void *param);
extern void bt_ble_adv_enable(u8 enable);
extern void bredr_fcc_init(u8 mode, u8 fre);

#if 0

static void bt_dut_api(void)
{
    log_info("bt in dut \n");
#if TCFG_AUTO_SHUT_DOWN_TIME
    extern void sys_auto_shut_down_disable(void);
    sys_auto_shut_down_disable();
#endif

#if TCFG_USER_TWS_ENABLE
    extern 	void tws_cancle_all_noconn();
    tws_cancle_all_noconn() ;
#else
    //sys_timer_del(app_var.auto_stop_page_scan_timer);
    extern void bredr_close_all_scan();
    bredr_close_all_scan();
#endif

#if TCFG_USER_BLE_ENABLE
#if (CONFIG_BT_MODE == BT_NORMAL)
    bt_ble_adv_enable(0);
#endif
#endif
}

void bit_clr_ie(unsigned char index);
/* !!!Notice:when this api is called and sleep mode should be sure to exit; */
void bt_fix_fre_api(u8 fre)
{
    bt_dut_api();

    bit_clr_ie(IRQ_BREDR_IDX);
    bit_clr_ie(IRQ_BT_CLKN_IDX);

    bredr_fcc_init(BT_FRE, fre);
}
#endif

//ble test api
enum {
    PAYLOAD_TYPE_PRBS9 = 0,
    PAYLOAD_TYPE_11110000,
    PAYLOAD_TYPE_10101010,
    PAYLOAD_TYPE_PRBS15,
    PAYLOAD_TYPE_11111111,
    PAYLOAD_TYPE_00000000,
    PAYLOAD_TYPE_00001111,
    PAYLOAD_TYPE_01010101,

    PAYLOAD_TYPE_SINGLE_CARRIER = 0xf0,
};

struct ble_dut_param_set {
    u8 ch_index;		//tx ch index;(0~39 -> 2402~2480)
    u8 payload_type;	//tx payload type
    u8 payload_len;		//payload_len(0~0xff) when continuous_tx = 0;
    u8 continuous_tx;	//enable or disable continuous transmission mode(0/1)
};

/* !!!Notice:when this api is called and sleep mode should be sure to exit; */
void ble_fix_fre_api()
{
#if TCFG_USER_BLE_ENABLE
#if (CONFIG_BT_MODE == BT_NORMAL)
    bt_ble_adv_enable(0);
#endif
    os_time_dly(10);

    struct ble_dut_param_set dut_param = {
        .ch_index = 0,
        .payload_type = PAYLOAD_TYPE_10101010,
        .payload_len = 0x20,
        .continuous_tx = 1,
    };
    ble_enter_dut_tx_mode(&dut_param);
#endif
}

