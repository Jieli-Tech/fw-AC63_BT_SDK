[icon_build]:./../../doc/stuff/build_passing.svg

# APP - Bluetooth Dual-Mode SPP+BLE ![Build result][icon_build]
---------------

代码工程：`apps\spp_and_le\board\bd29\AC631X_spp_and_le.cbp` 

## 1.1 概述
支持蓝牙双模透传传输功能。CLASSIC蓝牙使用标准串口SPP profile协议；而BLE蓝牙使用自定义的profile协议，
提供ATT的READ、WRITE、WRITE_WITHOUT_RESPONSE，NOTIFY和INDICATE等属性传输收发数据。

支持的板级： bd29、br25、br23、br30、bd19、br34
支持的芯片： AC631N、AC636N、AC635N、AC637N、AC632N、AC638N

## 1.2 工程配置
代码工程：apps\spp_and_le\board\bd29\AC631N_spp_and_le.cbp 

*  先配置板级board_config.h和对应配置文件中蓝牙双模使能
```C
1./*
2. *  板级配置选择
3. */
4.#define CONFIG_BOARD_AC631N_DEMO
5.// #define CONFIG_BOARD_AC6311_DEMO
6.// #define CONFIG_BOARD_AC6313_DEMO
7.// #define CONFIG_BOARD_AC6318_DEMO
8.// #define CONFIG_BOARD_AC6319_DEMO
9.
10.#include "board_ac631n_demo_cfg.h"
11.#include "board_ac6311_demo_cfg.h"
12.#include "board_ac6313_demo_cfg.h"
13.#include "board_ac6318_demo_cfg.h"
14.#include "board_ac6319_demo_cfg.h"
```
//在board_ac631n_demo_cfg.h中配置是否打开 edr 和ble模块
```C
1.#define TCFG_USER_BLE_ENABLE                      1   //BLE功能使能
2.#define TCFG_USER_EDR_ENABLE                      1   //EDR功能使能
```
* 配置app选择：“app_config.h”
```C
1.//apps example 选择,只能选1个,要配置对应的board_config.h
2.#define CONFIG_APP_SPP_LE                 1 //SPP + LE or LE's client
3.#define CONFIG_APP_MULTI                  0 //蓝牙LE多连 + spp
4.#define CONFIG_APP_DONGLE                 0 //usb + 蓝牙(ble 主机),PC hid设备
5.#define CONFIG_APP_CENTRAL                0 //ble client,中心设备
6.#define CONFIG_APP_LL_SYNC                0 //腾讯连连
7.#define CONFIG_APP_BEACON                 0 //蓝牙BLE ibeacon
8.#define CONFIG_APP_NONCONN_24G            0 //2.4G 非连接收发
9.#define CONFIG_APP_TUYA                   0 //涂鸦协议
10.#define CONFIG_APP_AT_COM                0 //AT com HEX格式命令
11.#define CONFIG_APP_AT_CHAR_COM           0 //AT com 字符串格式命令
12.#define CONFIG_APP_IDLE                  0 //空闲任务
```
* 配置对应case需求：“app_config.h”
```C
1.#if CONFIG_APP_SPP_LE
2.//配置双模同名字，同地址
3.#define DOUBLE_BT_SAME_NAME                0 //同名字
4.#define DOUBLE_BT_SAME_MAC                 0 //同地址
5.#define CONFIG_APP_SPP_LE_TO_IDLE          0 //SPP_AND_LE To IDLE Use
```

## SPP数据通信

### 2.1 代码文件 `spp_trans.c`

### 2.2 接口说明

* SPP模块初始化:

```C 
1.void transport_spp_init(void)
2.{
3.    log_info("trans_spp_init\n");
4.    log_info("spp_file: %s", __FILE__);
5.#if (USER_SUPPORT_PROFILE_SPP==1)
6.    spp_state = 0;
7.    spp_get_operation_table(&spp_api);
8.    spp_api->regist_recieve_cbk(0, transport_spp_recieve_cbk);
9.    spp_api->regist_state_cbk(0, transport_spp_state_cbk);
10.    spp_api->regist_wakeup_send(NULL, transport_spp_send_wakeup);
11.#endif
12.
13.#if TEST_SPP_DATA_RATE
14.    spp_timer_handle  = sys_timer_add(NULL, test_timer_handler, SPP_TIMER_MS);
15.#endif
16.
17.}
```

* SPP连接和断开事件处理:
  
```C  
1.static void transport_spp_state_cbk(u8 state)
2.{
3.    spp_state = state;
4.    switch (state) {
5.    case SPP_USER_ST_CONNECT:
6.        log_info("SPP_USER_ST_CONNECT ~~~\n");
7.
8.        break;
9.
10.    case SPP_USER_ST_DISCONN:
11.        log_info("SPP_USER_ST_DISCONN ~~~\n");
12.        spp_channel = 0;
13.
14.        break;
15.
16.    default:
17.        break;
18.    }
19.
20.}

```

* SPP发送数据接口，发送前先调用接口transport_spp_send_data_check检查:

```C
1.int transport_spp_send_data(u8 *data, u16 len)
2.{
3.    if (spp_api) {
4.        log_info("spp_api_tx(%d) \n", len);
5.        /* log_info_hexdump(data, len); */
6.        /* clear_sniff_cnt(); */
7.        bt_comm_edr_sniff_clean();
8.        return spp_api->send_data(NULL, data, len);
9.    }
10.    return SPP_USER_ERR_SEND_FAIL;
11.}
```

* SPP检查是否可以往协议栈发送数据:

```C
1.int transport_spp_send_data_check(u16 len)
2.{
3.    if (spp_api) {
4.        if (spp_api->busy_state()) {
5.            return 0;
6.        }
7.    }
8.    return 1;
9.}
```

* SPP发送完成回调，表示可以继续往协议栈发数，用来触发继续发数:
  
```C
    static void transport_spp_send_wakeup(void)  
    {  
        putchar('W');  
    }  
```

* SPP接收数据接口

```C
static void transport_spp_recieve_cbk(void *priv, u8 *buf, u16 len)
{
    spp_channel = (u16)priv;
    log_info("spp_api_rx(%d) \n", len);
    log_info_hexdump(buf, len);
    clear_sniff_cnt();
```

### 3.收发测试
* 代码已经实现收到手机的SPP数据后，会主动把数据回送，测试数据收发:

```C
    //loop send data for test
if (transport_spp_send_data_check(len)) {
    log_info("-loop send\n");
    transport_spp_send_data(buf, len);
}
```
### 4、串口的UUID：“lib_profile_config.c”
串口的UUID默认是16bit的0x1101。若要修改可自定义的16bit 或128bit UUID，可修改SDP的S信息结构体sdp_spp_service_data，具体查看16it和128bit UUID填写示例。
```C
1.#if (USER_SUPPORT_PROFILE_SPP==1)
2.u8 spp_profile_support = 1;
3.SDP_RECORD_HANDLER_REGISTER(spp_sdp_record_item) = {
4.    .service_record = (u8 *)sdp_spp_service_data,
5.    .service_record_handle = 0x00010004,
6.};
7.#endif

16it和128bit UUID填写示例如下：
1./*128 bit uuid:  11223344-5566-7788-aabb-8899aabbccdd  */  
2.const u8 sdp_test_spp_service_data[96] = {  
3.    0x36, 0x00, 0x5B, 0x09, 0x00, 0x00, 0x0A, 0x00, 0x01, 0x00, 0x04, 0x09, 0x00, 0x01, 0x36, 0x00,  
4.    0x11, 0x1C,   
5.      
6.    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0xaa, 0xbb, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, //uuid128  
7.     
8.    0x09, 0x00, 0x04, 0x36, 0x00, 0x0E, 0x36, 0x00, 0x03, 0x19, 0x01, 0x00, 0x36, 0x00,  
9.    0x05, 0x19, 0x00, 0x03, 0x08, 0x02, 0x09, 0x00, 0x09, 0x36, 0x00, 0x17, 0x36, 0x00, 0x14, 0x1C, 
10.      
11.    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0xaa, 0xbb, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, //uuid128  
12.      
13.    0x09, 0x01, 0x00, 0x09, 0x01, 0x00, 0x25, 0x06, 0x4A, 0x4C, 0x5F, 0x53, 0x50, 0x50, 0x00, 0x00,  
14.};  
15.  
16.//spp 16bit uuid  11 01  
17.const u8 sdp_spp_update_service_data[70] = {  
18.    0x36, 0x00, 0x42, 0x09, 0x00, 0x00, 0x0A, 0x00, 0x01, 0x00, 0x0B, 0x09, 0x00, 0x01, 0x36, 0x00,  
19.    0x03, 0x19,   
20.      
21.    0x11, 0x01, //uuid16  
22.      
23.    0x09, 0x00, 0x04, 0x36, 0x00, 0x0E, 0x36, 0x00, 0x03, 0x19, 0x01, 0x00,  
24.    0x36, 0x00, 0x05, 0x19, 0x00, 0x03, 0x08, 0x08, 0x09, 0x00, 0x09, 0x36, 0x00, 0x09, 0x36, 0x00,  
25.    0x06, 0x19,   
26.      
27.    0x11, 0x01, //uuid16  
28.      
29.    0x09, 0x01, 0x00, 0x09, 0x01, 0x00, 0x25, 0x09, 0x4A, 0x4C, 0x5F, 0x53,  
30.    0x50, 0x50, 0x5F, 0x55, 0x50, 0x00,  
31.};  
```

## BLE数据通信


### 3.1 代码文件 `ble_trans.c`

* Profile生成的trans_profile_data数据表放在ble_trans_profile.h。用户可用工具make_gatt_services自定义修改,重新配置GATT服务和属性等。

```C
static const uint8_t trans_profile_data[] = {  
        //////////////////////////////////////////////////////
//
// 0x0001 PRIMARY_SERVICE  1800
//
//////////////////////////////////////////////////////
0x0a, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x28, 0x00, 0x18,

/* CHARACTERISTIC,  2a00, READ | WRITE | DYNAMIC, */
// 0x0002 CHARACTERISTIC 2a00 READ | WRITE | DYNAMIC
0x0d, 0x00, 0x02, 0x00, 0x02, 0x00, 0x03, 0x28, 0x0a, 0x03, 0x00, 0x00, 0x2a,
// 0x0003 VALUE 2a00 READ | WRITE | DYNAMIC
0x08, 0x00, 0x0a, 0x01, 0x03, 0x00, 0x00, 0x2a,

//////////////////////////////////////////////////////
//
// 0x0004 PRIMARY_SERVICE  ae30
//
//////////////////////////////////////////////////////
0x0a, 0x00, 0x02, 0x00, 0x04, 0x00, 0x00, 0x28, 0x30, 0xae,

/* CHARACTERISTIC,  ae01, WRITE_WITHOUT_RESPONSE | DYNAMIC, */
// 0x0005 CHARACTERISTIC ae01 WRITE_WITHOUT_RESPONSE | DYNAMIC
0x0d, 0x00, 0x02, 0x00, 0x05, 0x00, 0x03, 0x28, 0x04, 0x06, 0x00, 0x01, 0xae,
// 0x0006 VALUE ae01 WRITE_WITHOUT_RESPONSE | DYNAMIC
0x08, 0x00, 0x04, 0x01, 0x06, 0x00, 0x01, 0xae,
```

## 3.2接口说明
* 蓝牙和GATT事件回调处理，主要是连接、断开等事件

```C
    /* LISTING_START(packetHandler): Packet Handler */
static int trans_event_packet_handler(int event, u8 *packet, u16 size, u8 *ext_param)
{
    switch (event) {
    case GATT_COMM_EVENT_CONNECTION_COMPLETE:
        trans_con_handle = little_endian_read_16(packet, 0);
        trans_connection_update_enable = 1;

        log_info("connection_handle:%04x\n", little_endian_read_16(packet, 0));
        log_info("connection_handle:%04x, rssi= %d\n", trans_con_handle, ble_vendor_get_peer_rssi(trans_con_handle));
        log_info("peer_address_info:");
        put_buf(&ext_param[7], 7);

        log_info("con_interval = %d\n", little_endian_read_16(ext_param, 14 + 0));
        log_info("con_latency = %d\n", little_endian_read_16(ext_param, 14 + 2));
        log_info("cnn_timeout = %d\n", little_endian_read_16(ext_param, 14 + 4));
        break;
```

* ATT读事件处理

```C
static uint16_t trans_att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, \
uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    uint16_t  att_value_len = 0;
    uint16_t handle = att_handle;

    log_info("read_callback,conn_handle =%04x, handle=%04x,buffer=%08x\n", connection_handle, handle, (u32)buffer);

    switch (handle) {
    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE: {
        char *gap_name = ble_comm_get_gap_name();
        att_value_len = strlen(gap_name);

        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }

        if (buffer) {
            memcpy(buffer, &gap_name[offset], buffer_size);
            att_value_len = buffer_size;
            log_info("\n------read gap_name: %s\n", gap_name);
        }
    }
    break;

}
```

* ATT写事件处理

```C
static int trans_att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, \
uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    int result = 0;
    u16 tmp16;

    u16 handle = att_handle;

    log_info("write_callback,conn_handle =%04x, handle =%04x,size =%d\n", connection_handle, handle, buffer_size);

    switch (handle) {

    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
        break;
```

* NOTIFY 和 INDICATE 发送示例，发送前调接口ble_comm_att_check_send检查是否可以往协议栈发送数据

```C
if (ble_comm_att_check_send(connection_handle, buffer_size)) {
	log_info("-loop send1\n");
	ble_comm_att_send_data(connection_handle, ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE, \
	buffer, buffer_size, ATT_OP_AUTO_READ_CCC);
}
	
if (ble_comm_att_check_send(connection_handle, buffer_size)) {
    log_info("-loop send2\n");
    ble_comm_att_send_data(connection_handle, ATT_CHARACTERISTIC_ae05_01_VALUE_HANDLE, \
	buffer, buffer_size, ATT_OP_AUTO_READ_CCC);
}
```

* 协议栈发送完成回调，表示可以继续发数，用来无缝触发发数

    ```C
    case GATT_COMM_EVENT_CAN_SEND_NOW:
        break;
    ```


* 收发测试
使用手机NRF软件，连接设备后；使能notify和indicate的UUID (AE02 和 AE05) 的通知功能后；
可以通过向write的UUID (AE01 或 AE03) 发送数据；测试UUID (AE02 或 AE05)是否收到数据。

```C
    case ATT_CHARACTERISTIC_ae01_01_VALUE_HANDLE:
        log_info("\n-ae01_rx(%d):", buffer_size);
        put_buf(buffer, buffer_size);

        //收发测试，自动发送收到的数据;for test
        if (ble_comm_att_check_send(connection_handle, buffer_size)) {
            log_info("-loop send1\n");
            ble_comm_att_send_data(connection_handle, ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE, \
			buffer, buffer_size, ATT_OP_AUTO_READ_CCC);
        }
        break;

    case ATT_CHARACTERISTIC_ae03_01_VALUE_HANDLE:
        log_info("\n-ae_rx(%d):", buffer_size);
        put_buf(buffer, buffer_size);

        //收发测试，自动发送收到的数据;for test
        if (ble_comm_att_check_send(connection_handle, buffer_size)) {
            log_info("-loop send2\n");
            ble_comm_att_send_data(connection_handle, ATT_CHARACTERISTIC_ae05_01_VALUE_HANDLE, \
			buffer, buffer_size, ATT_OP_AUTO_READ_CCC);
        }
        break;  
```

# APP - Bluetooth：AT 命令

## 4.1 概述
主要功能是在普通数传BLE和EDR的基础上增加了由上位机或其他MCU可以通过UART对接蓝牙芯片进行基本配置、
状态获取、控制扫描、连接断开以及数据收发等操作。
AT控制透传支持从机模式和主机模式, 编译的时候只能二选一, 从机模式支持双模, 主机模式只支持BLE。
定义一套串口的控制协议，具体请查看协议文档《蓝牙AT协议》。
支持的板级： bd29、br25、br23、br30、bd19、br34
支持的芯片： AC631N、AC636N、AC635N、AC637N、AC632N、AC638N

## 4.2 工程配置
代码工程：apps\spp_and_le\board\bd29\AC631N_spp_and_le.cbp

* 先配置板级board_config.h和对应配置文件中蓝牙双模使能
```C
1./* 
2. *  板级配置选择 
3. */  
4.#define CONFIG_BOARD_AC630X_DEMO  
5.// #define CONFIG_BOARD_AC6311_DEMO  
6.// #define CONFIG_BOARD_AC6313_DEMO  
7.// #define CONFIG_BOARD_AC6318_DEMO  
8.// #define CONFIG_BOARD_AC6319_DEMO  
```
* 配置对应的board_acxxx_demo_cfg.h文件使能BLE或EDR(主机不支持EDR), 以board_ac630x_demo_cfg.h为例
```C
3.#define TCFG_USER_BLE_ENABLE                      1   //BLE功能使能  
4.#define TCFG_USER_EDR_ENABLE                      1   //EDR功能使能
```
* 配置app_config.h, 使能AT
```C
1.//apps example 选择,只能选1个,要配置对应的board_config.h
2.#define CONFIG_APP_SPP_LE                 0 //SPP + LE or LE's client
3.#define CONFIG_APP_MULTI                  0 //蓝牙LE多连 + spp
4.#define CONFIG_APP_DONGLE                 0 //usb + 蓝牙(ble 主机),PC hid设备
5.#define CONFIG_APP_CENTRAL                0 //ble client,中心设备
6.#define CONFIG_APP_LL_SYNC                0 //腾讯连连
7.#define CONFIG_APP_BEACON                 0 //蓝牙BLE ibeacon
8.#define CONFIG_APP_NONCONN_24G            0 //2.4G 非连接收发
9.#define CONFIG_APP_TUYA                   0 //涂鸦协议
10.#define CONFIG_APP_AT_COM                1 //AT com HEX格式命令
11.#define CONFIG_APP_AT_CHAR_COM           0 //AT com 字符串格式命令
12.#define CONFIG_APP_IDLE                  0 //空闲任务
```

* 配置app_config.h, 选择AT主机或AT从机(二选一)
```C
#define TRANS_AT_COM                      0 
#define TRANS_AT_CLIENT                   1 //选择主机AT 
```

## 4.3 简单说明代码文件
|代码文件                              |描述说明                                |
|:------------------------------------:|------------------------------------|
|app_at_com.c                           |任务主要实现，流程|
|at_uart.c                              |串口配置，数据收发|
|at_cmds.c                              |AT协议解析处理|
|le_at_com.c                            |ble控制实现|
|spp_at_com.c                           |spp控制实现|
|le_at_client.c                         |主机ble控制实现|

# APP- Bluetooth Dual-Mode Central

## 5.1 概述
Central 中心设备是使用 GATT 的 client 角色，在 SDK 中是以主机 Master 的方式实现，主动发
起搜索和连接其他BLE 设备。连接成功后遍历从机GATT 的Services信息数据。最大支持16个Sevices
遍历。
SDK 的例子是以杰理的数传 SDK 的 BLE 的设备中 Services 为搜索目标，用户根据需求也可自
行搜索过滤其他的 Services。

支持的板级： bd29、br25、br23、br30、bd19、br34
支持的芯片： AC631N、AC636N、AC635N、AC637N、AC632N、AC638N

## 5.2 工程配置
代码工程：apps\spp_and_le\board\bd29\AC631N_spp_and_le.cbp

* 先配置板级 board_config.h 和对应配置文件中蓝牙双模使能
```C
 #define CONFIG_BOARD_AC631N_DEMO

 #include "board_ac631n_demo_cfg.h"
```
* 配置只支持 ble 模块
```C
 #define TCFG_USER_BLE_ENABLE 1 //BLE 功能使能
 #define TCFG_USER_EDR_ENABLE 0 //EDR 功能使能

```
* 配置 app 选择
```C
//apps example 选择,只能选1个,要配置对应的board_config.h
#define CONFIG_APP_SPP_LE                 0 //SPP + LE or LE's client
#define CONFIG_APP_MULTI                  0 //蓝牙LE多连 + spp
#define CONFIG_APP_DONGLE                 0 //usb + 蓝牙(ble 主机),PC hid设备
#define CONFIG_APP_CENTRAL                1 //ble client,中心设备
#define CONFIG_APP_LL_SYNC                0 //腾讯连连
#define CONFIG_APP_BEACON                 0 //蓝牙BLE ibeacon
#define CONFIG_APP_NONCONN_24G            0 //2.4G 非连接收发
#define CONFIG_APP_TUYA                   0 //涂鸦协议
#define CONFIG_APP_AT_COM                 0 //AT com HEX格式命令
#define CONFIG_APP_AT_CHAR_COM            0 //AT com 字符串格式命令
#define CONFIG_APP_IDLE                   0 //空闲任务
```

## 5.3 主要代码说明
BLE 实现文件 ble_central.c，负责模块初始化、处理 GATT 模块事件和命令数据控制发送等。

2.5.3 主要代码说明
BLE 实现文件 ble_central.c，负责模块初始化、处理 GATT 模块事件和命令数据控制发送等。

1、配置 GATT client 的模块基本要素。
```C
1. static const sm_cfg_t cetl_sm_init_config = {
2.
3. static gatt_ctrl_t cetl_gatt_control_block = {
4.
5. static const gatt_client_cfg_t central_client_init_cfg = {
6. .event_packet_handler = cetl_client_event_packet_handler,
7. };
```

2、配置搜索的 GATT 服务以及记录搜索到的信息，支持 16bit 和 128 bit 的 UUID。

（1）配置扫描匹配的设备
```C
1. //配置多个扫描匹配设备
2. static const u8 cetl_test_remoter_name1[] = "AC897N_MX(BLE)";//
3. static client_match_cfg_t cetl_match_device_table[] = {
4. #if MATCH_CONFIG_NAME
```
   搜索匹配设备会发聩事件处理
```C
1. case GATT_COMM_EVENT_SCAN_DEV_MATCH: {
2. log_info("match_dev:addr_type= %d\n", packet[0]);
3. put_buf(&packet[1], 6);
4. if (packet[8] == 2) {
5. log_info("is TEST_BOX\n");
6. }
7. client_match_cfg_t *match_cfg = ext_param;
8. if (match_cfg) {
9. log_info("match_mode: %d\n", match_cfg->create_conn_mode);
10. if (match_cfg->compare_data_len) {
```

（2）配置连上后搜索的 uuid
```C
1. //指定搜索uuid
2. //指定搜索uuid
3. static const target_uuid_t jl_cetl_search_uuid_table[] = {
4.
5. // for uuid16
6. // PRIMARY_SERVICE, ae30
7. // CHARACTERISTIC, ae01, WRITE_WITHOUT_RESPONSE | DYNAMIC,
8. // CHARACTERISTIC, ae02, NOTIFY,
9.
10. {
11. .services_uuid16 = 0xae30,
12. .characteristic_uuid16 = 0xae01,
13. .opt_type = ATT_PROPERTY_WRITE_WITHOUT_RESPONSE,
14. },
15.
16. {
17. .services_uuid16 = 0xae30,
18. .characteristic_uuid16 = 0xae02,
19. .opt_type = ATT_PROPERTY_NOTIFY,
20. },
```

查找到匹配的 UUID 和 handle，处理事件，并且可以记录操作 handle。
```C
1. case GATT_COMM_EVENT_GATT_SEARCH_MATCH_UUID: {
2. opt_handle_t *opt_hdl = packet;
3. log_info("match:server_uuid= %04x,charactc_uuid= %04x,value_handle= %04x\n",
4. opt_hdl->search_uuid->services_uuid16, opt_hdl->search_uuid->charac
teristic_uuid16, opt_hdl->value_handle);
5. #if cetl_TEST_WRITE_SEND_DATA
6. //for test
7. if (opt_hdl->search_uuid->characteristic_uuid16 == 0xae01) {
8. 	cetl_ble_client_write_handle = opt_hdl->value_handle;
9. }
10. #endif
11. }
12. break;
```

2、配置扫描和连接参数
扫描参数以 0.625ms 为单位，设置如下图：
```C
1. #define SET_SCAN_TYPE SCAN_ACTIVE
2. #define SET_SCAN_INTERVAL 48
3. #define SET_SCAN_WINDOW 16
```

连接参数 interval 是以 1.25ms 为单位，timeout 是以 10ms 为单位，如下图：
```C
1. #define SET_CONN_INTERVAL 0x30
2. #define SET_CONN_LATENCY 0
3. #define SET_CONN_TIMEOUT 0x180
```
以上两组参数请慎重修改，必须按照蓝牙的协议规范来定义修改。


 # APP- Bluetooth BLE Dongle
 ## 6.1 概述
 蓝牙 dongle 符合 USB 和 BLE 或 EDR 传输标准，具有即插即用，方便实用的特点。它可用于蓝
牙设备之间的数据传输，让电脑能够和周边的蓝牙设备进行无线连接和数据的通讯，自动发现和管
理远程蓝牙设备、资源和服务，实现蓝牙设备之间的绑定和自动连接。

 蓝牙 dongle 支持 BLE 和 2.4G 两种连接模式；支持连接指定蓝牙名或 mac 地址；应用示例是连
接杰理的鼠标。
 蓝牙 dongle 支持 EDR，支持连接指定蓝牙名或 mac 地址，连接杰理的键盘设备。
 蓝牙双模 dongle 不能同时打开，会降低搜索效率。

 支持的板级：br25、br30、bd19、br34

 支持的芯片：AC636N、AC637N、AC632N、AC638N
 ## 6.2 工程配置
 代码工程：apps\spp_and_le\board\br25\AC636N_spp_and_le.cbp
* APP 选择，配置 app_config.h
```C
 18. //app case 选择,只能选 1 个,要配置对应的 board_config.h
 21. #define CONFIG_APP_DONGLE 1
```
* 板级选择, 配置 board_config.h。目前只有 AC6368B_DONGLE 板子支持蓝牙 dongle
```C
22. //#define CONFIG_BOARD_AC636N_DEMO
23. #define CONFIG_BOARD_AC6368B_DONGLE //CONFIG_APP_DONGLE
24. // #define CONFIG_BOARD_AC6363F_DEMO
25. // #define CONFIG_BOARD_AC6366C_DEMO
26. // #define CONFIG_BOARD_AC6368A_DEMO
27. // #define CONFIG_BOARD_AC6369F_DEMO
28. // #define CONFIG_BOARD_AC6369C_DEMO
```
* 使能 USB 和 BLE ，需配置 board_ac6368b_dongle_cfg.h
```C
29. #define TCFG_PC_ENABLE ENABLE_THIS_MOUDLE//PC 模块使能
30. #define TCFG_UDISK_ENABLE DISABLE_THIS_MOUDLE//U 盘模块使能
31. #define TCFG_OTG_USB_DEV_EN BIT(0)//USB0 = BIT(0) USB1 = BIT(1)
32. #define TCFG_USER_BLE_ENABLE 1 //BLE 或 2.4G 功能使能
33. #define TCFG_USER_EDR_ENABLE 0 //EDR 功能使能
```
* 模式选择，配置 BLE 或 2.4G 模式；若选择 2.4G 配对码必须跟对方的配对码一致
```C
34. //2.4G 模式: 0---ble, 非 0---2.4G 配对码
35. #define CFG_RF_24G_CODE_ID (0) //<=24bits

```
* 如果选择 BLE 模式，则蓝牙 dongle 默认是按蓝牙名连接从机，需要配置连接的从机蓝牙名
```C
1. static const u8 dongle_remoter_name1[] = "AC696X_1(BLE)";//
2. static const u8 dongle_remoter_name2[] = "AC630N_HID123(BLE)";// 自动连接同名的从机
```

* 默认上电 10 秒根据信号强度 rssi 配对近距离的设备，若配对失败，停止搜索。回连已有的配
对设备。
```C
1. //dongle 上电开配对管理,若配对失败，没有配对设备，停止搜索
2. #define POWER_ON_PAIR_START (1)//
3. #define POWER_ON_PAIR_TIME (10000)//unit ms,持续配对搜索时间
4. #define DEVICE_RSSI_LEVEL (-50)
5. #define POWER_ON_KEEP_SCAN (0)//配对失败，保持一直搜索配对
```

* 主要代码说明
 蓝牙 dongle 实现文件 app_dongle.c 和 ble_dg_central，负责模块初始化、处理协议栈事件和命令
数据控制发送等。
 * HID 描述符, 描述为一个鼠标
```C
    26. static const u8 sHIDReportDesc[] = {
    27. 0x05, 0x01, // Usage Page (Generic Desktop Ctrls)
    28. 0x09, 0x02, // Usage (Mouse)
    29. 0xA1, 0x01, // Collection (Application)
    30. 0x85, 0x01, // Report ID (1)
    31. 0x09, 0x01, // Usage (Pointer)
    32.
```
* 使用指定的 uuid 与从机通信, 需要与从机配合, 省掉了搜索 uuid 的时间
```C
    33. static const target_uuid_t dongle_search_ble_uuid_table[] = {
    34. {
    35.     .services_uuid16 = 0x1812,
    36.     .characteristic_uuid16 = 0x2a4d,
    37.     .opt_type = ATT_PROPERTY_NOTIFY,
    38. },
    39.
    40. {
    41.     .services_uuid16 = 0x1812,
    42.     .characteristic_uuid16 = 0x2a33,
    43.     .opt_type = ATT_PROPERTY_NOTIFY,
    44. },
    45.
    46. {
    47.     .services_uuid16 = 0x1801,
    48.     .characteristic_uuid16 = 0x2a05,
    49.     .opt_type = ATT_PROPERTY_INDICATE,
    50. },
    51.
    52. };

```

```C
    1. /*
    2. 确定留给从机发数据的 3 个 notify handle
    3. */
    4. static const u16 mouse_notify_handle[3] = {0x0027, 0x002b, 0x002f};
```

* 用于监听从机 notify 数据, 并通过 USB 向 PC 转发蓝牙数据
```C
1.//edr 接收设备数据
2.static void dongle_edr_hid_input_handler(u8 *packet, u16 size, u16 channel)
3.{
4.    log_info("hid_data_input:chl=%d,size=%d", channel, size);
5.    put_buf(packet, size);

1.//ble 接收设备数据
2.void dongle_ble_hid_input_handler(u8 *packet, u16 size)
3.{
4.#if TCFG_PC_ENABLE
5.    hid_send_data(packet, size);
6.#endif
7.}

```
* 配置 BLE 的连接方式配置，可以选择通过地址或设备名等方式连接
```C
1. static const client_match_cfg_t dg_match_device_table[] = {
2. {
3. .create_conn_mode = BIT(CLI_CREAT_BY_NAME),
4. .compare_data_len = sizeof(dg_test_remoter_name1) - 1, //去结束符
5. .compare_data = dg_test_remoter_name1,
6. },
```

# Bluetooth Dual-Mode AT Moudle (char)
## 7.1 概述
主要功能是在普通数传 BLE 和 EDR 的基础上增加了由上位机或其他 MCU 可以通过 UART 对
接蓝牙芯片进行基本配置、状态获取、控制扫描、连接断开以及数据收发等操作。

AT 控制透传主从多机模式。

定义一套串口的控制协议，具体请查看协议文档《蓝牙 AT_CHAR 协议》。

支持的板级： bd29、br25、br23、br30、bd19、br34

支持的芯片： AC631N、AC636N、AC635N、AC637N、AC632N、AC638N

注意不同芯片可以使用 RAM 的空间有差异，有可能会影响性能。

## 7.2 工程配置
* 先配置板级 board_config.h 和对应配置文件中蓝牙双模使能

```C
 /*
 * 板级配置选择
 */
 #define CONFIG_BOARD_AC630X_DEMO
 // #define CONFIG_BOARD_AC6311_DEMO
 // #define CONFIG_BOARD_AC6313_DEMO
 // #define CONFIG_BOARD_AC6318_DEMO
 // #define CONFIG_BOARD_AC6319_DEMO
```

* 配置对应的 board_acxxx_demo_cfg.h 文件使能 BLE 以 board_ac630x_demo_cfg.h 为例

```C
 #define TCFG_USER_BLE_ENABLE 1 //BLE 功能使能
 #define TCFG_USER_EDR_ENABLE 0 //EDR 功能使能
```

* 配置 app_config.h, 选择 CONFIG_APP_AT_CHAR_COM
```C
//apps example 选择,只能选1个,要配置对应的board_config.h
    #define CONFIG_APP_SPP_LE                 0 //SPP + LE or LE's client
    #define CONFIG_APP_MULTI                  0 //蓝牙LE多连 + spp
    #define CONFIG_APP_DONGLE                 0 //usb + 蓝牙(ble 主机),PC hid设备
    #define CONFIG_APP_CENTRAL                0 //ble client,中心设备
    #define CONFIG_APP_LL_SYNC                0 //腾讯连连
    #define CONFIG_APP_BEACON                 0 //蓝牙BLE ibeacon
    #define CONFIG_APP_NONCONN_24G            0 //2.4G 非连接收发
    #define CONFIG_APP_TUYA                   0 //涂鸦协议
    #define CONFIG_APP_AT_COM                 0 //AT com HEX格式命令
    #define CONFIG_APP_AT_CHAR_COM            1 //AT com 字符串格式命令
    #define CONFIG_APP_IDLE                   0 //空闲任务
```
## 7.3 主要代码说明at_char_cmds.c
* 命令包头
```C
static const char at_head_at_cmd[]     = "AT+";
static const char at_head_at_chl[]     = "AT>";
static const char at_str_enter[]       = "\r\n";
static const char at_str_ok[]          = "OK";
static const char at_str_err[]         = "ERR";

static const str_info_t at_head_str_table[] = {
       INPUT_STR_INFO(STR_ID_HEAD_AT_CMD, at_head_at_cmd),
       INPUT_STR_INFO(STR_ID_HEAD_AT_CHL, at_head_at_chl),
 };
```
* 命令类型
```C
static const char at_str_gver[]        = "GVER";
static const char at_str_gcfgver[]     = "GCFGVER";
static const char at_str_name[]        = "NAME";
static const char at_str_lbdaddr[]     = "LBDADDR";
static const char at_str_baud[]        = "BAUD";

static const char at_str_adv[]         = "ADV";
static const char at_str_advparam[]    = "ADVPARAM";
static const char at_str_advdata[]     = "ADVDATA";
static const char at_str_srdata[]      = "SRDATA";
static const char at_str_connparam[]   = "CONNPARAM";

static const char at_str_scan[]        = "SCAN";
static const char at_str_targetuuid[]  = "TARGETUUID";
static const char at_str_conn[]        = "CONN";
static const char at_str_disc[]        = "DISC";
static const char at_str_ota[]         = "OTA";


//static const char at_str_[]  = "";
//static const char at_str_[]  = "";
//static const char at_str_[]  = "";
static const char specialchar[]        = {'+', '>', '=', '?', '\r', ','};

enum {
    AT_CMD_OPT_NULL = 0,
    AT_CMD_OPT_SET, //设置
    AT_CMD_OPT_GET, //查询
};

#define INPUT_STR_INFO(id,string)  {.str_id = id, .str = string, .str_len = sizeof(string)-1,}

static const str_info_t at_head_str_table[] = {
    INPUT_STR_INFO(STR_ID_HEAD_AT_CMD, at_head_at_cmd),
    INPUT_STR_INFO(STR_ID_HEAD_AT_CHL, at_head_at_chl),
};

static const str_info_t at_cmd_str_table[] = {
    INPUT_STR_INFO(STR_ID_GVER, at_str_gver),
    INPUT_STR_INFO(STR_ID_GCFGVER, at_str_gcfgver),
    INPUT_STR_INFO(STR_ID_NAME, at_str_name),
    INPUT_STR_INFO(STR_ID_LBDADDR, at_str_lbdaddr),
    INPUT_STR_INFO(STR_ID_BAUD, at_str_baud),

    INPUT_STR_INFO(STR_ID_ADV, at_str_adv),
    INPUT_STR_INFO(STR_ID_ADVPARAM, at_str_advparam),
    INPUT_STR_INFO(STR_ID_ADVDATA, at_str_advdata),
    INPUT_STR_INFO(STR_ID_SRDATA, at_str_srdata),
    INPUT_STR_INFO(STR_ID_CONNPARAM, at_str_connparam),

    INPUT_STR_INFO(STR_ID_SCAN, at_str_scan),
    INPUT_STR_INFO(STR_ID_TARGETUUID, at_str_targetuuid),
    INPUT_STR_INFO(STR_ID_CONN, at_str_conn),
    INPUT_STR_INFO(STR_ID_DISC, at_str_disc),
    INPUT_STR_INFO(STR_ID_OTA, at_str_ota),

//    INPUT_STR_INFO(, ),
//    INPUT_STR_INFO(, ),
};
```

* 串口收数中断回调

  进入收数中断之前会进行初步校验, 数据结尾不是’\r’, 则无法唤醒中断
```C
 static void at_packet_handler(u8 *packet, int size)
```

* AT 命令解析
```C
68.        /*比较数据包头*/
    str_p = at_check_match_string(parse_pt, parse_size, at_head_str_table, sizeof(at_head_str_table));
    if (!str_p) {
        log_info("###1unknow at_head:%s", packet);
        at_respond_send_err(ERR_AT_CMD);
        return;
    }
    parse_pt   += str_p->str_len;
    parse_size -= str_p->str_len;

    if (str_p->str_id == STR_ID_HEAD_AT_CMD) {
        str_p = at_check_match_string(parse_pt, parse_size, at_cmd_str_table, sizeof(at_cmd_str_table));
        if (!str_p) {
            log_info("###2unknow at_cmd:%s", packet);
            at_respond_send_err(ERR_AT_CMD);
            return;
        }

        parse_pt    += str_p->str_len;
        parse_size -= str_p->str_len;
        if (parse_pt[0] == '=') {
            operator_type = AT_CMD_OPT_SET;
        } else if (parse_pt[0] == '?') {
            operator_type = AT_CMD_OPT_GET;
        }
        parse_pt++;
    } else if (str_p->str_id == STR_ID_HEAD_AT_CMD) {
        operator_type = AT_CMD_OPT_SET;
    }

    //    if(operator_type == AT_CMD_OPT_NULL)
    //    {
    //        AT_STRING_SEND(at_str_err);
    //        log_info("###3unknow operator_type:%s", packet);
    //        return;
    //    }

    log_info("str_id:%d", str_p->str_id);

    par = parse_param_split(parse_pt, ',', '\r');

    log_info("\n par->data: %s", par->data);

    switch (str_p->str_id) {
    case STR_ID_HEAD_AT_CHL: {
        u8 tmp_cid = func_char_to_dec(par->data, '\0');
        if (tmp_cid == 9) {
            black_list_check(0, NULL);
        }

        log_info("STR_ID_HEAD_AT_CHL:%d\n", tmp_cid);
        AT_STRING_SEND("OK");  //响应
        cur_atcom_cid = tmp_cid;
    }
    break;
```

* 命令的参数获取与遍历

```C
133.par = parse_param_split(parse_pt,',','\r');
134./*parameter
135.*packet: 参数指针
136.split_char: 参数之间的间隔符, 一般是’,’
137.end_char:  参数的结束符, 一般是’\r’
138.*/
139.static at_param_t *parse_param_split(const u8 *packet,u8 split_char,u8 end_char)
140.{
141.    u8 char1;
142.    int i = 0;
143.    at_param_t *par = parse_buffer;
144.
145.    if (*packet == end_char) {
146.        return NULL;
147.    }
148.
149.    log_info("%s:%s",__FUNCTION__,packet);
150.
151.    par->len = 0;
152.
153.    while (1) {
154.        char1 = packet[i++];
155.        if (char1 == end_char) {
156.            par->data[par->len] = 0;
157.            par->next_offset = 0;
158.            break;
159.        } else if (char1 == split_char) {
160.            par->data[par->len] = 0;
161.            par->len++;
162.            par->next_offset = &par->data[par->len] - parse_buffer;
163.
164.            //init next par
165.            par = &par->data[par->len];
166.            par->len = 0;
167.        } else {
168.            par->data[par->len++] = char1;
169.        }
170.
171.        if (&par->data[par->len] - parse_buffer >= PARSE_BUFFER_SIZE) {
172.            log_error("parse_buffer over");
173.            par->next_offset = 0;
174.            break;
175.        }
176.    }
177.    return (void *)parse_buffer;
178.}
```

* 当有多个参数时, 需要遍历获取, 以连接参数为例

```C
while (par) {  //遍历所有参数
    log_info("len=%d,par:%s", par->len, par->data);
     conn_param[i] = func_char_to_dec(par->data, '\0');  //获取参数
        if (par->next_offset) {
            par = AT_PARAM_NEXT_P(par);
            } else {
                break;
            }
            i++;
                }
```

* 默认信息

```C
//默认参数
#define G_VERSION "JL_test"
#define CONFIG_VERSION "2021_02_04"
char const device_name_default[] = "JL_device";
u16 adv_interval_default = 2048;
u32 uart_baud = 115200;
```

* 在代码中添加新的命令(以查询、设置波特率为例)

  * 添加波特率枚举成员

```C
    enum {
    STR_ID_NULL = 0,
    STR_ID_HEAD_AT_CMD,
    STR_ID_HEAD_AT_CHL,

    STR_ID_OK = 0x10,
    STR_ID_ERROR,

    STR_ID_GVER = 0x20,
    STR_ID_GCFGVER,
    STR_ID_NAME,
    STR_ID_LBDADDR,
    STR_ID_BAUD,

    STR_ID_ADV,
    STR_ID_ADVPARAM,
    STR_ID_ADVDATA,
    STR_ID_SRDATA,
    STR_ID_CONNPARAM,

    STR_ID_SCAN,
    STR_ID_TARGETUUID,
    STR_ID_CONN,
    STR_ID_DISC,
    STR_ID_OTA,
//    STR_ID_,
//    STR_ID_,
};
```

  * 添加波特率命令类型
```C
static const char at_head_at_cmd[]     = "AT+";
static const char at_head_at_chl[]     = "AT>";
static const char at_str_enter[]       = "\r\n";
static const char at_str_ok[]          = "OK";
static const char at_str_err[]         = "ERR";

static const char at_str_gver[]        = "GVER";
static const char at_str_gcfgver[]     = "GCFGVER";
static const char at_str_name[]        = "NAME";
static const char at_str_lbdaddr[]     = "LBDADDR";
static const char at_str_baud[]        = "BAUD";

static const char at_str_adv[]         = "ADV";
static const char at_str_advparam[]    = "ADVPARAM";
static const char at_str_advdata[]     = "ADVDATA";
static const char at_str_srdata[]      = "SRDATA";
static const char at_str_connparam[]   = "CONNPARAM";

static const char at_str_scan[]        = "SCAN";
static const char at_str_targetuuid[]  = "TARGETUUID";
static const char at_str_conn[]        = "CONN";
static const char at_str_disc[]        = "DISC";
static const char at_str_ota[]         = "OTA";
```

  * 添加命令到命令列表

```C
   static const str_info_t at_cmd_str_table[] = {
    INPUT_STR_INFO(STR_ID_GVER, at_str_gver),
    INPUT_STR_INFO(STR_ID_GCFGVER, at_str_gcfgver),
    INPUT_STR_INFO(STR_ID_NAME, at_str_name),
    INPUT_STR_INFO(STR_ID_LBDADDR, at_str_lbdaddr),
    INPUT_STR_INFO(STR_ID_BAUD, at_str_baud),

    INPUT_STR_INFO(STR_ID_ADV, at_str_adv),
    INPUT_STR_INFO(STR_ID_ADVPARAM, at_str_advparam),
    INPUT_STR_INFO(STR_ID_ADVDATA, at_str_advdata),
    INPUT_STR_INFO(STR_ID_SRDATA, at_str_srdata),
    INPUT_STR_INFO(STR_ID_CONNPARAM, at_str_connparam),

    INPUT_STR_INFO(STR_ID_SCAN, at_str_scan),
    INPUT_STR_INFO(STR_ID_TARGETUUID, at_str_targetuuid),
    INPUT_STR_INFO(STR_ID_CONN, at_str_conn),
    INPUT_STR_INFO(STR_ID_DISC, at_str_disc),
    INPUT_STR_INFO(STR_ID_OTA, at_str_ota),

//    INPUT_STR_INFO(, ),
//    INPUT_STR_INFO(, ),
};
```
  * 在at_packet_handler函数中添加命令的处理与响应
```C
    case STR_ID_BAUD:
        log_info("STR_ID_BAUD\n");
        {
            if (operator_type == AT_CMD_OPT_SET) { //2.7
                uart_baud = func_char_to_dec(par->data, '\0');
                log_info("set baud= %d", uart_baud);
                if (uart_baud == 9600 || uart_baud == 19200 || uart_baud == 38400 || uart_baud == 115200 ||
                    uart_baud == 230400 || uart_baud == 460800 || uart_baud == 921600) {
                    AT_STRING_SEND("OK");
                    ct_uart_change_baud(uart_baud);
                } else { //TODO返回错误码
                    at_respond_send_err(ERR_AT_CMD);
                }
            } else {                            //2.6

                sprintf(buf, "+BAUD:%d", uart_baud);
                at_cmd_send(buf, strlen(buf));
                AT_STRING_SEND("OK");
            }
        }
        break;
```
  * 串口发数api, 用于发送响应信息
```C
    285./*
    286.parameter
    287.packet: 数据包
    288.size: 数据长度
    289.*/
    290.at_uart_send_packet(const u8 *packet, int size);
```
  * 用于回复带”\r\n”的响应
```C
void at_cmd_send(const u8 *packet, int size)
{
    log_info("######at_cmd_send(%d):", size);
    // put_buf(packet, size);

    at_uart_send_packet(at_str_enter, 2);
    at_uart_send_packet(packet, size);
    at_uart_send_packet(at_str_enter, 2);
}
```

# APP - Nonconn_24G
## 8.1 概述
主要功能是在蓝牙BLE架构基础上自定义不可见的2.4G非连接模式数据传输示例。
支持的板级： bd29、br25、br23、br30、bd19、br34
支持的芯片： AC631N、AC636N、AC635N、AC637N、AC632N、AC638N

## 8.2 工程配置
代码工程：apps\spp_and_le\board\bdxx\AC63xN_spp_and_le.cbp

* 配置app选择(apps\spp_and_le\include\app_config.h)，如下选择对应的应用示例。
```C
//apps example 选择,只能选1个,要配置对应的board_config.h
    #define CONFIG_APP_SPP_LE                 0 //SPP + LE or LE's client
    #define CONFIG_APP_MULTI                  0 //蓝牙LE多连 + spp
    #define CONFIG_APP_DONGLE                 0 //usb + 蓝牙(ble 主机),PC hid设备
    #define CONFIG_APP_CENTRAL                0 //ble client,中心设备
    #define CONFIG_APP_LL_SYNC                0 //腾讯连连
    #define CONFIG_APP_BEACON                 0 //蓝牙BLE ibeacon
    #define CONFIG_APP_NONCONN_24G            1 //2.4G 非连接收发
    #define CONFIG_APP_TUYA                   0 //涂鸦协议
    #define CONFIG_APP_AT_COM                 0 //AT com HEX格式命令
    #define CONFIG_APP_AT_CHAR_COM            0 //AT com 字符串格式命令
    #define CONFIG_APP_IDLE                   0 //空闲任务
```
* 先配置板级board_config.h(apps\hid\board\brxx\board_config.h)，选择对应的开发板，可以使用默认的板级。
```C
1.#define CONFIG_BOARD_AC632N_DEMO
2.// #define CONFIG_BOARD_AC6321A_DEMO
3.
4.#include "board_ac632n_demo_cfg.h"
5.#include "board_ac6321a_demo_cfg.h"
```
* 只需要使能BLE就可以了
```C
 #define TCFG_USER_BLE_ENABLE                      1   //BLE功能使能
 #define TCFG_USER_EDR_ENABLE                      0   //EDR功能使能
```
## 8.3 数据收发模块
实现代码文件在ble_24g_deal.c

* 主要配置宏如下
```C
1.//------------------------------------------------------
2.#define CFG_RF_24G_CODE_ID    0x13 // 24g 识别码(24bit),发送接收都要匹配
3.//------------------------------------------------------
4.//配置收发角色
5.#define CONFIG_TX_MODE_ENABLE     1 //发射器
6.#define CONFIG_RX_MODE_ENABLE     0 //接收器
7.//------------------------------------------------------
8.//TX发送配置
9.#define TX_DATA_COUNT             3  //发送次数,决定os_time_dly 多久
10.#define TX_DATA_INTERVAL          20 //发送间隔>=20ms
11.
12.#define ADV_INTERVAL_VAL          ADV_SCAN_MS(TX_DATA_INTERVAL)//
13.#define RSP_TX_HEAD               0xff
14.//------------------------------------------------------
15.//RX接收配置
16.//搜索类型
17.#define SET_SCAN_TYPE       SCAN_ACTIVE
18.//搜索 周期大小
19.#define SET_SCAN_INTERVAL   ADV_SCAN_MS(200)//
20.//搜索 窗口大小
21.#define SET_SCAN_WINDOW     ADV_SCAN_MS(200)//
```
* 发射器发送接口
```C
1.//发送数据, len support max is 60
2.int ble_tx_send_data(const u8 *data, u8 len)
```
* 接收器接收接口
```C
void ble_rx_data_handle(const u8 *data, u8 len)
```

# APP - Tecent LL
## 9.1 概述
本案例用于实现腾讯连连协议，使用腾讯连连微信小程序与设备连接后可以对设备进行控制。
支持的板级： bd29、br25、br23、br30、bd19、br34
支持的芯片： AC631N、AC636N、AC635N、AC637N、AC632N、AC638N
## 9.2 工程配置
代码工程：apps\spp_and_le\board\bdxx\AC63xN_spp_and_le.cbp

* 配置app选择(apps\spp_and_le\include\app_config.h)，如下图选择对应的应用示例。
```C
//apps example 选择,只能选1个,要配置对应的board_config.h
    #define CONFIG_APP_SPP_LE                 0 //SPP + LE or LE's client
    #define CONFIG_APP_MULTI                  0 //蓝牙LE多连 + spp
    #define CONFIG_APP_DONGLE                 0 //usb + 蓝牙(ble 主机),PC hid设备
    #define CONFIG_APP_CENTRAL                0 //ble client,中心设备
    #define CONFIG_APP_LL_SYNC                1 //腾讯连连
    #define CONFIG_APP_BEACON                 0 //蓝牙BLE ibeacon
    #define CONFIG_APP_NONCONN_24G            0 //2.4G 非连接收发
    #define CONFIG_APP_TUYA                   0 //涂鸦协议
    #define CONFIG_APP_AT_COM                 0 //AT com HEX格式命令
    #define CONFIG_APP_AT_CHAR_COM            0 //AT com 字符串格式命令
    #define CONFIG_APP_IDLE                   0 //空闲任务
```
* 配置板级蓝牙设置（apps\spp_and_le\board\brxx\board_acxxxx_demo.cfg）,只开BLE不开EDR
```C
1.//**********************************************************//
2.//                    蓝牙配置                               //
3.//**********************************************************//
4.#define TCFG_USER_TWS_ENABLE                      0   //tws功能使能
5.#define TCFG_USER_BLE_ENABLE                      1   //BLE功能使能
6.#define TCFG_USER_EDR_ENABLE                      0   //EDR功能使能
```
## 模块开发
详细参考 《腾讯连连开发文档》。

# APP - TUYA
## 10.1 概述
本案例用于实现涂鸦协议，使用涂鸦智能或者涂鸦云测APP与设备连接后可以对设备进行控制。
支持的板级： bd29、br25、br23、br30、bd19、br34
支持的芯片： AC631N、AC636N、AC635N、AC637N、AC632N、AC638N

## 10.2 工程配置
代码工程：apps\spp_and_le\board\bdxx\AC63xN_spp_and_le.cbp

* 配置app选择(apps\spp_and_le\include\app_config.h)，如下图选择对应的应用示例。
```C
3.//app case 选择,只能选1个,要配置对应的board_config.h
4.#define CONFIG_APP_TUYA                0 //涂鸦协议/
```
* 配置板级蓝牙设置（apps\spp_and_le\board\brxx\board_acxxxx_demo.cfg）,只开BLE不开EDR
```C
7.//**********************************************************//
8.//                    蓝牙配置                               //
9.//**********************************************************//
10.#define TCFG_USER_TWS_ENABLE                      0   //tws功能使能
11.#define TCFG_USER_BLE_ENABLE                      1   //BLE功能使能
12.#define TCFG_USER_EDR_ENABLE                      0   //EDR功能使能
```
## 10.3 模块开发
详细参考 《涂鸦协议开发文档》。

# APP - IBEACON
## 11.1 概述
蓝牙beacon即蓝牙信标，是通过BLE广播特定数据格式的广播包，接收终端通过扫描获取BLE广播包信息，再根据协议进行解析。
接收终端和蓝牙beacon之间的通信，不需要建立蓝牙连接。
目前的应用：
1.蓝牙信标室内定位，具有简单，低功耗，手机兼容性好的优点。
2.消息推送，见于大型商场的推销活动等。3.考勤打卡，身份识别。

支持板级：bd19、br23、br25、bd29、br30
支持芯片：AC632N、AC635N、AC636N、AC631N、AC637N

## 11.2 工程配置
代码工程：apps\spp_and_le\board\bdxx\AC63xN_spp_and_le.cbp

* 在app_config.文件中进行IBEACON应用的配置。
```C
//apps example 选择,只能选1个,要配置对应的board_config.h
    #define CONFIG_APP_SPP_LE                 0 //SPP + LE or LE's client
    #define CONFIG_APP_MULTI                  0 //蓝牙LE多连 + spp
    #define CONFIG_APP_DONGLE                 0 //usb + 蓝牙(ble 主机),PC hid设备
    #define CONFIG_APP_CENTRAL                0 //ble client,中心设备
    #define CONFIG_APP_LL_SYNC                0 //腾讯连连
    #define CONFIG_APP_BEACON                 0 //蓝牙BLE ibeacon
    #define CONFIG_APP_NONCONN_24G            0 //2.4G 非连接收发
    #define CONFIG_APP_TUYA                   1 //涂鸦协议
    #define CONFIG_APP_AT_COM                 0 //AT com HEX格式命令
    #define CONFIG_APP_AT_CHAR_COM            0 //AT com 字符串格式命令
    #define CONFIG_APP_IDLE                   0 //空闲任务

35.//配置对应的APP的蓝牙功能
36.#if CONFIG_APP_SPP_LE
37.#define TRANS_DATA_EN                     0 //蓝牙双模透传
38.#define TRANS_CLIENT_EN                   0 //蓝牙(ble主机)透传
39.#define BEACON_MODE_EN                    1 //蓝牙BLE ibeacon
40.#define XM_MMA_EN                          0
```
## 11.3 主要代码说明
* 配置应用case之后，根据定义好的数据格式，制作信标数据包,该函数位于le_beacon.c文件。
```C
41.static u8 make_beacon_packet(u8 *buf, void *packet, u8 packet_type, u8 *web)
42.{
43.    switch (packet_type) {
44.    case IBEACON_PACKET:
45.    case EDDYSTONE_UID_PACKET:
46.    case EDDYSTONE_TLM_PACKET:
47.        memcpy(buf, (u8 *)packet, packet_type);
48.        break;
49.    case EDDYSTONE_EID_PACKET:
50.        memcpy(buf, (u8 *)packet, packet_type);
51.        break;
52.    case EDDYSTONE_ETLM_PACKET:
53.        memcpy(buf, (u8 *)packet, packet_type);
54.        break;
55.    case EDDYSTONE_URL_PACKET:
56.        packet_type = make_eddystone_url_adv_packet(buf, packet, web);
57.        break;
58.    }
59.    return packet_type;
60.}
```
* 例如下面是eddystone_etlm的数据格式，将该数据制作成数据包后，通过make_set_adv_data()将数据发送出去，
该函数同样位于le_beacon.c文件中，同时信标的广播类型应是广播不连接的ADV_NONCONN_IND类型。
```C
61.static const EDDYSTONE_ETLM eddystone_etlm_adv_packet = {
62.    .length = 0x03,
63.    .ad_type1 = 0x03,
64.    .complete_list_uuid = 0xabcd,
65.    .length_last = 0x15,
66.    .ad_type2 = 0x16,
67.    .eddystone_uuid = 0xfeaa,
68.    .frametype = 0x20,
69.    .tlm_version = 0x01，
70.    .etml = {
71.        0, 0, 0, 0,
72.        0, 0, 0, 0,
73.        0, 0, 0, 0
74.    },         //12字节加密数据
75.    .random = 1,               //随机数,要与加密时用到的随机数相同
76.    .check = 2，                //AES-EAX计算出来的校验和
77.};

```
* 将数据以广播包的形式发送出去，客户端通过扫描获取到信标发出去的数据。
```C
78.static int make_set_adv_data(void)
79.{
80.    u8 offset = 0;
81.    u8 *buf = adv_data;
82.  /* offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, 0x06, 1); */
     offset+=make_beacon_packet(&buf[offset],&eddystone_etlm_adv_packet,EDDYSTONE_ETLM_PACKET, NULL);
83.  offset+=make_beacon_packet(&buf[offset],&eddystone_eid_adv_packet,EDDYSTONE_EID_PACKET, NULL); */
84. //offset+=make_beacon_packet(&buf[offset],&eddystone_url_adv_packet,EDDYSTONE_URL_PACKET, "https://fanyi.baidu.com/");
85. //offset+=make_beacon_packet(&buf[offset],&eddystone_tlm_adv_packet,EDDYSTONE_TLM_PACKET, NULL);
86. //offset+=make_beacon_packet(&buf[offset],&eddystone_uid_adv_packet,EDDYSTONE_UID_PACKET, NULL);
87. // offset += make_beacon_packet(&buf[offset], &ibeacon_adv_packet, IBEACON_PACKET, NULL);
88.    if (offset > ADV_RSP_PACKET_MAX) {
89.        puts("***adv_data overflow!!!!!!\n");
90.        return -1;
91.    }
```

# APP - Bluetooth Multi connections
## 12.1 概述
支持蓝牙双模透传传输数传，支持蓝牙LE多连接功能。CLASSIC蓝牙使用标准串口SPP profile协议，支持可发现搜索连接功能。
蓝牙LE目前支持GAP  1主1从，或者2主的角色等应用。

支持的板级： bd29、br25、br23、br30、bd19、br34
支持的芯片： AC631N、AC636N、AC635N、AC637N、AC632N、AC638N
注意不同芯片可以使用RAM的空间有差异，有可能会影响性能。

## 12.2 工程配置
代码工程：apps\spp_and_le\board\br30\AC637N_spp_and_le.cbp
在工程代码中找到对应的文件(apps\hid\include\app_config.h)进行APP选择，本案例中选择翻页器，其结果如下所示：   
```C
//apps example 选择,只能选1个,要配置对应的board_config.h
    #define CONFIG_APP_SPP_LE                 0 //SPP + LE or LE's client
    #define CONFIG_APP_MULTI                  1 //蓝牙LE多连 + spp
    #define CONFIG_APP_DONGLE                 0 //usb + 蓝牙(ble 主机),PC hid设备
    #define CONFIG_APP_CENTRAL                0 //ble client,中心设备
    #define CONFIG_APP_LL_SYNC                0 //腾讯连连
    #define CONFIG_APP_BEACON                 0 //蓝牙BLE ibeacon
    #define CONFIG_APP_NONCONN_24G            0 //2.4G 非连接收发
    #define CONFIG_APP_TUYA                   0 //涂鸦协议
    #define CONFIG_APP_AT_COM                 0 //AT com HEX格式命令
    #define CONFIG_APP_AT_CHAR_COM            0 //AT com 字符串格式命令
    #define CONFIG_APP_IDLE                   0 //空闲任务
8.//蓝牙多连接
9.#if CONFIG_APP_MULTI
10.//spp+le 组合enable,多开注意RAM的使用
11.//le 多连
12.#define TRANS_MULTI_BLE_EN                1 //蓝牙BLE多连:1主1从,或者2主
13.#define TRANS_MULTI_BLE_SLAVE_NUMS        1 //range(0~1)
14.#define TRANS_MULTI_BLE_MASTER_NUMS       1 //range(0~2)
```
* 板级选择
接着在文件(apps\hid\board\bd30\board_config.h)下进行对应的板级选择如下:
```C
1./*
2. *  板级配置选择
3. */
4.
5.#define CONFIG_BOARD_AC637N_DEMO
6.// #define CONFIG_BOARD_AC6373B_DEMO
7.// #define CONFIG_BOARD_AC6376F_DEMO
8.// #define CONFIG_BOARD_AC6379B_DEMO
9.
10.#include "board_ac637n_demo_cfg.h"
11.#include "board_ac6373b_demo_cfg.h"
12.#include "board_ac6376f_demo_cfg.h"
13.#include "board_ac6379b_demo_cfg.h"
14.#endif
```
* 对应的板级头文件，配置是否打开 edr 和ble模块
```C
17.#define TCFG_USER_BLE_ENABLE                      1   //BLE功能使能  
18.#define TCFG_USER_EDR_ENABLE                      0   //EDR功能使能
```
* 蓝牙协议栈配置lib_btstack_config.c
```C
1.#elif TRANS_MULTI_BLE_EN
2.const int config_le_hci_connection_num = 2;//支持同时连接个数
3.const int config_le_sm_support_enable = 0; //是否支持加密配对
4.const int config_le_gatt_server_num = 1;   //支持server角色个数
5.const int config_le_gatt_client_num = 1;   //支持client角色个数
```
* 蓝牙控制器配置lib_btctrler_config.c
```C
1.#elif (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_MULTI)
2.const uint64_t config_btctler_le_features = LE_ENCRYPTION;
3.const int config_btctler_le_roles    = (LE_ADV | LE_SLAVE) | (LE_SCAN | LE_INIT | LE_MASTER);

1.#elif (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_MULTI)
2.// Master AFH
3.const int config_btctler_le_afh_en = 0;
4.// LE RAM Control
5.const int config_btctler_le_hw_nums = 2; //控制器的状态机个数
6.const int config_btctler_le_rx_nums = 10; //接收acl 的条数
7.const int config_btctler_le_acl_packet_length = 27;//acl 包的长度 range(27~251)
8.const int config_btctler_le_acl_total_nums = 10;//发送acl 的条数
```
* APPS 配置 ATT 的MTU大小和发送缓存CBUF的大小
```C
1.#define MULTI_ATT_MTU_SIZE       (512) //ATT MTU的值
2.
3.//ATT发送的包长,    note: 20 <= need >= MTU
4.#define MULTI_ATT_LOCAL_PAYLOAD_SIZE    (MULTI_ATT_MTU_SIZE)                   //
5.//ATT缓存的buffer大小,  note: need >= 20,可修改
6.#define MULTI_ATT_SEND_CBUF_SIZE        (512*3)                 //
```
## 12.3 主要代码说明
主任务处理文件apps/hid/app_multi_conn.c

* APP注册处理(函数位于apps/hid/app_multi_conn.c) 

在系统进行初始化的过程中，根据以下信息进行APP注册。执行的大致流程为：
REGISTER_APPLICATION--->state_machine--->app_start()--->sys_key_event_enable();
这条流程主要进行设备的初始化设置以及一些功能使能。
REGISTER_APPLICATION--->event_handler--->app_key_event_handler()--->app_key_deal_test();
这条流程在event_handler之下有多个case,上述选择按键事件的处理流程进行代码流说明，主要展示按键事件发生时，程序的处理流程。

```C
1.static const struct application_operation app_multi_ops = {
2.    .state_machine  = state_machine,
3.    .event_handler  = event_handler,
4.};
5.
6./*
7. * 注册AT Module模式
8. */
9.REGISTER_APPLICATION(app_multi) = {
10.    .name  = "multi_conn",
11.    .action = ACTION_MULTI_MAIN,
12.    .ops  = &app_multi_ops,
13.    .state  = APP_STA_DESTROY,
14.};
15.
16.//-----------------------
17.//system check go sleep is ok
18.static u8 app_state_idle_query(void)
19.{
20.    return !is_app_active;
21.}
22.
23.REGISTER_LP_TARGET(app_state_lp_target) = {
24.    .name = "app_state_deal",
25.    .is_idle = app_state_idle_query,
26.};
```
* APP状态机

状态机有create，start，pause，resume，stop，destory状态，根据不同的状态执行对应的分支。APP注册后进行初始运行，进入APP_STA_START分支，开始APP运行。
```C
1. static int state_machine(struct application *app, enum app_state state, struct intent *it)  
2.{    switch (state) {  
3.    case APP_STA_CREATE:  
4.        break;  
5.    case APP_STA_START:  
6.        if (!it) {  
7.            break;          }  
8.        switch (it->action) {  
9.        case ACTION_MULTI_MAIN:  
10.            app_start();  
```
* 进入app_start()函数后进行对应的初始化，时钟初始化，模式选择，蓝牙初始，低功耗初始化，以及外部事件使能。
```C
1.static void app_start()  
2.{  
3.    log_info("=======================================");  
4.    log_info("--------multi_conn demo----------------");  
5.    log_info("=======================================");  
```
*  APP事件处理机制
事件的定义(代码位于Headers\include_lib\system\even.h中)
```C
1.struct sys_event {  
2.    u16 type;  
3.    u8 consumed;  
4.    void *arg;  
5.    union {  
6.        struct key_event key;  
7.        struct axis_event axis;  
8.        struct codesw_event codesw;  
```
* 事件的产生（include_lib\system\event.h）
```C
void sys_event_notify(struct sys_event *e);  	
```
* 事件的处理(app_mutil.c)
函数执行的大致流程为：evevt_handler()--->app_key_event_handler()--->app_key_deal_test().
```C
1.static int event_handler(struct application *app, struct sys_event *event)  

1.static void app_key_event_handler(struct sys_event *event)  

1.static void app_key_deal_test(u8 key_type, u8 key_value)  
```
* LE公共处理apps/hid/ble_multi_conn.c
模块初始化和退出接口
```C
1.void bt_ble_init(void)
2.
3.void bt_ble_exit(void)
```
模块功能开发使能
```C
1.void ble_module_enable(u8 en)
```
协议栈profile初始化
```C
1.void ble_profile_init(void)
```
协议栈回调HCI事件和SM事件处理函数
```C
1.static void cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
2.
3.void cbk_sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
```

* LE 公共处理 ble_multi_conn.c
配置 Gatt common 模块初始化，蓝牙初始化等操作。

* LE的GATT server 实现ble_multi_peripheral.c
   配置Gatt Server端的广播、连接、事件处理等。

* LE的GATT server 实现ble_multi_central.c
   配置Gatt Cleint端的搜索、连接、事件处理等。
