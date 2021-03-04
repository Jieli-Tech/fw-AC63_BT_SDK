#include "init.h"
#include "app_config.h"
#include "system/includes.h"
#include "asm/chargestore.h"
#include "user_cfg.h"
#include "app_chargestore.h"
#include "app_action.h"
#include "app_main.h"

#define LOG_TAG_CONST       APP_CHARGESTORE
#define LOG_TAG             "[APP_CHARGESTORE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_TEST_BOX_ENABLE

#define CMD_BOX_UPDATE              0x20//测试盒升级
#define CMD_UNDEFINE                0xff//未知命令回复

extern u8 get_jl_chip_id(void);
extern u8 get_jl_chip_id2(void);
void app_chargestore_data_deal(u8 *buf, u8 len)
{
    u8 send_buf[2];
    send_buf[0] = buf[0];
    switch (buf[0]) {
    case CMD_BOX_UPDATE:
        //0x02是USB updata串口升级命令
        if (buf[13] == 0x02 || buf[13] == get_jl_chip_id() || buf[13] == get_jl_chip_id2()) {
            chargestore_set_update_ram();
            cpu_reset();
        } else {
            send_buf[1] = 0x01;//chip id err
            chargestore_api_write(send_buf, 2);
        }
        break;
    default:
        send_buf[0] = CMD_UNDEFINE;
        chargestore_api_write(send_buf, 1);
        break;
    }
}

void chargestore_clear_connect_status(void)
{
}

u8 chargestore_get_testbox_status(void)
{
    return 0;
}

CHARGESTORE_PLATFORM_DATA_BEGIN(chargestore_data)
.io_port                = TCFG_CHARGESTORE_PORT,
 CHARGESTORE_PLATFORM_DATA_END()

 __BANK_INIT
 int app_chargestore_init(void)
{
    chargestore_api_init(&chargestore_data);
    return 0;
}
__initcall(app_chargestore_init);

#endif

