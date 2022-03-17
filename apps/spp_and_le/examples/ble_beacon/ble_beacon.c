

#include "system/app_core.h"
#include "system/includes.h"

#include "app_config.h"
#include "app_action.h"

#include "btstack/btstack_task.h"
#include "btstack/bluetooth.h"
#include "user_cfg.h"
#include "vm.h"
#include "btcontroller_modules.h"
#include "bt_common.h"
#include "3th_profile_api.h"
#include "le_common.h"
#include "rcsp_bluetooth.h"
#include "JL_rcsp_api.h"
#include "custom_cfg.h"
#include "ble_beacon.h"


#if CONFIG_APP_BEACON

#if LE_DEBUG_PRINT_EN
extern void printf_buf(u8 *buf, u32 len);
/* #define log_info          printf */
#define log_info(x, ...)  printf("[LE-BEACON]" x " ", ## __VA_ARGS__)
#define log_info_hexdump  printf_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

#define uint8 unsigned char
/* #define u8 unsigned char */


// # of URL Scheme Prefix types
#define EDDYSTONE_URL_PREFIX_MAX        4
// # of encodable URL words
#define EDDYSTONE_URL_ENCODING_MAX      14




// Array of URL Scheme Prefices
static char *eddystoneURLPrefix[EDDYSTONE_URL_PREFIX_MAX] = {
    "http://www.",
    "https://www.",
    "http://",
    "https://"
};

// Array of URLs to be encoded
static char *eddystoneURLEncoding[EDDYSTONE_URL_ENCODING_MAX] = {
    ".com/",
    ".org/",
    ".edu/",
    ".net/",
    ".info/",
    ".biz/",
    ".gov/",
    ".com/",
    ".org/",
    ".edu/",
    ".net/",
    ".info/",
    ".biz/",
    ".gov/"
};


/**
将普通网址, 转成特制的url
参数:
    urlOrg: 原始网址,纯字符串格式
    urlEnc: 输出的特制url网址,
返回值: 返回urlEnc的实际长度, 错误返回0,
注意事项: 输入的网址长度不能太长.

**/
static uint8 SimpleEddystoneBeacon_encodeURL(char *urlOrg, uint8 *urlEnc)
{
    uint8 i, j;
    uint8 urlLen;
    uint8 tokenLen;

    urlLen = (uint8) strlen(urlOrg);

    // search for a matching prefix
    for (i = 0; i < EDDYSTONE_URL_PREFIX_MAX; i++) {
        tokenLen = strlen(eddystoneURLPrefix[i]);
        if (strncmp(eddystoneURLPrefix[i], urlOrg, tokenLen) == 0) {
            break;
        }
    }
    if (i == EDDYSTONE_URL_PREFIX_MAX) {
        return 0;       // wrong prefix
    }

    // use the matching prefix number
    urlEnc[0] = i;
    urlOrg += tokenLen;
    urlLen -= tokenLen;
    // search for a token to be encoded
    for (i = 0; i < urlLen; i++) {
        for (j = 0; j < EDDYSTONE_URL_ENCODING_MAX; j++) {
            tokenLen = strlen(eddystoneURLEncoding[j]);
            if (strncmp(eddystoneURLEncoding[j], urlOrg + i, tokenLen) == 0) {
                // matching part found
                break;
            }
        }

        if (j < EDDYSTONE_URL_ENCODING_MAX) {
            memcpy(&urlEnc[1], urlOrg, i);
            // use the encoded byte
            urlEnc[i + 1] = j;
            break;
        }
    }

    if (i < urlLen) {
        memcpy(&urlEnc[i + 2], urlOrg + i + tokenLen, urlLen - i - tokenLen);
        return urlLen - tokenLen + 2;
    }

    memcpy(&urlEnc[1], urlOrg, urlLen);
    return urlLen + 1;
}



static u8 make_eddystone_url_adv_packet(u8 *buf, EDDYSTONE_URL *eds_url_packet, u8 *web)
{
    u8 ret = 0;

    ret = SimpleEddystoneBeacon_encodeURL(web, eds_url_packet->url_cheme);

    log_info(" ret = %d \n ", ret);

    eds_url_packet->length_last = ret + 5;  //长度

    memcpy(buf, (u8 *)eds_url_packet, ret + 10);

    return ret + 10;
}


//u8  State[16]=// !>（例子）明文；
//    {0x02, 0x13, 0x24, 0x35, 0x46, 0x57, 0x68,
//     0x79, 0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13};

u8 share_key[16] = {  0x4c, 0x68, 0x38, 0x41,
                      0x39, 0xf5, 0x74, 0xd8,
                      0x36, 0xbc, 0xf3, 0x4e,
                      0x90, 0xfb, 0x01, 0xbf
                   }; //16字节身份共享密钥(与终端共享)


//指数运算
static u32 my_pow(u32 di, u32 top)
{
    int i = 0;
    for (i = 0; i < top; i++) {
        di *= di;
    }
    return di;
}



extern int eddy_aes_authcrypt_eax(unsigned char *key,
                                  int mode, /* 加密 解密模式选择 */
                                  const unsigned char *nonce,
                                  size_t nonce_length,
                                  const unsigned char *header,
                                  size_t header_length,
                                  size_t message_length,
                                  const unsigned char *input,
                                  unsigned char *output,
                                  unsigned char *tag,
                                  size_t tag_length);


/*
参数,
share_key:与app共享的密钥
cnt: 信标计时器,单位1s,即1s自加,最大误差为150 ppm
K: 旋转周期指数,用于控制EID的更新频率; 范围是0到15; 信标每2^K产生一个新的标识符,
tlm_data: 需要加密的tlm数据
etml_out: 输出16字节, 包括etml, random, check;

数据全部是大端数据
*/
static void get_etml(u8 *share_key, u32 cnt, u8 K, EDDYSTONE_TLM *tlm_data, u8 *etml_out)
{
#define TLM_DATA_LEN 12
#define MIC_LEN 2
#define MBEDTLS_AES_ENCRYPT     1 /**< AES encryption. */
#define MBEDTLS_AES_DECRYPT     0 /**< AES decryption. */
    u8 nonce[6] = {0};
    u16 random = rand32();
    u8 *input = &(tlm_data->v_battle);
    u8 emptyHeader[1];
    u8 mic[2] = {0};


    cnt = (cnt >> K) << K; //清除低K位
    memcpy(nonce, &cnt, 4);
    memcpy(nonce + 4, &random, 2); //制作48位随机数


    eddy_aes_authcrypt_eax(share_key, MBEDTLS_AES_ENCRYPT, nonce, 6, emptyHeader, 0, TLM_DATA_LEN, input, etml_out, mic, MIC_LEN);
    memcpy(etml_out + 12, &random, 2);
    memcpy(etml_out + 14, &mic, 2);

}



/*
参数,
share_key:与app共享的密钥
cnt: 信标计时器,单位1s,即1s自加,最大误差为150 ppm
K: 旋转周期指数,用于控制EID的更新频率; 范围是0到15; 信标每2^K产生一个新的标识符,
eid_out: 输出8字节的eid
数据全部是大端数据
*/
static void get_eid(u8 *share_key, u32 cnt, u8 K, u8 *eid_out)
{
    static u32 last_cnt = 0;
    u8 key_linshi[16] = {0};
    u8 pt1[16] = {0};
    u8 eid[16] = {0};

    if ((cnt - last_cnt) > my_pow(2, K)) { //检查是否到了更新时间
        last_cnt = cnt;
        pt1[11] = 0xff;
        pt1[14] = cnt;
        pt1[15] = cnt >> 8;
        aes128_calc_cyphertext(share_key, pt1, key_linshi);  //计算临时密钥

        pt1[11] = K;
        cnt = (cnt >> K) << K; //清除低K位

        pt1[12] = cnt;
        pt1[13] = cnt >> 8;
        pt1[14] = cnt >> 16;
        pt1[15] = cnt >> 24;

        aes128_calc_cyphertext(key_linshi, pt1, eid);  //使用临时密钥加密

        memcpy(eid_out, eid, 8);
    }
}


static u8 make_beacon_packet(u8 *buf, void *packet, u8 packet_type, u8 *web)
{
    switch (packet_type) {

    case IBEACON_PACKET:
    case EDDYSTONE_UID_PACKET:
    case EDDYSTONE_TLM_PACKET:
        memcpy(buf, (u8 *)packet, packet_type);
        break;

    case EDDYSTONE_EID_PACKET:
        memcpy(buf, (u8 *)packet, packet_type);

        break;
    case EDDYSTONE_ETLM_PACKET:
        memcpy(buf, (u8 *)packet, packet_type);

        break;



    case EDDYSTONE_URL_PACKET:
        packet_type = make_eddystone_url_adv_packet(buf, packet, web);
        break;
    }


    return packet_type;
}


//--------------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------------
//---------------
// 广播周期 (unit:0.625ms)
#define ADV_INTERVAL_MIN          (160*2)
static u8 adv_data_len;
static u8 adv_data[ADV_RSP_PACKET_MAX];//max is 31
static u8 scan_rsp_data_len;
static u8 scan_rsp_data[ADV_RSP_PACKET_MAX];//max is 31

static char gap_device_name[BT_NAME_LEN_MAX] = "becaon_test";
static u8 gap_device_name_len = 0; //名字长度，不包含结束符
static u8 ble_work_state = 0;      //ble 状态变化
static u8 adv_ctrl_en;             //广播控制

static void set_ble_work_state(ble_state_e state)
{
    if (state != ble_work_state) {
        log_info("ble_work_st:%x->%x\n", ble_work_state, state);
        ble_work_state = state;
    }
}

static ble_state_e get_ble_work_state(void)
{
    return ble_work_state;
}

static const EDDYSTONE_UID eddystone_uid_adv_packet  = {

    .length = 0x03,                  //长度值为0x03;(固定)
    .ad_type1 = 0x03,                //广播数据类型:0x03(表明是16位完整列表uuid) (固定)
    .complete_list_uuid = 0xabcd,     //complete_list_uuid
    .length_last     = 0x17,             //剩余数据长度0x17; (固定)
    .ad_type2        = 0x16,                //广播数据类型为服务数据类型: 0x16 (固定)
    .eddystone_uuid = 0xfeaa,         //不知道这个是什么, 固定为0xfeaa吧(固定)
    .frametype     =  0x00,               //uid的框架类型, 为0x00; (固定)
    .tx_power      = 0x00,                //注意, 这里是0米处射频信号强度校准功率
    .name_space    =
    { 0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, //命名空间;可以用于给信标分组
    .instance = {0x63, 0x00, 0x00, 0x00, 0x00, 0x00},       //可以用于区分同组信标
    .reserve = 0x00,

};


static const IBEACON_PARA ibeacon_adv_packet = {

    .length = 0x1a,
    .ad_type  = 0xff,  //
    .company_id = 0x004c,
    .type = 0x02,
    .last_length = 0x15,  //
    .major = 0x00ff,
    .minor  = 0xee00,
    .tx_power = 12,
    .uuid =
    {
        0xfd, 0xa5, 0x06, 0x93,
        0xe2, 0x4f, 0xb1, 0xaf,
        0xc6, 0xeb, 0x07, 0x64,
        0x25, 0xa4, 0xaf, 0x4f
    }

};


static const EDDYSTONE_TLM eddystone_tlm_adv_packet = {
    .length = 0x03,
    .ad_type1 = 0x03,
    .complete_list_uuid = 0xabcd,
    .length_last = 0x11,
    .ad_type2 = 0x16,
    .eddystone_uuid = 0xfeaa,
    .frametype = 0x20,
    .tlm_version = 0x00,
    .v_battle = 0x0100,
    .temp = 0x0100,
    .adv_cnt = 0,
    .sec_cnt = 0
};

static const EDDYSTONE_URL eddystone_url_adv_packet = {
    .length = 0x03,
    .ad_type1 = 0x03,
    .complete_list_uuid = 0xABCD,
    .ad_type2 = 0x16,
    .eddystone_uuid = 0XFEAA,
    .frametype = 0X10,
    .tx_power = 0X00,
};

static const EDDYSTONE_EID eddystone_eid_adv_packet = {

    .length = 0x03,
    .ad_type1 = 0x03,
    .complete_list_uuid = 0xABCD,

    .length_last = 0x0d,             //剩余数据长度;
    .ad_type2 = 0x16,
    .eddystone_uuid = 0XFEAA,
    .frametype = 0X30,
    .tx_power = 0X00,
    .eid = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    }
};


static const EDDYSTONE_ETLM eddystone_etlm_adv_packet = {
    .length = 0x03,
    .ad_type1 = 0x03,
    .complete_list_uuid = 0xabcd,
    .length_last = 0x15,
    .ad_type2 = 0x16,
    .eddystone_uuid = 0xfeaa,
    .frametype = 0x20,
    .tlm_version = 0x01,

    .etml = {
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    },         //12字节加密数据
    .random = 1,               //随机数,要与加密时用到的随机数相同
    .check = 2                //AES-EAX计算出来的校验和
};

static int make_set_adv_data(void)
{
    u8 offset = 0;
    u8 *buf = adv_data;

    /* offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, 0x06, 1); */
    offset += make_beacon_packet(&buf[offset], &eddystone_etlm_adv_packet, EDDYSTONE_ETLM_PACKET, NULL);
    /* offset += make_beacon_packet(&buf[offset], &eddystone_eid_adv_packet, EDDYSTONE_EID_PACKET, NULL); */
    //    offset += make_beacon_packet(&buf[offset], &eddystone_url_adv_packet, EDDYSTONE_URL_PACKET, "https://fanyi.baidu.com/");
    //    offset += make_beacon_packet(&buf[offset], &eddystone_tlm_adv_packet, EDDYSTONE_TLM_PACKET, NULL);
    //    offset += make_beacon_packet(&buf[offset], &eddystone_uid_adv_packet, EDDYSTONE_UID_PACKET, NULL);
    //    offset += make_beacon_packet(&buf[offset], &ibeacon_adv_packet, IBEACON_PACKET, NULL);

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***adv_data overflow!!!!!!\n");
        return -1;
    }
    log_info("adv_data(%d):", offset);
    log_info_hexdump(buf, offset);
    adv_data_len = offset;
    ble_op_set_adv_data(offset, buf);
    return 0;
}

static int make_set_rsp_data(void)
{
    u8 offset = 0;
    u8 *buf = scan_rsp_data;

    u8 name_len = gap_device_name_len;
    u8 vaild_len = ADV_RSP_PACKET_MAX - (offset + 2);
    if (name_len > vaild_len) {
        name_len = vaild_len;
    }
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)gap_device_name, name_len);

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***rsp_data overflow!!!!!!\n");
        return -1;
    }

    log_info("rsp_data(%d):", offset);
    log_info_hexdump(buf, offset);
    scan_rsp_data_len = offset;
    ble_op_set_rsp_data(offset, buf);
    return 0;
}

#define DISPLAY_NAME_DEBUG     0 //rsp 显示 name
//广播参数设置
static void advertisements_setup_init()
{
#if DISPLAY_NAME_DEBUG
    uint8_t adv_type = ADV_NONCONN_IND; //no rsp_data
#else
    uint8_t adv_type = ADV_SCAN_IND;
#endif

    uint8_t adv_channel = ADV_CHANNEL_ALL;
    int   ret = 0;

    ble_op_set_adv_param(ADV_INTERVAL_MIN, adv_type, adv_channel);

    ret |= make_set_adv_data();

#if DISPLAY_NAME_DEBUG
    ret |= make_set_rsp_data();
#endif

    if (ret) {
        puts("advertisements_setup_init fail !!!!!!\n");
        return;
    }
}

static int set_adv_enable(void *priv, u32 en)
{
    ble_state_e next_state, cur_state;

    if (!adv_ctrl_en && en) {
        return APP_BLE_OPERATION_ERROR;
    }

    if (en) {
        next_state = BLE_ST_ADV;
    } else {
        next_state = BLE_ST_IDLE;
    }

    cur_state =  get_ble_work_state();
    switch (cur_state) {
    case BLE_ST_ADV:
    case BLE_ST_IDLE:
    case BLE_ST_INIT_OK:
    case BLE_ST_NULL:
        break;
    default:
        return APP_BLE_OPERATION_ERROR;
        break;
    }

    if (cur_state == next_state) {
        return APP_BLE_NO_ERROR;
    }
    log_info("adv_en:%d\n", en);
    set_ble_work_state(next_state);

    if (en) {
        advertisements_setup_init();
    }
    ble_op_adv_enable(en);
    return APP_BLE_NO_ERROR;
}

void ble_app_disconnect(void)
{
}

void bt_ble_adv_enable(u8 enable)
{
    set_adv_enable(0, enable);
}


void ble_module_enable(u8 en)
{
    log_info("mode_en:%d\n", en);
    if (en) {
        adv_ctrl_en = 1;
        bt_ble_adv_enable(1);
    } else {
        bt_ble_adv_enable(0);
        adv_ctrl_en = 0;
    }
}

void ble_profile_init(void)
{
    log_info("%s\n", __FUNCTION__);
}

static const char ble_ext_name[] = "(BLE)";
void bt_ble_init(void)
{
    log_info("***** ble_init******\n");
    log_info("ble_file: %s", __FILE__);

    char *name_p;
    u8 ext_name_len = sizeof(ble_ext_name) - 1;

    name_p = bt_get_local_name();
    gap_device_name_len = strlen(name_p);
    if (gap_device_name_len > BT_NAME_LEN_MAX - ext_name_len) {
        gap_device_name_len = BT_NAME_LEN_MAX - ext_name_len;
    }

    //增加后缀，区分名字
    memcpy(gap_device_name, name_p, gap_device_name_len);
    memcpy(&gap_device_name[gap_device_name_len], "(BLE)", ext_name_len);
    gap_device_name_len += ext_name_len;

    log_info("ble name(%d): %s \n", gap_device_name_len, gap_device_name);

    set_ble_work_state(BLE_ST_INIT_OK);
    ble_module_enable(1);

}

void bt_ble_exit(void)
{
    log_info("***** ble_exit******\n");
    ble_module_enable(0);
}

#endif

