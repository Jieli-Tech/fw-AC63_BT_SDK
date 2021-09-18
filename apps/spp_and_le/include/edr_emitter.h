#ifndef _EDR_EMTTER_H
#define _EDR_EMTTER_H

#define    BT_EMITTER_EN     1
/* #define    BT_RECEIVER_EN    2 */


void bt_emitter_init(void);
void bt_search_device(void);
void bt_emitter_search_noname(u8 status, u8 *addr, u8 *name);
void bt_emitter_search_complete(u8 result);
void bt_emitter_role_set(u8 flag);
void bt_emitter_start_search_device(void);
void bt_emitter_stop_search_device(void);
u8   bt_emitter_search_result(char *name, u8 name_len, u8 *addr, u32 dev_class, char rssi);
void bt_emitter_set_match_name(const char *name_table, u8 name_count);

// #define bt_em_debug_trace()  r_printf("%s: %d\n",__FUNCTION__,__LINE__)

#endif
