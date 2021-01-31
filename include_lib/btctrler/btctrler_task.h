#ifndef BTCTRLER_TASK_H
#define BTCTRLER_TASK_H

#include "typedef.h"
#include "system/task.h"

enum {
    LMP_EVENT = Q_USER +  1,
    LMP_HCI_CMD,
    LMP_HCI_CMD_TO_CONN,
    HCI_COMMON_CMD,
    LL_EVENT,
    HCI_CMD_TO_LL,
    TWS_LMP_EVENT,
    MSG_BT_UPDATA_START,
    MSG_BT_UPDATE_LOADER_DOWNLOAD_START,
    MSG_BLE_TEST_UPDATA_START,
    MSG_BLE_TEST_OTA_LOADER_DOWNLOAD_START,
    MSG_TASK_READY,
    MSG_TASK_DEL,
};

enum {
    BTCTRLER_EVENT_RESUME_REQ = 1,
};

#define SYS_EVENT_FROM_CTRLER   (('C' << 24) | ('T' << 16) | ('R' << 8) | '\0')




int bredr_link_event(void *link, int argc, ...);

int bredr_tws_link_event(void *link, int argc, ...);

int btctrler_hci_cmd_to_task(int cmd, int argc, ...);

int lmp_hci_cmd_to_conn_for_handle(u16 handle, int argc, ...);

int lmp_hci_cmd_to_conn_for_addr(u8 *addr, int argc, ...);

int lmp_hci_cmd_to_conn(void *conn, int argc, ...);


#define lmp_hci_cmd_to_task(argc, ...)      btctrler_hci_cmd_to_task(LMP_HCI_CMD, argc, ## __VA_ARGS__)

#define ll_hci_cmd_to_task(argc, ...)       btctrler_hci_cmd_to_task(HCI_CMD_TO_LL, argc, ## __VA_ARGS__)


int btctrler_task_init(const void *transport, const void *config);

void btctrler_resume_req();

void btctrler_resume();

int btctrler_suspend(u8 suepend_rx_bulk);

int btctrler_task_ready();

int btctrler_task_exit();

int btctrler_task_close_bredr();
void  btctrler_task_init_bredr();

void set_idle_period_slot(u16 slot);
enum {
    TESTBOX_INFO_VBAT_VALUE = 0,	//(u16 (*handle)(void))
    TESTBOX_INFO_VBAT_PERCENT,		//(u8 (*handle)(void))
    TESTBOX_INFO_BURN_CODE,			//(u8 *(*handle)(u8 *len))
    TESTBOX_INFO_SDK_VERSION,		//(u8 *(*handle)(u8 *len))
};

void bt_testbox_ex_info_get_handle_register(u8 info_type, void *handle);
#endif
