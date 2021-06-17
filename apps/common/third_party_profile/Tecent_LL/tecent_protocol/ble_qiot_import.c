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

int ble_get_product_id(char *product_id)
{
    log_info("ble_get_product_id");
    memcpy(product_id, PRODUCT_ID, strlen(PRODUCT_ID));
    put_buf(product_id, strlen(product_id));
    return 0;
}

int ble_get_device_name(char *device_name)
{
    log_info("ble_get_device_name");
    memcpy(device_name, DEVICE_NAME, strlen(DEVICE_NAME));
    put_buf(device_name, strlen(device_name));
    return strlen(device_name);
}

int ble_get_psk(char *psk)
{
    log_info("ble_get_psk");
    memcpy(psk, SECRET_KEY, strlen(SECRET_KEY));
    put_buf(psk, strlen(psk));
    return 0;
}

extern const u8 *ble_get_mac_addr(void);
int ble_get_mac(char *mac)
{
    log_info("ble_get_mac");
    memcpy(mac, ble_get_mac_addr(), 6);
    put_buf(mac, 6);
    return 0;
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
    ble_op_set_adv_data(offset, buf);
    return 0;
}

ble_qiot_ret_status_t ble_advertising_start(adv_info_s *adv)
{
    log_info("ble_advertising_start\n");
    //log_info_hexdump(adv->manufacturer_info.adv_data, adv->manufacturer_info.adv_data_len);

    llsync_set_adv_data(adv);
    llsync_set_rsp_data();

    int ret;
    ret = ble_op_adv_enable(1);
    if (ret) {
        log_info("ble_op_adv_enable error:%d\n", ret);
    }
    return 0;
}

ble_qiot_ret_status_t ble_advertising_stop(void)
{
    log_info("ble_advertising_stop\n");
    ble_op_adv_enable(0);
    return 0;
}

extern int llsync_app_send_user_data_do(void *priv, u8 *data, u16 len);
ble_qiot_ret_status_t ble_send_notify(uint8_t *buf, uint8_t len)
{
    llsync_app_send_user_data_do(NULL, buf, len);
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

