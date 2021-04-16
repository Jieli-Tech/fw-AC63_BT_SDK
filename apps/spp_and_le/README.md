[icon_build]:./../../doc/stuff/build_passing.svg

# APP - Bluetooth：SPP 透传 ![Build result][icon_build]
---------------

代码工程：`apps\spp_and_le\board\bd29\AC631X_spp_and_le.cbp` 

## 1.模块使能

    #define TRANS_DATA_EN                             1   //蓝牙双模透传    
    #define TCFG_USER_BLE_ENABLE                      1   //BLE功能使能  
    #define TCFG_USER_EDR_ENABLE                      1   //EDR功能使能

## 2.SPP数据通信

### 2.1 代码文件 `spp_trans_data.c`

### 2.2 接口说明

* SPP模块初始化:

    ```C 
    void transport_spp_init(void)
    { 
        spp_state = 0; 
        spp_get_operation_table(&spp_api);  
        spp_api->regist_recieve_cbk(0, transport_spp_recieve_cbk);  
        spp_api->regist_state_cbk(0, transport_spp_state_cbk);  
        spp_api->regist_wakeup_send(NULL, transport_spp_send_wakeup);  
    ```

* SPP连接和断开事件处理:
  
    ```C  
    static void transport_spp_state_cbk(u8 state)  
    {  
        spp_state = state;  
        switch (state) {  
        case SPP_USER_ST_CONNECT:  
            log_info("SPP_USER_ST_CONNECT ~~~\n");  
            break;  
      
        case SPP_USER_ST_DISCONN:  
            log_info("SPP_USER_ST_DISCONN ~~~\n");  1.
            break;  
        default:  
            break;  
        }  
    }   
    ```

* SPP发送数据接口，发送前先调用接口transport_spp_send_data_check检查:

    ```C
    int transport_spp_send_data(u8 *data, u16 len)  
    {  
        if (spp_api) {  
            log_info("spp_api_tx(%d) \n", len);  
            /* log_info_hexdump(data, len); */  
            clear_sniff_cnt();  
            return spp_api->send_data(NULL, data, len);  
        }  
        return SPP_USER_ERR_SEND_FAIL;  
    }
    ```

* SPP检查是否可以往协议栈发送数据:

    ```C
    int transport_spp_send_data_check(u16 len)  
    {  
        if (spp_api) {  
            if (spp_api->busy_state()) {  
                return 0;  
            }  
        }  
        return 1;  
    } 
    ```

* SPP发送完成回调，表示可以继续往协议栈发数，用来触发继续发数:
  
    ```C
    static void transport_spp_send_wakeup(void)  
    {  
        putchar('W');  
    }  
    ```

* SPP接收数据接口

    ```C
    static void transport_spp_recieve_cbk(void *priv, u8 *buf, u16 len)  
    {  
        log_info("spp_api_rx(%d) \n", len);  
        log_info_hexdump(buf, len);  
        clear_sniff_cnt();  
    ```

## 3.收发测试
* 代码已经实现收到手机的SPP数据后，会主动把数据回送，测试数据收发:

    ```C
    //loop send data for test  
    if (transport_spp_send_data_check(len)) {  
        transport_spp_send_data(buf, len);  
    }  
    ```

APP - Bluetooth：LE 透传
---------------

### 3.1 代码文件 `le_trans_data.c`

* Profile生成的profile_data数据表放在le_trans_data.h。用户可用工具make_gatt_services自定义修改,重新配置GATT服务和属性等。

    ```C
    static const uint8_t profile_data[] = {  
        //////////////////////////////////////////////////////  
        //  
        // 0x0001 PRIMARY_SERVICE  1800  
        //  
        //////////////////////////////////////////////////////  
        0x0a, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x28, 0x00, 0x18,  
        
        /* CHARACTERISTIC,  2a00, READ | WRITE | DYNAMIC, */  
        // 0x0002 CHARACTERISTIC 2a00 READ | WRITE | DYNAMIC  
        0x0d, 0x00, 0x02, 0x00, 0x02, 0x00, 0x03, 0x28, 0x0a, 0x03, 0x00, 0x00, 0x2a,  
        // 0x0003 VALUE 2a00 READ | WRITE | DYNAMIC  
        0x08, 0x00, 0x0a, 0x01, 0x03, 0x00, 0x00, 0x2a,  
        
        //////////////////////////////////////////////////////  
        //  
        // 0x0004 PRIMARY_SERVICE  ae30  
        //  
        //////////////////////////////////////////////////////  
        0x0a, 0x00, 0x02, 0x00, 0x04, 0x00, 0x00, 0x28, 0x30, 0xae,  
        
        /* CHARACTERISTIC,  ae01, WRITE_WITHOUT_RESPONSE | DYNAMIC, */  
        // 0x0005 CHARACTERISTIC ae01 WRITE_WITHOUT_RESPONSE | DYNAMIC  
        0x0d, 0x00, 0x02, 0x00, 0x05, 0x00, 0x03, 0x28, 0x04, 0x06, 0x00, 0x01, 0xae,  
        // 0x0006 VALUE ae01 WRITE_WITHOUT_RESPONSE | DYNAMIC  
        0x08, 0x00, 0x04, 0x01, 0x06, 0x00, 0x01, 0xae,  
    ```

## 3.2接口说明
* 协议栈事件回调处理，主要是连接、断开等事件

    ```C
    /* LISTING_START(packetHandler): Packet Handler */  
    static void cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)  
    {  
        int mtu;  
        u32 tmp;  
        u8 status;  
      
        switch (packet_type) {  
        case HCI_EVENT_PACKET:  
            switch (hci_event_packet_get_type(packet)) {  
    
            /* case DAEMON_EVENT_HCI_PACKET_SENT: */  
            /* break; */  
            case ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE:  
                log_info("ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE\n");  
            case ATT_EVENT_CAN_SEND_NOW:  
                can_send_now_wakeup();  
                break;  
      
            case HCI_EVENT_LE_META:  
                switch (hci_event_le_meta_get_subevent_code(packet)) {  
                case HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE:  
    ```

* ATT读事件处理

    ```C
    static uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)  
    {  
      
        uint16_t  att_value_len = 0;  
        uint16_t handle = att_handle;  
      
        log_info("read_callback, handle= 0x%04x,buffer= %08x\n", handle, (u32)buffer);  
      
        switch (handle) {  
        case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:  
            att_value_len = gap_device_name_len;  
      
            if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {  
                break;  
            }  
      
            if (buffer) {  
                memcpy(buffer, &gap_device_name[offset], buffer_size);  
                att_value_len = buffer_size;  
                log_info("\n------read gap_name: %s \n", gap_device_name);  
            }  
            break;
    ```

* ATT写事件处理

    ```C
    static int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)  
    {  
        int result = 0;  
        u16 tmp16;  
      
        u16 handle = att_handle;  
      
        log_info("write_callback, handle= 0x%04x,size = %d\n", handle, buffer_size);  
      
        switch (handle) {  
      
        case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:  
            break;
    ```

* NOTIFY 和 INDICATE 发送接口，发送前调接口app_send_user_data_check检查

    ```C
    static int app_send_user_data(u16 handle, u8 *data, u16 len, u8 handle_type)  
    {  
        u32 ret = APP_BLE_NO_ERROR;  
        if (!con_handle) {  
            return APP_BLE_OPERATION_ERROR;  
        }  
        if (!att_get_ccc_config(handle + 1)) {  
            log_info("fail,no write ccc!!!,%04x\n", handle + 1);  
            return APP_BLE_NO_WRITE_CCC;  
        }  
        ret = ble_user_cmd_prepare(BLE_CMD_ATT_SEND_DATA, 4, handle, data, len, handle_type);  
    ```

* 检查是否可以往协议栈发送数据

    ```C
    static int app_send_user_data_check(u16 len)  
    {  
        u32 buf_space = get_buffer_vaild_len(0);  
        if (len <= buf_space) {  
            return 1;  
        }  
        return 0;  
    } 
    ```

* 发送完成回调，表示可以继续往协议栈发数，用来触发继续发数

    ```C
    static void can_send_now_wakeup(void)  
    {  
        /* putchar('E'); */  
        if (ble_resume_send_wakeup) {  
            ble_resume_send_wakeup();  
        } 
    ```

* 调用示例

    ```C
    //收发测试，自动发送收到的数据;for test  
    if (app_send_user_data_check(buffer_size)) {  
        app_send_user_data(ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE, buffer, buffer_size, ATT_OP_AUTO_READ_CCC);  
    } 
    ```

* 收发测试
使用手机NRF软件，连接设备后；使能notify和indicate的UUID (AE02 和 AE05) 的通知功能后；可以通过向write的UUID (AE01 或 AE03) 发送数据；测试UUID (AE02 或 AE05)是否收到数据。

    ```C
    case ATT_CHARACTERISTIC_ae01_01_VALUE_HANDLE:  
            printf("\n-ae01_rx(%d):", buffer_size);  
            printf_buf(buffer, buffer_size);  
      
            //收发测试，自动发送收到的数据;for test  
            if (app_send_user_data_check(buffer_size)) {  
                app_send_user_data(ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE, buffer, buffer_size, ATT_OP_AUTO_READ_CCC);  
            }  
      
    #if TEST_SEND_DATA_RATE  
            if ((buffer[0] == 'A') && (buffer[1] == 'F')) {  
                test_data_start = 1;//start  
            } else if ((buffer[0] == 'A') && (buffer[1] == 'A')) {  
                test_data_start = 0;//stop  
            }  
    #endif  
            break;  
      
        case ATT_CHARACTERISTIC_ae03_01_VALUE_HANDLE:  
            printf("\n-ae_rx(%d):", buffer_size);  
            printf_buf(buffer, buffer_size);  
      
            //收发测试，自动发送收到的数据;for test  
            if (app_send_user_data_check(buffer_size)) {  
                app_send_user_data(ATT_CHARACTERISTIC_ae05_01_VALUE_HANDLE, buffer, buffer_size, ATT_OP_AUTO_READ_CCC);  
            }  
            break;  
    ```

APP - Bluetooth：AT 命令
--------------- 

## 4. 模块使能

    #define TRANS_AT_COM                               1 //串口控制对接蓝牙双模透传 
    #define TCFG_USER_BLE_ENABLE                      1   //BLE功能使能  
    #define TCFG_USER_EDR_ENABLE                      1   //EDR功能使能


### 4.1 概述
主要功能是在数传SPP+BLE的基础上，增加了由上位机或其他MCU可以通过UART对接蓝牙芯片进行基本的配置、状态获取、控制连接断开以及数据收发等操作。
定义一套串口的控制协议，具体请查看协议文档《蓝牙AT协议》。

简单说明代码文件
|代码文件                              |描述说明                                |
|:------------------------------------:|------------------------------------|
|app_at_com.c                              |任务主要实现，流程|
|at_uart.c                              |串口配置，数据收发|
|at_cmds.c                              |	AT协议解析处理|
|le_at_com.c                              |ble控制实现|
|spp_at_com.c                              |spp控制实现|

# APP- Bluetooth Dual-Mode Client
## 5.1 概述
Client 角色在 SDK 中是以主机 Master 的方式实现，主动发起搜索和连接其他 BLE 设备。连接
成功后遍历从机 GATT 的 Services 信息数据。最大支持 16 个 Sevices 遍历。

SDK 的例子是以杰理的数传 SDK 的 BLE 的设备中 Services 为搜索目标，用户根据需求也可自
行搜索过滤其他的 Services。

支持的板级： bd29、br25、br23、br30、bd19

支持的芯片： AC631N、AC636N、AC635N、AC637N、AC632N
## 5.2 工程配置
代码工程：apps\spp_and_le\board\bd29\AC631N_spp_and_le.cbp

* 先配置板级 board_config.h 和对应配置文件中蓝牙双模使能
```
23. /*
24. * 板级配置选择
25. */
26. #define CONFIG_BOARD_AC630X_DEMO
27. // #define CONFIG_BOARD_AC6311_DEMO
28. // #define CONFIG_BOARD_AC6313_DEMO
29. // #define CONFIG_BOARD_AC6318_DEMO
30. // #define CONFIG_BOARD_AC6319_DEMO
31.
32. #include "board_ac630x_demo_cfg.h"
33. #include "board_ac6311_demo_cfg.h"
34. #include "board_ac6313_demo_cfg.h"
35. #include "board_ac6318_demo_cfg.h"
36. #include "board_ac6319_demo_cfg.h"
```
* 配置只支持 ble 模块
```
7. #define TCFG_USER_BLE_ENABLE 1 //BLE 功能使能
8. #define TCFG_USER_EDR_ENABLE 1 //EDR 功能使能

```
* 配置 app 选择
```
14. //app case 选择,只能选 1 个,要配置对应的 board_config.h
15. #define CONFIG_APP_SPP_LE 1
16. #define CONFIG_APP_AT_COM 0
17. #define CONFIG_APP_DONGLE 0
```

```
1. //配置对应的 APP 的蓝牙功能
2. #if CONFIG_APP_SPP_LE
3. #define TRANS_DATA_EN 0 //蓝牙双模透传
4. #define TRANS_CLIENT_EN 1 //蓝牙(ble 主机)透传
```
## 5.3 主要代码说明
BLE 实现文件 le_client_demo.c，负责模块初始化、处理协议栈事件和命令数据控制发送等。

* 主机接口说明

接口|备注说明
---|:---|:--:
bt_ble_init |模块初始化
bt_ble_exit |模块初退出
ble_module_enable |模块开关使能
client_disconnect |断开连接
cbk_packet_handler |协议栈事件处理
cbk_sm_packet_handler |配对加密事件处理
scanning_setup_init |扫描参数配置
client_report_adv_data |扫描到的广播信息处理
bt_ble_scan_enable |扫描开关
client_create_connection |创建连接监听
client_create_connection_cannel |取消连接监听
client_search_profile_start |启动搜索从机的 GATT 服务
user_client_report_search_result |回调搜索 GATT 服务的结果
check_target_uuid_match |检查搜索到的 GATT 服务是否匹配目标
do_operate_search_handle |搜索完成后，处理搜索到的目标 handle。例如：对 handle 的读操作，对 handle 写使能通知功能等
user_client_report_data_callback |接收到从机的数据，例如 notify 和 indicate 数据
app_send_user_data |发送数据接口
app_send_user_data_check |检查是否可以往协议栈发送数据
l2cap_connection_update_request_just | 是否接受从机连接参数的调整求，接受后协议栈会自动更新参数
client_send_conn_param_update |主动更新连接参数调整
connection_update_complete_success |连接参数调整成功
get_buffer_vaild_len |获取发送 buf 可写如长度
can_send_now_wakeup | 协议栈发送完成回调，用来触发继续发数
client_write_send | 向从机写数据，需要等待从机回复应答
client_write_without_respond_send |向从机写数据，不需要从回复应答
client_read_value_send | 向从机发起读数据，|只能读到<=MTU 的长度数据
client_read_long_value_send |向从机发起读数据，一次读完所有数据

* 配置搜索的 GATT 服务以及记录搜索到的信息，支持 16bit 和 128 bit 的 UUID。

    * 配置搜索的 services
    ```
    1.typedef struct {  
    2.    uint16_t services_uuid16;  
    3.    uint16_t characteristic_uuid16;  
    4.    uint8_t services_uuid128[16];  
    5.    uint8_t characteristic_uuid128[16];  
    6.    uint16_t opt_type;  
    7.} target_uuid_t;  
    8.  
    9.#if TRANS_CLIENT_EN  
    10.//指定搜索uuid  
    11.static const target_uuid_t  search_uuid_table[] = {  
    13.  // for uuid16  
    14.    // PRIMARY_SERVICE, ae30  
    15.    // CHARACTERISTIC,  ae01, WRITE_WITHOUT_RESPONSE | DYNAMIC,  
    16.    // CHARACTERISTIC,  ae02, NOTIFY,    
    17.    {  
    18.        .services_uuid16 = 0xae30,  
    19.        .characteristic_uuid16 = 0xae01,  
    20.        .opt_type = ATT_PROPERTY_WRITE_WITHOUT_RESPONSE,  
    21.    },  
    22.  
    23.    {  
    24.        .services_uuid16 = 0xae30,  
    25.        .characteristic_uuid16 = 0xae02,  
    26.        .opt_type = ATT_PROPERTY_NOTIFY,  
    27.    },    

    ```
    * 记录搜索匹配的 handle 等信息
    ```
    1. #define SEARCH_UUID_MAX (sizeof(search_uuid_table)/sizeof(target_uuid_t))
    2.
    3. typedef struct {
    4.      target_uuid_t *search_uuid;
    5.      uint16_t value_handle;
    6.      /* uint8_t properties; */
    7. } opt_handle_t;
    8.
    9. //搜索操作记录的 handle
    10. #define OPT_HANDLE_MAX 16
    11. static opt_handle_t opt_handle_table[OPT_HANDLE_MAX];
    12. static u8 opt_handle_used_cnt;
    13.
    14. typedef struct {
    15.     uint16_t read_handle;
    16.     uint16_t read_long_handle;
    17.     uint16_t write_handle;
    18.     uint16_t write_no_respond;
    19.     uint16_t notify_handle;
    20.     uint16_t indicate_handle;
    21. } target_hdl_t;
    22.
    23. //记录 handle 使用
    24. static target_hdl_t target_handle;
    ```
    * 查找到匹配的 UUID 和 handle，会执行对 handle 的读写使能通知功能等操作，如下
    ```
    1. //操作 handle，完成 write ccc
    2. static void do_operate_search_handle(void)
    3. {
    4.      u16 tmp_16;
    5.      u16 i, cur_opt_type;
    6.      opt_handle_t *opt_hdl_pt;
    7.      
    8.      log_info("find target_handle:");
    9.      log_info_hexdump(&target_handle, sizeof(target_hdl_t));
    10.
    11.     if (0 == opt_handle_used_cnt) {
    12.         return;
    13. }

    ```

* 配值发起连接的条件

创建连接可根据名字、地址、厂家自定义标识等匹配发起连接对应的设备。用户可修改创建
连接条件，如下图是可选择的条件：
```
1. enum {
2.      CLI_CREAT_BY_ADDRESS = 0,//指定地址创建连接
3.      CLI_CREAT_BY_NAME,//指定设备名称创建连接
4.      CLI_CREAT_BY_TAG,//匹配厂家标识创建连接
5.      CLI_CREAT_BY_LAST_SCAN,
6. };
```
```
1. static const char user_tag_string[] = {0xd6, 0x05, 'j', 'i', 'e', 'l', 'i' };
2. static const u8 create_conn_mode = BIT(CLI_CREAT_BY_NAME);// BIT(CLI_CREAT_BY_ADDRESS) | BIT(CLI_CREAT_BY_NAME)
3. static const u8 create_conn_remoter[6] = {0x11, 0x22, 0x33, 0x88, 0x88, 0x88};

```
* 配置扫描和连接参数

扫描参数以 0.625ms 为单位，设置如下图：
```
1. #define SET_SCAN_TYPE SCAN_ACTIVE
2. #define SET_SCAN_INTERVAL 48
3. #define SET_SCAN_WINDOW 16

```
连接参数 interval 是以 1.25ms 为单位，timeout 是以 10ms 为单位，如下图：
```
1. #define SET_CONN_INTERVAL 0x30
2. #define SET_CONN_LATENCY 0
3. #define SET_CONN_TIMEOUT 0x180
```
以上两组参数请慎重修改，必须按照蓝牙的协议规范来定义修改。

# APP- Bluetooth BLE Dongle
## 6.1 概述
蓝牙 dongle 符合 USB 和 BLE 传输标准，具有即插即用，方便实用的特点。它可用于 BLE 设备
之间的数据传输，让电脑能够和周边的 BLE 设备进行无线连接和数据的通讯，自动发现和管理远程
BLE 设备、资源和服务，实现 BLE 设备之间的绑定和自动连接。

蓝牙 dongle 支持 BLE 和 2.4G 两种连接模式。蓝牙 dongle 支持连接指定蓝牙名或 mac 地址。

支持的板级：br25、br30、bd19

支持的芯片：AC636N、AC637N、AC632N
## 6.2 工程配置
代码工程：apps\spp_and_le\board\br25\AC636N_spp_and_le.cbp
* APP 选择，配置 app_config.h
```
18. //app case 选择,只能选 1 个,要配置对应的 board_config.h
19. #define CONFIG_APP_SPP_LE 0
20. #define CONFIG_APP_AT_COM 0
21. #define CONFIG_APP_DONGLE 1
```
* 板级选择, 配置 board_config.h。目前只有 AC6368B_DONGLE 板子支持蓝牙 dongle
```
22. //#define CONFIG_BOARD_AC636N_DEMO
23. #define CONFIG_BOARD_AC6368B_DONGLE //CONFIG_APP_DONGLE
24. // #define CONFIG_BOARD_AC6363F_DEMO
25. // #define CONFIG_BOARD_AC6366C_DEMO
26. // #define CONFIG_BOARD_AC6368A_DEMO
27. // #define CONFIG_BOARD_AC6369F_DEMO
28. // #define CONFIG_BOARD_AC6369C_DEMO
```
* 使能 USB 和 BLE ，需配置 board_ac6368b_dongle_cfg.h
```
29. #define TCFG_PC_ENABLE ENABLE_THIS_MOUDLE//PC 模块使能
30. #define TCFG_UDISK_ENABLE DISABLE_THIS_MOUDLE//U 盘模块使能
31. #define TCFG_OTG_USB_DEV_EN BIT(0)//USB0 = BIT(0) USB1 = BIT(1)
32. #define TCFG_USER_BLE_ENABLE 1 //BLE 或 2.4G 功能使能
33. #define TCFG_USER_EDR_ENABLE 0 //EDR 功能使能
```
* 模式选择，配置 BLE 或 2.4G 模式；若选择 2.4G 配对码必须跟对方的配对码一致
```
34. //2.4G 模式: 0---ble, 非 0---2.4G 配对码
35. #define CFG_RF_24G_CODE_ID (0) //<=24bits

```
* 如果选择 BLE 模式，则蓝牙 dongle 默认是按蓝牙名连接从机，需要配置连接的从机蓝牙名
```
1. static const u8 dongle_remoter_name1[] = "AC696X_1(BLE)";//
2. static const u8 dongle_remoter_name2[] = "AC630N_HID123(BLE)";// 自动连接同名的从机
```

* 默认上电 10 秒根据信号强度 rssi 配对近距离的设备，若配对失败，停止搜索。回连已有的配
对设备。
```
1. //dongle 上电开配对管理,若配对失败，没有配对设备，停止搜索
2. #define POWER_ON_PAIR_START (1)//
3. #define POWER_ON_PAIR_TIME (10000)//unit ms,持续配对搜索时间
4. #define DEVICE_RSSI_LEVEL (-50)
5. #define POWER_ON_KEEP_SCAN (0)//配对失败，保持一直搜索配对
```

* 主要代码说明( app_dongle.c )
    * HID 描述符, 描述为一个鼠标
    ```
    26. static const u8 sHIDReportDesc[] = {
    27. 0x05, 0x01, // Usage Page (Generic Desktop Ctrls)
    28. 0x09, 0x02, // Usage (Mouse)
    29. 0xA1, 0x01, // Collection (Application)
    30. 0x85, 0x01, // Report ID (1)
    31. 0x09, 0x01, // Usage (Pointer)
    32.
    ```
    * 使用指定的 uuid 与从机通信, 需要与从机配合, 省掉了搜索 uuid 的时间
    ```
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

    ```
    1. /*
    2. 确定留给从机发数据的 3 个 notify handle
    3. */
    4. static const u16 mouse_notify_handle[3] = {0x0027, 0x002b, 0x002f};
    ```

    * 用于监听从机 notify 数据, 并通过 USB 向 PC 转发蓝牙数据
    ```
    1. static void ble_report_data_deal(att_data_report_t *report_data, target_uuid_t *sear
    ch_uuid)
    2. {
    3.      log_info_hexdump(report_data->blob, report_data->blob_length);
    4.      
    5.      switch (report_data->packet_type) {
    6.      case GATT_EVENT_NOTIFICATION: { //notify
    7.      u8 packet[4];
    ...
    ...

    ```
    * 连接从机的方式配置，可以选择通过地址或设备名等方式连接
    ```
    1. static const client_match_cfg_t match_dev01 = {
    2.      .create_conn_mode = BIT(CLI_CREAT_BY_NAME), //连接从机设备的方式:有地址连接,设备名连接,厂家标识连接
    3.      .compare_data_len = sizeof(dongle_remoter_name1) - 1, //去结束符(连接内容长度)
    4.      .compare_data = dongle_remoter_name1, //根据连接方式,填内容
    5.      .bonding_flag = 0,//不绑定 //是否配对,如果配对的话, 下次就会直接连接
    6. };
    ```
    * 添加从机的链接方式, 将配置好的 client_match_cfg_t 结构体挂载到 client_conn_cfg_t 结构体上
    ```
    1. static const client_conn_cfg_t dongle_conn_config = {
    2.      .match_dev_cfg[0] = &match_dev01,
    3.      .match_dev_cfg[1] = &match_dev02,
    4.      .match_dev_cfg[2] = NULL, //需要链接的从机设备,
    5.      .report_data_callback = ble_report_data_deal,
    6.      .search_uuid_cnt = 0, //配置不搜索 profile，加快回连速度
    7.      /* .search_uuid_cnt = (sizeof(dongle_search_ble_uuid_table) / sizeof(target_uuid_t)), */
    8.      /* .search_uuid_table = dongle_search_ble_uuid_table, */
    9.      .security_en = 1,
    10.     .event_callback = dongle_event_callback,
    11. };

    ```
# Bluetooth Dual-Mode AT Moudle (char)
## 7.1 概述
主要功能是在普通数传 BLE 和 EDR 的基础上增加了由上位机或其他 MCU 可以通过 UART 对
接蓝牙芯片进行基本配置、状态获取、控制扫描、连接断开以及数据收发等操作。

AT 控制透传主从多机模式。

定义一套串口的控制协议，具体请查看协议文档《蓝牙 AT_CHAR 协议》。

支持的板级： bd29、br25、br23、br30、bd19

支持的芯片： AC631N、AC636N、AC635N、AC637N、AC632N

注意不同芯片可以使用 RAM 的空间有差异，有可能会影响性能。

## 7.2 工程配置
* 先配置板级 board_config.h 和对应配置文件中蓝牙双模使能

  ```
  37. /*
  38. * 板级配置选择
  39. */
  40. #define CONFIG_BOARD_AC630X_DEMO
  41. // #define CONFIG_BOARD_AC6311_DEMO
  42. // #define CONFIG_BOARD_AC6313_DEMO
  43. // #define CONFIG_BOARD_AC6318_DEMO
  44. // #define CONFIG_BOARD_AC6319_DEMO
  ```

* 配置对应的 board_acxxx_demo_cfg.h 文件使能 BLE 以 board_ac630x_demo_cfg.h 为例

  ```
  21. #define TCFG_USER_BLE_ENABLE 1 //BLE 功能使能
  22. #define TCFG_USER_EDR_ENABLE 0 //EDR 功能使能
  ```

* 配置 app_config.h, 选择 CONFIG_APP_AT_CHAR_COM

  ```
  36. #define CONFIG_APP_SPP_LE 0 //SPP + LE or LE's client
  37. #define CONFIG_APP_AT_COM 0 //ATcom HEX 格式命令
  38. #define CONFIG_APP_AT_CHAR_COM 1 //ATcom 字符串格式命令
  39. #define CONFIG_APP_DONGLE 0 //board_dongle ,TCFG_PC_ENABLE
  40. #define CONFIG_APP_MULTI 0 //蓝牙 LE 多连
  ```

## 7.3 主要代码说明 <at_char_cmds.c>

* 命令包头

  ```
  41. static const char at_head_at_cmd[] = "AT+";
  42. static const char at_head_at_chl[] = "AT>";
  43.
  44. static const str_info_t at_head_str_table[] = {
  45.       INPUT_STR_INFO(STR_ID_HEAD_AT_CMD, at_head_at_cmd),
  46.       INPUT_STR_INFO(STR_ID_HEAD_AT_CHL, at_head_at_chl),
  47. };
  ```

* 命令类型

  ```
  49. static const char at_str_gver[] = "GVER";
  50. static const char at_str_gcfgver[] = "GCFGVER";
  51. static const char at_str_name[] = "NAME";
  52. static const char at_str_lbdaddr[] = "LBDADDR";
  53. static const char at_str_baud[] = "BAUD";
  54.
  55. static const char at_str_adv[] = "ADV";
  56. static const char at_str_advparam[] = "ADVPARAM";
  57. static const char at_str_advdata[] = "ADVDATA";
  58. static const char at_str_srdata[] = "SRDATA";
  59. static const char at_str_connparam[] = "CONNPARAM";
  60.
  61. static const char at_str_scan[] = "SCAN";
  62. static const char at_str_targetuuid[] = "TARGETUUID";
  63. static const char at_str_conn[] = "CONN";
  64. static const char at_str_disc[] = "DISC";
  65. static const char at_str_ota[] = "OTA";
  66.
  67. static const str_info_t at_cmd_str_table[] = {
  68. INPUT_STR_INFO(STR_ID_GVER, at_str_gver),
  69. INPUT_STR_INFO(STR_ID_GCFGVER, at_str_gcfgver),
  70. INPUT_STR_INFO(STR_ID_NAME, at_str_name),
  71. INPUT_STR_INFO(STR_ID_LBDADDR, at_str_lbdaddr),
  72. INPUT_STR_INFO(STR_ID_BAUD, at_str_baud),
  73.
  74. INPUT_STR_INFO(STR_ID_ADV, at_str_adv),
  75. INPUT_STR_INFO(STR_ID_ADVPARAM, at_str_advparam),
  76. INPUT_STR_INFO(STR_ID_ADVDATA, at_str_advdata),
  77. INPUT_STR_INFO(STR_ID_SRDATA, at_str_srdata),
  78. INPUT_STR_INFO(STR_ID_CONNPARAM, at_str_connparam),
  79.
  80. INPUT_STR_INFO(STR_ID_SCAN, at_str_scan),
  81. INPUT_STR_INFO(STR_ID_TARGETUUID, at_str_targetuuid),
  82. INPUT_STR_INFO(STR_ID_CONN, at_str_conn),
  83. INPUT_STR_INFO(STR_ID_DISC, at_str_disc),
  84. INPUT_STR_INFO(STR_ID_OTA, at_str_ota),
  85.
  86. // INPUT_STR_INFO(, ),
  87. // INPUT_STR_INFO(, ),
  88. };
  ```

* 串口收数中断回调

  进入收数中断之前会进行初步校验, 数据结尾不是’\r’, 则无法唤醒中断

```
89. static void at_packet_handler(u8 *packet, int size)
```

* AT 命令解析

```
68.        /*比较数据包头*/
69.    str_p=at_check_match_string(parse_pt, parse_size,at_head_str_table,sizeof(at_head_str_table));
70.    if (!str_p)
71.    {
72.        log_info("###1unknow at_head:%s", packet);
73.        AT_STRING_SEND(at_str_err);
74.        return;
75.    }
76.    parse_pt   += str_p->str_len;
77.    parse_size -= str_p->str_len;
78.
79.    /*普通命令*/
80.    if(str_p->str_id == STR_ID_HEAD_AT_CMD)
81.    {
82.        /*比较命令*/
83.        str_p=at_check_match_string(parse_pt, parse_size,at_cmd_str_table,sizeof(at_cmd_str_table));
84.        if (!str_p)
85.        {
86.            log_info("###2unknow at_cmd:%s", packet);
87.            AT_STRING_SEND(at_str_err);
88.            return;
89.        }
90.
91.        parse_pt += str_p->str_len;
92.        parse_size -= str_p->str_len;
93.        /*判断当前是命令类型,查询或设置命令*/
94.        if(parse_pt[0] == '=')
95.        {
96.            operator_type = AT_CMD_OPT_SET;
97.        }
98.        else if(parse_pt[0] == '?')
99.        {
100.            operator_type = AT_CMD_OPT_GET;
101.        }
102.        parse_pt++;
103.    }
104.
105.    /*通道切换命令*/
106.    else if(str_p->str_id == STR_ID_HEAD_AT_CHL)
107.    {
108.        operator_type = AT_CMD_OPT_SET;
109.    }
110.
111.//    if(operator_type == AT_CMD_OPT_NULL)
112.//    {
113.//        AT_STRING_SEND(at_str_err);
114.//        log_info("###3unknow operator_type:%s", packet);
115.//        return;
116.//    }
117.
118.
119.    log_info("str_id:%d", str_p->str_id);
120.  
121.    /*解析并返回命令参数par*/
122.    par = parse_param_split(parse_pt,',','\r');
123.
124.    log_info("\n par->data: %s",par->data);
125.
126.    /*命令处理与响应*/
127.    switch (str_p->str_id)
128.    {
129.        case STR_ID_HEAD_AT_CHL:
130.            log_info("STR_ID_HEAD_AT_CHL\n");
131.        break;
132.......................................
```

* 命令的参数获取与遍历

```
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

* 当有多个参数是, 需要遍历获取, 以连接参数为例

```
201. while (par) { //遍历所有参数
202.    log_info("len=%d,par:%s", par->len, par->data);
203.    conn_param[i] = func_char_to_dec(par->data, '\0'); //遍历获取连接参数
204.    if (par->next_offset) {
205.    par =AT_PARAM_NEXT_P(par);
206.    } else {
207.        break;
208.    }
209.    i++;
210. }
```

* 默认信息

```
211. #define G_VERSION "BR30_2021_01_31"
212. #define CONFIG_VERSION "2021_02_04" /*版本信息*/
213. u32 uart_baud = 115200; /*默认波特率*/
214. char dev_name_default[] = "jl_test"; /*默认 dev name*/
```

* 在代码中添加新的命令(以查询、设置波特率为例)

  * 添加波特率枚举成员

    ```
    215. enum {
    216. STR_ID_NULL = 0,
    217. STR_ID_HEAD_AT_CMD,
    218. STR_ID_HEAD_AT_CHL,
    219.
    220. STR_ID_OK = 0x10,
    221. STR_ID_ERROR,
    222.
    223. STR_ID_GVER = 0x20,
    224. STR_ID_GCFGVER,
    225. STR_ID_NAME,
    226. STR_ID_LBDADDR,
    227. STR_ID_BAUD, /*波特率成员*/
    228.
    229. STR_ID_ADV,
    230. STR_ID_ADVPARAM,
    231. STR_ID_ADVDATA,
    232. STR_ID_SRDATA,
    233. STR_ID_CONNPARAM,
    234.
    235. STR_ID_SCAN,
    236. STR_ID_TARGETUUID,
    237. STR_ID_CONN,
    238. STR_ID_DISC,
    239. STR_ID_OTA,
    240. // STR_ID_,
    241. // STR_ID_,
    242. };
    ```

  * 添加波特率命令类型

    ```
    243. static const char at_str_gver[] = "GVER";
    244. static const char at_str_gcfgver[] = "GCFGVER";
    245. static const char at_str_name[] = "NAME";
    246. static const char at_str_lbdaddr[] = "LBDADDR";
    247. static const char at_str_baud[] = "BAUD"; /*波特率命令类型*/
    248.
    249. static const char at_str_adv[] = "ADV";
    250. static const char at_str_advparam[] = "ADVPARAM";
    251. static const char at_str_advdata[] = "ADVDATA";
    252. static const char at_str_srdata[] = "SRDATA";
    253. static const char at_str_connparam[] = "CONNPARAM";
    254.
    255. static const char at_str_scan[] = "SCAN";
    256. static const char at_str_targetuuid[] = "TARGETUUID";
    257. static const char at_str_conn[] = "CONN";
    258. static const char at_str_disc[] = "DISC";
    259. static const char at_str_ota[] = "OTA";
    ```

  * 添加命令到命令列表

    ```
    260. static const str_info_t at_cmd_str_table[] = {
    261.    INPUT_STR_INFO(STR_ID_GVER, at_str_gver),
    262.    INPUT_STR_INFO(STR_ID_GCFGVER, at_str_gcfgver),
    263.    INPUT_STR_INFO(STR_ID_NAME, at_str_name),
    264.    INPUT_STR_INFO(STR_ID_LBDADDR, at_str_lbdaddr),
    265.    INPUT_STR_INFO(STR_ID_BAUD, at_str_baud), /*波特率命令*/
    267.    INPUT_STR_INFO(STR_ID_ADV, at_str_adv),
    268.    INPUT_STR_INFO(STR_ID_ADVPARAM, at_str_advparam),
    269.    INPUT_STR_INFO(STR_ID_ADVDATA, at_str_advdata),
    270.    INPUT_STR_INFO(STR_ID_SRDATA, at_str_srdata),
    271.    INPUT_STR_INFO(STR_ID_CONNPARAM, at_str_connparam),
    272.    
    273.    INPUT_STR_INFO(STR_ID_SCAN, at_str_scan),
    274.    INPUT_STR_INFO(STR_ID_TARGETUUID, at_str_targetuuid),
    275.    INPUT_STR_INFO(STR_ID_CONN, at_str_conn),
    276.    INPUT_STR_INFO(STR_ID_DISC, at_str_disc),
    277.    INPUT_STR_INFO(STR_ID_OTA, at_str_ota),
    278.    
    279.    // INPUT_STR_INFO(, ),
    280.    // INPUT_STR_INFO(, ),
    281. };
    ```
  * 在at_packet_handler函数中添加命令的处理与响应
    ```
    260.        case STR_ID_BAUD:
    261.            log_info("STR_ID_BAUD\n");
    262.            {
    263.                if(operator_type == AT_CMD_OPT_SET) //设置波特率
    264.                {
    265.                    uart_baud = func_char_to_dec(par->data, '\0');
    266.                    if(uart_baud==9600||uart_baud==19200||uart_baud==38400||uart_baud==115200||
    267.                       uart_baud==230400||uart_baud==460800||uart_baud==921600)
    268.                    {
    269.                        AT_STRING_SEND("OK"); /*返回响应*/
    270.                        ct_uart_init(uart_baud);
    271.                    }
    272.                    else{   //TODO返回错误码
    273.
    274.
    275.                    }
    276.                }
    277.                else{                               //读取波特率
    278.
    279.                    sprintf( buf, "+BAUD:%d", uart_baud);
    280.                    at_cmd_send(buf, strlen(buf)); /*返回波特率数据*/
    281.                    AT_STRING_SEND("OK"); /*返回响应*/
    282.                }
    283.            }
    284.        break;
    ```
  * 串口发数api, 用于发送响应信息
    ```
    285./*
    286.parameter
    287.packet: 数据包
    288.size: 数据长度
    289.*/
    290.at_uart_send_packet(const u8 *packet, int size);
    ```
  * 用于回复带”\r\n”的响应
    ```
    291.void at_cmd_send(const u8 *packet, int size)
    292.{
    293.    at_uart_send_packet(at_str_enter, 2);
    294.    at_uart_send_packet(packet, size);
    295.    at_uart_send_packet(at_str_enter, 2);
    296.}
    ```