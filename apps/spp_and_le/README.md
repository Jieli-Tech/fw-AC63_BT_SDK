[icon_build]:./../../doc/stuff/build_passing.svg

# APP - Bluetooth：SPP 透传 ![Build result][icon_build]
---------------

代码工程 <br>
AC631 ：[apps\spp_and_le\board\bd29\AC631X_spp_and_le.cbp](../../apps/spp_and_le/board/bd29) <br>
AC636 ：[apps\spp_and_le\board\br25\AC636X_spp_and_le.cbp](../../apps/spp_and_le/board/br25) 

 
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

APP - Bluetooth：LE 透传 ![Build result][icon_build]
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

APP - Bluetooth：AT 命令![Build result][icon_build]
--------------- 

## 4. 模块使能

    #define TRANS_AT_COM                               1 //串口控制对接蓝牙双模透传 
    #define TCFG_USER_BLE_ENABLE                      1   //BLE功能使能  
    #define TCFG_USER_EDR_ENABLE                      1   //EDR功能使能


### 4.1 概述
主要功能是在数传SPP+BLE的基础上，增加了由上位机或其他MCU可以通过UART对接蓝牙芯片进行基本的配置、状态获取、控制连接断开以及数据收发等操作。
定义一套串口的控制协议，具体请查看协议文档[《蓝牙AT协议》](../../doc/蓝牙AT协议)。

简单说明代码文件
|代码文件                              |描述说明                                |
|:------------------------------------:|------------------------------------|
|app_at_com.c                              |任务主要实现，流程|
|at_uart.c                              |串口配置，数据收发|
|at_cmds.c                              |	AT协议解析处理|
|le_at_com.c                              |ble控制实现|
|spp_at_com.c                              |spp控制实现|
