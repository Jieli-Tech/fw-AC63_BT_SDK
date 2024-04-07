#include "ble_qiot_import.h"
/* #include "le/ble_api.h" */
#include "vm.h"
#include "le_common.h"
#include "dual_bank_updata_api.h"

#define PRODUCT_ID "YSUM4IEDOH"
#define DEVICE_NAME "dev001"
#define SECRET_KEY "6WNLgmVK4fThhVIgdOBmKQ=="

#define LOG_TAG             "[ble_qiot_import]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define ADV_INTERVAL_MIN          (160)

static u8 scan_rsp_data[ADV_RSP_PACKET_MAX];//max is 31
static char d_name[7] = "LLMAIN";
static u8 d_name_len = 6; //名字长度，不包含结束符
static u8 ble_work_state = 0;      //ble 状态变化
static u8 adv_ctrl_en;             //广播控制
static u16 protocol_MTU;             //广播控制

static u8 adv_data_len;
static u8 adv_data[ADV_RSP_PACKET_MAX];//max is 31
static u8 scan_rsp_data_len;
static u8 scan_rsp_data[ADV_RSP_PACKET_MAX];//max is 31

#define LL_SYNC_INFO    1

#if LL_SYNC_INFO
#define LLSYNC_DEVICE_NAME 6
uint8_t ll_sync_product_id[10] = "0QA0PL5DIV";
uint8_t ll_sync_device_name[LLSYNC_DEVICE_NAME] = "dev001";
uint8_t ll_sync_device_secret[24] = "0A2hmQBKnaImsZKSYctR/Q==";
#else
#define LLSYNC_DEVICE_NAME 6
static uint8_t ll_sync_product_id[10] = "0";
static uint8_t ll_sync_device_name[LLSYNC_DEVICE_NAME] = "0";
static uint8_t ll_sync_device_secret[24] = "0";
#endif /* LL_SYNC_INFO */

#define LLSYNC_LEGAL_CHAR(c)       ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '+') || (c == '/') || (c == '=') || (c == '-'))
#define LLSYNC_LIC_OFFSET       80

typedef struct __llsync_flash_of_lic_para_head {
    s16 crc;
    u16 string_len;
    const u8 para_string[];
} __attribute__((packed)) _llsync_flash_of_lic_para_head;

static u16 llsync_get_one_info(const u8 *in, u8 *out)
{
    int read_len = 0;
    const u8 *p = in;

    while (LLSYNC_LEGAL_CHAR(*p) && *p != ',') { //read product_uuid
        *out++ = *p++;
        read_len++;
    }
    return read_len;
}

static bool license_para_head_check(u8 *para)
{
    _llsync_flash_of_lic_para_head *head;

    //fill head
    head = (_llsync_flash_of_lic_para_head *)para;

    ///crc check
    u8 *crc_data = (u8 *)(para + sizeof(((_llsync_flash_of_lic_para_head *)0)->crc));
    u32 crc_len = sizeof(_llsync_flash_of_lic_para_head) - sizeof(((_llsync_flash_of_lic_para_head *)0)->crc)/*head crc*/ + (head->string_len)/*content crc,include end character '\0'*/;
    s16 crc_sum = 0;

    crc_sum = CRC16(crc_data, crc_len);

    if (crc_sum != head->crc) {
        printf("license crc error !!! %x %x \n", (u32)crc_sum, (u32)head->crc);
        return false;
    }

    return true;
}

const u8 *llsync_get_license_ptr(void)
{
    u32 flash_capacity = sdfile_get_disk_capacity();
    u32 flash_addr = flash_capacity - 256 + LLSYNC_LIC_OFFSET;
    u8 *lic_ptr = NULL;
    _llsync_flash_of_lic_para_head *head;

    printf("flash capacity:%x \n", flash_capacity);
    lic_ptr = (u8 *)sdfile_flash_addr2cpu_addr(flash_addr);

    //head length check
    head = (_llsync_flash_of_lic_para_head *)lic_ptr;
    if (head->string_len >= 0xff) {
        printf("license length error !!! \n");
        return NULL;
    }

    ////crc check
    if (license_para_head_check(lic_ptr) == (false)) {
        printf("license head check fail\n");
        return NULL;
    }

    //put_buf(lic_ptr, 128);

    lic_ptr += sizeof(_llsync_flash_of_lic_para_head);
    return lic_ptr;
}

static uint8_t read_llsync_product_info_from_flash(uint8_t *read_buf, u16 buflen)
{
    uint8_t *rp = read_buf;
    const uint8_t *llsync_ptr = (uint8_t *)llsync_get_license_ptr();
    //printf("llsync_ptr:");
    //put_buf(llsync_ptr, 69);

    if (llsync_ptr == NULL) {
        return FALSE;
    }
    int data_len = 0;
    data_len = llsync_get_one_info(llsync_ptr, rp);
    printf("product_id:");
    put_buf(rp, data_len);
    if (data_len != 10) {
        printf("read product_id err, data_len:%d", data_len);
        put_buf(rp, data_len);
        return FALSE;
    }
    llsync_ptr += 10 + 1;

    rp = read_buf + 10;

    data_len = llsync_get_one_info(llsync_ptr, rp);
    printf("name:");
    put_buf(rp, data_len);
    if (data_len != LLSYNC_DEVICE_NAME) {
        printf("read device_name(mac) err, data_len:%d", data_len);
        put_buf(rp, data_len);
        return FALSE;
    }
    llsync_ptr += LLSYNC_DEVICE_NAME + 1;

    rp = read_buf + 10 + LLSYNC_DEVICE_NAME;
    data_len = llsync_get_one_info(llsync_ptr, rp);
    printf("psk:");
    put_buf(rp, data_len);
    if (data_len != 24) {
        printf("read psk err, data_len:%d", data_len);
        put_buf(rp, data_len);
        return FALSE;
    }
    llsync_ptr += 24 + 1;

    return TRUE;
}

void llsync_dev_info_get()
{
    int ret;
    u8 read_buf[10 + 12 + 24 + 1] = {0};
    // add your code here
#if LL_SYNC_INFO
    ret = TRUE;
#else
    ret = read_llsync_product_info_from_flash(read_buf, sizeof(read_buf));
    if (ret == TRUE) {
        memcpy(ll_sync_product_id, read_buf, 10);
        memcpy(ll_sync_device_name, read_buf + 10, LLSYNC_DEVICE_NAME);
        memcpy(ll_sync_device_secret, read_buf + 10 + LLSYNC_DEVICE_NAME, 24);
        printf("llsync read license success:%s\n", read_buf);
    }
#endif /* LL_SYNC_INFO */
}


int ble_get_product_id(char *product_id)
{
    log_info("ble_get_product_id");
    memcpy(product_id, ll_sync_product_id, 10);
    put_buf(product_id, 10);
    return 0;
}

int ble_get_device_name(char *device_name)
{
    log_info("ble_get_device_name");
    memcpy(device_name, ll_sync_device_name, LLSYNC_DEVICE_NAME);
    put_buf(device_name, LLSYNC_DEVICE_NAME);
    return LLSYNC_DEVICE_NAME;
}

int ble_get_psk(char *psk)
{
    log_info("ble_get_psk");
    memcpy(psk, ll_sync_device_secret, 24);
    put_buf(psk, 24);
    return 0;
}

extern const u8 *ble_get_mac_addr(void);
int ble_get_mac(char *mac)
{
    log_info("ble_get_mac");
    le_controller_get_mac(mac);
    put_buf(mac, 6);
    return 0;
}

static void (*app_set_adv_data)(u8 *adv_data, u8 adv_len) = NULL;
static void (*app_set_rsp_data)(u8 *rsp_data, u8 rsp_len) = NULL;
void app_set_adv_data_register(void (*handler)(u8 *adv_data, u8 adv_len))
{
    app_set_adv_data = handler;
}
void app_set_rsp_data_register(void (*handler)(u8 *rsp_data, u8 rsp_len))
{
    app_set_rsp_data = handler;
}

static int llsync_set_rsp_data(void)
{
    u8 offset = 0;
    u8 *buf = scan_rsp_data;

    u8 name_len = sizeof(d_name);
    u8 vaild_len = ADV_RSP_PACKET_MAX - (offset + 2);
    if (name_len > vaild_len) {
        name_len = vaild_len;
    }
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)d_name, d_name_len);

    if (offset > ADV_RSP_PACKET_MAX) {
        printf("***rsp_data overflow!!!!!!\n");
        return -1;
    }

    //printf("rsp_data(%d):", offset);
    //printf_buf(buf, offset);
    scan_rsp_data_len = offset;
    if (app_set_rsp_data) {
        app_set_rsp_data(scan_rsp_data, scan_rsp_data_len);
    }
    ble_op_set_rsp_data(offset, buf);
    return 0;
}

static int llsync_set_adv_data(adv_info_s *adv)
{
    u8 offset = 0;
    u8 *buf = adv_data;
    u8 manufacturer_buf[20];

    memcpy(&manufacturer_buf[0], &adv->manufacturer_info.company_identifier, 2);
    memcpy(&manufacturer_buf[2], adv->manufacturer_info.adv_data, adv->manufacturer_info.adv_data_len);
    printf("manufacturer_buf\n");
    put_buf(manufacturer_buf, adv->manufacturer_info.adv_data_len + 2);

    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, 0x06, 1);
    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_16BIT_SERVICE_UUIDS, IOT_BLE_UUID_SERVICE, 2);
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_MANUFACTURER_SPECIFIC_DATA, manufacturer_buf, adv->manufacturer_info.adv_data_len + 2);

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***adv_data overflow!!!!!!\n");
        printf("offset = %d, limite is %d\n", offset, ADV_RSP_PACKET_MAX);
        return -1;
    }

    //log_info("adv_data(%d):", offset);
    //log_info_hexdump(buf, offset);
    adv_data_len = offset;
    if (app_set_adv_data) {
        app_set_adv_data(adv_data, adv_data_len);
    }
    ble_op_set_adv_data(offset, buf);
    return 0;
}

static void (*llsync_ble_module_enable)(u8 en) = NULL;
ble_qiot_ret_status_t ble_advertising_start(adv_info_s *adv)
{
    log_info("ble_advertising_start\n");
    //log_info_hexdump(adv->manufacturer_info.adv_data, adv->manufacturer_info.adv_data_len);

    llsync_set_adv_data(adv);
    llsync_set_rsp_data();

    if (llsync_ble_module_enable) {
        llsync_ble_module_enable(1);
    }
    return 0;
}

void llsync_ble_module_enable_register(void (*handler)(u8 en))
{
    llsync_ble_module_enable = handler;
}

ble_qiot_ret_status_t ble_advertising_stop(void)
{
    log_info("ble_advertising_stop\n");
    if (llsync_ble_module_enable) {
        llsync_ble_module_enable(0);
    }
    return 0;
}

static int (*llsync_send_data)(void *priv, u8 *data, u16 len) = NULL;
void llsync_send_data_register(int (*handler)(void *priv, void *buf, u16 len))
{
    llsync_send_data = handler;
}

ble_qiot_ret_status_t ble_send_notify(uint8_t *buf, uint8_t len)
{
    if (llsync_send_data) {
        llsync_send_data(NULL, buf, len);
    } else {
        log_info("llsync_send_data no register");
    }
    return 0;
}

uint16_t ble_get_user_data_mtu_size(void)
{
    return BLE_QIOT_PACKAGE_LENGTH + 6;
}

uint8_t ble_ota_is_enable(const char *version, u32 file_size, u32 file_crc)
{
    log_info("ota version: %s, enable ota", version);
    if (dual_bank_passive_update_init(file_crc, file_size, BLE_QIOT_OTA_BUF_SIZE, NULL) == 0) {
        if (dual_bank_update_allow_check(file_size) == 0) {
            return BLE_OTA_ENABLE;
        }
    }
    dual_bank_passive_update_exit(NULL);
    return 0;
}

extern s32 vm_open(u16 index);
extern s32 vm_read(vm_hdl hdl, u8 *data_buf, u16 len);
extern s32 vm_write(vm_hdl hdl, u8 *data_buf, u16 len);

int ble_read_flash(uint32_t flash_addr, char *read_buf, uint16_t read_len)
{
    int ret;
    s32 hdl = vm_open(flash_addr);

    ret = vm_read(hdl, read_buf, read_len);
    if (ret < 0) {
        log_info("\n\nread flash error:%d", ret);
    }
    return read_len;
}

int ble_write_flash(uint32_t flash_addr, const char *write_buf, uint16_t write_len)
{
    int ret;
    s32 hdl = vm_open(flash_addr);

    ret = vm_write(hdl, write_buf, write_len);
    if (ret < 0) {
        log_info("\n\nwrite flash error:%d", ret);
    }
    return write_len;
}

