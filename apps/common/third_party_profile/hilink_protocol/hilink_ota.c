#include "hilink_ota.h"
#include "dual_bank_updata_api.h"

#if 1
extern void printf_buf(u8 *buf, u32 len);
#define log_info(x, ...)  printf("[HILINK_OTA]" x " ", ## __VA_ARGS__)
#define log_info_hexdump  printf_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

static void ota_reply(uint8_t clean_map);
extern dev_info_t *hilink_info;
extern uint16_t hilink_mtu;

static int old_sys_clk = 0;
static uint16_t offset_seg_id;

static uint8_t file_hash_value1[64];
static uint8_t file_hash_value2[64];
static uint8_t ota_signature[256];

static hilink_ota_t hilink_ota;
static hilink_hash_list_t hilink_hash;
static hilink_signature_t hilink_signature;

// module_device_name, module_sub_device_name填写方式:
// ProdID的数字前面加0，字母前面加1
// 例如ProdID为2hrs,实际应该填021h1r1s
static __ota_device_info ota_device_info = {
    .module_device_name = "021h1r1s",
    .module_sub_device_name = "021h1r1s",
    .device_type = "0",
    .device_id = "1234567890AB",
    .model_firmware_version = "1.0.0",
    .mcu_device_name = "0",
    .mcu_sub_device_name = "0",
    .mcu_device_type = "0",
    .mcu_device_id = "0",
    .mcu_firmware_version = "0",
};

extern u8 dual_bank_update_verify_without_crc(void);

static void hilink_ota_rsp_send(uint16_t op, uint8_t *buf, uint16_t len)
{
    uint8_t *data = malloc(len + 2);

    uint8_t opcode[2];
    opcode[0] = op >> 8;
    opcode[1] = op & 0xff;

    memcpy(&data[0], opcode, 2);
    memcpy(&data[2], buf, len);
    log_info("hilink_ota_rsp_send len:%d", len + 2);
    put_buf(data, len + 2);

    hilink_ota_data_send(data, len + 2);
    free(data);
}

static void hilink_ota_info_report()
{
    log_info("hilink_ota_info_report");
    uint8_t mdn_len = strlen(ota_device_info.module_device_name) + 1;
    uint8_t msdn_len = strlen(ota_device_info.module_sub_device_name) + 1;
    uint8_t mdt_len = strlen(ota_device_info.device_type) + 1;
    uint8_t mdi_len = strlen(ota_device_info.device_id) + 1;
    uint8_t mfv_len = strlen(ota_device_info.model_firmware_version) + 1;
    uint8_t mcu_dn_len = strlen(ota_device_info.mcu_device_name) + 1;
    uint8_t mcu_sdn_len = strlen(ota_device_info.mcu_sub_device_name) + 1;
    uint8_t mcu_dt_len = strlen(ota_device_info.mcu_device_type) + 1;
    uint8_t mcu_di_len = strlen(ota_device_info.mcu_device_id) + 1;
    uint8_t mcu_fv_len = strlen(ota_device_info.mcu_firmware_version) + 1;

    log_info("module_device_name:%s", ota_device_info.module_device_name);
    log_info("module_sub_device_name:%s", ota_device_info.module_sub_device_name);
    log_info("module_device_type:%s", ota_device_info.device_type);
    log_info("module_devce_id:%s", ota_device_info.device_id);
    log_info("module_firm_ver:%s", ota_device_info.model_firmware_version);
    log_info("mcu_device_name:%s", ota_device_info.mcu_device_name);
    log_info("mcu_sub_device_name:%s", ota_device_info.mcu_sub_device_name);
    log_info("mcu_device_type:%s", ota_device_info.mcu_device_type);
    log_info("mcu_device_id:%s", ota_device_info.mcu_device_id);
    log_info("mcu_firmware_version:%s", ota_device_info.mcu_firmware_version);

    uint8_t data_len = mdn_len + msdn_len + mdt_len + mdi_len + mfv_len + \
                       mcu_dn_len + mcu_sdn_len + mcu_dt_len + mcu_di_len + mcu_fv_len + 2 * 10;

    uint8_t *data = malloc(data_len);

    uint8_t *data_ptr = data;

    __ota_tlv *module_dev_name = malloc(2 + mdn_len);
    module_dev_name->type = 0x00;
    module_dev_name->len = mdn_len;
    memcpy(module_dev_name->value, ota_device_info.module_device_name, mdn_len);
    memcpy(data_ptr, module_dev_name, mdn_len + 2);
    data_ptr += mdn_len + 2;
    free(module_dev_name);

    __ota_tlv *module_sub_dev_name = malloc(2 + msdn_len);
    module_sub_dev_name->type = 0x01;
    module_sub_dev_name->len = msdn_len;
    memcpy(module_sub_dev_name->value, ota_device_info.module_sub_device_name, msdn_len);
    memcpy(data_ptr, module_sub_dev_name, msdn_len + 2);
    data_ptr += msdn_len + 2;
    free(module_sub_dev_name);

    __ota_tlv *module_dev_type = malloc(2 + mdt_len);
    module_dev_type->type = 0x02;
    module_dev_type->len = mdt_len;
    memcpy(module_dev_type->value, ota_device_info.device_type, mdt_len);
    memcpy(data_ptr, module_dev_type, mdt_len + 2);
    data_ptr += mdt_len + 2;
    free(module_dev_type);

    __ota_tlv *module_dev_id = malloc(2 + mdi_len);
    module_dev_id->type = 0x03;
    module_dev_id->len = mdi_len;
    memcpy(module_dev_id->value, ota_device_info.device_id, mdi_len);
    memcpy(data_ptr, module_dev_id, mdi_len + 2);
    data_ptr += mdi_len + 2;
    free(module_dev_id);

    __ota_tlv *module_firm_ver = malloc(2 + mfv_len);
    module_firm_ver->type = 0x04;
    module_firm_ver->len = mfv_len;
    memcpy(module_firm_ver->value, ota_device_info.model_firmware_version, mfv_len);
    memcpy(data_ptr, module_firm_ver, mfv_len + 2);
    data_ptr += mfv_len + 2;
    free(module_firm_ver);

    __ota_tlv *mcu_dev_name = malloc(2 + mcu_dn_len);
    mcu_dev_name->type = 0x10;
    mcu_dev_name->len = mcu_dn_len;
    memcpy(mcu_dev_name->value, ota_device_info.mcu_device_name, 1);
    memcpy(data_ptr, mcu_dev_name, 2 + mcu_dn_len);
    data_ptr += 2 + mcu_dn_len;
    free(mcu_dev_name);

    __ota_tlv *mcu_sub_dev_name = malloc(2 + mcu_sdn_len);
    mcu_sub_dev_name->type = 0x11;
    mcu_sub_dev_name->len = mcu_sdn_len;
    memcpy(mcu_sub_dev_name->value, ota_device_info.mcu_sub_device_name, 1);
    memcpy(data_ptr, mcu_sub_dev_name, 2 + mcu_sdn_len);
    data_ptr += 2 + mcu_sdn_len;
    free(mcu_sub_dev_name);

    __ota_tlv *mcu_dev_type = malloc(2 + mcu_dt_len);
    mcu_dev_type->type = 0x12;
    mcu_dev_type->len = mcu_dt_len;
    memcpy(mcu_dev_type->value, ota_device_info.mcu_device_type, 1);
    memcpy(data_ptr, mcu_dev_type, 2 + mcu_dt_len);
    data_ptr += 2 + mcu_dt_len;
    free(mcu_dev_type);

    __ota_tlv *mcu_dev_id = malloc(2 + mcu_di_len);
    mcu_dev_id->type = 0x13;
    mcu_dev_id->len = mcu_di_len;
    memcpy(mcu_dev_id->value, ota_device_info.mcu_device_id, 1);
    memcpy(data_ptr, mcu_dev_id, 2 + mcu_di_len);
    data_ptr += 2 + mcu_di_len;
    free(mcu_dev_id);

    __ota_tlv *mcu_firm_ver = malloc(2 + mcu_fv_len);
    mcu_firm_ver->type = 0x14;
    mcu_firm_ver->len = mcu_fv_len;
    memcpy(mcu_firm_ver->value, ota_device_info.mcu_firmware_version, 1);
    memcpy(data_ptr, mcu_firm_ver, 2 + mcu_fv_len);
    data_ptr += 2 + mcu_fv_len;
    free(mcu_firm_ver);

    hilink_ota_rsp_send(HILINK_OTA_CTL_DEV_INFO_RPT, data, data_len);
    free(data);
}

static void hilink_ota_max_cache_cnt_rsp()
{
    log_info("hilink_ota_max_cache_cnt_rsp");
    uint8_t cache_num = MAX_CACHE_CNT;
    hilink_ota_rsp_send(HILINK_OTA_CTL_MAX_CACHE_CNT_RSP, &cache_num, 1);
}

static void hilink_ota_prepare()
{
    log_info("hilink_ota_prepare");
    if (hilink_ota.ota_timer) {
        sys_timer_remove(hilink_ota.ota_timer);
        hilink_ota.ota_timer = 0;
    }
    hilink_ota.expect_seg_id = 0;
    if (!hilink_ota.ota_ready) {
        hilink_ota.hilink_ota_store_buf = malloc(OTA_WRITE_BUF_SIZE);
    }
    hilink_ota.ota_ready = 1;
    offset_seg_id = 0;
    hilink_ota.recv_len = 0;
}

void hilink_ota_head_info_rsp(uint8_t *data, uint16_t len)
{
    uint8_t tlv_type;
    uint8_t tlv_len;
    uint8_t *tlv_value;
    uint8_t ret = HILINK_OTA_ERROR_NONE;
    uint8_t *info_ptr = data;
    uint32_t file_size = 0;

    while (info_ptr < data + len - 1) {
        tlv_type = info_ptr[0];
        tlv_len = info_ptr[1];
        tlv_value = malloc(tlv_len);
        memcpy(tlv_value, &info_ptr[2], tlv_len);
        switch (tlv_type) {
        case HILINK_OTA_NEGOTIATE_PID:
            log_info("pid:%s", tlv_value);
            if (memcmp(hilink_info->prodId, tlv_value, tlv_len)) {
                ret = HILINK_OTA_ERROR_PID_NO_MATCH;
                goto NEGOTIATE_RET;
            }
            break;
        case HILINK_OTA_NEGOTIATE_FIRM_VER:
            log_info("firm ver:%s", tlv_value);
            if (memcmp(hilink_info->swv, tlv_value + 1, tlv_len - 1) == 0) {
                log_info("Device's ver is the same as the ota ver, no need ota!");
                ret = HILINK_OTA_ERROR_VER_SAME;
                goto NEGOTIATE_RET;
            }
            break;
        case HILINK_OTA_NEGOTIATE_FIRM_SIZE:
            for (int i = 0; i < tlv_len; i ++) {
                file_size += tlv_value[i] << (8 * (tlv_len - i - 1));
            }
            log_info("OTA file size:%d", file_size);
            break;
        case HILINK_OTA_NEGOTIATE_MCU_VER:
            if (!HILINK_MCU) {
                break;
            }
            log_info("mcu ver:%d", tlv_value);
            break;
        case HILINK_OTA_NEGOTIATE_MCU_SIZE:
            if (!HILINK_MCU) {
                break;
            }
            log_info("mcu size:%s", tlv_value);
            if (0) {
                goto NEGOTIATE_RET;
            }
            break;
        default:
            log_info("unknow negotiate type:0x%x", tlv_type);
            break;
        }
        info_ptr += 2 + tlv_len;
        free(tlv_value);
    }
    dual_bank_passive_update_init(0, file_size, OTA_WRITE_BUF_SIZE, NULL);
    if (dual_bank_update_allow_check(file_size) != 0) {
        log_info("have no enought space for ota file!");
        dual_bank_passive_update_exit(NULL);
        ret = HILINK_OTA_ERROR_FILE_SIZE_ERR;
    }
    hilink_ota.file_size = file_size;
NEGOTIATE_RET:
    if (ret != 0) {
        log_info("hilink_ota check error:%d", ret);
    } else {
        hilink_ota_prepare();
    }
    hilink_ota_rsp_send(HILINK_OTA_CTL_NEGOTIATE_RESULT, &ret, 1);
}

static void hilink_hash_parse(uint8_t *data)
{
    log_info("hilink_hash_list:");
    log_info("str:%s", data);
    char pid[5] = {0};
    char version[7] = {0};
    char hash_1[7] = {0};
    char hash_2[7] = {0};
    char hash_value_1[65] = {0};
    char hash_value_2[65] = {0};

    cJSON *root;
    root = cJSON_Parse(data);
    if (root == NULL) {
        log_info("json pack into cjson error...");
        return;
    }

    memcpy(pid, cJSON_GetObjectItem(root, "productId")->valuestring, 4);
    memcpy(version, cJSON_GetObjectItem(root, "version")->valuestring, 6);

    cJSON *file_1;
    file_1 = cJSON_GetObjectItem(root, "image2_all_ota2.bin");
    memcpy(hash_1, cJSON_GetObjectItem(file_1, "hash")->valuestring, 6);
    memcpy(hash_value_1, cJSON_GetObjectItem(file_1, "value")->valuestring, 64);

    cJSON *file_2;
    file_2 = cJSON_GetObjectItem(root, "image2_all_ota1.bin");
    memcpy(hash_2, cJSON_GetObjectItem(file_2, "hash")->valuestring, 6);
    memcpy(hash_value_2, cJSON_GetObjectItem(file_2, "value")->valuestring, 64);

    for (int i = 0; i < 64; i ++) {
        file_hash_value1[i] = hi_ascii_to_hex(hash_value_1[i]);
        file_hash_value2[i] = hi_ascii_to_hex(hash_value_2[i]);
    }
    cJSON_Delete(root);
    free(hilink_hash.data);
}

static void ota_reply(uint8_t clean_map)
{
    log_info("ota_msg_reply");
    uint8_t data[10];
    data[0] = (hilink_ota.expect_seg_id & 0xff00) >> 8;
    data[1] = hilink_ota.expect_seg_id & 0xff;
    memcpy(&data[2], hilink_ota.ota_bit_map, 8);

    hilink_ota_rsp_send(HILINK_OTA_CTL_SEG_RSP, data, 10);
    if (clean_map) {
        memset(hilink_ota.ota_bit_map, 0, 8);
    }
}

static void ota_hash_list_get(uint8_t *data, uint16_t len)
{
    uint16_t payload_len;
    uint16_t seg_id = (data[0] << 8) + data[1];
    log_info("ota_hash_list package[%d]", seg_id);

    if (seg_id == 0) {
        hilink_hash.data_index = 0;
        hilink_hash.len = (data[7] << 24) + (data[8] << 16) + (data[9] << 8) + data[10];
        hilink_hash.data = malloc(hilink_hash.len);
        payload_len = len - 11;
        memcpy(&hilink_hash.data[hilink_hash.data_index], &data[11], payload_len);
        hilink_hash.data_index += payload_len;
    } else {
        payload_len = len - 2;
        memcpy(&hilink_hash.data[hilink_hash.data_index], &data[2], payload_len);
        hilink_hash.data_index += payload_len;
    }

    if (hilink_hash.data_index == hilink_hash.len) {
        hilink_hash_parse(hilink_hash.data);
        ota_reply(1);
        sys_timer_re_run(hilink_ota.ota_timer);
    }
}

static void ota_signature_get(uint8_t *data, uint16_t len)
{
    uint16_t payload_len;
    uint16_t seg_id = (data[0] << 8) + data[1];
    log_info("ota_signature package[%d]", seg_id);
    if (seg_id == 0) {
        hilink_signature.data_index = 0;
        hilink_signature.len = (data[7] << 24) + (data[8] << 16) + (data[9] << 8) + data[10];
        hilink_signature.data = malloc(hilink_signature.len);
        payload_len = len - 11;
        memcpy(&hilink_signature.data[hilink_signature.data_index], &data[11], payload_len);
        hilink_signature.data_index += payload_len;
    } else {
        payload_len = len - 2;
        memcpy(&hilink_signature.data[hilink_signature.data_index], &data[2], payload_len);
        hilink_signature.data_index += payload_len;
    }

    if (hilink_signature.data_index == 256) {
        memcpy(ota_signature, hilink_signature.data, 256);
        /* log_info("ota_signature:"); */
        /* put_buf(ota_signature, 256); */
        free(hilink_signature.data);
        ota_reply(1);
        sys_timer_re_run(hilink_ota.ota_timer);
    }
}

static uint8_t map_check_cnt_get()
{
    if (MAX_CACHE_CNT % 8) {
        return (MAX_CACHE_CNT / 8) + 1;
    } else {
        return MAX_CACHE_CNT / 8;
    }
}

static uint8_t ota_check_map_full()
{
    uint8_t set_bit;
    uint8_t check_map_byte;
    int i, j;
    int check_cnt = map_check_cnt_get();

    for (i = 0; i < check_cnt; i ++) {
        check_map_byte = 0;
        if (check_cnt > 1 && (i < (MAX_CACHE_CNT / 8))) {
            check_map_byte = 0xff;
        } else {
            set_bit = MAX_CACHE_CNT % 8;
            for (j = 0; j < set_bit; j++) {
                check_map_byte |= BIT(7 - j);
            }
        }
        /* log_info("check_map_byte:0x%x, ota_bit_map[%d]:0x%x", check_map_byte, i, hilink_ota.ota_bit_map[i]); */
        if ((hilink_ota.ota_bit_map[i] & check_map_byte) != check_map_byte) {
            return 0;
        }
    }
    return 1;
}

void hilink_ota_error_report(uint8_t errcode)
{
    hilink_ota_rsp_send(HILINK_OTA_CTL_ERROR_RSP, errcode, 1);
}

int hilink_ota_boot_info_cb(int err)
{
    uint8_t ret = err;
    clk_set("sys", old_sys_clk);     //恢复时钟
    sys_timeout_add(NULL, cpu_reset, 2000);
    hilink_ota_rsp_send(HILINK_OTA_CTL_UPDATE_RESULT, &ret, 1);
    return err;
}

int hlink_ota_file_end_response(void *priv)
{
    old_sys_clk = clk_get("sys");
    int ret = 0;

    clk_set("sys", 120 * 1000000L);     //提升系统时钟提高校验速度
    uint8_t result = dual_bank_update_verify_without_crc();
    if (result == 0) {
        log_info("UPDATE SUCCESS");
        ret = 0;
        dual_bank_update_burn_boot_info(hilink_ota_boot_info_cb);
    } else {
        log_info("UPDATE FAILURE:%d", result);
        ret = 0x02;
        clk_set("sys", old_sys_clk);     //恢复时钟
        hilink_ota_rsp_send(HILINK_OTA_CTL_UPDATE_RESULT, &ret, 1);
    }
    return ret;
}

static int hilink_update_write_cb(void *priv)
{
    local_irq_disable();
    uint8_t ret = (int)priv;
    if (ret) {
        log_info("hilink_update_write error:%d", ret);
        hilink_ota_error_report(0x03);
    } else {
        ota_reply(1);
        hilink_ota.expect_seg_id += MAX_CACHE_CNT;
        offset_seg_id += MAX_CACHE_CNT;
        hilink_ota.buff_size = 0;
    }
    local_irq_enable();
    return ret;
}

static void ota_data_store(uint8_t *data, uint16_t len)
{
    uint16_t seg_id = (data[0] << 8) + data[1];
    uint8_t first_turn = seg_id < MAX_CACHE_CNT ? 1 : 0;
    log_info("ota_data package[%d]", seg_id);

    if (seg_id == 0) {
        hilink_ota.buff_size += len - 11;
        hilink_ota.recv_len += len - 11;
        memcpy(hilink_ota.hilink_ota_store_buf, &data[11], len - 11);
    } else if (first_turn) {
        hilink_ota.buff_size += len - 2;
        hilink_ota.recv_len += len - 2;

        // 第一轮下发的第一包的数据比较短 151byte
        uint16_t write_index = 151 + 160 * (seg_id - 1);
        memcpy(&hilink_ota.hilink_ota_store_buf[write_index], &data[2], len - 2);
    } else {
        hilink_ota.buff_size += len - 2;
        hilink_ota.recv_len += len - 2;

        uint16_t write_index = 160 * (seg_id % MAX_CACHE_CNT);
        memcpy(&hilink_ota.hilink_ota_store_buf[write_index], &data[2], len - 2);
    }

    if (ota_check_map_full()) {
        log_info("Get this turn all package! write flash, len:%d", hilink_ota.buff_size);
        //put_buf(hilink_ota.hilink_ota_store_buf, hilink_ota.buff_size);
        dual_bank_update_write(hilink_ota.hilink_ota_store_buf, hilink_ota.buff_size, hilink_update_write_cb);
    }

    log_info("recv_len:%d, file_size:%d", hilink_ota.recv_len, hilink_ota.file_size);

    // 收包完毕
    if (hilink_ota.recv_len == hilink_ota.file_size) {
        log_info("----ota data transfer complete!");
        if (seg_id == 0) {
            // 最后一包长度减去seg_id长度
            dual_bank_update_write(hilink_ota.hilink_ota_store_buf, len - 2, hlink_ota_file_end_response);
        } else if (seg_id == 1) {
            // 第一包长度 + 最后一包长度中data部分
            dual_bank_update_write(hilink_ota.hilink_ota_store_buf, OTA_DATA_SIZE + len - 2, hlink_ota_file_end_response);
        } else {
            // 第一包长度 + 中间包长度 + 最后一包长度中data部分
            dual_bank_update_write(hilink_ota.hilink_ota_store_buf, OTA_DATA_SIZE * (seg_id % MAX_CACHE_CNT) + len - 2, hlink_ota_file_end_response);
        }
        sys_timer_remove(hilink_ota.ota_timer);
    }
}

void hilink_ota_data_deal(u8 *data, u32 len)
{
    /* log_info("hilink_ota_data_deal,len:%d", len); */
    /* put_buf(data, len); */

    uint16_t seg_id = (data[0] << 8) + data[1];
    uint8_t set_byte = (seg_id - offset_seg_id) / 8;
    uint8_t set_bit = 7 - ((seg_id - offset_seg_id) % 8);
    log_info("seg_id:%d", seg_id);
    if ((int)seg_id - (int)offset_seg_id < 0) {
        log_info("get msg seg_id error, offset_seg_id:%d, seg_id:%d", offset_seg_id, seg_id);
        return;
    }
    hilink_ota.ota_bit_map[set_byte] |= BIT(set_bit);
    //log_info("seg_id:%d, bitmap set_byte:0x%x, set_bit:0x%x", seg_id, set_byte, set_bit);
    //log_info("ota_bit_map[%d]:0x%x", set_byte, hilink_ota.ota_bit_map[set_byte]);

    if (seg_id == 0) {
        hilink_ota.current_msg_type = data[6];
    }
    switch (hilink_ota.current_msg_type) {
    // Module Hash List
    case 0x00:
        ota_hash_list_get(data, len);
        break;
    // Module signature
    case 0x01:
        ota_signature_get(data, len);
        break;
    // Module File
    case 0x02:
        ota_data_store(data, len);
        break;
    default:
        log_info("hilink_ota.current_msg_type erorr:0x%x", hilink_ota.current_msg_type);
        break;
    }

    if (hilink_ota.ota_timer) {
        sys_timer_re_run(hilink_ota.ota_timer);
    } else {
        hilink_ota.ota_timer = sys_timer_add(0, ota_reply, OTA_BUF_TIMEOUT);
    }
}

void hilink_ota_ctl_deal(u8 *data, u32 len)
{
    uint16_t cmd = (data[0] << 8) + data[1];
    log_info("ota ctl cmd:0x%04x, data_len:%d", cmd, len);
    put_buf(data, len);

    switch (cmd) {
    // 0001
    case HILINK_OTA_CTL_MAX_CACHE_CNT:
        hilink_ota_max_cache_cnt_rsp();
        break;
    // 0002
    case HILINK_OTA_CTL_HEAD_INFO:
        hilink_ota_head_info_rsp(&data[2], len - 2);
        break;
    // 0004
    case HILINK_OTA_CTL_DEV_INFO_REQ:
        hilink_ota_info_report();
        break;
    }
}

void hilink_ota_exit()
{
    log_info("hilink_ota_exit");
    if (hilink_ota.ota_timer) {
        sys_timer_remove(hilink_ota.ota_timer);
        hilink_ota.ota_timer = 0;
    }
    hilink_ota.expect_seg_id = 0;
    hilink_ota.ota_ready = 0;
    hilink_ota.buff_size = 0;
    dual_bank_passive_update_exit(NULL);
    if (hilink_ota.hilink_ota_store_buf) {
        free(hilink_ota.hilink_ota_store_buf);
    }
}

