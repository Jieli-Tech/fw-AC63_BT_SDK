#ifndef _RCSP_HID_INTER_H_
#define _RCSP_HID_INTER_H_

#include "typedef.h"

struct rcsp_hid_operation_t {
    int (*send_data)(void *priv, void *buf, u16 len);
    int(*regist_recieve_cbk)(void *priv, void *cbk);
    int(*regist_state_cbk)(void *priv, void *cbk);
};

bool JL_rcsp_hid_fw_ready(void *priv);
void rcsp_hid_get_operation_table(struct rcsp_hid_operation_t **interface_pt);

void rcsp_hid_recieve(void *priv, void *buf, u16 len);
u8 rcsp_hid_auth_flag_get(void);

#endif
