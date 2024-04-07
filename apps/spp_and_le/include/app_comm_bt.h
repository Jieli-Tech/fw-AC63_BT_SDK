#ifndef _APP_COMM_BT_H_
#define _APP_COMM_BT_H_

#include "typedef.h"
#include "system/event.h"
#include "btstack_typedef.h"

enum {
    SNIFF_MODE_DEF = 0,
    SNIFF_MODE_ANCHOR,
};

typedef struct {
    u16 max_interval_slots;
    u16 min_interval_slots;
    u8 attempt_slots;
    u8 timeout_slots;
    u16 check_timer_period; //检查周期
    u8 cnt_time;//<空闲多少秒之后进入sniff模式
    u8 sniff_mode;
} edr_sniff_par_t;

typedef struct {
    u32 class_type;//搜索显示图标
    u16 page_timeout;
    u16 super_timeout;
    u8 io_capabilities: 2;
    u8 authentication_req: 3;
    u8 oob_data: 2;
    u8 passkey_enable: 1;
    const edr_sniff_par_t *sniff_param;
} edr_init_cfg_t;

typedef struct {
    //ble跟edr的地址一样
    u8 same_address;
    u16 appearance; //搜索显示图标
} ble_init_cfg_t;


#define HCI_EVENT_INQUIRY_COMPLETE                            0x01
#define HCI_EVENT_CONNECTION_COMPLETE                         0x03
#define HCI_EVENT_DISCONNECTION_COMPLETE                      0x05
#define HCI_EVENT_PIN_CODE_REQUEST                            0x16
#define HCI_EVENT_IO_CAPABILITY_REQUEST                       0x31
#define HCI_EVENT_USER_CONFIRMATION_REQUEST                   0x33
#define HCI_EVENT_USER_PASSKEY_REQUEST                        0x34
#define HCI_EVENT_USER_PRESSKEY_NOTIFICATION			      0x3B
#define HCI_EVENT_VENDOR_NO_RECONN_ADDR                       0xF8
#define HCI_EVENT_VENDOR_REMOTE_TEST                          0xFE
#define BTSTACK_EVENT_HCI_CONNECTIONS_DELETE                  0x6D


#define ERROR_CODE_SUCCESS                                    0x00
#define ERROR_CODE_PAGE_TIMEOUT                               0x04
#define ERROR_CODE_AUTHENTICATION_FAILURE                     0x05
#define ERROR_CODE_PIN_OR_KEY_MISSING                         0x06
#define ERROR_CODE_CONNECTION_TIMEOUT                         0x08
#define ERROR_CODE_SYNCHRONOUS_CONNECTION_LIMIT_TO_A_DEVICE_EXCEEDED  0x0A
#define ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS                      0x0B
#define ERROR_CODE_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES       0x0D
#define ERROR_CODE_CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR    0x0F
#define ERROR_CODE_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED         0x10
#define ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION          0x13
#define ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST        0x16

#define CUSTOM_BB_AUTO_CANCEL_PAGE                            0xFD  //// app cancle page
#define BB_CANCEL_PAGE                                        0xFE  //// bb cancle page

//-----------------------
//默认断开BT等待时间
#define WAIT_DISCONN_TIME_MS     (300)

void btstack_ble_start_before_init(const ble_init_cfg_t *cfg, int param);
void btstack_ble_start_after_init(int param);
void btstack_ble_exit(int param);
int bt_comm_ble_status_event_handler(struct bt_event *bt);
int bt_comm_ble_hci_event_handler(struct bt_event *bt);
int bt_comm_edr_hci_event_handler(struct bt_event *bt);
void bt_wait_connect_active_enable(u8 enable);
void bt_wait_phone_connect_control_ext(u8 inquiry_en, u8 page_scan_en);

void btstack_edr_start_before_init(const edr_init_cfg_t *cfg, int param);
void btstack_edr_start_after_init(int param);
void btstack_edr_exit(int param);
int bt_comm_edr_status_event_handler(struct bt_event *bt);
//退出&清除sniff
int bt_comm_edr_sniff_clean(void);
void bt_comm_edr_mode_enable(u8 enable);
void sys_auto_sniff_controle(u8 enable, u8 *addr);
void bt_comm_edr_get_remote_address(bd_addr_t address);

void lmp_set_sniff_disable(void);
extern void lmp_sniff_t_slot_attemp_reset(u16 slot, u16 attemp);
extern const int sniff_support_reset_anchor_point;   //sniff状态下是否支持reset到最近一次通信点，用于HID
/*简易配对开关接口*/
void __set_simple_pair_flag(bool flag);
void __set_keep_spp_connect(u8 en);


#endif    //_APP_BT_COMMON_H_
