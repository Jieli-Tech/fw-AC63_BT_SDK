
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

#include "edr_hid_user.h"
#include "standard_hid.h"
#include "app_comm_bt.h"

#if TCFG_USER_EDR_ENABLE && (USER_SUPPORT_PROFILE_HID ==1)

#if 1
//#define log_info            y_printf
extern void put_buf(const u8 *buf, int len);
#define log_info(x, ...)  printf("[EDR_HID]" x " ", ## __VA_ARGS__)
#define log_info_hexdump  put_buf

#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

#define TEST_USER_HID_EN               0

/*message type*/   /*hex*/  /*sent by*/
#define HID_HANDSHAKE       0x00     /*Device*/
#define HID_CONTROL         0x10     /*Device&host*/
/**0x20,0x30 Reserved*/
#define HID_GET_REPORT      0x40     /*host*/
#define HID_SET_REPORT      0x50     /*host*/
#define HID_GET_PROTOCOL    0x60     /*host*/
#define HID_SET_PROTOCOL    0x70     /*host*/
#define HID_GET_IDLE        0x80     /*host DEPRECATED*/
#define HID_SET_IDLE        0x90     /*host DEPRECATED*/
#define HID_DATA            0xA0     /*Device&host*/
#define HID_DATC            0xB0     /*Device&host  DEPRECATED*/
/*C-F  Reserved*/


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
static u16 hid_channel;//inter_channel
static u16 hid_ctrl_channel;//ctrl_channel
static volatile u8 hid_run = 0;
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


typedef struct {
    u8 report_type;
    u8 report_id;
    u8 data[HID_SEND_MAX_SIZE - 2];
} hid_data_info_t;

//-----------------------------------------------------
/*************************************************************************************************/
/*!
 *  \brief      post event
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/

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

/*************************************************************************************************/
/*!
 *  \brief     参考识别手机系统
 *
 *  \param      [in]
 enum {
 REMOTE_DEV_UNKNOWN  = 0,
 REMOTE_DEV_ANDROID		,
 REMOTE_DEV_IOS			,
 };

 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void sdp_callback_remote_type(u8 remote_type)
{
    log_info("edr_hid:remote_type= %d\n", remote_type);
    edr_bt_evnet_post(SYS_BT_EVENT_FORM_COMMON, COMMON_EVENT_EDR_REMOTE_TYPE, NULL, remote_type);
    //to do
}

/*************************************************************************************************/
/*!
 *  \brief      read send data form send_buf
 *
 *  \param      [in]
 *
 *  \return     len
 *
 *  \note
 */
/*************************************************************************************************/
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

/*************************************************************************************************/
/*!
 *  \brief      try send
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
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

/*************************************************************************************************/
/*!
 *  \brief      write data to send buffer
 *
 *  \param      [in]
 *
 *  \return     len
 *
 *  \note
 */
/*************************************************************************************************/
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

/*************************************************************************************************/
/*!
 *  \brief      设置搜索显示的图标
 *
 *  \param      [in]class_type,define in avctp_user.h
 *
 *  \return
 *
 *  \note       协议栈初始化前调用
 */
/*************************************************************************************************/
void user_hid_set_icon(u32 class_type)
{
    __change_hci_class_type(class_type);//
}


/*************************************************************************************************/
/*!
 *  \brief      设置描述符
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note       协议栈初始化前调用
 */
/*************************************************************************************************/
void user_hid_set_ReportMap(u8 *map, u16 size)
{
    report_map = map;
    report_map_size = size;

    if (hid_run) {
        hid_sdp_init(HID_REPORT_MAP_DATA, HID_REPORT_MAP_SIZE);
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
/*************************************************************************************************/
/*!
 *  \brief      timer
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
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

/*************************************************************************************************/
/*!
 *  \brief      check is idle, enter sleep
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static u8 user_hid_idle_query(void)
{
    return !is_hid_active;
}

REGISTER_LP_TARGET(hid_user_target) = {
    .name = "hid_user",
    .is_idle = user_hid_idle_query,
};

/*************************************************************************************************/
/*!
 *  \brief      协议栈发送hid数据包完成 回调
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note       可用于唤醒及时往协议栈发送数据操作
 */
/*************************************************************************************************/
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

/*************************************************************************************************/
/*!
 *  \brief      注册唤醒发送回调
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void user_hid_regiser_wakeup_send(void *cbk)
{
    user_hid_send_wakeup = cbk;
}

/*************************************************************************************************/
/*!
 *  \brief      disconnect
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void user_hid_disconnect(void)
{
    if (hid_channel) {
        user_send_cmd_prepare(USER_CTRL_HID_DISCONNECT, 0, NULL);
    }
}

/*************************************************************************************************/
/*!
 *  \brief      hid channel,send data
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
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

/*************************************************************************************************/
/*!
 *  \brief      hid event handle
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void user_hid_msg_handler(u32 msg, u8 *packet, u32 packet_size)
{
    u16 tmp_chl;
    switch (msg) {
    case 1: {
        hid_ctrl_channel = little_endian_read_16(packet, 0); //ctrl_channel
        hid_channel = little_endian_read_16(packet, 2); //inter_channel
        bt_send_busy = 0;
        log_info("hid connect ########################,%d,%d\n", hid_ctrl_channel, hid_channel);
        cbuf_init(&user_send_cbuf, hid_tmp_buffer, HID_TMP_BUFSIZE);
    }
    break;

    case 2:
        log_info("hid disconnect ########################\n");
        hid_channel = 0;
        hid_ctrl_channel = 0;
        cbuf_init(&user_send_cbuf, hid_tmp_buffer, HID_TMP_BUFSIZE);
        break;

    case 3:
        if (hid_channel == little_endian_read_16(packet, 0) ||
            hid_ctrl_channel == little_endian_read_16(packet, 0)) {
            user_hid_send_ok_callback();
        }
        break;

    default:
        break;
    }
}

/*************************************************************************************************/
/*!
 *  \brief      init
 *
 *  \param      [in] output_handle
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void user_hid_init(void (*user_hid_output_handler)(u8 *packet, u16 size, u16 channel))
{
    log_info("%s\n", __FUNCTION__);
    hid_channel = 0;
    hid_ctrl_channel = 0;
    hid_diy_regiest_callback(user_hid_msg_handler, user_hid_output_handler);

    if (!hid_run) {
        log_info("hid_sdp_init\n");
        hid_sdp_init(HID_REPORT_MAP_DATA, HID_REPORT_MAP_SIZE);

#if TEST_USER_HID_EN
        if (!hid_timer_id) {
            hid_timer_id = sys_s_hi_timer_add((void *)0, user_hid_timer_handler, 5000);
        }
#endif
        cbuf_init(&user_send_cbuf, hid_tmp_buffer, HID_TMP_BUFSIZE);
        hid_run = 1;
    }
}

/*************************************************************************************************/
/*!
 *  \brief      exit
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void user_hid_exit(void)
{
    log_info("%s\n", __FUNCTION__);

    if (hid_run) {
        log_info("hid free\n");
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
}

/*************************************************************************************************/
/*!
 *  \brief      module_enable
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void user_hid_enable(u8 en)
{
    log_info("%s:%d\n", __FUNCTION__, en);

    if (en) {
        ;
    } else {
        user_hid_disconnect();
    }
}

/*************************************************************************************************/
/*!
 *  \brief      hid send report data
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
int edr_hid_data_send_ext(u8 report_type, u8 report_id, u8 *data, u16 len)
{
    if (!hid_channel) {
        return 1;
    }

    if (len > HID_SEND_MAX_SIZE - 2) {
        log_info("buffer limitp!!!\n");
        return 2;
    }

    bt_comm_edr_sniff_clean();

    putchar('@');
    hid_data_info_t data_info;
    data_info.report_type = report_type;
    data_info.report_id = report_id;
    memcpy(data_info.data, data, len);

    if (!user_data_write_sub((u8 *)&data_info, 2 + len)) {
        log_info("hid buffer full!!!\n");
        return 3;
    }

    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      hid send report data
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
int edr_hid_data_send(u8 report_id, u8 *data, u16 len)
{
    return edr_hid_data_send_ext(HID_DATA | DATA_INPUT, report_id, data, len);
}

/*************************************************************************************************/
/*!
 *  \brief       send test key demo
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void edr_hid_key_deal_test(u16 key_msg)
{
    //default report_id 1
    edr_hid_data_send(1, (u8 *)&key_msg, 2);

    if (key_msg) {
        key_msg = 0;
        edr_hid_data_send(1, (u8 *)&key_msg, 2);
    }
}

/*************************************************************************************************/
/*!
 *  \brief      check connect
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
int edr_hid_is_connected(void)
{
    return hid_channel;
}

/*************************************************************************************************/
/*!
 *  \brief      check btstack send busy
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
int edr_hid_is_busy(void)
{
    return bt_send_busy;
}

/*************************************************************************************************/
/*!
 *  \brief      check tx buffer
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
extern u32 lmp_private_get_tx_remain_buffer();
int edr_hid_tx_buff_is_ok(void)
{
    if (lmp_private_get_tx_remain_buffer() < 0x300) {
        putchar('b');
        return 0;
    }
    return 1;
}

/*************************************************************************************************/
/*!
 *  \brief     hook，处理ctrl通道命令，返回0则由库来处理。非0执行回复命令发送
 *  \param      [in]channel l2cap连接通道
 *  \param      [in]packet  接收命令buffer
 *  \param      [in]size    命令长度
 *  \param      [out] respond_buf_ptr 回复的buf指针
 *  \return     回复命令的长度
 *  \note
 */
/*************************************************************************************************/
int hid_ctrl_data_parse_hook(u16 channel, const u8 *packet, int size, int *respond_buf_ptr)
{
    int respone_len = 0;
    static u8 hid_respone_buffer[8];
    switch (packet[0] & 0xf0) {
    case HID_GET_REPORT:
        if (packet[1] == 0x01) {//report id,应答是对应app_keyboard.c的描述符
            hid_respone_buffer[0] = HID_DATA | DATA_INPUT;
            //report id  + payload;根据描述符来回复长度
            hid_respone_buffer[1] = packet[1];
            hid_respone_buffer[2] = 0;
            hid_respone_buffer[3] = 0;
            respone_len = 4;
        }
        break;
    default:
        break;
    }
    if (respone_len) {
        *respond_buf_ptr = hid_respone_buffer;
    }
    return respone_len;
}



#endif
