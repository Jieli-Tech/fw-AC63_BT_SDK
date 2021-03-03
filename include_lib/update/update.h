#ifndef _UPDATE_H_
#define _UPDATE_H_

#include "typedef.h"

extern u32 UPDATA_BEG;

#define UPDATA_FLAG_ADDR		((void *)((u32)&UPDATA_BEG + 0x08))
#define BOOT_STATUS_ADDR		((void *)((u32)&UPDATA_BEG)) //预留8个bytes

#define UPDATA_MAGIC            (0x5A00)        //防止CRC == 0 的情况

typedef enum {
    UPDATA_NON = UPDATA_MAGIC,
    UPDATA_READY,
    UPDATA_SUCC,
    UPDATA_PARM_ERR,
    UPDATA_DEV_ERR,
    UPDATA_KEY_ERR,
} UPDATA_RESULT;

typedef enum {
    USB_UPDATA = UPDATA_MAGIC,      //0x5A00
    SD0_UPDATA,                     //0x5A01
    SD1_UPDATA,
    PC_UPDATA,
    UART_UPDATA,
    BT_UPDATA,
    BLE_APP_UPDATA,
    SPP_APP_UPDATA,
    DUAL_BANK_UPDATA,
    BLE_TEST_UPDATA,
    NORFLASH_UPDATA,

    NON_DEV = 0xFFFF,
} UPDATA_TYPE;

// sd
enum {
    SD_CONTROLLER_0 = 1,
    SD_CONTROLLER_1,
};
enum {
    SD0_IO_A = 1,
    SD0_IO_B,
    SD1_IO_A,
    SD1_IO_B,
    SD0_IO_C,
    SD0_IO_D,
    SD0_IO_E,
    SD0_IO_F,
};
typedef struct _UPDATA_SD {
    u8 control_type;
    u8 control_io;
    u8 online_check_way;
    u8 max_data_baud;
    u16 wDevTimeOutMax;
    u8 per_online_status;
    u8 hc_mode;
    u8(*io_det_func)(void);
    u8 power;
} UPDATA_SD;

// uart
typedef struct _UPDATA_UART {
    u32  control_io_tx;      //<IO口对接
    u32  control_io_rx;      //<IO口对接
    u32  control_baud;        //<波特率
    u32  control_timeout;  //<超时，单位10ms
} UPDATA_UART; /*共12个bytes*/


#define UPDATE_PARAM_MAGIC		0x5441
#if (USE_SDFILE_NEW == 1)
typedef struct _UPDATA_PARM {
    u16 parm_crc;
    u16 parm_type;              //UPDATA_TYPE:sdk pass parm to uboot
    u16 parm_result;            //UPDATA_TYPE:uboot return result to sdk
    u16 magic;					//0x5441
    u8  file_patch[32];         //updata file patch
    u8  parm_priv[32];          //sd updata
    u32 ota_addr;
    u16 ext_arg_len;
    u16 ext_arg_crc;
} UPDATA_PARM;
#else
typedef struct _UPDATA_PARM {
    u16 parm_crc;
    u16 parm_type;              //UPDATA_TYPE:sdk pass parm to uboot
    u16 parm_result;            //UPDATA_TYPE:uboot return result to sdk
    u8  file_patch[32];         //updata file patch
    u8  parm_priv[32];          //sd updata
} UPDATA_PARM;
#endif
#define UPDATE_PRIV_PARAM_LEN	32

void update_mode_api(UPDATA_TYPE type, ...);
u16 update_result_get(void);
bool device_is_first_start();
int update_result_deal();
void update_result_set(u16 result);
bool update_success_boot_check(void);

typedef u8(*update_handler_t)(void);

struct update_target {
    char *name;
    update_handler_t driver_close;
};

#define REGISTER_UPDATE_TARGET(target) \
        const struct update_target target sec(.update_target)


extern const struct update_target update_target_begin[];
extern const struct update_target update_target_end[];

#define list_for_each_update_target(p) \
    for (p = update_target_begin; p < update_target_end; p++)

#endif
