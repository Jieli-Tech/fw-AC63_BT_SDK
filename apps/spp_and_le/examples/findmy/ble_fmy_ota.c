/*********************************************************************************************
    *   Filename        : ble_fmy_ota.c

    *   Description     :

    *   Author          : JM

    *   Email           : zh-jieli.com

    *   Last modifiled  : 2024-03-05 15:14

    *   Copyright:(c)JIELI  2011-2026  @ , All Rights Reserved.
*********************************************************************************************/
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
#include "btstack/btstack_event.h"
#include "gatt_common/le_gatt_common.h"
#include "ble_fmy.h"
#include "ble_fmy_ota.h"
#include "system/malloc.h"

//#define rr_printf(x, ...)  printf("\e[31m\e[1m" x "\e[0m", ## __VA_ARGS__)
//#define gg_printf(x, ...)  printf("\e[32m\e[1m" x "\e[0m", ## __VA_ARGS__)
//#define yy_printf(x, ...)  printf("\e[33m\e[1m" x "\e[0m", ## __VA_ARGS__)

#if LE_DEBUG_PRINT_EN
#define log_info(x, ...)  printf("[FMY_OTA]" x "\r\n", ## __VA_ARGS__)
#define log_error(x, ...) printf("[FMY_OTA_ERR]" x "\r\n", ## __VA_ARGS__)
#define log_info_hexdump  put_buf

#else
#define log_info(...)
#define log_error(...)
#define log_info_hexdump(...)
#endif

#if FMY_OTA_SUPPORT_CONFIG

//OTA每笔写入flash的数据大小
#define OTA_WRITE_FLASH_SIZE       (1024)

typedef struct {
    volatile uint32_t rx_len;
    uint32_t file_size;
    uint32_t version;
    uint16_t ota_packet_num;
    volatile uint8_t ota_state;
    uint8_t res_byte;
    uint8_t  buff[OTA_WRITE_FLASH_SIZE + 16];
} fmy_ota_t;

static fmy_ota_t *fmy_ota;

#define FMY_OTA_IS_ENABLE()   (fmy_ota != NULL)

//--------------------------------------------------------------------------------------------
static void fmy_ota_set_state(uint8_t state);
//--------------------------------------------------------------------------------------------
/*************************************************************************************************/
/*!
 *  \brief      ota 模块初始化
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int fmy_ota_init(void)
{
    log_info("%s", __FUNCTION__);
    dual_bank_passive_update_exit(NULL);
    if (!fmy_ota) {
        fmy_ota = malloc(sizeof(fmy_ota_t));
    }
    if (fmy_ota) {
        memset(fmy_ota, 0, sizeof(fmy_ota_t));
        set_ota_status(1);
        fmy_state_idle_set_active(true);
        return FMNA_OTA_OP_SUCC;
    }
    log_error("%s malloc fail", __FUNCTION__);
    return FMNA_OTA_OP_MALLOC_FAIL;
}

/*************************************************************************************************/
/*!
 *  \brief  ota模块退出
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int fmy_ota_exit(void)
{
    log_info("%s", __FUNCTION__);
    dual_bank_passive_update_exit(NULL);
    set_ota_status(0);
    if (fmy_ota) {
        local_irq_disable();
        free(fmy_ota);
        fmy_ota = NULL;
        local_irq_enable();
    }
    fmy_ota_set_state(FMY_OTA_STATE_IDLE);
    fmy_state_idle_set_active(false);
    return FMNA_OTA_OP_SUCC;
}

/*************************************************************************************************/
/*!
 *  \brief      ota状态更新
 *
 *  \param      [in] state
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void fmy_ota_set_state(uint8_t state)
{
    if (FMY_OTA_IS_ENABLE()) {
        fmy_ota->ota_state = state;
    }
}

/*************************************************************************************************/
/*!
 *  \brief      获取ota状态
 *
 *  \param      [in]
 *
 *  \return     fmy_ota_st_e
 *
 *  \note
 */
/*************************************************************************************************/
static fmy_ota_st_e fmy_ota_get_state(void)
{
    if (FMY_OTA_IS_ENABLE()) {
        return fmy_ota->ota_state;
    }
    return FMY_OTA_STATE_IDLE;
}

/*************************************************************************************************/
/*!
 *  \brief      ota 完成 重启系统
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void fmy_ota_reset(void *priv)
{
    log_info("cpu_reset!");
    cpu_reset();
}

/*************************************************************************************************/
/*!
 *  \brief      升级启动文件检测回调
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int fmy_ota_boot_info_cb(int err)
{
    log_info("fmy_ota_boot_info_cb:%d", err);
    if (err == 0) {
        sys_timeout_add(NULL, fmy_ota_reset, 2000);
    } else {
        log_error("update head fail");
    }
    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      检测ota文件crc回调
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int fmy_clk_resume(int priv)
{
    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      传输文件数据写入结束
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int fmy_ota_write_complete(void)
{
    int status = FMNA_OTA_OP_SUCC;
    if (dual_bank_update_verify_without_crc(fmy_clk_resume) == 0) {
        log_info("update succ");
        fmy_ota_set_state(FMY_OTA_STATE_IDLE);
        dual_bank_update_burn_boot_info(fmy_ota_boot_info_cb);
    } else {
        log_error("update fail");
        status = FMNA_OTA_OP_CRC_FAIL;
    }

    set_ota_status(0);
    return status;
}

/*************************************************************************************************/
/*!
 *  \brief      每一笔数据写入flash的回调
 *
 *  \param      [in] err id
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int fmy_update_write_cb(int err)
{
    log_info("flash write %u bytes Complete,err %d", fmy_ota->rx_len, err);

    if (err) {
        fmy_ota_set_state(FMY_OTA_STATE_WRITE_ERROR);
    }

    fmy_ota->rx_len = 0;
    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      ota升级主要消息流程处理
 *
 *  \param      [in] cmd_type, recv_data,recv_len
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
int fmy_ota_process(uarp_cmd_type_t cmd_type, uint8_t *recv_data, uint32_t recv_len)
{
    int ret = 0;
    int status = FMNA_OTA_OP_SUCC;
    log_info("cmd_type= %d,len= %u", cmd_type, recv_len);

//检测配置
    if (!FMY_OTA_SUPPORT_CONFIG) {
        log_error("OTA config err!");
        return FMNA_OTA_OP_OTHER_ERR;
    }

//检测设备的能力
    if (false == fmy_check_capabilities_is_enalbe(FMY_CAPABILITY_SUPPORTS_FW_UPDATE_SERVICE)) {
        log_error("OTA not support!");
        return  FMNA_OTA_OP_INIT_FAIL;
    }

//检测OTA初始化是否处理
    if (!FMY_OTA_IS_ENABLE() && cmd_type != FMNA_UARP_OTA_REQ) {
        log_error("OTA not running!");
        return  FMNA_OTA_OP_INIT_FAIL;
    }

    switch (cmd_type) {
    case FMNA_UARP_OTA_REQ:
//请求升级命令
        status = fmy_ota_init();
        if (status == FMNA_OTA_OP_SUCC) {
            fmy_ota_set_state(FMY_OTA_STATE_REQ);
            fmy_ota->version = little_endian_read_32(recv_data, 0);
            log_info("OTA_REQ,version= %08x", fmy_ota->version);
        }
        break;

    case FMNA_UARP_OTA_FILE_INFO:
//升级文件信息下发，检测空间
        fmy_ota_set_state(FMY_OTA_STATE_CHECK_FILESIZE);
        fmy_ota->file_size = little_endian_read_32(recv_data, 0);
        log_info("OTA_FILE_INFO file_size= %u", fmy_ota->file_size);
        ret = dual_bank_passive_update_init(0, fmy_ota->file_size, OTA_WRITE_FLASH_SIZE, NULL);

        if (0 == ret) {
            ret = dual_bank_update_allow_check(fmy_ota->file_size);
            if (ret) {
                log_error("check err: %d", ret);
//检测空间不足
                status = FMNA_OTA_OP_NO_SPACE;
            }
        } else {
            log_error("init err: %d", ret);
            status = FMNA_OTA_OP_INIT_FAIL;
        }
        break;

    case FMNA_UARP_OTA_DATA: {
//每一笔升级文件数据写入
        if (fmy_ota_get_state() == FMY_OTA_STATE_WRITE_ERROR) {
            ret = FMNA_OTA_OP_WRITE_FAIL;
            goto write_end;
        }

        int wait_cnt = 200;//wait flash write complete, timeout is 2 second
        while (wait_cnt && fmy_ota->rx_len >= OTA_WRITE_FLASH_SIZE) {
            putchar('&');
            wait_cnt--;
            os_time_dly(1);
        }

        if (fmy_ota->rx_len && wait_cnt == 0) {
            ret = FMNA_OTA_OP_OTHER_ERR;
            goto write_end;
        }

        fmy_ota_set_state(FMY_OTA_STATE_WRITE_DATA);
        fmy_ota->ota_packet_num++;
        memcpy(&fmy_ota->buff[fmy_ota->rx_len], recv_data, recv_len);
        fmy_ota->rx_len += recv_len;
        log_info("OTA_DATA: file_size= %u,packet_num= %d,rx_len= %u", \
                 fmy_ota->file_size, fmy_ota->ota_packet_num, fmy_ota->rx_len);

        log_info_hexdump(fmy_ota->buff, 32);
        ret = dual_bank_update_write(fmy_ota->buff, fmy_ota->rx_len, fmy_update_write_cb);

write_end:
        if (ret) {
            log_error("dual_write err %d", ret);
            status = FMNA_OTA_OP_WRITE_FAIL;
            /* fmy_ota_boot_info_cb(0);//to reset */
        }
    }
    break;

    case FMNA_UARP_OTA_END:
//传输文件结束
        log_info("OTA_END");
        fmy_ota_set_state(FMY_OTA_STATE_COMPLETE);
        status = fmy_ota_write_complete();
        break;

    case FMNA_UARP_OTA_DISCONNECT:
//链路断开
        log_info("OTA_DISCONNECT");
        if (fmy_ota_get_state() != FMY_OTA_STATE_IDLE) {
            log_error("OTA fail");
            /* fmy_ota_boot_info_cb(0); //to reset */
        }
        status = fmy_ota_exit();
        break;

    default:
        log_error("unknow uarp cmd");
        break;

    }

    if (status) {
        log_error("ota_process err= %d", status);
        fmy_ota_exit();
    }
    return status;
}

#endif


