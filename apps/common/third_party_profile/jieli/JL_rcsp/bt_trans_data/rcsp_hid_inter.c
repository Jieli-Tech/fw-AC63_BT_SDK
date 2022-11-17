#include "app_config.h"
#include "rcsp_hid_inter.h"
#include "usb/device/custom_hid.h"
#include "custom_cfg.h"
#include "JL_rcsp_api.h"

static void (*rcsp_hid_recieve_callback)(void *priv, void *buf, u16 len) = NULL;

void rcsp_hid_recieve(void *priv, void *buf, u16 len)
{
    if (rcsp_hid_recieve_callback) {
        rcsp_hid_recieve_callback(priv, buf, len);
    }
}

bool JL_rcsp_hid_fw_ready(void *priv)
{
#if TCFG_USB_CUSTOM_HID_ENABLE
    return custom_hid_get_ready(0) ? true : false;
#else
    return 0;
#endif
}

static int update_send_user_data_do(void *priv, void *data, u16 len)
{
#if TCFG_USB_CUSTOM_HID_ENABLE
    //-------------------!!!!!!!!!!考虑关闭RCSP_BTMATE_EN使能编译报错
    extern void dongle_send_data_to_pc_3(u8 * data, u16 len);
    dongle_send_data_to_pc_3(data, len);
#endif
    return 0;
}

static int update_regiest_recieve_cbk(void *priv, void *cbk)
{
    printf("%s, %x\n", __func__, cbk);
    rcsp_hid_recieve_callback = cbk;
    return 0;
}

static int update_regiest_state_cbk(void *priv, void *cbk)
{
    return 0;
}

static const struct rcsp_hid_operation_t rcsp_hid_update_operation = {
    .send_data = update_send_user_data_do,
    .regist_recieve_cbk = update_regiest_recieve_cbk,
    .regist_state_cbk = update_regiest_state_cbk,
};

void rcsp_hid_get_operation_table(struct rcsp_hid_operation_t **interface_pt)
{
    *interface_pt = (void *)&rcsp_hid_update_operation;
}

u8 rcsp_hid_auth_flag_get(void)
{
#if (0 == BT_CONNECTION_VERIFY)
    return JL_rcsp_get_auth_flag();
#else
    return 0;
#endif
}

