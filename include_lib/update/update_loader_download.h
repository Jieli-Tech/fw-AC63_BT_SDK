#ifndef _UPDATE_LOADER_DOWNLOAD_H_
#define _UPDATE_LOADER_DOWNLOAD_H_

#include "typedef.h"

extern const int config_update_mode;
#define UPDATE_MODULE_IS_SUPPORT(x) 		(config_update_mode & x)
#define UPDATE_SUPPORT_DEV_IS_NULL()		(config_update_mode == UPDATE_DEV_NULL)

struct __tws_ota_para {
    u32 fm_size;
    u16 fm_crc16;
    u16 max_pkt_len;
};

typedef struct _update_op_api_tws {
    //for tws ota start
    int (*tws_ota_start)(void *priv);
    int (*tws_ota_data_send)(u8 *buf, u16 len);
    int (*tws_ota_err)(u8);
    u16(*enter_verfiy_hdl)(void *priv);
    u16(*exit_verify_hdl)(u8 *, u8 *);
    u16(*update_boot_info_hdl)(void *priv);
    int (*tws_ota_result_hdl)(u8);
    int (*tws_ota_data_send_pend)(void);
    //for tws ota end
} update_op_tws_api_t;  //给tws同步升级用的接口

typedef struct _update_op_api_t {
    void (*ch_init)(void (*resume_hdl)(void *priv), int (*sleep_hdl)(void *priv));
    u16(*f_open)(void);
    u16(*f_read)(void *fp, u8 *buff, u16 len);
    int (*f_seek)(void *fp, u8 type, u32 offset);
    u16(*f_stop)(u8 err);
    int (*notify_update_content_size)(void *priv, u32 size);
} update_op_api_t;

extern const update_op_api_t lmp_ch_update_op;
extern const update_op_api_t strg_ch_update_op;
extern const update_op_api_t rcsp_update_op;

#define UPDATE_SEAGNMENT_EN		1

enum {
    UPDATE_LOADER_OK  = 1,
    UPDATE_LOADER_ERR,
};

//update result code bitmap
#define UPDATE_RESULT_FLAG_BITMAP 		BIT(7)

//update result code;
enum {
    UPDATE_RESULT_FILE_SIZE_ERR = 0x1, //文件大小错误
    UPDATE_RESULT_LOADER_SIZE_ERR = 0x2, //loader大小错误
    UPDATE_RESULT_LOADER_VERIFY_ERR,    //update loader校验失败
    UPDATE_RESULT_REMOTE_FILE_HEAD_ERR, //读升级文件头错误

    UPDATE_RESULT_LOCAL_FILE_HEAD_ERR = 0x5, //读flash文件头错误
    UPDATE_RESULT_NOT_FIND_TARGET_FILE_ERR, //找不到目标文件
    UPDATE_RESULT_FILE_OPERATION_ERR,       //文件操作失败
    UPDATE_RESULT_FLASH_DATA_VERIFY_ERR,    //flash数据校验失败

    UPDATE_RESULT_UBOOT_NOT_MATCH = 0x09,  //UBOOT不匹配
    UPDATE_RESULT_PRODUCT_INFO_NOT_MATCH = 0x0a, //芯片型号不匹配
    UPDATE_RESULT_EX_DSP_UPDATE_ERR,		//外部IC升级出错;
    UPDATE_RESULT_CFG_UPDATE_ERR,			//配置升级出错

    UPDATE_RESULT_FLASH_ERASE_ERR = 0x0d,	//flash 擦失败(可能是写保护)
    UPDATE_RESULT_REMOTE_FILE_NOT_MATCH,    //升级文件不匹配
    UPDATE_RESULT_OTA_TWS_NO_RSP,
};

typedef struct _update_type_info_t {
    int type;
    u8  task_en;
    void (*cb)(void *priv, int type, u8 cmd);
    void *cb_priv;
    update_op_api_t *p_op_api;
} update_type_info_t;

#define UPDATE_DEV_NULL			0
#define UPDATE_BT_LMP_EN		BIT(0)
#define UPDATE_STORAGE_DEV_EN	BIT(1)
#define UPDATE_UART_EN			BIT(2)
#define UPDATE_APP_EN	     	BIT(3)          //包括APP升级还有其他升级方式，如串口升级（非测试盒方式）
#define UPDATE_BLE_TEST_EN		BIT(4)



void bt_lmp_update_loader_download_init(void);
void ble_test_update_loader_download_init(void);
void storage_update_loader_download_init(
    int  type,
    char *update_path,
    void (*cb)(void *priv, int type, u8 cmd),
    void *cb_priv,
    u8 task_en
);
void rcsp_update_loader_download_init(int update_type, void (*result_cbk)(void *priv, u8 type, u8 cmd));

void app_update_loader_downloader_init(
    int update_type,
    void (*result_cbk)(void *priv, u8 type, u8 cmd),
    void *cbk_priv,
    update_op_api_t *p_op_api);

void update_tws_api_register(const update_op_tws_api_t *op);
#endif /*_UPDATE_LOADER_DOWNLOAD_H_*/
