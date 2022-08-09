
#include "app_config.h"
#include "app_action.h"

#include "system/includes.h"
#include "spp_user.h"
#include "string.h"
#include "circular_buf.h"
#include "bt_common.h"
#include "btstack/avctp_user.h"
#include "spp_at_trans.h"
#include "at.h"

#if 1
/* #define log_info          printf */
#define log_info(x, ...)  printf("[SPP_AT]" x " ", ## __VA_ARGS__)
#define log_info_hexdump  put_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif


#if CONFIG_APP_AT_COM
#if TCFG_USER_EDR_ENABLE && USER_SUPPORT_PROFILE_SPP


#define TEST_SPP_DATA_RATE        0

#if TEST_SPP_DATA_RATE
#define SPP_TIMER_MS            (100)
static u32 test_data_count;
static u32 spp_timer_handle = 0;
static u32 spp_test_start;
#endif

static struct spp_operation_t *spp_api = NULL;
static u8 spp_state;
static u8 bt_edr_status;
static void (*at_send_event_callbak)(u8 event_type, const u8 *packet, int size);

extern const char *bt_get_local_name();
extern void bt_set_local_name(char *name, u8 len);
extern const u8 *bt_get_mac_addr();
extern void bt_set_mac_addr(u8 *addr);
extern void lmp_hci_write_local_address(const u8 *addr);
extern void lmp_hci_write_local_name(const char *name);
extern void lmp_hci_write_class_of_device(int class);

int bt_comm_edr_sniff_clean(void);

static void edr_at_send_event(u8 event_type, const u8 *packet, int size);
extern void bt_sniff_ready_clean(void);

int at_spp_send_data(u8 *data, u16 len)
{
    if (spp_api) {
        bt_comm_edr_sniff_clean();
        log_info("spp_api_tx(%d) \n", len);
        /* log_info_hexdump(data, len); */
        return spp_api->send_data(NULL, data, len);
    }
    return SPP_USER_ERR_SEND_FAIL;
}

int at_spp_send_data_check(u16 len)
{
    if (spp_api) {
        if (spp_api->busy_state()) {
            return 0;
        }
    }
    return 1;
}

static void at_spp_send_wakeup(void)
{
    putchar('W');
}

static void at_spp_recieve_cbk(void *priv, u8 *buf, u16 len)
{
    log_info("spp_api_rx(%d) \n", len);
    log_info_hexdump(buf, len);
    bt_comm_edr_sniff_clean();

#if TEST_SPP_DATA_RATE
    if ((buf[0] == 'A') && (buf[1] == 'F')) {
        spp_test_start = 1;//start
    } else if ((buf[0] == 'A') && (buf[1] == 'A')) {
        spp_test_start = 0;//stop
    }
#endif

    //loop send data for test
    /* if (at_spp_send_data_check(len)) { */
    /* at_spp_send_data(buf, len); */
    /* } */
    edr_at_send_event(AT_EVT_SPP_DATA_RECEIVED, buf, len);

}

static void at_spp_state_cbk(u8 state)
{
    switch (state) {
    case SPP_USER_ST_CONNECT:
        log_info("SPP_USER_ST_CONNECT ~~~\n");
        bt_edr_status |= BIT(ST_BIT_SPP_CONN);
        edr_at_send_event(AT_EVT_BT_CONNECTED, NULL, 0);
        break;

    case SPP_USER_ST_DISCONN:
        log_info("SPP_USER_ST_DISCONN ~~~\n");
        bt_edr_status &= ~BIT(ST_BIT_SPP_CONN);
        edr_at_send_event(AT_EVT_BT_DISCONNECTED, NULL, 0);
        break;

    default:
        break;
    }
}



#if TEST_SPP_DATA_RATE
static void test_spp_send_data(void)
{
    u16 send_len = 250;
    if (at_spp_send_data_check(send_len)) {
        test_data_count += send_len;
        at_spp_send_data(&test_data_count, send_len);
    }
}

static void test_timer_handler(void)
{
    static u32 t_count = 0;

    if (SPP_USER_ST_CONNECT != spp_state) {
        test_data_count = 0;
        spp_test_start = 0;
        return;
    }

    if (spp_test_start) {
        test_spp_send_data();
    }

    if (++t_count < (1000 / SPP_TIMER_MS)) {
        return;
    }
    t_count = 0;

    if (test_data_count) {
        log_info("\n-spp_data_rate: %d bps-\n", test_data_count * 8);
        test_data_count = 0;
    }
}
#endif

static void at_spp_init(void)
{
    spp_state = 0;
    bt_edr_status = 0;
    spp_get_operation_table(&spp_api);
    spp_api->regist_recieve_cbk(0, at_spp_recieve_cbk);
    spp_api->regist_state_cbk(0, at_spp_state_cbk);
    spp_api->regist_wakeup_send(NULL, at_spp_send_wakeup);

#if TEST_SPP_DATA_RATE
    spp_timer_handle  = sys_timer_add(NULL, test_timer_handler, SPP_TIMER_MS);
#endif

}

//配置流控制的参数，蓝牙协议栈初始化前调用配置
void transport_spp_flow_cfg(void)
{
}

void transport_spp_init(void)
{
    log_info("spp_file: %s", __FILE__);
    at_spp_init();
}

void transport_spp_disconnect(void)
{
    if (bt_edr_status & BIT(ST_BIT_SPP_CONN)) {
        log_info("transport_spp_disconnect\n");
        user_send_cmd_prepare(USER_CTRL_SPP_DISCONNECT, 0, NULL);
    }
}

//----------------------------------------------------------------

int edr_at_set_address(u8 *addr)
{
    bt_set_mac_addr(addr);
    lmp_hci_write_local_address(addr);
    return SPP_USER_ERR_NONE;
}

int edr_at_get_address(u8 *addr)
{
    memcpy(addr, bt_get_mac_addr(), 6);
    return SPP_USER_ERR_NONE;
}

int edr_at_set_name(u8 *name, u8 len)
{
    bt_set_local_name(name, len);
    name[len] = 0;
    lmp_hci_write_local_name(name);
    return SPP_USER_ERR_NONE;
}

int edr_at_get_name(u8 *name)
{
    u8 *tmp_name = 	bt_get_local_name();
    u8 name_len = strlen(tmp_name);
    memcpy(name, tmp_name, name_len);
    return name_len;
}

static void set_edr_connect_control_ext(u8 inquiry_en, u8 page_scan_en)
{
#if TCFG_USER_EDR_ENABLE
    if (inquiry_en) {
        user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
    } else {
        user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
    }

    if (page_scan_en) {
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
    } else {
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
    }
#endif
}


int edr_at_set_visibility(u8 inquiry_en, u8 page_scan_en)
{
    if (inquiry_en) {
        bt_edr_status |= BIT(ST_BIT_INQUIRY);
    } else {
        bt_edr_status &= ~BIT(ST_BIT_INQUIRY);
    }

    if (page_scan_en) {
        bt_edr_status |= BIT(ST_BIT_PAGE_SCAN);
    } else {
        bt_edr_status &= ~BIT(ST_BIT_PAGE_SCAN);
    }
    set_edr_connect_control_ext(inquiry_en, page_scan_en);
    return SPP_USER_ERR_NONE;
}

int edr_at_set_pair_mode(u8 mode)
{
    //to do
    switch (mode) {
    case 0://pincode
        break;

    default:
        break;
    }
    return SPP_USER_ERR_NONE;
}

int edr_at_set_pincode(u8 *pincode)
{
    //to do
    return SPP_USER_ERR_NONE;
}

int edr_at_disconnect(void)
{
    //to do
    if (bt_edr_status & BIT(ST_BIT_SPP_CONN)) {
        spp_api->disconnect(0);
    }
    return SPP_USER_ERR_NONE;
}

extern void __change_hci_class_type(u32 class);
int edr_at_set_cod(u8 *cod_data)
{
    u32 class_type;

    memcpy(&class_type, cod_data, 3);
    __change_hci_class_type(class_type);
    lmp_hci_write_class_of_device(class_type);
    return SPP_USER_ERR_NONE;
}

int edr_at_send_spp_data(u8 *data, u8 len)
{
    if (bt_edr_status & BIT(ST_BIT_SPP_CONN)) {
        if (at_spp_send_data_check(len)) {
            return spp_api->send_data(0, data, len);
        } else {
            return SPP_USER_ERR_SEND_BUFF_BUSY;
        }
    } else {
        return SPP_USER_ERR_SEND_FAIL;
    }
}

u8 edr_at_get_staus(void)
{
    return bt_edr_status;
}

void edr_at_register_event_cbk(void *cbk)
{
    at_send_event_callbak = cbk;
}

static void edr_at_send_event(u8 event_type, const u8 *packet, int size)
{
    if (at_send_event_callbak) {
        at_send_event_callbak(event_type, packet, size);
    }
}


#endif
#endif
