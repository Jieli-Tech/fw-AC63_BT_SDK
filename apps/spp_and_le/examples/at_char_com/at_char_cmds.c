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
#include "at.h"
#include "le/ble_api.h"
#include "le_common.h"
#include "ble_at_char_com.h"
#include "ble_at_char_client.h"
#include "app_power_manage.h"

#define LOG_TAG_CONST       AT_CHAR_COM
/* #define LOG_TAG             "[AT_CHAR_CMD]" */
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

/* #undef log_info */
/* #undef log_error */
/* #define log_info(x, ...)    r_printf("[AT_CHAR_CMD]" x " ", ## __VA_ARGS__) */
/* #define log_error log_info */

#if  CONFIG_APP_AT_CHAR_COM


static struct at_layer {
    void *config;     //ci transport config
    u8 *pAT_buffer;   //ci data path memory
    // transport component with configuration
    ci_transport_t *dev_transport;

};

#define AT_BUFFER_SIZE      0x100

#ifdef HAVE_MALLOC
static struct at_layer *hdl;

#define __this      (hdl)
#else
static struct at_layer hdl;

#define __this      (&hdl)

static u8 pAT_buffer_static[AT_BUFFER_SIZE];   //ci data path memory
#endif

/* static u8 respond_buffer_static[32];   //ci data path memory */

#define PARSE_BUFFER_SIZE      0x100

static u8 parse_buffer[PARSE_BUFFER_SIZE] __attribute__((aligned(4)));

static u8 test_tcp_datasend = 0;
/* static u8 sprintf_buffer[64]__attribute__((aligned(4)));   //ci data path memory */

static u8 BT_UART_BUF[BT_UART_FIFIO_BUFFER_SIZE];
cbuffer_t bt_to_uart_cbuf;

int le_at_client_creat_cannel(void);
void at_set_low_power_mode(u8 enable);
u8 at_get_low_power_mode(void);
/* void at_set_soft_poweroff(void); */
extern void atchar_power_event_to_user(u8 event);
//------------------------------------------



//默认参数
#define G_VERSION "JL_test"
#define CONFIG_VERSION "2021_02_04"
char const device_name_default[] = "JL_device";
u16 adv_interval_default = 2048;
u32 uart_baud = 115200;
extern int ct_uart_init(u32 baud);
//0-7:主机通道(主动连出通道); 8:从机通道; 9:AT指令通道
u8 cur_atcom_cid = 9;

//------------------------------------------

typedef struct {
    u16 str_id;
    u16 str_len;
    const char *str;
} str_info_t;

enum {
    STR_ID_NULL = 0,
    STR_ID_HEAD_AT_CMD,
    STR_ID_HEAD_AT_CHL,

    STR_ID_OK = 0x10,
    STR_ID_ERROR,

    STR_ID_GVER = 0x20,
    STR_ID_GCFGVER,
    STR_ID_NAME,
    STR_ID_LBDADDR,
    STR_ID_BAUD,

    STR_ID_ADV,
    STR_ID_ADVPARAM,
    STR_ID_ADVDATA,
    STR_ID_SRDATA,
    STR_ID_CONNPARAM,

    STR_ID_SCAN,
    STR_ID_TARGETUUID,
    STR_ID_CONN,
    STR_ID_DISC,
    STR_ID_OTA,
    STR_ID_CONN_CANNEL,
    STR_ID_POWER_OFF,
    STR_ID_LOW_POWER,
    //    STR_ID_,
//    STR_ID_,
};



static const char at_head_at_cmd[]     = "AT+";
static const char at_head_at_chl[]     = "AT>";
static const char at_str_enter[]       = "\r\n";
static const char at_str_ok[]          = "OK";
static const char at_str_err[]         = "ERR";

static const char at_str_gver[]        = "GVER";
static const char at_str_gcfgver[]     = "GCFGVER";
static const char at_str_name[]        = "NAME";
static const char at_str_lbdaddr[]     = "LBDADDR";
static const char at_str_baud[]        = "BAUD";

static const char at_str_adv[]         = "ADV";
static const char at_str_advparam[]    = "ADVPARAM";
static const char at_str_advdata[]     = "ADVDATA";
static const char at_str_srdata[]      = "SRDATA";
static const char at_str_connparam[]   = "CONNPARAM";

static const char at_str_scan[]        = "SCAN";
static const char at_str_targetuuid[]  = "TARGETUUID";
static const char at_str_conn[]        = "CONN";
static const char at_str_disc[]        = "DISC";
static const char at_str_ota[]         = "OTA";
static const char at_str_conn_cannel[] = "CONN_CANNEL";
static const char at_str_power_off[]   = "POWEROFF";
static const char at_str_lowpower[]    = "LOWPOWER";


//static const char at_str_[]  = "";
//static const char at_str_[]  = "";
//static const char at_str_[]  = "";
static const char specialchar[]        = {'+', '>', '=', '?', '\r', ','};

enum {
    AT_CMD_OPT_NULL = 0,
    AT_CMD_OPT_SET, //设置
    AT_CMD_OPT_GET, //查询
};

#define INPUT_STR_INFO(id,string)  {.str_id = id, .str = string, .str_len = sizeof(string)-1,}

static const str_info_t at_head_str_table[] = {
    INPUT_STR_INFO(STR_ID_HEAD_AT_CMD, at_head_at_cmd),
    INPUT_STR_INFO(STR_ID_HEAD_AT_CHL, at_head_at_chl),
};

static const str_info_t at_cmd_str_table[] = {
    INPUT_STR_INFO(STR_ID_GVER, at_str_gver),
    INPUT_STR_INFO(STR_ID_GCFGVER, at_str_gcfgver),
    INPUT_STR_INFO(STR_ID_NAME, at_str_name),
    INPUT_STR_INFO(STR_ID_LBDADDR, at_str_lbdaddr),
    INPUT_STR_INFO(STR_ID_BAUD, at_str_baud),

    INPUT_STR_INFO(STR_ID_ADV, at_str_adv),
    INPUT_STR_INFO(STR_ID_ADVPARAM, at_str_advparam),
    INPUT_STR_INFO(STR_ID_ADVDATA, at_str_advdata),
    INPUT_STR_INFO(STR_ID_SRDATA, at_str_srdata),
    INPUT_STR_INFO(STR_ID_CONNPARAM, at_str_connparam),

    INPUT_STR_INFO(STR_ID_SCAN, at_str_scan),
    INPUT_STR_INFO(STR_ID_TARGETUUID, at_str_targetuuid),
    INPUT_STR_INFO(STR_ID_CONN, at_str_conn),
    INPUT_STR_INFO(STR_ID_DISC, at_str_disc),
    INPUT_STR_INFO(STR_ID_OTA, at_str_ota),
    INPUT_STR_INFO(STR_ID_CONN_CANNEL, at_str_conn_cannel),
    INPUT_STR_INFO(STR_ID_POWER_OFF, at_str_power_off),
    INPUT_STR_INFO(STR_ID_LOW_POWER, at_str_lowpower),

//    INPUT_STR_INFO(, ),
//    INPUT_STR_INFO(, ),
};

//------------------------------------------

#define AT_STRING_SEND(a) at_cmd_send(a,strlen(a))


#define AT_PARAM_NEXT_P(a) (at_param_t*)&parse_buffer[a->next_offset]
//------------------------------------------
static u16 at_str_length(const u8 *packet, u8 end_char)
{
    u16 len = 0;
    while (*packet++ != end_char) {
        len++;
    }
    return len;
}

typedef struct {
    volatile u8 len;//长度不包含结束符
    u8 next_offset;
    u8 data[0];     //带结束符0
} at_param_t;

static at_param_t *parse_param_split(const u8 *packet, u8 split_char, u8 end_char)
{
    u8 char1;
    int i = 0;
    at_param_t *par = parse_buffer;

    if (*packet == end_char) {
        return NULL;
    }

    log_info("%s:%s", __FUNCTION__, packet);

    par->len = 0;

    while (1) {
        char1 = packet[i++];
        if (char1 == end_char) {
            par->data[par->len] = 0;
            par->next_offset = 0;
            break;
        } else if (char1 == split_char) {
            par->data[par->len] = 0;
            par->len++;
            par->next_offset = &par->data[par->len] - parse_buffer;

            //init next par
            par = &par->data[par->len];
            par->len = 0;
        } else {
            par->data[par->len++] = char1;
        }

        if (&par->data[par->len] - parse_buffer >= PARSE_BUFFER_SIZE) {
            log_error("parse_buffer over");
            par->next_offset = 0;
            break;
        }
    }

#if 0
    par = parse_buffer;
    log_info("param_split:");
    while (par) {
        log_info("len=%d,par:%s", par->len, par->data);
        if (par->next_offset) {
            par = AT_PARAM_NEXT_P(par);
        } else {
            break;
        }
    }
#endif

    return (void *)parse_buffer;
}

//返回实际应该比较的长度
static u8 compara_specialchar(u8 *packet)
{
    int i = 0, j = 0;

    while (1) {
        for (j = 0; j < sizeof(specialchar); j++) {
            if (packet[i] == specialchar[j]) {
                if (j < 2) { //如果是+或>则应返回3
                    return 3;
                } else {
                    return i;
                }
            }
        }
        i++;
    }
}

static str_info_t *at_check_match_string(u8 *packet, u16 size, char *str_table, int table_size)
{
    int max_count = table_size / sizeof(str_info_t);
    str_info_t *str_p = str_table;
    u8 compara_len = 0;

    compara_len = compara_specialchar(packet);

    while (max_count--) {
        if (str_p->str_len <= size) {

            if (str_p->str_len == compara_len) {
                if (0 == memcmp(packet, str_p->str, str_p->str_len)) {
                    return str_p;
                }
            }
        }
        str_p++;
    }
    return NULL;
}

static uint8_t key_str_to_hex(uint8_t str_val)
{
    uint8_t key_value = str_val;

    if ((key_value >= '0') && (key_value <= '9')) {
        key_value = key_value - '0';
    } else if ((key_value >= 'a') && (key_value <= 'f')) {
        key_value = key_value - 'a' + 0x0a;
    } else if ((key_value >= 'A') && (key_value <= 'F')) {
        key_value = key_value - 'A' + 0x0a;
    } else {
        key_value = 0;
    }
    return key_value;
}

//字符转换HEX BUF, 00AABBCD -> 0x00,0xaa,0xbb,0xcd,返回长度
//static uint16_t change_str_to_hex(uint8_t *str, uint16_t len,uint8_t *hex_buf)
//{
//    uint8_t value_h, value_l;
//    uint16_t cnt, s_cn;
//
//    s_cn = 0;
//    for (cnt = 0; cnt < len; cnt+= 2)
//    {
//        if(str[cnt] == 0)
//        {
//            break;
//        }
//
//        value_h = key_str_to_hex(str[cnt++]);
//        value_l = key_str_to_hex(str[cnt]);
//
//        str[s_cn] = (value_h << 4) + value_l;
//        s_cn++;
//    }
//    return s_cn;
//}


static uint8_t key_hex_to_char(uint8_t hex_val)
{
    uint8_t key_value = hex_val;

    if (key_value <= 9) {
        key_value = key_value + '0';
    } else if ((key_value >= 0x0a) && (key_value <= 0xf)) {
        key_value = key_value + 'A';
    } else {
        key_value = '0';
    }
    return key_value;
}
//hex 转换字符串,加结束符,返回长度不包含结束符
//len是输入数组的长度
//static uint16_t func_hex_to_str(uint8_t *hex_buf, uint16_t len,uint8_t *output_str)
//{
//    uint8_t value_h, value_l;
//    uint16_t cnt, s_cn;
//
//    s_cn = 0;
//    for (cnt = 0; cnt < len; cnt++)
//    {
//        output_str[s_cn++] = key_hex_to_char(hex_buf[cnt]>>4);
//        output_str[s_cn++] = key_hex_to_char(hex_buf[cnt]&0x0f);
//    }
//    output_str[s_cn] = 0;
//    return s_cn;
//}


void at_cmd_send(const u8 *packet, int size)
{
    log_info("###at_cmd_send(%d):", size);
    // put_buf(packet, size);
    at_send_uart_data(at_str_enter, 2, 0);
    at_send_uart_data(packet, size, 0);
    at_send_uart_data(at_str_enter, 2, 1);

}

void at_cmd_send_no_end(const u8 *packet, int size)
{
    at_send_uart_data(at_str_enter, 2, 0);
    at_send_uart_data(packet, size, 1);
}

/* static void at_ack_data_input(u16 cnt) */
/* { */
/* u8 tmp[32]; */
/* sprintf(tmp, "Recv %d bytes\r\n", cnt); */
/* at_send_uart_data(tmp, at_str_length(tmp, '\n') + 1,1); */
/* } */

/* static void at_send_data_output(u8 *data, u16 cnt) */
/* { */
/* u8 tmp[32]; */
/* sprintf(tmp, "+IPD,%d:\r\n", cnt); */
/* at_send_uart_data(tmp, at_str_length(tmp, '\r'),0); */
/* at_send_uart_data(data, cnt,0); */
/* at_send_uart_data("\r\n", 2,1); */
/* } */

u32 hex_2_str(u8 *hex, u32 hex_len, u8 *str)
{
    u32 str_len = 0;
    for (u32 i = 0; i < hex_len; i++) { //hex to string
        if (hex[i] < 0x10) {
            sprintf(str + i * 2, "0%x", hex[i]);
        } else {
            sprintf(str + i * 2, "%x", hex[i]);
        }
    }
    str_len = hex_len * 2;
    return str_len;
}

u32 str_2_hex(u8 *str, u32 str_len, u8 *hex)
{
    u32 hex_len = 0;
    u32 i = 0;
    for (i = 0; i < (str_len / 2); i++) {
        hex[i] = key_str_to_hex(str[i * 2]);
        hex[i] <<= 4;
        hex[i] += key_str_to_hex(str[i * 2 + 1]);
        log_info("hex----> %x", hex[i]);
    }
    hex_len = str_len / 2;
    return hex_len;
}


//字符转换十进制，返回值,
static u32 func_char_to_dec(u8 *char_buf, u8 end_char)
{
    u32 cnt = 0;
    u32 value = 0;
    u32 negative_flag = 0;
    u8 val;

    while (char_buf[cnt] != end_char) {
        val = char_buf[cnt];
        cnt++;

        if (val == ' ') {
            continue;
        } else if (val == '-') {
            negative_flag = 1;
        } else {
            if ((val >= '0') && (val <= '9')) {
                value = value * 10 + (val - '0');
            }
        }
    }

    if (value && negative_flag) {
        value = value * (-1);
    }
    return value;
}

//------------------------------------------
void at_respond_send_err(u32 err_id)
{
    u8 buf[32];
    sprintf(buf, "ERR:%d", err_id);
    AT_STRING_SEND(buf);
}
//------------------------------------------
extern u8 connect_last_device_from_vm();
extern const char at_change_channel_cmd[];

static void at_packet_handler(u8 *packet, int size)
{
    at_param_t *par;
    str_info_t *str_p;
    int ret = -1;
    u8 operator_type = AT_CMD_OPT_NULL; //
    u8 *parse_pt = packet;
    int parse_size = size;
    u8 buf[128] = {0};

#if FLOW_CONTROL
    if (cur_atcom_cid < 8) {
#if CONFIG_BT_GATT_CLIENT_NUM
        log_info("###le_client_data(%d):", size);
        /* put_buf(packet, size); */
        do {
            ret = le_att_client_send_data(cur_atcom_cid, packet, size);
            os_time_dly(1);
        } while (ret != 0);
#endif
        return;
    } else if (cur_atcom_cid == 8) {
        log_info("###le_server_data(%d):", size);
        /* put_buf(packet, size); */
        do {
            ret = le_att_server_send_data(cur_atcom_cid, packet, size);
            os_time_dly(1);
        } while (ret != 0);
        return;
    }
#else
    if (cur_atcom_cid < 8) {
#if CONFIG_BT_GATT_CLIENT_NUM
        log_info("###le_client_data(%d):", size);
        /* put_buf(packet, size); */
        le_att_client_send_data(cur_atcom_cid, packet, size);
#endif
        return;
    } else if (cur_atcom_cid == 8) {
        log_info("###le_server_data(%d):", size);
        /* put_buf(packet, size); */
        le_att_server_send_data(cur_atcom_cid, packet, size);
        return;
    }
#endif
    else {

    }

at_cmd_parse_start:
    str_p = at_check_match_string(parse_pt, parse_size, at_head_str_table, sizeof(at_head_str_table));
    if (!str_p) {
        log_info("###1unknow at_head:%s", packet);
        at_respond_send_err(ERR_AT_CMD);
        return;
    }
    parse_pt   += str_p->str_len;
    parse_size -= str_p->str_len;

    if (str_p->str_id == STR_ID_HEAD_AT_CMD) {
        str_p = at_check_match_string(parse_pt, parse_size, at_cmd_str_table, sizeof(at_cmd_str_table));
        if (!str_p) {
            log_info("###2unknow at_cmd:%s", packet);
            at_respond_send_err(ERR_AT_CMD);
            return;
        }

        parse_pt    += str_p->str_len;
        parse_size -= str_p->str_len;
        if (parse_pt[0] == '=') {
            operator_type = AT_CMD_OPT_SET;
        } else if (parse_pt[0] == '?') {
            operator_type = AT_CMD_OPT_GET;
        }
        parse_pt++;
    } else if (str_p->str_id == STR_ID_HEAD_AT_CMD) {
        operator_type = AT_CMD_OPT_SET;
    }

    //    if(operator_type == AT_CMD_OPT_NULL)
    //    {
    //        AT_STRING_SEND(at_str_err);
    //        log_info("###3unknow operator_type:%s", packet);
    //        return;
    //    }

    log_info("str_id:%d", str_p->str_id);

    par = parse_param_split(parse_pt, ',', '\r');

    log_info("\n par->data: %s", par->data);

    switch (str_p->str_id) {
    case STR_ID_HEAD_AT_CHL: {
        u8 tmp_cid = func_char_to_dec(par->data, '\0');
        if (tmp_cid == 9) {
            black_list_check(0, NULL);
        }

        log_info("STR_ID_HEAD_AT_CHL:%d\n", tmp_cid);
        AT_STRING_SEND("OK");  //响应
        cur_atcom_cid = tmp_cid;
    }
    break;

    case STR_ID_ERROR:
        log_info("STR_ID_ERROR\n");
        break;

    case STR_ID_GVER:                   //2.1;
        log_info("STR_ID_GVER\n");
        {
            AT_STRING_SEND(G_VERSION);
            AT_STRING_SEND("OK");  //响应
        }
        break;

    case STR_ID_GCFGVER:                    //2.2
        log_info("STR_ID_GCFGVER\n");
        {
            AT_STRING_SEND(CONFIG_VERSION);
            AT_STRING_SEND("OK");  //响应
        }
        break;

    case STR_ID_NAME:
        log_info("STR_ID_NAME\n");
        {
            if (operator_type == AT_CMD_OPT_SET) { //2.4
                ble_at_set_name(par->data, par->len);
                AT_STRING_SEND("OK");
            } else {                            //2.3
                sprintf(buf, "+NAME:");
                u8 len = 0;
                len = strlen(buf);

                len = ble_at_get_name(&buf[0] + len) + len;
                at_cmd_send(buf, len);
                AT_STRING_SEND("OK");
            }
        }
        break;

    case STR_ID_LBDADDR:                        //2.5
        log_info("STR_ID_LBDADDR\n");
        if (operator_type == AT_CMD_OPT_GET) {
            u8 buf[30] = "+LBDADDR:";
            u8 ble_addr[6] = {0};
            u8 len = 0;

            len = strlen(buf);
            sprintf(buf, "+LBDADDR:");
            ble_at_get_address(ble_addr);
            hex_2_str(ble_addr, 6, &buf[0] + len);

            at_cmd_send(buf, len + 12);
            AT_STRING_SEND("OK");
        } else {
            at_respond_send_err(ERR_AT_CMD);
        }
        break;

    case STR_ID_BAUD:
        log_info("STR_ID_BAUD\n");
        {
            if (operator_type == AT_CMD_OPT_SET) { //2.7
                uart_baud = func_char_to_dec(par->data, '\0');
                log_info("set baud= %d", uart_baud);
                if (uart_baud == 9600 || uart_baud == 19200 || uart_baud == 38400 || uart_baud == 115200 ||
                    uart_baud == 230400 || uart_baud == 460800 || uart_baud == 921600) {
                    AT_STRING_SEND("OK");
                    ct_uart_change_baud(uart_baud);
                } else { //TODO返回错误码
                    at_respond_send_err(ERR_AT_CMD);
                }
            } else {                            //2.6

                sprintf(buf, "+BAUD:%d", uart_baud);
                at_cmd_send(buf, strlen(buf));
                AT_STRING_SEND("OK");
            }
        }
        break;

    case STR_ID_POWER_OFF:  //2.18
        log_info("STR_ID_POWER_OFF\n");
        {
            AT_STRING_SEND("OK");
            // TODO ,需要返回错误码
            sys_timeout_add((void *)POWER_EVENT_POWER_SOFTOFF, atchar_power_event_to_user, 100);
        }
        break;

    case STR_ID_LOW_POWER:  //2.18
        log_info("STR_ID_LOW_POWER\n");
        {
            u8 lp_state;
            if (operator_type == AT_CMD_OPT_SET) { //2.9
                AT_STRING_SEND("OK");
                lp_state = func_char_to_dec(par->data, '\0');
                log_info("set lowpower: %d\n", lp_state);
#if (defined CONFIG_CPU_BD19)
                at_set_low_power_mode(lp_state);
                extern void board_at_uart_wakeup_enalbe(u8 enalbe);
                board_at_uart_wakeup_enalbe(lp_state);
#endif
            } else {
                lp_state = at_get_low_power_mode();
                log_info("get lowpower: %d\n", lp_state);
                sprintf(buf, "+LOWPOWER:%d", lp_state);
                at_cmd_send(buf, strlen(buf));
                AT_STRING_SEND("OK");
            }
        }
        break;

    case STR_ID_ADV:
        log_info("STR_ID_ADV\n");
        {
            u8 adv_state;
            if (operator_type == AT_CMD_OPT_SET) { //2.9
                adv_state = func_char_to_dec(par->data, '\0');
                ret = ble_at_adv_enable(adv_state);
                if (ret == 0) {
                    AT_STRING_SEND("OK");
                } else {
                    // TODO ,需要返回错误码
                    at_respond_send_err(ERR_AT_CMD);
                }
            } else {                            //2.8
                u8 adv_state = ble_at_get_adv_state();  //0广播关闭,1打开

                sprintf(buf, "+ADV:%d", adv_state);
                at_cmd_send(buf, strlen(buf));
                AT_STRING_SEND("OK");
            }
        }
        break;


    case STR_ID_ADVPARAM:
        log_info("STR_ID_ADVPARAM\n");
        {
            u16 adv_interval;
            if (operator_type == AT_CMD_OPT_SET) { //2.11
                adv_interval = func_char_to_dec(par->data, '\0');
                log_info("set_adv_interval: %d", adv_interval);
                ble_at_set_adv_interval(adv_interval);
                //ret = ble_op_set_adv_param(adv_interval, ADV_IND, ADV_CHANNEL_ALL);
                AT_STRING_SEND("OK");
            } else {                            //2.10
                adv_interval = ble_at_get_adv_interval();
                sprintf(buf, "+ADVPARAM:%d", adv_interval);
                at_cmd_send(buf, strlen(buf));
                AT_STRING_SEND("OK");
            }
        }
        break;

    case STR_ID_ADVDATA:
        log_info("STR_ID_ADVDATA\n");
        {
            u8 i = 0;
            if (operator_type == AT_CMD_OPT_SET) { //2.13
                u8 adv_data[35] = {0};
                u8 adv_data_len = 0;  //广播hex数据的长度

                //将par->data转换成hex
                adv_data_len = str_2_hex(par->data, par->len, adv_data);
                if (adv_data_len > 31) {
                    ret = 1;
                } else {
                    ret = ble_at_set_adv_data(adv_data, adv_data_len);
                }

                if (ret == 0) {
                    AT_STRING_SEND("OK");
                } else {
                    // TODO ,需要返回错误码
                    at_respond_send_err(ERR_AT_CMD);
                }

            } else { //2.12
                u8 adv_data_len = 0;  //广播hex数据的长度
                u8 *adv_data = ble_at_get_adv_data(&adv_data_len);

                sprintf(buf, "+ADVDATA:");
                u8 len = 0;
                len = strlen(buf);
                if (adv_data_len) {
                    hex_2_str(adv_data, adv_data_len, &buf[len]);
                }
                at_cmd_send(buf, len + adv_data_len * 2);
                AT_STRING_SEND("OK");
            }
        }
        break;

    case STR_ID_SRDATA:
        log_info("STR_ID_SRDATA\n");
        {
            u8 i = 0;
            if (operator_type == AT_CMD_OPT_SET) { //2.15
                u8 scan_rsp_data[35] = {0};
                u8 scan_rsp_data_len = 0;   //hex长度

                scan_rsp_data_len = str_2_hex(par->data, par->len, scan_rsp_data);

                if (scan_rsp_data_len > 31) {
                    ret = 1;
                } else {
                    ret = ble_at_set_rsp_data(scan_rsp_data, scan_rsp_data_len);
                }

                if (ret == 0) {
                    AT_STRING_SEND("OK");
                } else {
                    // TODO ,需要返回错误码
                    at_respond_send_err(ERR_AT_CMD);
                }

            } else { //2.14
                u8 rsp_data_len = 0;  //广播hex数据的长度
                u8 *rsp_data = ble_at_get_rsp_data(&rsp_data_len);

                sprintf(buf, "+SRDATA:");
                u8 len = strlen(buf);

                if (rsp_data_len) {
                    hex_2_str(rsp_data, rsp_data_len, &buf[0] + len);
                }

                log_info("rsp_data_len  = %d", rsp_data_len);
                at_cmd_send(buf, len + rsp_data_len * 2);
                AT_STRING_SEND("OK");
            }
        }
        break;

    case STR_ID_CONNPARAM:
        log_info("STR_ID_CONNPARAM\n");
        {
#if CONFIG_BT_GATT_CLIENT_NUM
            u16 conn_param[4] = {0}; //interva_min, interva_max, conn_latency, conn_timeout;
            u8 i = 0;

            if (operator_type == AT_CMD_OPT_SET) { //2.17
                while (par) {  //遍历所有参数
                    log_info("len=%d,par:%s", par->len, par->data);
                    conn_param[i] = func_char_to_dec(par->data, '\0');  //获取参数
                    if (par->next_offset) {
                        par = AT_PARAM_NEXT_P(par);
                    } else {
                        break;
                    }
                    i++;
                }

                ret = le_at_client_set_conn_param(conn_param);
                log_info("\n conn_param = %d %d %d %d", conn_param[0], conn_param[1], conn_param[2], conn_param[3]);
                if (ret == 0) {
                    AT_STRING_SEND("OK");
                } else {
                    // TODO ,需要返回错误码
                    at_respond_send_err(ERR_AT_CMD);
                }

            } else {                    //2.16
                sprintf(buf, "+CONNPARAM:");
                u8 len = strlen(buf);

                le_at_client_get_conn_param(conn_param);

                for (i = 0; i < sizeof(conn_param) / sizeof(conn_param[0]); i++) {
                    sprintf(&buf[0] + len, "%d", conn_param[i]);
                    len = strlen(buf);
                    buf[len] = ',';
                    len += 1;
                }
                len -= 1;     //清掉最后一个逗号

                at_cmd_send(buf, len);
                AT_STRING_SEND("OK");
            }
#endif
        }
        break;

    case STR_ID_SCAN:  //2.18
        log_info("STR_ID_SCAN\n");
#if CONFIG_BT_GATT_CLIENT_NUM
        {
            ret = le_at_client_scan_enable(func_char_to_dec(par->data, '\0'));

            if (ret == 0) {
                AT_STRING_SEND("OK");
            } else {
                // TODO ,需要返回错误码
                at_respond_send_err(ERR_AT_CMD);
            }
        }
#endif
        break;

    case STR_ID_TARGETUUID:  //2.19 TODO
        log_info("STR_ID_TARGETUUID\n");
#if CONFIG_BT_GATT_CLIENT_NUM
        {
            u16 tag_uuid;
            if (operator_type == AT_CMD_OPT_SET) {
                str_2_hex(par->data, 2, (u8 *)&tag_uuid + 1);
                str_2_hex(par->data + 2, 2, &tag_uuid); //先填高位,在填低位

                log_info("target_uuid:%04x", tag_uuid);
                le_at_client_set_target_uuid16(tag_uuid);
                AT_STRING_SEND("OK");
            } else {
                at_respond_send_err(ERR_AT_CMD);
            }
        }
#endif
        break;

    case STR_ID_CONN:           //2.20
        log_info("STR_ID_CONN\n");
#if CONFIG_BT_GATT_CLIENT_NUM
        {
            struct create_conn_param_t create_conn_par;
            str_2_hex(par->data, par->len, create_conn_par.peer_address);
            put_buf(create_conn_par.peer_address, 6);
            create_conn_par.peer_address_type = 0;

            le_at_client_scan_enable(0);
            if (!le_at_client_creat_connection(create_conn_par.peer_address, 0)) {
                AT_STRING_SEND("OK");
            } else {
                at_respond_send_err(ERR_AT_CMD);
            }
        }
#endif
        break;

    case STR_ID_CONN_CANNEL:           //2.20
        log_info("STR_ID_CONN_CANNEL\n");
#if CONFIG_BT_GATT_CLIENT_NUM
        if (!le_at_client_creat_cannel()) {
            AT_STRING_SEND("OK");
        } else {
            at_respond_send_err(ERR_AT_CMD);
        }
#endif
        break;

    case STR_ID_DISC:           //2.21
        log_info("STR_ID_DISC\n");
        {
            u8 tmp_cid = func_char_to_dec(par->data, '\0');
            if (tmp_cid < 7) {
#if CONFIG_BT_GATT_CLIENT_NUM
                le_at_client_disconnect(tmp_cid);
#endif
            } else if (tmp_cid == 8) {
                ble_app_disconnect();
            } else {
                // TODO ,需要返回错误码
                at_respond_send_err(ERR_AT_CMD);
                break;
            }
            AT_STRING_SEND("OK");
        }
        break;

    case STR_ID_OTA:
        log_info("STR_ID_OTA\n");
        ret = 0;
        AT_STRING_SEND(at_str_ok);
        break;

    default:
        break;
    }
}

void at_send_conn_result(u8 cid, u8 is_sucess)
{
    if (is_sucess) {
        u8 tmp[16];
        AT_STRING_SEND("OK");
        sprintf(tmp, "IM_CONN:%d", cid);
        AT_STRING_SEND(tmp);
        cur_atcom_cid = cid;
    } else {
        at_respond_send_err(ERR_AT_CMD);
    }
}


static void disconnect_timeout(void *prvi)
{
    black_list_check(2, NULL);
}

void black_list_check(u8 sta, u8 *peer_addr)
{
    return;
    static u8 last_addr[6] = {0};
    u8 static timeout_flag = 0;
    switch (sta) {
    case 0:
        log_info(" cur_atcom_cid = %d  ", cur_atcom_cid);
        if (cur_atcom_cid == 8 + 9) {
            ble_app_disconnect();
            sys_timeout_add(NULL, disconnect_timeout, 1000L * 60 * 5);
            timeout_flag = 1;
            log_info("i am here");

        }
        break; //断开从机连接,并开始计时

    case 1:
        if (memcmp(last_addr, peer_addr, 6) == 0);
        {
            if (timeout_flag == 1) {
                ble_app_disconnect();
                return;
            }
        }
        memcpy(last_addr, peer_addr, 6);
        break; //5分钟内拒绝重连

    case 2:
        timeout_flag = 0;
        break;  //恢复可重连
    }
}


void at_send_disconnect(u8 cid)
{
    u8 tmp[16];
    u8 tmp_id = cur_atcom_cid;

    sprintf(tmp, "IM_DISC:%d", cid);
    AT_STRING_SEND(tmp);
    cur_atcom_cid = 9;
}

void at_send_connected(u8 cid)
{
    u8 tmp[16];
    u8 tmp_id = cur_atcom_cid;
    cur_atcom_cid = 9;

    sprintf(tmp, "IM_CONN:%d", cid);
    AT_STRING_SEND(tmp);
    cur_atcom_cid = cid;
}

void at_send_string(u8 *str)
{
    AT_STRING_SEND(str);
}

int at_send_uart_data(u8 *packet, u16 size, int post_event)
{
    struct sys_event e;
    /* put_buf(packet, size); */
    u16 ret = cbuf_write(&bt_to_uart_cbuf, packet, size);
    if (ret < size) {
        log_info("bt so fast, uart lose data!!,%d", size);
        return 0;
    }

    if (post_event) {
        e.type = SYS_BT_EVENT;
        e.arg  = (void *)SYS_BT_EVENT_FORM_AT;
        e.u.dev.event = 0;
        e.u.dev.value = 0;
        sys_event_notify(&e);
    }
    return size;
}

void at_send_rx_cid_data(u8 cid, u8 *packet, u16 size)
{
    struct sys_event e;
    int ret = -1;
    if (cur_atcom_cid == cid) {
        log_info("cid=%d,send_data(%d):", cid, size);
        at_send_uart_data(packet, size, 1);
    } else {
        log_info("lose %d,send_data(%d):", cid, size);
    }
}



void at_cmd_init(void)
{
    log_info("%s,%d\n", __func__, __LINE__);

#ifdef HAVE_MALLOC
    __this = malloc(sizeof(struct at_layer));
    ASSERT(__this, "Fatal Error");
    memset(__this, 0x0, sizeof(struct at_layer));

    __this->pAT_buffer = malloc(AT_BUFFER_SIZE);
    ASSERT(__this, "Fatal Error");
    memset(__this->pAT_buffer, 0x0, CI_BUFFER_SIZE);
#else
    log_info("Static");
    __this->pAT_buffer = pAT_buffer_static;
#endif
    /* __this->config = config; */
    /* at_transport_setup(void); */
    at_uart_init(at_packet_handler);

    ble_at_set_adv_interval(adv_interval_default);
    cbuf_init(&bt_to_uart_cbuf, BT_UART_BUF, BT_UART_FIFIO_BUFFER_SIZE);

    log_info("at com is ready");
    AT_STRING_SEND("IM_READY");
}

#endif

