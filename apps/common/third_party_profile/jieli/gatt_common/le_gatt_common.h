
#ifndef _LE_GATT_COMMON_H
#define _LE_GATT_COMMON_H

#include <stdint.h>
#include "app_config.h"
#include "le_client_demo.h"
#include "btcontroller_config.h"
#include "ble_config.h"
//注释编译宏，关闭蓝牙功能可以编译通过
#if 1//TCFG_USER_BLE_ENABLE && CONFIG_BT_GATT_COMMON_ENABLE
//----------------------------------------------------------------------------------------
#define SUPPORT_MAX_GATT_SERVER       CONFIG_BT_GATT_SERVER_NUM
#define SUPPORT_MAX_GATT_CLIENT       CONFIG_BT_GATT_CLIENT_NUM

#define GATT_ROLE_CLIENT         1
#define GATT_ROLE_SERVER         0

#define INVAIL_INDEX            ((s8)-1)
#define INVAIL_CONN_HANDLE      (0)

#define CPU_RUN_TRACE()         //log_info("%s: %d\n", __FUNCTION__,__LINE__)

/*for ext and periodic_adv test*/
//二级广播 adv + scan + connect流程   config_btctler_le_hw_nums 需要在原来基础上加2, BLE_HW_RX_SIZE需要相应加大
//config_btctler_le_features 需要或上LE_EXTENDED_ADVERTISING 或  LE_PERIODIC_ADVERTISING

#ifdef APP_TO_ALLOW_EXT_ADV
#define EXT_ADV_MODE_EN             1
#else
#define EXT_ADV_MODE_EN             0
#endif
#define PERIODIC_ADV_MODE_EN        0             //周期广播+sync流程

#define CUR_ADVERTISING_SID         0
#define CUR_ADV_HANDLE              0

extern const int config_le_hci_connection_num;//支持同时连接个数
extern const int config_le_sm_support_enable; //是否支持加密配对
extern const int config_le_gatt_server_num;   //支持server角色个数
extern const int config_le_gatt_client_num;   //支持client角色个数
extern const int config_le_sm_sub_sc_enable;   //支持SC加密方式

#define STACK_IS_SUPPORT_GATT_SERVER()  (config_le_gatt_server_num)
#define STACK_IS_SUPPORT_GATT_CLIENT()  (config_le_gatt_client_num)
#define STACK_IS_SUPPORT_GATT_CONNECT() (config_le_hci_connection_num)
#define STACK_IS_SUPPORT_SM_PAIR()      (config_le_sm_support_enable)
#define STACK_IS_SUPPORT_SM_SUB_SC()    (config_le_sm_sub_sc_enable)

typedef enum {
    /*======master + slave,ble common*/
    GATT_COMM_EVENT_NULL = 0, /**/
    GATT_COMM_EVENT_CONNECTION_COMPLETE,/*蓝牙链路连接完成*/
    GATT_COMM_EVENT_DISCONNECT_COMPLETE,/*断开连接完成*/
    GATT_COMM_EVENT_CONNECTION_COMPLETE_FAIL,/*连接建立失败*/
    GATT_COMM_EVENT_ENCRYPTION_REQUEST,/*加密请求*/
    GATT_COMM_EVENT_ENCRYPTION_CHANGE,/*加密完成*/
    GATT_COMM_EVENT_CAN_SEND_NOW,/*协议栈发送成功,通知上层可以填数*/
    GATT_COMM_EVENT_CONNECTION_UPDATE_COMPLETE,/*链路连接参数更新完成*/
    GATT_COMM_EVENT_CONNECTION_PHY_UPDATE_COMPLETE,/*链路速率更新*/
    GATT_COMM_EVENT_CONNECTION_DATA_LENGTH_CHANGE,/*DLE更新*/

    /*======master + slave*/
    //type:gatt common
    GATT_COMM_EVENT_MTU_EXCHANGE_COMPLETE = 0x20,/*ATT的MTU交换完成*/

    /*======slave + server event*/
    //type:ble slave
    GATT_COMM_EVENT_CONNECTION_UPDATE_REQUEST_RESULT = 0x30,/*请求更新参数，反馈结果*/
    GATT_COMM_EVENT_DIRECT_ADV_TIMEOUT,/*定向广播超时,未被连上*/

    //type:gatt server
    GATT_COMM_EVENT_SERVER_STATE = 0x40,/*状态变化*/
    GATT_COMM_EVENT_SERVER_INDICATION_COMPLETE,/*INDICATE应答结束*/

    /*======master + client event*/
    //type:ble master
    GATT_COMM_EVENT_SCAN_DEV_MATCH = 0x50, /*扫描到匹配的设备*/
    GATT_COMM_EVENT_SCAN_ADV_REPORT,   /*没有加指定搜索,直接输出adv report*/

    //type:gatt client
    GATT_COMM_EVENT_CLIENT_STATE = 0x60,/*状态变化*/
    GATT_COMM_EVENT_CREAT_CONN_TIMEOUT,/*建立连接超时*/
    GATT_COMM_EVENT_GATT_SEARCH_PROFILE_START,/*搜索profile开始*/
    GATT_COMM_EVENT_GATT_SEARCH_MATCH_UUID,/*搜索到匹配的UUID*/
    GATT_COMM_EVENT_GATT_SEARCH_PROFILE_COMPLETE,/*搜索profile结束*/
    GATT_COMM_EVENT_GATT_DATA_REPORT,/*接收到server端的数据*/
    GATT_COMM_EVENT_GATT_SEARCH_DESCRIPTOR_RESULT,/*搜索descriptor内容*/

    /*======slave + server, sm*/
    GATT_COMM_EVENT_SM_PASSKEY_INPUT = 0x90,/*输入key*/

} gatt_comm_event_e;

typedef enum {
    GATT_OP_RET_SUCESS =  0, //执行成功

    //对应ble_api接口返回的错误
    GATT_CMD_RET_BUSY = -100, //命令处理忙
    GATT_CMD_PARAM_OVERFLOW,  //传参数溢出
    GATT_CMD_OPT_FAIL,        //操作失败
    GATT_BUFFER_FULL,         //缓存满了
    GATT_BUFFER_ERROR,        //缓存出错
    GATT_CMD_PARAM_ERROR,     //传参出错
    GATT_CMD_STACK_NOT_RUN,   //协议栈没有运行
    GATT_CMD_USE_CCC_FAIL,    //没有使能通知，导致NOTIFY或INDICATE发送失败，

    GATT_OP_ROLE_ERR = -200,   //命令处理忙

} gatt_op_ret_e;

enum {
    LINK_ENCRYPTION_NULL = 0,
    LINK_ENCRYPTION_PAIR_JUST_WORKS,
    LINK_ENCRYPTION_PAIR_SC,
    //add here

    LINK_ENCRYPTION_RECONNECT = 0xf,
};

#define USE_SET_LOCAL_ADDRESS_TAG     (0x5a)

/* ================ gatt server 配置 ================*/
typedef struct {
    const u8 *adv_data; /*无定向广播adv包数据*/
    const u8 *rsp_data; /*无定向广播respone包数据*/
    u8  adv_data_len;   /*无定向广播adv包长度*/
    u8  rsp_data_len;   /*无定向广播respone包长度*/
    u16 adv_interval;	/*无定向广播周期,(unit:0.625ms),Range: 0x0020 to 0x4000 */
    u8  adv_auto_do: 4; /*是否gatt模块自动打开广播（使能，断开等状态下）*/
    u8  adv_type: 4;	/*广播类型，包含：无定向可连接广播，无定向不可连接广播，定向广播等*/
    u8  adv_channel;	/*广播使用的通道，bit0~2对应 channel 37~38*/
    u8  direct_address_info[7]; /*定向广播使用主机的地址信息,addr_type + address*/
    u8  set_local_addr_tag;     /*= USE_SET_LOCAL_ADDRESS_TAG,指定使用当前local_address_info,可以用于开多机指定设备地址*/
    u8  local_address_info[7];  /*可以指定设备地址开广播,addr_type + address*/
} adv_cfg_t;

typedef struct {
    /*server端被主机操作 读写操作回调*/
    u16(*att_read_cb)(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
    int (*att_write_cb)(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
    /*协议栈事件回调处理*/
    int (*event_packet_handler)(int event, u8 *packet, u16 size, u8 *ext_param);
} gatt_server_cfg_t;

/* ================ gatt client 配置 ================*/
typedef struct {
    //common
    u8 scan_auto_do: 4;	       /*是否gatt模块自动打开搜索（使能，断开等状态下）*/
    u8 creat_auto_do: 4;       /*是否gatt模块搜索到匹配的设备自动发起连接*/
    u8  set_local_addr_tag;    /*= USE_SET_LOCAL_ADDRESS_TAG,指定使用当前local_address_info,可以用于开多机指定设备地址*/
    u8  local_address_info[7]; /*可以指定设备地址开scan,addr_type + address*/

    //scan
    u8 scan_type: 4;    /*搜索类型*/
    u8 scan_filter: 4;  /*是否开搜索重复过滤*/
    u16 scan_interval;  /*搜索周期,(unit:0.625ms),>= scan_window,   Range: 0x0004 to 0x4000 */
    u16 scan_window;    /*搜索窗口,(unit:0.625ms),<= scan_interval, Range: 0x0004 to 0x4000 */

    //creat
    u16 creat_conn_interval;        /*创建连接的周期,(unit:1.25ms),Range: 0x0006 to 0x0c08*/
    u16 creat_conn_latency;	        /*忽略连接周期的个数, 建议interval*latency的时间要 <= 2秒 */
    u16 creat_conn_super_timeout;	/*连接周期没收到包超时时间,(unit:10ms),Range: 0x000a to 0x0c08,建议值600 */

    //control
    u32 creat_state_timeout_ms;	    /*创建连接后,超时未连上会取消连接,重新开搜索;=0,只能手动取消建立连接*/
    u8  conn_update_accept;	        /*连接过程,接受从机的连接参数请求使能*/
} scan_conn_cfg_t;

typedef struct {
    /*未连接,扫描设备配置*/
    const client_match_cfg_t  *match_devices;     /*扫描匹配设备表*/
    u16   match_devices_count;    /*搜索devices的个数*/
    u8    match_rssi_enable;      /*creat_auto_do 建立连接,是否检测rssi*/
    s8    match_rssi_value;       /*至少的rssi强度值*/

    /*连接后,profile 搜索配置*/
    const target_uuid_t       *search_uuid_group; /*搜索uuid表*/
    u16 search_uuid_count;    /*搜索uuid的个数*/
    u8  auto_enable_ccc;      /*是否执行使能匹配的 NOTIFY和INDICATE 通知功能*/
} gatt_search_cfg_t;


typedef struct {
    /*协议栈事件回调处理*/
    int (*event_packet_handler)(int event, u8 *packet, u16 size, u8 *ext_param);
} gatt_client_cfg_t;

/* ================ gatt common 公共配置 ================*/
typedef struct {
    //sm
    u8 master_security_auto_req: 1; /*主机主动发起加密*/
    u8 master_set_wait_security: 1; /*主机等待加密完成再执行profile搜索*/
    u8 slave_security_auto_req: 1;  /*从机发起加密请求命令*/
    u8 slave_set_wait_security: 1;  /*从机等待加密处理*/
    u8 io_capabilities: 4;          /*加密io能力配置*/
    u8 authentication_req_flags;    /*加密认证配置*/
    u8 min_key_size;                /*加密key支持的最小长度,range:7~16*/
    u8 max_key_size;                /*加密key支持的最大长度,range:7~16*/
    /*sm 回调，保留未用*/
    int (*sm_cb_packet_handler)(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
} sm_cfg_t;

typedef struct {
    //connect
    u16 mtu_size;         /*mtu配置大小, range:23 ~517*/
    u16 cbuffer_size;     /*缓存buffer大小，>= mtu_size*/
    u8  multi_dev_flag;    /*多机使用标识*/

    //config
    gatt_server_cfg_t *server_config; /*gatt server 配置*/
    gatt_client_cfg_t *client_config; /*gatt client 配置*/
    sm_cfg_t *sm_config; /**/

    /*hci 回调，保留未用*/
    int (*hci_cb_packet_handler)(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
} gatt_ctrl_t;



//common
u32  ble_comm_cbuffer_vaild_len(u16 conn_handle);
int ble_comm_att_send_data(u16 conn_handle, u16 att_handle, u8 *data, u16 len, att_op_type_e op_type);
bool ble_comm_att_check_send(u16 conn_handle, u16 pre_send_len);
const char *ble_comm_get_gap_name(void);
int ble_comm_disconnect(u16 conn_handle);
u8 ble_comm_dev_get_handle_state(u16 handle, u8 role);
void ble_comm_dev_set_handle_state(u16 handle, u8 role, u8 state);
void ble_comm_register_state_cbk(void (*cbk)(u16 handle, u8 state));
s8 ble_comm_dev_get_index(u16 handle, u8 role);
s8 ble_comm_dev_get_idle_index(u8 role);
u8 ble_comm_dev_get_handle_role(u16 handle);
u16 ble_comm_dev_get_handle(u8 index, u8 role);
void ble_comm_set_config_name(const char *name_p, u8 add_ext_name);
void ble_comm_init(const gatt_ctrl_t *control_blk);
void ble_comm_exit(void);
void ble_comm_module_enable(u8 en);
int ble_comm_set_connection_data_length(u16 conn_handle, u16 tx_octets, u16 tx_time);
int ble_comm_set_connection_data_phy(u16 conn_handle, u8 tx_phy, u8 rx_phy, u16 phy_options);

//server
void ble_gatt_server_init(gatt_server_cfg_t *server_cfg);
void ble_gatt_server_exit(void);
ble_state_e ble_gatt_server_get_work_state(void);
ble_state_e ble_gatt_server_get_connect_state(u16 conn_handle);
int ble_gatt_server_adv_enable(u32 en);
void ble_gatt_server_module_enable(u8 en);
void ble_gatt_server_disconnect_all(void);
int  ble_gatt_server_connetion_update_request(u16 conn_handle, const struct conn_update_param_t *update_table, u16 table_count);
int ble_gatt_server_characteristic_ccc_set(u16 conn_handle, u16 att_ccc_handle, u16 ccc_config);
u16 ble_gatt_server_characteristic_ccc_get(u16 conn_handle, u16 att_ccc_handle);
void ble_gatt_server_set_update_send(u16 conn_handle, u16 att_handle, u8 att_handle_type);
void ble_gatt_server_receive_update_data(void *priv, void *buf, u16 len);
void ble_gatt_server_set_adv_config(adv_cfg_t *adv_cfg);
void ble_gatt_server_set_profile(const u8 *profile_table, u16 size);

//client
void ble_gatt_client_init(gatt_client_cfg_t *client_cfg);
void ble_gatt_client_exit(void);
void ble_gatt_client_set_scan_config(scan_conn_cfg_t *scan_conn_cfg);
void ble_gatt_client_set_search_config(gatt_search_cfg_t *gatt_search_cfg);
ble_state_e ble_gatt_client_get_work_state(void);
ble_state_e ble_gatt_client_get_connect_state(u16 conn_handle);
int ble_gatt_client_create_connection_request(u8 *address, u8 addr_type, int mode);
int ble_gatt_client_create_connection_cannel(void);
int ble_gatt_client_scan_enable(u32 en);
void ble_gatt_client_module_enable(u8 en);
void ble_gatt_client_disconnect_all(void);
void ble_gatt_just_search_profile_start(u16 conn_handle);
void ble_gatt_just_search_profile_stop(u16 conn_handle);
u8 ble_comm_dev_get_connected_nums(u8 role);
#endif
#endif
