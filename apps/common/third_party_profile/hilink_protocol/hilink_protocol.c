#include "hilink_protocol.h"
#include "hilink_task.h"
#include "gpio.h"

#if 1
extern void printf_buf(u8 *buf, u32 len);
#define log_info(x, ...)  printf("[HILINK_PROTOCOL]" x " ", ## __VA_ARGS__)
#define log_info_hexdump  printf_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

#define LED_PIN IO_PORTA_00

static int hilink_cmd_deal(uint8_t *buf);
static void hilink_state_encry_report();

extern uint16_t hilink_mtu;

static uint16_t auth_info_vm_id;
static uint16_t attr_info_vm_id;
static hi_auth_info_t hilink_auth;
static hi_attribute_t hilink_attr;
static uint8_t hilink_msg_id;
static int hilink_seq;

static hi_msg_store_t hi_msg_store;

dev_info_t *hilink_info = NULL;

u8 hi_hex_to_ascii(u8 in)
{
    if (in >= 0 && in <= 9) {
        return in + '0';
    } else if (in >= 0xa && in <= 0xf) {
        return in - 0x0a + 'A';
    } else {
        printf("hilink hex to ascii error, data:0x%x", in);
        return 0;
    }
}

u8 hi_ascii_to_hex(u8 in)
{
    if (in >= '0' && in <= '9') {
        return in - '0';
    } else if (in >= 'a' && in <= 'f') {
        return in - 'a' + 0x0a;
    } else if (in >= 'A' && in <= 'F') {
        return in - 'A' + 0x0a;
    } else {
        printf("hilink ascii to hex error, data:0x%x", in);
        return 0;
    }
}

static hilink_led_set(uint8_t state)
{
    gpio_direction_output(LED_PIN, state);
}

static uint8_t get_hilink_msg_id()
{
    if (hilink_msg_id >= 0 && hilink_msg_id < 255) {
        hilink_msg_id += 1;
        return hilink_msg_id;
    } else {
        hilink_msg_id = 0;
        return hilink_msg_id;
    }
}

static uint16_t get_hilink_seq()
{
    if (hilink_seq >= 0 && hilink_seq < 65535) {
        hilink_seq += 1;
        return hilink_seq;
    } else {
        hilink_seq = 0;
        return hilink_seq;
    }
}

void hilink_set_auth_info_store_vm_id(uint16_t value)
{
    auth_info_vm_id = value;
    log_info("auth_info_vm_id set to:%d", auth_info_vm_id);
}

void hilink_set_attr_info_store_vm_id(uint16_t value)
{
    attr_info_vm_id = value;
    log_info("attr_info_vm_id set to:%d", attr_info_vm_id);
}

static void hilink_auth_info_store()
{
    int ret;
    ret = syscfg_write(auth_info_vm_id, hilink_auth, sizeof(hilink_auth));
    if (ret != sizeof(hilink_auth)) {
        printf("hilink_auth_info_store err:%d", ret);
    }
}

static void hilink_attr_info_store()
{
    int ret;
    ret = syscfg_write(attr_info_vm_id, hilink_attr, sizeof(hilink_attr));
    if (ret != sizeof(hilink_attr)) {
        printf("hilink_attr_info_store err:%d", ret);
    }
}

/*
 * 读取鉴权流程保存的加解密信息
 */
void hilink_auth_info_read()
{
    int ret;
    ret = syscfg_read(auth_info_vm_id, hilink_auth, sizeof(hilink_auth));
    if (ret != sizeof(hilink_auth)) {
        printf("hilink_auth_info_read err:%d", ret);
        return;
    }

    log_info("authcode:");
    put_buf(hilink_auth.hilink_authcode, 16);

    log_info("authcode_tmp:%s", hilink_auth.hilink_authcodeid_tmp);

    log_info("sessid:");
    put_buf(hilink_auth.sessid, 32);

    log_info("hmac:");
    put_buf(hilink_auth.hilink_hmackey, 32);

    log_info("sessionkey:");
    put_buf(hilink_auth.hilink_sessionkey, 16);

    log_info("pair_flag:%d", hilink_auth.hilink_pair_flag);
}

/*
 * 读取属性信息，本实例是灯，所以属性只有开关
 */
void hilink_attr_info_read()
{
    int ret;
    ret = syscfg_read(attr_info_vm_id, hilink_attr, sizeof(hilink_attr));
    if (ret != sizeof(hilink_attr)) {
        printf("hilink_attr_info_read err:%d", ret);
        return;
    }

    log_info("store onoff:%d:", hilink_attr.onoff);
    hilink_led_set(hilink_attr.onoff);
}

static void hilink_get_mac_str(uint8_t *mac_str)
{
    int i = 0;
    uint8_t ble_addr_tmp[6];
    uint8_t ble_addr[6];
    le_controller_get_mac(ble_addr_tmp);
    for (i = 0; i < 6; i++) {
        ble_addr[i] = ble_addr_tmp[5 - i];
    }

    for (int i = 0; i < 6; i++) {
        mac_str[3 * i] = hi_hex_to_ascii((ble_addr[i] & 0xf0) >> 4);
        mac_str[3 * i + 1] = hi_hex_to_ascii(ble_addr[i] & 0x0f);
        mac_str[3 * i + 2] = ':';
    }
    mac_str[17] = '\0';
}

static void hilink_set_mac(uint8_t *sn, uint8_t *mac_str)
{
    int i;
    uint8_t mac[6];
    for (i = 0; i < 6; i++) {
        mac[i] = (hi_ascii_to_hex(sn[11 - 2 * i - 1]) << 4) + hi_ascii_to_hex(sn[11 - 2 * i]);
        mac_str[3 * i] = sn[2 * i];
        mac_str[3 * i + 1] = sn[2 * i + 1];
        mac_str[3 * i + 2] = ':';
    }
    mac_str[17] = '\0';
    log_info("hi_mac:");
    put_buf(mac, 6);
    le_controller_set_mac(mac);
}

/*
 * 初始化时设置的设备信息
 */
void hilink_info_set(dev_info_t *info)
{
    char hilink_dev_mac[18];

    hilink_info = info;

    // 根据sn码修改蓝牙地址
    hilink_set_mac(hilink_info->sn, hilink_dev_mac);
    memcpy(hilink_info->mac, hilink_dev_mac, 18);
    memcpy(&hilink_info, &info, sizeof(dev_info_t));

    log_info("pid:%s", hilink_info->prodId);
    log_info("sn:%s", hilink_info->sn);
    log_info("devid:%s", hilink_info->dev_id);
    log_info("model:%s", hilink_info->model);
    log_info("dev_t:%s", hilink_info->dev_t);
    log_info("manu:%s", hilink_info->manu);
    log_info("mac:%s", hilink_info->mac);
    log_info("hiv:%s", hilink_info->hiv);
    log_info("fwv:%s", hilink_info->fwv);
    log_info("hwv:%s", hilink_info->hwv);
    log_info("swv:%s", hilink_info->swv);
    log_info("prot_t:%s", hilink_info->prot_t);
}

/*
 * hilink消息回复组包函数
 */
static int hilink_data_rsp(uint8_t msg_id, uint8_t *payload, uint16_t payload_len, uint8_t cmd_type, uint8_t encry)
{
    log_info("hilink_data_rsp");
    int i;
    uint16_t buf_len = 7 + payload_len;

    // 超过mtu的后面会拆包发送,前面的包按mtu大小设置
    if (buf_len > hilink_mtu) {
        buf_len = hilink_mtu;
    }
    HILINK_MSG_T *buf = malloc(buf_len);
    memset(buf, 0, buf_len);
    buf->cmd_type = cmd_type;
    buf->msg_id = msg_id;
    if (encry) {
        buf->encry = 0x03;
    } else {
        buf->encry = 0x00;
    }

    // 超过mtu的要拆包发送，这里计数包数
    uint8_t total_frame = (payload_len / (hilink_mtu - 7)) + ((payload_len % (hilink_mtu - 7)) > 0);

    if (total_frame == 1) {
        buf->total_frame = 1;
        memcpy(buf->data, payload, payload_len);
        hilink_data_send(buf, buf_len);
    } else {
        buf->total_frame = total_frame;
        for (i = 0; i < total_frame; i++) {
            buf->frame_seq = i;
            // 末尾包和非末尾包长度有区别
            if (buf->frame_seq < total_frame - 1) {
                //非末尾包
                memcpy(&buf->data[0], &payload[(hilink_mtu - 7) * i], hilink_mtu - 7);
                hilink_data_send(buf, hilink_mtu);
            } else {
                // 末尾包
                memcpy(buf->data, &payload[(hilink_mtu - 7) * i], payload_len - ((hilink_mtu - 7) * i));
                hilink_data_send(buf, 7 + payload_len - (hilink_mtu - 7) * i);
            }
        }
    }
    free(buf);
}

/*
 * 生成无需加密发数payload
 */
static uint8_t *hilink_payload_create(uint8_t name_len, uint8_t *service_name, uint16_t body_len, uint8_t *body)
{
    uint16_t payload_len = 4 + name_len + body_len;
    //printf("hilink_payload_create, payload_len = %d", payload_len);
    uint8_t *payload = malloc(payload_len);
    payload[0] = 0x11; // 保留字节默认填0x11
    payload[1] = name_len;
    memcpy(&payload[2], service_name, name_len);
    payload[2 + name_len] = body_len & 0xff;
    payload[3 + name_len] = body_len >> 8;
    memcpy(&payload[4 + name_len], body, body_len);

    return payload;
}

/*
 * 生成加密发数payload
 */
static uint8_t *hilink_payload_create_with_hmackey(uint8_t name_len, uint8_t *service_name, uint16_t body_len, uint8_t *body)
{
    uint16_t payload_len = 4 + name_len + body_len + 32;
    /* printf("hilink_payload_create_with_hmackey, payload_len = %d", payload_len); */

    uint8_t *payload = malloc(payload_len);
    payload[0] = 0x11; // 保留字节默认填0x11
    payload[1] = name_len;
    memcpy(&payload[2], service_name, name_len);
    payload[2 + name_len] = body_len & 0xff;
    payload[3 + name_len] = body_len >> 8;
    memcpy(&payload[4 + name_len], body, body_len);

    uint16_t hmac_buf_len = 4 + name_len + body_len;
    uint8_t *hmac_buf = malloc(hmac_buf_len);
    uint8_t hmac[32];
    memcpy(hmac_buf, payload, hmac_buf_len);

    const mbedtls_md_info_t *mdInfo = NULL;
    mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_hmac(mdInfo,
                    hilink_auth.hilink_hmackey, //hmacKey。
                    32, //hmacKey 长度， 32Byte
                    hmac_buf, //打包的 payload 数据
                    hmac_buf_len, //payload 数据
                    hmac//Hmac 校验值
                   );

    memcpy(&payload[4 + name_len + body_len], hmac, 32);

    free(hmac_buf);
    return payload;
}

static void create_sessionkey(sn1, sn2)
{
    uint8_t salt[16];
    unsigned int iterCount = 1;
    memcpy(&salt[0], sn1, 8);
    memcpy(&salt[8], sn2, 8);

    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t *mdInfo = NULL;
    mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_init(&ctx);
    /* 数字 1 表示使用 HMAC */
    mbedtls_md_setup(&ctx, mdInfo, 1);
    mbedtls_pkcs5_pbkdf2_hmac(&ctx,
                              hilink_auth.hilink_authcode, //设备代理注册生成的 authCode
                              16, //authCode 的长度， 16Byte
                              salt, //SN1|SN2 生成的盐值
                              16, //salt 长度， 16Byte
                              1, //设置值为 1
                              16, //sessionKey 的长度， 16Byte
                              hilink_auth.hilink_sessionkey); //sessionKey
    /* printf("create_sessionkey:"); */
    /* put_buf(hilink_auth.hilink_sessionkey, 16); */

    mbedtls_md_free(&ctx);

    mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_init(&ctx);
    /* 数字 1 表示使用 HMAC */
    mbedtls_md_setup(&ctx, mdInfo, 1);
    mbedtls_pkcs5_pbkdf2_hmac(&ctx,
                              hilink_auth.hilink_sessionkey, //上一步生成的 sessionKey
                              16, //sessionKey 的长度， 16Byte
                              salt, //SN1|SN2 生成的盐值
                              16, //salt 长度， 16Byte
                              1, //设置值为 1
                              32, //hmacKey 长度,32 Byte
                              hilink_auth.hilink_hmackey); //hmacKey
    /* printf("create_hmackey:"); */
    /* put_buf(hilink_auth.hilink_hmackey, 32); */

    mbedtls_md_free(&ctx);
    hilink_auth_info_store();
}

/*
 * 连接鉴权步骤1: 获取配网协议版本
 */
static void hilink_netcfgver(uint8_t msg_id)
{
    log_info("hilink_netcfgver");
    uint8_t name_len = 9;
    uint8_t service_name[9] = "netCfgVer";

    cJSON *root = NULL;
    root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "ver", 100);

    char *cjson_str = cJSON_PrintUnformatted(root);
    uint16_t cjson_len = strlen(cjson_str);
    log_info("netcfgver len:%d, cJSON:\n%s", cjson_len, cjson_str);

    uint16_t payload_len = 4 + name_len + cjson_len;
    uint8_t *payload = hilink_payload_create(name_len, service_name, cjson_len, cjson_str);

    hilink_data_rsp(msg_id, payload, payload_len, CMD_TYPE_RSP, MSG_WITHOUT_ENCRY);

    cJSON_Delete(root);
    free(cjson_str);
    free(payload);
}

static void hilink_conn_reject(uint8_t msg_id, uint8_t *service_name, uint8_t name_len)
{
    cJSON *rsp = NULL;
    rsp = cJSON_CreateObject();

    cJSON_AddStringToObject(rsp, "errcode", "-1");
    char *cjson_str = cJSON_PrintUnformatted(rsp);
    uint16_t cjson_len = strlen(cjson_str);
    log_info("authSetup rsp len:%d, cJSON:\n%s", cjson_len, cjson_str);

    uint16_t payload_len = 4 + name_len + cjson_len;
    uint8_t *payload = hilink_payload_create(name_len, service_name, cjson_len, cjson_str);
    hilink_data_rsp(msg_id, payload, payload_len, CMD_TYPE_RSP, MSG_WITHOUT_ENCRY);

    cJSON_Delete(rsp);
    free(cjson_str);
    free(payload);
}

/*
 * 连接鉴权步骤2: 查询设备配网注册厂商信息
 */
static void hilink_deviceinfo(uint8_t msg_id)
{
    uint8_t name_len = 10;
    uint8_t service_name[10] = "deviceInfo";

    cJSON *root = NULL;
    cJSON *vendor = NULL;
    cJSON *deviceinfo = NULL;

    if (hilink_auth.hilink_pair_flag == 1) {
        log_info("Error: device has been paired!!!");
        hilink_conn_reject(msg_id, service_name, name_len);
        return;
    }
    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "productId", hilink_info->prodId);
    cJSON_AddStringToObject(root, "sn", hilink_info->sn);

    vendor = cJSON_CreateObject();
    cJSON_AddStringToObject(vendor, "devId", hilink_info->dev_id);

    deviceinfo = cJSON_CreateObject();
    cJSON_AddStringToObject(deviceinfo, "sn", hilink_info->sn);
    cJSON_AddStringToObject(deviceinfo, "model", hilink_info->model);
    cJSON_AddStringToObject(deviceinfo, "dev_t", hilink_info->dev_t);
    cJSON_AddStringToObject(deviceinfo, "manu", hilink_info->manu);
    cJSON_AddStringToObject(deviceinfo, "prodId", hilink_info->prodId);
    cJSON_AddStringToObject(deviceinfo, "mac", hilink_info->mac);
    cJSON_AddStringToObject(deviceinfo, "blemac", hilink_info->mac);
    cJSON_AddStringToObject(deviceinfo, "hiv", hilink_info->hiv);
    cJSON_AddStringToObject(deviceinfo, "fwv", hilink_info->fwv);
    cJSON_AddStringToObject(deviceinfo, "hwv", hilink_info->hwv);
    cJSON_AddStringToObject(deviceinfo, "swv", hilink_info->swv);
    cJSON_AddStringToObject(deviceinfo, "prot_t", hilink_info->prot_t);

    cJSON_AddItemToObject(vendor, "deviceInfo", deviceinfo);
    cJSON_AddItemToObject(root, "vendor", vendor);

    char *cjson_str = cJSON_PrintUnformatted(root);
    uint16_t cjson_len = strlen(cjson_str);

    uint16_t payload_len = 4 + name_len + cjson_len;
    log_info("deviceInfo rsp len:%d, data:\n%s", cjson_len, cjson_str);
    uint8_t *payload = hilink_payload_create(name_len, service_name, cjson_len, cjson_str);

    cJSON_Delete(root);
    hilink_data_rsp(msg_id, payload, payload_len, CMD_TYPE_RSP, MSG_WITHOUT_ENCRY);
    free(cjson_str);
    free(payload);
}

/*
 * 连接鉴权步骤3: authcode下发
 */
static void hilink_authsetup(uint8_t msg_id, uint8_t *data, uint16_t data_len)
{
    log_info("hilink_authsetup");
    uint8_t name_len = 9;
    uint8_t service_name[9] = "authSetup";
    uint8_t tmp_authcode[32];
    cJSON *rsp = NULL;
    rsp = cJSON_CreateObject();

    cJSON *root;
    root = cJSON_Parse(data);
    if (root == NULL) {
        log_info("json pack into cjson error...");
        return;
    }
    memcpy(tmp_authcode, cJSON_GetObjectItem(root, "authCode")->valuestring, 32);
    memcpy(hilink_auth.hilink_authcodeid_tmp, cJSON_GetObjectItem(root, "authCodeId")->valuestring, 32);
    log_info("tmp_authcode:%s", tmp_authcode);
    log_info("tmp_authcodeid:%s", hilink_auth.hilink_authcodeid_tmp);
    cJSON_Delete(root);

    for (int i = 0; i < 16; i++) {
        hilink_auth.hilink_authcode[i] = (hi_ascii_to_hex(tmp_authcode[2 * i]) << 4) + hi_ascii_to_hex(tmp_authcode[2 * i + 1]);
    }
    /* printf("hilink_authcode"); */
    /* put_buf(hilink_auth.hilink_authcode, 16); */

    cJSON_AddStringToObject(rsp, "errcode", "0");
    char *cjson_str = cJSON_PrintUnformatted(rsp);
    uint16_t cjson_len = strlen(cjson_str);
    log_info("authSetup rsp len:%d, cJSON:\n%s", cjson_len, cjson_str);

    uint16_t payload_len = 4 + name_len + cjson_len;
    uint8_t *payload = hilink_payload_create(name_len, service_name, cjson_len, cjson_str);

    hilink_data_rsp(msg_id, payload, payload_len, CMD_TYPE_RSP, MSG_WITHOUT_ENCRY);

    hilink_auth.hilink_pair_flag = 1;
    hilink_set_close_adv(1);

    hilink_auth_info_store();
    cJSON_Delete(rsp);
    free(cjson_str);
    free(payload);

    hilink_set_conn_finish(1);
}

/*
 * 创建会话
 */
static void hilink_createsession(uint8_t msg_id, uint8_t *data, uint16_t data_len)
{
    log_info("hilink_createsession");
    uint8_t name_len = 13;
    uint8_t service_name[13] = "createsession";
    uint8_t uuid[65] =       {0};
    uint8_t sn1[8] =         {0};
    uint8_t sn1_tmp[17] =    {0};
    uint8_t sn2[8] =         {0};
    uint8_t sn2_tmp[17] =    {0};
    uint8_t sessid[32] =     {0};
    uint8_t sessid_tmp[65] = {0};

    if (hilink_auth.hilink_pair_flag != 1) {
        return;
    }
    cJSON *rsp = NULL;
    rsp = cJSON_CreateObject();

    cJSON *root;
    root = cJSON_Parse(data);
    if (root == NULL) {
        log_info("json pack into cjson error...");
        return;
    }
    hilink_seq = cJSON_GetObjectItem(root, "seq")->valueint;
    memcpy(uuid, cJSON_GetObjectItem(root, "uuid")->valuestring, 36);
    memcpy(sn1_tmp, cJSON_GetObjectItem(root, "sn1")->valuestring, 16);

    for (int i = 0; i < 8; i++) {
        sn1[i] = (hi_ascii_to_hex(sn1_tmp[2 * i]) << 4) + hi_ascii_to_hex(sn1_tmp[2 * i + 1]);
    }

    srand(2);
    for (int i = 0; i < 8; i++) {
        sn2[i] = rand() % 15;
        sn2_tmp[2 * i] = hi_hex_to_ascii(sn2[i] >> 4);
        sn2_tmp[2 * i + 1] = hi_hex_to_ascii(sn2[i] & 0x0f);
    }

    for (int i = 0; i < 32; i++) {
        sessid[i] = rand() % 15;
        sessid_tmp[2 * i] = hi_hex_to_ascii(sessid[i] >> 4);
        sessid_tmp[2 * i + 1] = hi_hex_to_ascii(sessid[i] & 0x0f);
    }

    memcpy(&hilink_auth.sessid, sessid, 32);

    cJSON_AddNumberToObject(rsp, "seq", hilink_seq);
    cJSON_AddStringToObject(rsp, "uuid", uuid);
    cJSON_AddStringToObject(rsp, "sessId", sessid_tmp);
    cJSON_AddStringToObject(rsp, "sn2", sn2_tmp);
    cJSON_AddStringToObject(rsp, "authCodeId", hilink_auth.hilink_authcodeid_tmp);

    char *cjson_str = cJSON_PrintUnformatted(rsp);
    uint16_t cjson_len = strlen(cjson_str);
    log_info("createsession rsp len:%d, cJSON:\n%s", cjson_len, cjson_str);

    uint16_t payload_len = 4 + name_len + cjson_len;
    uint8_t *payload = hilink_payload_create(name_len, service_name, cjson_len, cjson_str);

    create_sessionkey(sn1, sn2);

    hilink_data_rsp(msg_id, payload, payload_len, CMD_TYPE_RSP, MSG_WITHOUT_ENCRY);
    cJSON_Delete(root);
    cJSON_Delete(rsp);
    free(cjson_str);
    free(payload);
}


static int hilink_cmd_respond(uint8_t *data, uint16_t data_len, uint8_t msg_id)
{
    log_info("hilink_cmd_respond");
    uint8_t name_len = 13;
    uint8_t service_name[13] = "customSecData";
    uint8_t IV[12];
    uint8_t *input = NULL;
    uint8_t *output = NULL;
    uint16_t body_len;
    uint8_t *body = NULL;

    input = malloc(data_len);
    output = malloc(data_len);

    for (int i = 0; i < 12; i++) {
        IV[i] = rand() % 15;
    }

    memcpy(input, data, data_len);

    unsigned char tag[GCM_TAG_LEN] = {0};
    mbedtls_gcm_context context;
    mbedtls_gcm_init(&context);
    //设置加密使用的 sessionKey。
    mbedtls_gcm_setkey(&context, MBEDTLS_CIPHER_ID_AES, hilink_auth.hilink_sessionkey, AES128_KEY_LENGTH);
    mbedtls_gcm_crypt_and_tag(&context,
                              MBEDTLS_GCM_ENCRYPT,
                              data_len, //需要加密数据的长度
                              IV, //设备侧生成 12 位随机数,拼包时放在 body 的前 12Byte
                              12, //IV 长度 12Byte
                              hilink_info->prodId, //productId，字符串
                              4, //productId 长度，值为 4
                              input, //需要加密的数据
                              output, //加密后的数据
                              GCM_TAG_LEN, //tag 长度 16Byte
                              tag); //tag，拼接在加密数据后面

    // IV + crydata + tag + sessid
    body_len = 12 + data_len + 16 + 32;
    body = malloc(body_len);
    memcpy(body, IV, 12);
    memcpy(body + 12, output, data_len);
    memcpy(body + 12 + data_len, tag, 16);
    memcpy(body + 12 + data_len + 16, hilink_auth.sessid, 32);

    uint16_t payload_len = 4 + name_len + body_len + 32;
    uint8_t *payload = hilink_payload_create_with_hmackey(name_len, service_name, body_len, body);
    hilink_data_rsp(msg_id, payload, payload_len, CMD_TYPE_RSP, MSG_ENCRY);

    mbedtls_gcm_free(&context);
    free(input);
    free(output);
    free(body);
    free(payload);
}

/*
 * 设备控制消息处理
 */
static void hilink_customsecdata(uint8_t msg_id, uint8_t *data, uint16_t data_len)
{
    log_info("hilink_customSecData");

    unsigned char tag[GCM_TAG_LEN] = {0};
    uint8_t IV[12];
    uint16_t encrydata_len = data_len - 12 - 32;
    uint8_t *encrydata = malloc(encrydata_len);
    uint8_t *decrydata = malloc(encrydata_len);

    // 取 body 前 12BYTE 为 IV
    memcpy(IV, data, 12);

    // 跳过 IV 获取加密的数据
    memcpy(encrydata, data + 12, encrydata_len);

    // 取 Encrydata 后 16Byte 生成 tag
    memcpy(tag, data + 12 + encrydata_len - GCM_TAG_LEN, GCM_TAG_LEN);

    // 设置解密使用的 sessionKey
    mbedtls_gcm_context context;
    mbedtls_gcm_init(&context);
    mbedtls_gcm_setkey(&context, MBEDTLS_CIPHER_ID_AES, hilink_auth.hilink_sessionkey, AES128_KEY_LENGTH);
    mbedtls_gcm_auth_decrypt(&context,
                             encrydata_len - GCM_TAG_LEN, // 需要解密数据的长度
                             IV, //IV 值取 body 前 12Byte
                             12, //IV 长度 12Byte
                             hilink_info->prodId, //productId，字符串
                             4, //productId 长度，值为 4
                             tag, //取自 EncryData 末尾 16Byte
                             16, //tag 长度 16Byte
                             encrydata, //Encrydata
                             decrydata); //解密输出的数据
    log_info("decrydata:%s", decrydata);

    cJSON *rsp = NULL;
    rsp = cJSON_CreateObject();
    cJSON_AddStringToObject(rsp, "errcode", "0");
    char *cjson_str = cJSON_PrintUnformatted(rsp);
    uint16_t cjson_len = strlen(cjson_str);

    log_info("hilink_customsecdata rsp len:%d, data:\n%s", cjson_len, cjson_str);
    hilink_cmd_respond(cjson_str, cjson_len, msg_id);

    hilink_cmd_deal(decrydata);

    mbedtls_gcm_free(&context);
    cJSON_Delete(rsp);
    free(cjson_str);
    free(encrydata);
    free(decrydata);
}

/*
 * 设备加解密密钥清理
 */
void hilink_auth_reset()
{
    uint8_t tmp_auth[16];
    //authcode不清理
    memcpy(tmp_auth, hilink_auth.hilink_authcode, 16);
    memset(&hilink_auth, 0, sizeof(hilink_auth));
    memcpy(hilink_auth.hilink_authcode, tmp_auth, 16);
    hilink_auth_info_store();
}

/*
 * 设备重置
 */
static void hilink_factory_reset()
{
    log_info("hilink_factory_reset");
    hilink_msg_id = 0;
    hilink_seq = 0;
    hilink_auth_reset();
    hilink_set_close_adv(1);
}

uint8_t hilink_get_pair_state()
{
    return hilink_auth.hilink_pair_flag;
}

/*
 * 设备恢复出厂设置消息处理
 */
static void hilink_cleardevreginfo(uint8_t msg_id, uint8_t *data, uint16_t data_len)
{
    log_info("hilink_cleardevreginfo");

    unsigned char tag[GCM_TAG_LEN] = {0};
    uint8_t IV[12];
    uint16_t encrydata_len = data_len - 12 - 32;
    uint8_t *encrydata = malloc(encrydata_len);
    uint8_t *decrydata = malloc(encrydata_len);

    // 取 body 前 12BYTE 为 IV
    memcpy(IV, data, 12);

    // 跳过 IV 获取加密的数据
    memcpy(encrydata, data + 12, encrydata_len);

    // 取 Encrydata 后 16Byte 生成 tag
    memcpy(tag, data + 12 + encrydata_len - GCM_TAG_LEN, GCM_TAG_LEN);

    // 设置解密使用的 sessionKey
    mbedtls_gcm_context context;
    mbedtls_gcm_init(&context);
    mbedtls_gcm_setkey(&context, MBEDTLS_CIPHER_ID_AES, hilink_auth.hilink_sessionkey, AES128_KEY_LENGTH);
    mbedtls_gcm_auth_decrypt(&context,
                             encrydata_len - GCM_TAG_LEN, // 需要解密数据的长度
                             IV, //IV 值取 body 前 12Byte
                             12, //IV 长度 12Byte
                             hilink_info->prodId, //productId，字符串
                             4, //productId 长度，值为 4
                             tag, //取自 EncryData 末尾 16Byte
                             16, //tag 长度 16Byte
                             encrydata, //Encrydata
                             decrydata); //解密输出的数据
    log_info("decrydata:%s", decrydata);

    cJSON *rsp = NULL;
    rsp = cJSON_CreateObject();
    cJSON_AddStringToObject(rsp, "errcode", "0");
    char *cjson_str = cJSON_PrintUnformatted(rsp);
    uint16_t cjson_len = strlen(cjson_str);

    log_info("hilink_cleardevreginfo rsp len:%d, data:\n%s", cjson_len, cjson_str);
    hilink_cmd_respond(cjson_str, cjson_len, msg_id);

    hilink_factory_reset();

    cJSON_Delete(rsp);
    mbedtls_gcm_free(&context);
    free(cjson_str);
    free(encrydata);
    free(decrydata);
}

/*
 * 设备消息加密后上报
 */
static void hilink_encry_report_send(uint8_t *data, uint16_t len)
{
    uint8_t service_name[13] = "customSecData";
    uint16_t name_len = 13;
    uint8_t IV[12];
    uint8_t *input = NULL;
    uint8_t *output = NULL;
    uint16_t body_len;
    uint8_t *body = NULL;

    output = malloc(len);

    for (int i = 0; i < 12; i++) {
        IV[i] = rand() % 15;
    }

    unsigned char tag[GCM_TAG_LEN] = {0};
    mbedtls_gcm_context context;
    mbedtls_gcm_init(&context);
    //设置加密使用的 sessionKey。
    mbedtls_gcm_setkey(&context, MBEDTLS_CIPHER_ID_AES, hilink_auth.hilink_sessionkey, AES128_KEY_LENGTH);
    mbedtls_gcm_crypt_and_tag(&context,
                              MBEDTLS_GCM_ENCRYPT,
                              len, //需要加密数据的长度
                              IV, //设备侧生成 12 位随机数,拼包时放在 body 的前 12Byte
                              12, //IV 长度 12Byte
                              hilink_info->prodId, //productId，字符串
                              4, //productId 长度，值为 4
                              data, //需要加密的数据
                              output, //加密后的数据
                              GCM_TAG_LEN, //tag 长度 16Byte
                              tag); //tag，拼接在加密数据后面

    // IV(12) + cjson_data + TAG(16) + sessid(32)
    body_len = 12 + len + 16 + 32;
    body = malloc(body_len);
    memcpy(body, IV, 12);
    memcpy(body + 12, output, len);
    memcpy(body + 12 + len, tag, 16);
    memcpy(body + 12 + len + 16, hilink_auth.sessid, 32);

    uint16_t payload_len = 4 + name_len + body_len + 32;
    uint8_t *payload = hilink_payload_create_with_hmackey(name_len, service_name, body_len, body);

    uint8_t msg_id = get_hilink_msg_id();
    hilink_data_rsp(msg_id, payload, payload_len, CMD_TYPE_RPT, MSG_ENCRY);

    mbedtls_gcm_free(&context);
    free(output);
    free(body);
    free(payload);
}

/*
 * 设备状态上报,本实例为开关,只上报开关状态
 */
static int hilink_state_report()
{
    cJSON *root = cJSON_CreateObject();

    cJSON_AddNumberToObject(root, "seq", get_hilink_seq());

    cJSON *vendor = cJSON_CreateObject();
    cJSON_AddStringToObject(vendor, "sid", "switch");

    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "on", hilink_attr.onoff);

    cJSON_AddItemToObject(vendor, "data", data);
    cJSON_AddItemToObject(root, "vendor", vendor);

    char *cjson_str = cJSON_PrintUnformatted(root);
    uint16_t cjson_len = strlen(cjson_str);

    log_info("hilink_state_report len:%d, data:\n%s", cjson_len, cjson_str);

    hilink_encry_report_send(cjson_str, cjson_len);

    cJSON_Delete(root);
    free(cjson_str);

    return 0;
}

/*
 * 设备时间戳上报
 */
static int hilink_time_info_report()
{
    cJSON *root = cJSON_CreateObject();

    cJSON_AddNumberToObject(root, "seq", get_hilink_seq());

    cJSON *vendor = cJSON_CreateObject();
    cJSON_AddStringToObject(vendor, "sid", "time");

    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "time", hilink_auth.time);

    cJSON_AddItemToObject(vendor, "data", data);
    cJSON_AddItemToObject(root, "vendor", vendor);

    char *cjson_str = cJSON_PrintUnformatted(root);
    uint16_t cjson_len = strlen(cjson_str);

    log_info("hilink_time_info_report len:%d, data:\n%s", cjson_len, cjson_str);

    hilink_encry_report_send(cjson_str, cjson_len);

    cJSON_Delete(root);
    free(cjson_str);
}

/*
 * 设备开关状态设置并上报
 */
void hilink_set_report()
{
    hilink_attr.onoff = !hilink_attr.onoff;
    hilink_led_set(hilink_attr.onoff);
    log_info("hilink led set to:%d", hilink_attr.onoff);
    if (hilink_auth.hilink_pair_flag && hilink_get_ble_conn_state()) {
        hilink_state_report();
    }
    hilink_attr_info_store();
}

/*
 * hilink消息接收处理函数
 */
static int hilink_msg_prase(uint8_t *buf, uint16_t len)
{
    log_info("hilink_msg_prase\n");

    uint8_t *service_name = NULL;
    uint8_t *body = NULL;
    uint8_t service_len = 0;
    uint16_t body_len = 0;
    HILINK_MSG_T *msg_head = NULL;
    msg_head = buf;

    log_info("version:     %d", msg_head->version);
    log_info("cmd_type:    %d", msg_head->cmd_type);
    log_info("msg_id:      %d", msg_head->msg_id);
    log_info("total_frame: %d", msg_head->total_frame);
    log_info("frame_seq:   %d", msg_head->frame_seq);
    log_info("rev:         %d", msg_head->rev);
    log_info("encry:       %d", msg_head->encry);
    log_info("ret:         %d", msg_head->ret);
    log_info("payload_rev: %d", msg_head->data[0]);
    log_info("service_len: %d", msg_head->data[1]);

    hilink_msg_id = msg_head->msg_id;

    service_len = msg_head->data[1];
    service_name = malloc(service_len + 1);
    memset(service_name, 0, service_len + 1);
    memcpy(service_name, &msg_head->data[2], service_len);
    log_info("service_name:%s", service_name);

    body_len = msg_head->data[2 + service_len] + (msg_head->data[3 + service_len] << 8);
    log_info("body_len:%d\n", body_len);
    if (body_len <= 0) {
        log_info("this msg is no body");
    } else {
        body = malloc(body_len);
        memcpy(body, &msg_head->data[4 + service_len], body_len);
        if (msg_head->encry = 0) {
            log_info("body:%s", &msg_head->data[4 + service_len]);
        }
    }

    // 根据消息的json数据服务名确认消息类型
    if (memcmp(service_name,        "netCfgVer", service_len) == 0) {
        hilink_netcfgver(msg_head->msg_id);
    } else if (memcmp(service_name, "deviceInfo", service_len) == 0) {
        hilink_deviceinfo(msg_head->msg_id);
    } else if (memcmp(service_name, "authSetup", service_len) == 0) {
        hilink_authsetup(msg_head->msg_id, &msg_head->data[4 + service_len], body_len);
    } else if (memcmp(service_name, "createSession", service_len) == 0) {
        hilink_createsession(msg_head->msg_id, &msg_head->data[4 + service_len], body_len);
    } else if (memcmp(service_name, "customSecData", service_len) == 0) {
        hilink_customsecdata(msg_head->msg_id, &msg_head->data[4 + service_len], body_len);
    } else if (memcmp(service_name, "clearDevRegInfo", service_len) == 0) {
        hilink_cleardevreginfo(msg_head->msg_id, &msg_head->data[4 + service_len], body_len);
    } else {
        log_info("unknow msg service_name");
    }

    if (service_name) {
        free(service_name);
    }
    if (body) {
        free(body);
    }
}


/*
 * hilink消息预处理函数,根据消息是否分段消息进行组包或者直接解析
 */
void hilink_msg_handle(uint8_t *buf, uint16_t len)
{
    uint16_t body_len = 0;
    uint16_t service_len = 0;
    HILINK_MSG_T *msg_head = buf;

    // 单独包处理
    if (msg_head->total_frame == 1) {
        hilink_msg_prase(buf, len);
    }

    // 分段包处理
    if (msg_head->total_frame > 1) {
        log_info("seg msg: %d/%d ", msg_head->frame_seq + 1, msg_head->total_frame);
        if (msg_head->frame_seq == 0) {
            service_len = msg_head->data[1];
            body_len = msg_head->data[2 + service_len] + (msg_head->data[3 + service_len] << 8);
            hi_msg_store.msg_id = msg_head->msg_id;
            if (msg_head->encry == 0x03) {
                hi_msg_store.data_len = body_len + service_len + 11 + 32;
            } else {
                hi_msg_store.data_len = body_len + service_len + 11;
            }
            hi_msg_store.data = malloc(hi_msg_store.data_len);
            memcpy(hi_msg_store.data, msg_head, 7);
            hi_msg_store.data_index += 7;
        }

        memcpy(&hi_msg_store.data[hi_msg_store.data_index], &msg_head->data[0], len - 7);
        hi_msg_store.data_index += len - 7;

        if (msg_head->frame_seq + 1 == msg_head->total_frame) {
            if (hi_msg_store.data_index == hi_msg_store.data_len) {
                hilink_msg_prase(hi_msg_store.data, hi_msg_store.data_len);
                free(hi_msg_store.data);
                hi_msg_store.data_index = 0;
            } else {
                log_info("seg msg assemble error, len: %d/%d", hi_msg_store.data_index, hi_msg_store.data_len);
                free(hi_msg_store.data);
            }
        }
    }
}

/*
 * hilink命令处理
 */
static int hilink_cmd_deal(uint8_t *buf)
{
    cJSON *root = cJSON_Parse(buf);
    if (root == NULL) {
        log_info("json pack into cjson error...");
        return -1;
    }
    cJSON *vendor = cJSON_GetObjectItem(root, "vendor");
    if (vendor == NULL) {
        printf("root no vendor");
        return -1;
    }
    cJSON *sid = cJSON_GetObjectItem(vendor, "sid");
    if (sid == NULL) {
        printf("vendor no sid");
        return -1;
    }
    cJSON *data = cJSON_GetObjectItem(vendor, "data");
    if (data == NULL) {
        printf("vendor no data");
        return -1;
    }
    cJSON *seq = cJSON_GetObjectItem(root, "seq");
    if (seq == NULL) {
        printf("root no seq");
        return -1;
    }
    hilink_seq = seq->valueint;
    log_info("sid:%s", sid->valuestring);

    if (memcmp(sid->valuestring, "switch", 6) == 0) {
        cJSON *state = cJSON_GetObjectItem(data, "on");
        hilink_attr.onoff = state->valueint;
        log_info("led set to %d", hilink_attr.onoff);
        hilink_led_set(hilink_attr.onoff);
        hilink_state_report();
        hilink_attr_info_store();
    } else if (memcmp(sid->valuestring, "deviceTime", 6) == 0) {
        cJSON *time = cJSON_GetObjectItem(data, "time");
        memcpy(hilink_auth.time, time->valuestring, 12);
        log_info("time sync:%s", hilink_auth.time);
        hilink_time_info_report();
        hilink_state_report();
        hilink_set_conn_finish(1);
    } else {
        log_info("unknow sid:%s", sid->valuestring);
    }
    cJSON_Delete(root);
    return 0;
}

