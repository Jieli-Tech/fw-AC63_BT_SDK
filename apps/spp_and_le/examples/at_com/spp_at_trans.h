#ifndef __SPP_AT_TRANS_H__
#define __SPP_AT_TRANS_H__

#include "typedef.h"
#include "bt_common.h"

int edr_at_set_address(u8 *addr);
int edr_at_get_address(u8 *addr);
int edr_at_set_name(u8 *name, u8 len);
int edr_at_get_name(u8 *name);
int edr_at_set_visibility(u8 inquiry_en, u8 page_scan_en);
int edr_at_set_pair_mode(u8 mode);
int edr_at_set_pincode(u8 *pincode);
int edr_at_disconnect(void);
int edr_at_set_cod(u8 *cod_data);
int edr_at_send_spp_data(u8 *data, u8 len);
void edr_at_register_event_cbk(void *cbk);
void at_spp_init(void);
u8 edr_at_get_staus(void);

#endif//__SPP_USER_H__
