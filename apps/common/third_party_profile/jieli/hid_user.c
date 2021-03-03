
#include "app_config.h"
#include "app_action.h"

#include "system/includes.h"
#include "spp_user.h"
#include "string.h"
#include "circular_buf.h"
#include "spp_trans_data.h"
#include "bt_common.h"
#include "btstack/avctp_user.h"
#include "circular_buf.h"

#include "hid_user.h"
#include "standard_hid.h"
#if (USER_SUPPORT_PROFILE_HID ==1)

#define log_info printf

#define HID_CHANGE_DESCRIPTOR          0
#define TEST_USER_HID_EN               0

#define HID_DATA            0xA0     /*Device&host*/
#define HID_DATC            0xB0     /*Device&host  DEPRECATED*/

/*DATA*/
#define DATA_OTHER       0x00
#define DATA_INPUT       0x01
#define DATA_OUTPUT      0x02
#define DATA_FEATURE     0x03

static  u8 *report_map;
static  u16 report_map_size;
#define HID_REPORT_MAP_DATA    report_map
#define HID_REPORT_MAP_SIZE    report_map_size

/* #define MOUSE_REPORT_TYPE   0xA1 */
/* #define MOUSE_REPORT_ID     1 */

static void (*user_hid_send_wakeup)(void) = NULL;
static u16 hid_channel;
static u8  hid_run = 0;
static volatile u8 is_hid_active = 0;
static volatile u8 hid_s_step = 0;
/* static int hid_timer_id = 0; */
int hid_timer_id = 0;


int edr_hid_timer_handle = 0;

#define HID_SEND_MAX_SIZE   (16) //描述符数据包的长度
/* static u8  edr_hid_one_packet[HID_SEND_MAX_SIZE]; */
/* static volatile u16 edr_send_packet_len = 0; */
static volatile u8  bt_send_busy = 0;
void (*user_led_status_callback)(u8 *buffer, u16 size) = NULL;

#define HID_TMP_BUFSIZE  (64*2)
#define cbuf_get_space(a) (a)->total_len
static cbuffer_t user_send_cbuf;
static u8 hid_tmp_buffer[HID_TMP_BUFSIZE];

extern void hid_diy_regiest_callback(void *cb, void *interrupt_cb);
extern void hid_sdp_init(const u8 *hid_descriptor, u16 size);
extern uint16_t little_endian_read_16(const uint8_t *buffer, int pos);

extern void put_buf(const u8 *buf, int len);
void edr_hid_data_send(u8 report_id, u8 *data, u16 len);

/* typedef struct { */
/* u8 report_type; */
/* u8 report_id; */
/* u8 button; */
/* s8 x; */
/* s8 y; */
/* } hid_ctl_info_t; */

typedef struct {
    u8 report_type;
    u8 report_id;
    u8 data[HID_SEND_MAX_SIZE - 2];
} hid_data_info_t;

//-----------------------------------------------------
static void edr_bt_evnet_post(u32 arg_type, u8 priv_event, u8 *args, u32 value)
{
    struct sys_event e;
    e.type = SYS_BT_EVENT;
    e.arg  = (void *)arg_type;
    e.u.bt.event = priv_event;
    if (args) {
        memcpy(e.u.bt.args, args, 7);
    }
    e.u.bt.value = value;
    sys_event_notify(&e);
}

/* enum { */
/* REMOTE_DEV_UNKNOWN  = 0, */
/* REMOTE_DEV_ANDROID		, */
/* REMOTE_DEV_IOS			, */
/* }; */
//参考识别手机系统
void sdp_callback_remote_type(u8 remote_type)
{
    log_info("edr_hid:remote_type= %d\n", remote_type);
    edr_bt_evnet_post(SYS_BT_EVENT_FORM_COMMON, COMMON_EVENT_EDR_REMOTE_TYPE, NULL, remote_type);
    //to do
}

//-----------------------------------------------------
static u16 user_data_read_sub(u8 *buf, u16 buf_size)
{
    u16 ret_len;
    if (0 == cbuf_get_data_size(&user_send_cbuf)) {
        return 0;
    }

    OS_ENTER_CRITICAL();
    cbuf_read(&user_send_cbuf, &ret_len, 2);
    if (ret_len && ret_len <= buf_size) {
        cbuf_read(&user_send_cbuf, buf, ret_len);
    }
    OS_EXIT_CRITICAL();

    return ret_len;
}

static void user_data_try_send(void)
{
    u16 send_len;

    if (bt_send_busy) {
        return;
    }
    bt_send_busy = 1;//hold

    u8 tmp_send_buf[HID_SEND_MAX_SIZE];

    send_len = user_data_read_sub(tmp_send_buf, HID_SEND_MAX_SIZE);

    if (send_len) {
        if (user_hid_send_data(tmp_send_buf, send_len)) {
            bt_send_busy = 0;
        }
    } else {
        //not send
        bt_send_busy = 0;
    }
}


static u32 user_data_write_sub(u8 *data, u16 len)
{
    u16 wlen = 0;

    u16 buf_space = cbuf_get_space(&user_send_cbuf) - cbuf_get_data_size(&user_send_cbuf);
    if (len + 2 > buf_space) {
        return 0;
    }

    OS_ENTER_CRITICAL();
    wlen = cbuf_write(&user_send_cbuf, &len, 2);
    wlen += cbuf_write(&user_send_cbuf, data, len);
    OS_EXIT_CRITICAL();

    user_data_try_send();
    return wlen;
}


//-----------------------------------------------------
void user_hid_set_icon(u32 class_type)
{
    __change_hci_class_type(class_type);//
}

void user_hid_set_ReportMap(u8 *map, u16 size)
{
    report_map = map;
    report_map_size = size;

    if (hid_run) {
#if !HID_CHANGE_DESCRIPTOR
        hid_sdp_init(HID_REPORT_MAP_DATA, HID_REPORT_MAP_SIZE);
#endif
    }

}



/* const hid_ctl_info_t test_key[5] = { */
/* {0xA1, 1, 0, 0, 0}, */
/* {0xA1, 1, 0, 100, 0}, */
/* {0xA1, 1, 0, -100, 0}, */
/* }; */

/* static void test_hid_send_step(void) */
/* { */
/* static u8 xy_step = 0; */
/* u8 send_len = 5; */
/* if (!hid_s_step) { */
/* return; */
/* } */

/* xy_step = !xy_step; */

/* if (xy_step) { */
/* hid_s_step = 1; */
/* } else { */
/* hid_s_step = 2; */
/* } */

/* if (0 == user_hid_send_data((u8 *)&test_key[hid_s_step], send_len)) { */
/* hid_s_step = 0; */
/* } */
/* } */

static void user_hid_timer_handler(void *para)
{
    /* static u8 count = 0; */

    if (!hid_channel) {
        return;
    }

    /* if (++count > 5) { */
    /* count = 0; */
    /* hid_s_step = 1; */
    /* test_hid_send_step(); */
    /* } */

    /* hid_key_send(CONSUMER_PLAY_PAUSE); */
}

static u8 user_hid_idle_query(void)
{
    return !is_hid_active;
}

REGISTER_LP_TARGET(hid_user_target) = {
    .name = "hid_user",
    .is_idle = user_hid_idle_query,
};


static void user_hid_send_ok_callback(void)
{
    /* putchar('K'); */
    bt_send_busy = 0;

    if (user_hid_send_wakeup) {
        user_hid_send_wakeup();
    }

    user_data_try_send();

    /* test_hid_send_step(); */
}

void user_hid_regiser_wakeup_send(void *cbk)
{
    user_hid_send_wakeup = cbk;
}

void user_hid_disconnect(void)
{
    if (hid_channel) {
        user_send_cmd_prepare(USER_CTRL_HID_DISCONNECT, 0, NULL);
    }
}


int user_hid_send_data(u8 *buf, u32 len)
{
    int ret;
    hid_s_param_t s_par;
    if (!hid_channel) {
        return -1;
    }

    s_par.chl_id = hid_channel;
    s_par.data_len = len;
    s_par.data_ptr = buf;

    ret = user_send_cmd_prepare(USER_CTRL_HID_SEND_DATA, sizeof(hid_s_param_t), (u8 *)&s_par);

    if (ret) {
        log_info("hid send fail!!! %d\n", ret);
    }
    return ret;
}

static void user_hid_msg_handler(u32 msg, u8 *packet, u32 packet_size)
{
    u16 tmp_chl;
    switch (msg) {
    case 1:
        hid_channel = little_endian_read_16(packet, 2); //inter_channel
        bt_send_busy = 0;
        log_info("hid connect ########################,%d\n", hid_channel);
        cbuf_init(&user_send_cbuf, hid_tmp_buffer, HID_TMP_BUFSIZE);
        break;

    case 2:
        log_info("hid disconnect ########################\n");
        hid_channel = 0;
        cbuf_init(&user_send_cbuf, hid_tmp_buffer, HID_TMP_BUFSIZE);
        break;

    case 3:
        if (hid_channel == little_endian_read_16(packet, 0)) {
            user_hid_send_ok_callback();
        }
        break;

    default:
        break;
    }
}

void user_hid_init(void (*user_hid_interrupt_handler)(u8 *packet, u16 size))
{
    log_info("%s\n", __FUNCTION__);
    hid_channel = 0;
    hid_diy_regiest_callback(user_hid_msg_handler, user_hid_interrupt_handler);

#if !HID_CHANGE_DESCRIPTOR
    hid_sdp_init(HID_REPORT_MAP_DATA, HID_REPORT_MAP_SIZE);
#endif
#if TEST_USER_HID_EN
    hid_timer_id = sys_s_hi_timer_add((void *)0, user_hid_timer_handler, 5000);
#endif

    cbuf_init(&user_send_cbuf, hid_tmp_buffer, HID_TMP_BUFSIZE);
    hid_run = 1;
}

void user_hid_exit(void)
{
    log_info("%s\n", __FUNCTION__);
    if (hid_timer_id) {
        user_hid_disconnect();
        sys_s_hi_timer_del(hid_timer_id);
        hid_timer_id = 0;
    }

    if (edr_hid_timer_handle) {
        sys_s_hi_timer_del(edr_hid_timer_handle);
        edr_hid_timer_handle = 0;
    }

    hid_run = 0;
}

void user_hid_enable(u8 en)
{
    log_info("%s:%d\n", __FUNCTION__, en);

    if (en) {
        ;
    } else {
        user_hid_disconnect();
    }
}

void edr_hid_data_send(u8 report_id, u8 *data, u16 len)
{
    if (!hid_channel) {
        return;
    }

    if (len > HID_SEND_MAX_SIZE - 2) {
        log_info("buffer limitp!!!\n");
        return;
    }

    putchar('@');
    hid_data_info_t data_info;
    data_info.report_type = HID_DATA | DATA_INPUT;
    data_info.report_id = report_id;
    memcpy(data_info.data, data, len);

    if (!user_data_write_sub((u8 *)&data_info, 2 + len)) {
        log_info("hid buffer full!!!\n");
        return;
    }

}

void edr_hid_key_deal_test(u16 key_msg)
{
    edr_hid_data_send(1, (u8 *)&key_msg, 2);

    if (key_msg) {
        key_msg = 0;
        edr_hid_data_send(1, (u8 *)&key_msg, 2);
    }
}


int edr_hid_is_connected(void)
{
    return hid_channel;
}

extern u32 lmp_private_get_tx_remain_buffer();
int edr_hid_tx_buff_is_ok(void)
{
    if (lmp_private_get_tx_remain_buffer() < 0x300) {
        putchar('b');
        return 0;
    }
    return 1;
}

#if HID_CHANGE_DESCRIPTOR
const u8 use_hid_descriptor[] = {
    0x05, 0x01,
    0x09, 0x02,
    0xA1, 0x01,
    0x85, 0x01,
    0x05, 0x09,
    0x19, 0x01,
    0x29, 0x03,
    0x15, 0x00,
    0x25, 0x01,
    0x95, 0x03,
    0x75, 0x01,
    0x81, 0x02,
    0x75, 0x05,
    0x95, 0x01,
    0x81, 0x03,
    0x05, 0x01,
    0x09, 0x01,
    0xA1, 0x00,
    0x09, 0x30,
    0x09, 0x31,
    0x16, 0x00, 0xD8,
    0x26, 0x00, 0x28,
    0x75, 0x10,
    0x95, 0x02,
    0x81, 0x06,
    0xC0,
    0xC0,

    0x05, 0x0C,
    0x09, 0x01,
    0xA1, 0x01,
    0x85, 0x02,
    0x15, 0x00,
    0x25, 0x01,
    0x09, 0x34,
    0x09, 0x40,
    0x0A, 0x23, 0x02,
    0x0A, 0x24, 0x02,
    0x09, 0xE9,
    0x09, 0xEA,
    0x09, 0xB0,
    0x09, 0xB1,
    0x09, 0xB3,
    0x09, 0xB4,
    0x09, 0xB5,
    0x09, 0xB6,
    0x09, 0xB7,
    0x09, 0xCD,
    0x75, 0x01,
    0x95, 0x0E,
    0x81, 0x22,
    0x75, 0x01,
    0x95, 0x02,
    0x81, 0x02,
    0xC0
};

struct user_hid_consumer_cmd {
    //Bluetooth HID Protocol Message Header Octet
    u8 HIDP_Hdr;
    //Bluetooth HID Boot Reports
    u8 report_id;
    u8 button;
} _GNU_PACKED_;
static struct user_hid_consumer_cmd u_consumer = {
    .HIDP_Hdr = 0xA1,
    .report_id = 0x02,
    .button = 0,
};
// consumer key
#define CONSUMER_MENU               0x01
#define CONSUMER_MENU_ESCAPE        0x02
#define CONSUMER_AC_HOME            0x04
void hid_consumer_send_test(u8 menu)
{
    if (menu == 1) {
        u_consumer.button = CONSUMER_MENU;
    }
    if (menu == 2) {
        u_consumer.button = CONSUMER_MENU_ESCAPE;
    }
    if (menu == 3) {
        u_consumer.button = CONSUMER_AC_HOME;
    }
    put_buf((u8 *)&u_consumer, sizeof(u_consumer));
    user_data_write_sub((u8 *)&u_consumer, sizeof(u_consumer));
    u_consumer.button = 0x00;
    user_data_write_sub((u8 *)&u_consumer, sizeof(u_consumer));
}

u8 sdp_make_hid_service_data[0x200];
static void user_hid_sdp_init(const u8 *hid_descriptor, u16 size)
{
    int real_size;
    real_size = sdp_create_diy_hid_service(sdp_make_hid_service_data, sizeof(sdp_make_hid_service_data), hid_descriptor, size);
    printf("dy_hid_service(%d):", real_size);
}
void user_hid_descriptor_init(void)
{

    //__change_hci_class_type(0x240418);  /*需要更换手机显示图标的可以自己改成以前的值*/
    user_hid_init();
    user_hid_sdp_init(use_hid_descriptor, sizeof(use_hid_descriptor));
}

//用户修改成自定义的描述符说明
//1、在void bredr_handle_register();中调用user_hid_descriptor_init;
//2、user_hid_sdp_init换成自己的表
//3、文件上方HID_CHANGE_DESCRIPTOR定义为1
//4、在bt_profile_config.c文件中extern sdp_make_hid_service_data数组，
//  把sdp_hid_service_data替换为sdp_make_hid_service_data.
//5、把const u8 hid_conn_depend_on_dev_company的值置0;
#endif

#endif
