[icon_build]:./../../doc/stuff/build_passing.svg
[pic_client_flowsheet]:./../../doc/stuff/client_flowsheet.jpg
[pic_server_flowsheet]:./../../doc/stuff/server_flowsheet.jpg
[gif_OnOff_client]:./../../doc/Generic_On_Off_Client.gif
[gif_OnOff_server]:./../../doc/Generic_On_Off_Server.gif
[gif_AliGenie_connect]:./../../doc/AliGenie_connect.gif
[icon_download]:https://api.bintray.com/packages/nordic/android/no.nordicsemi.android%3Amesh/images/download.svg
[download_address]:https://github.com/NordicSemiconductor/Android-nRF-Mesh-Library/releases/download/v2.4.1/nRF.Mesh.-.V.-.2.4.1.apk

[1]:https://www.bluetooth.com/specifications/mesh-specifications/
[2]:https://www.bluetooth.com/specifications/bluetooth-core-specification/

[3]:https://www.aligenie.com/doc/357554/
[4]:https://www.aligenie.com/doc/357554/gtgprq
[5]:https://www.aligenie.com/doc/357554/yh3tf3
[6]:https://iot.aligenie.com/account/login?spm=a2140w.13129968.0.0.375d643bZ07hUV&redirectURL=%2Fr%3Furl%3Dhttps%3A%2F%2Fiot.aligenie.com%2Fhome
[7]:https://www.aligenie.com/doc/357554/zq0un4
[8]:https://www.aligenie.com/doc/357554/tgllbp
# APP - Bluetooth: Mesh ![Build result][icon_build]
---------------

## 概述
:book: 标准 [蓝牙Mesh][1] 协议，基于 [蓝牙5 ble][2] 实现网内节点间通讯，具体功能如下：

- **Node features support**

`Relay` | `Proxy` | `Friend` | `Low Power`
:-:|:-:|:-:|:-:
:heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark: | :heavy_check_mark:

- **Provisioning bearer support**

`PB-GATT` | `PB-ADV`
:-:|:-:
:heavy_check_mark: | :heavy_check_mark:

- **Provisioning role support**

`Unprovisioned device` | `Provisioner`
:-:|:-:
:heavy_check_mark: | :x:

- **Model support**

`SIG Model` | `Vendor Model`
:-:|:-:
:heavy_check_mark: | :heavy_check_mark:

- **其它功能**

- [X] 设备上电自配网(不用网关就可实现设备在同一网内，支持`security key`自定义)
- [X] 发布(`Publish`)和订阅(`Subscribe`)地址可修改
- [X] 网络/节点信息断电保护
- [X] 已入网节点可复位为未配网设备

## 目录导航
-  ### [Start](./README.md#Start-1)
    - #### [mesh系列教程](./README.md#mesh系列教程-1)
    - #### [应用实例选择](./README.md#应用实例选择-1)
    - #### [Mesh 配置](./README.md#Mesh-配置-1)
    - #### [Board 配置](./README.md#Board-配置-1)
-  ### [应用实例](./README.md#应用实例-1)
    - #### [SIG Generic OnOff Client](./README.md#SIG-Generic-OnOff-Client-1)
    - #### [SIG Generic OnOff Server](./README.md#SIG-Generic-OnOff-Server-1)
    - #### [SIG AliGenie Socket](./README.md#SIG-AliGenie-Socket-1)
    - #### [SIG AliGenie Light](./README.md#SIG-AliGenie-Light-1)
    - #### [SIG AliGenie Fan](./README.md#SIG-AliGenie-Fan-1)

## Start
> :book: 目录文件结构 :book:
```java
▾ mesh/
    ▾ api/
        feature_correct.h
        mesh_config_common.c
        model_api.c
        model_api.h
        unix_timestamp.c
        unix_timestamp.h
    ▾ board/
      ▾ bd29/
          board_ac630x_demo.c
          board_ac630x_demo_cfg.h
          board_ac6311_demo.c
          board_ac6311_demo_cfg.h
          board_ac6313_demo.c
          board_ac6313_demo_cfg.h
          board_ac6318_demo.c
          board_ac6318_demo_cfg.h
          board_ac6319_demo.c
          board_ac6319_demo_cfg.h
          board_config.h
    ▾ examples/
        generic_onoff_client.c
        generic_onoff_server.c
        vendor_client.c
        vendor_server.c
        AliGenie_socket.c
        AliGenie_Light.c
        AliGenie_Fan.c
```
  >### mesh系列教程
  ---
  - [mesh入门教程](https://v.qq.com/x/page/w3247hvxcga.html?ptag=qqbrowser)

  >### 应用实例选择
  ---
  - 在 [**api/model_api.h**](./api/model_api.h) 下，通过配置`CONFIG_MESH_MODEL`选择相应例子，SDK提供了5个应用实例。默认选择`SIG_MESH_GENERIC_ONOFF_CLIENT`，即位于 [**examples/generic_onoff_client.c**](./examples/generic_onoff_client.c) 下的例子。
```C
//< Detail in "MshMDLv1.0.1"
#define SIG_MESH_GENERIC_ONOFF_CLIENT       0 // examples/generic_onoff_client.c
#define SIG_MESH_GENERIC_ONOFF_SERVER       1 // examples/generic_onoff_server.c
#define SIG_MESH_VENDOR_CLIENT              2 // examples/vendor_client.c
#define SIG_MESH_VENDOR_SERVER              3 // examples/vendor_server.c
#define SIG_MESH_ALIGENIE_SOCKET            4 // examples/AliGenie_socket.c
// more...

//< Config which example will use in <examples>
#define CONFIG_MESH_MODEL                   SIG_MESH_GENERIC_ONOFF_CLIENT
```
  >### Mesh 配置
  ---
在 [**api/mesh_config_common.c**](./api/mesh_config_common.c) 下，可以自由配置网络和节点特性，例如 **LPN/Friend** 节点特性、**Proxy** 下配网前后广播 **interval**、节点信息传递时广播  **interval** 和 **duration** 等。
如下举例了节点信息传递时广播 **interval** 和 **duration** 的配置，和 **PB-GATT** 下配网前后广播 **interval** 的配置。
```C
/**
* @brief Config adv bearer hardware param when node send messages
*/
/*-----------------------------------------------------------*/
const u16 config_bt_mesh_node_msg_adv_interval = ADV_SCAN_UNIT(10); // unit: ms
const u16 config_bt_mesh_node_msg_adv_duration = 100; // unit: ms

/**
* @brief Config proxy connectable adv hardware param
*/
/*-----------------------------------------------------------*/
const u16 config_bt_mesh_proxy_unprovision_adv_interval = ADV_SCAN_UNIT(30); // unit: ms
const u16 config_bt_mesh_proxy_pre_node_adv_interval = ADV_SCAN_UNIT(10); // unit: ms
_WEAK_
const u16 config_bt_mesh_proxy_node_adv_interval = ADV_SCAN_UNIT(300); // unit: ms
```

```C
注意：在常量前加上_WEAK_的声明，代表这个常量可以在其它文件重定义。如果想某一配置私有化到某一实例，应该在该文件下在该配置前加上_WEAK_声明，并在所在实例文件里重定义该常量配置。
```
  >### Board 配置
  ---
  在 [**board/xxxx/board_config.h**](./board/bd29/board_config.h) 下，可以根据不同封装选择不同 **board**，以 **AC63X** 为例，默认选择`CONFIG_BOARD_AC630X_DEMO`作为目标板。
```C
/*
*  板级配置选择
*/
#define CONFIG_BOARD_AC630X_DEMO
// #define CONFIG_BOARD_AC6311_DEMO
// #define CONFIG_BOARD_AC6313_DEMO
// #define CONFIG_BOARD_AC6318_DEMO
// #define CONFIG_BOARD_AC6319_DEMO

#include "board_ac630x_demo_cfg.h"
#include "board_ac6311_demo_cfg.h"
#include "board_ac6313_demo_cfg.h"
#include "board_ac6318_demo_cfg.h"
#include "board_ac6319_demo_cfg.h"
```

## 应用实例
  >### SIG Generic OnOff Client
  ---
  - #### 简介
    该实例通过手机`nRF Mesh`进行配网
    ```C
    -> 设备名称：OnOff_cli
    -> Node Features：Proxy + Relay
    -> Authentication方式：NO OOB
    -> Elements个数：1
    -> Model：Configuration Server + Generic On Off Client
    ```
  - #### 实际操作
    - 基本配置

      使用 **USB DP** 作为串口 **debug** 脚，波特率为 **1000000**

      使用 **PA3** 作为 **AD** 按键

      设备名称为`OnOff_cli`，**MAC** 地址为`11:22:33:44:55:66`

      设置 [**api/model_api.h**](./api/model_api.h)
      ```C
      #define CONFIG_MESH_MODEL                   SIG_MESH_GENERIC_ONOFF_CLIENT
      ```
      设置 [**board/xxxx/board_xxxx_demo_cfg.h**](./board/bd29/board_ac630x_demo_cfg.h)
      ```C
      #define TCFG_UART0_TX_PORT                  IO_PORT_DP
      #define TCFG_UART0_BAUDRATE                 1000000
      #define TCFG_ADKEY_ENABLE                   ENABLE_THIS_MOUDLE //是否使能AD按键
      #define TCFG_ADKEY_PORT                     IO_PORTA_03 //注意选择的IO口是否支持AD功能
      #define TCFG_ADKEY_AD_CHANNEL               AD_CH_PA3
      ```
      设置 [**examples/generic_onoff_client.c**](./examples/generic_onoff_client.c)
      ```C
      #define BLE_DEV_NAME                        'O', 'n', 'O', 'f', 'f', '_', 'c', 'l', 'i'
      #define CUR_DEVICE_MAC_ADDR                 0x112233445566
        ```
      对于 **MAC** 地址，如果想不同设备在第一次上电时使用随机值，可以按照以下操作，将`NULL`传入`bt_mac_addr_set`函数
      如果想用配置工具配置 **MAC** 地址，应不调用`bt_mac_addr_set`函数

      设置 [**examples/generic_onoff_client.c**](examples/generic_onoff_client.c)
      ```C
      void bt_ble_init(void)
      {
          u8 bt_addr[6] = {MAC_TO_LITTLE_ENDIAN(CUR_DEVICE_MAC_ADDR)};

          bt_mac_addr_set(NULL);

          mesh_setup(mesh_init);
      }
      ```

    - 编译工程并下载到目标板，接好串口，接好 **AD** 按键，上电或者复位设备
    - 下载手机`nRF Mesh`

      `Android` | `IOS`
      :-:|:-:
       [ ![Download][icon_download] ][download_address]| `App Store`

    - `nRF Mesh`配网操作演示

      ![OnOff_client][gif_OnOff_client]

      配网完成后节点结构如下：
      ```C
        ▾ Elements
          ▾ Element
              Configuration Server   #SIG Model ID: 0x0000
              Generic On Off Client  #SIG Model ID: 0x1001
      ```
    - 此时按下按键，就能将开关信息 **Publish** 到 **Group** 地址 **0xC000** 了，如果结合下一小节 [SIG Generic OnOff Server](./README.md#SIG-Generic-OnOff-Server-1)，就能控制这个 **server** 设备 **LED** 灯的亮和灭了

  - #### 代码解读
      ![client流程图][pic_client_flowsheet]

  >### SIG Generic OnOff Server
  ---
  - #### 简介
    该实例通过手机`nRF Mesh`进行配网
    ```C
    -> 设备名称：OnOff_srv
    -> Node Features：Proxy + Relay
    -> Authentication方式：NO OOB
    -> Elements个数：1
    -> Model：Configuration Server + Generic On Off Server
    ```
  - #### 实际操作
    - 基本配置

      使用 **USB DP** 作为串口 **debug** 脚，波特率为 **1000000**

      使用 **PA1** 控制 **LED** 灯

      设备名称为`OnOff_srv`，**MAC** 地址为`22:22:33:44:55:66`

      设置 [**api/model_api.h**](./api/model_api.h)
      ```C
      #define CONFIG_MESH_MODEL                   SIG_MESH_GENERIC_ONOFF_SERVER
      ```
      设置 [**board/xxxx/board_xxxx_demo_cfg.h**](./board/bd29/board_ac630x_demo_cfg.h)
      ```C
      #define TCFG_UART0_TX_PORT                  IO_PORT_DP
      #define TCFG_UART0_BAUDRATE                 1000000
      ```
      设置 [**examples/generic_onoff_server.c**](./examples/generic_onoff_server.c)
      ```C
      #define BLE_DEV_NAME                        'O', 'n', 'O', 'f', 'f', '_', 's', 'r', 'v'
      #define CUR_DEVICE_MAC_ADDR                 0x222233445566

      const u8 led_use_port[] = {

        IO_PORTA_01,

      };
      ```
      对于 **MAC** 地址，如果想不同设备在第一次上电时使用随机值，可以按照以下操作，将`NULL`传入`bt_mac_addr_set`函数
      如果想用配置工具配置 **MAC** 地址，应不调用`bt_mac_addr_set`函数

      设置 [**examples/generic_onoff_server.c**](./examples/generic_onoff_server.c)
      ```C
      void bt_ble_init(void)
      {
          u8 bt_addr[6] = {MAC_TO_LITTLE_ENDIAN(CUR_DEVICE_MAC_ADDR)};

          bt_mac_addr_set(NULL);

          mesh_setup(mesh_init);
      }
      ```

    - 编译工程并下载到目标板，接好串口，接好演示用 **LED** 灯，上电或者复位设备
    - 下载手机`nRF Mesh`

      `Android` | `IOS`
      :-:|:-:
       [ ![Download][icon_download] ][download_address]| `App Store`

    - `nRF Mesh`配网操作演示

      ![OnOff_server][gif_OnOff_server]

      配网完成后节点结构如下：
      ```C
        ▾ Elements
          ▾ Element
              Configuration Server   #SIG Model ID: 0x0000
              Generic On Off Server  #SIG Model ID: 0x1000
      ```
    - 结合上一小节 [SIG Generic OnOff Client](./README.md#SIG-Generic-OnOff-Client-1)，此时如果 **Client** 设备按下按键，那么本机的 **LED** 灯就会亮或者灭了

  - #### 代码解读
      ![server流程图][pic_server_flowsheet]

  >### SIG AliGenie Socket
  ---
  - #### 简介
    该实例按照阿里巴巴 [IoT开放平台][3] 关于 [天猫精灵蓝牙mesh软件基础规范][4]，根据 `硬件品类规范` 描述自己为一个 [插座][5]，通过 `天猫精灵` 语音输入进行发现连接(配网)和控制设备

  - #### 实际操作
    - 基本配置

      使用 **USB DP** 作为串口 **debug** 脚，波特率为 **1000000**

      使用 **PA1** 控制 **LED** 灯(模拟插座的开和关的操作)

      设备名称为`AG-Socket`

      **三元组**（`MAC 地址`、`ProductID`、`Secret`）在天猫精灵开发者网站申请

      设置 [**api/model_api.h**](./api/model_api.h)
      ```C
      #define CONFIG_MESH_MODEL                   SIG_MESH_ALIGENIE_SOCKET
      ```
      设置 [**board/xxxx/board_xxxx_demo_cfg.h**](./board/bd29/board_ac630x_demo_cfg.h)
      ```C
      #define TCFG_UART0_TX_PORT                  IO_PORT_DP
      #define TCFG_UART0_BAUDRATE                 1000000
      ```
      设置 [**examples/AliGenie_socket.c**](./examples/AliGenie_socket.c)
      ```C
      #define BLE_DEV_NAME                        'A', 'G', '-', 'S', 'o', 'c', 'k', 'e', 't'
      //< 三元组(本例以个人名义申请的插座类三元组)
      #define CUR_DEVICE_MAC_ADDR                 0x28fa7a42bf0d
      #define PRODUCT_ID                          12623
      #define DEVICE_SECRET                       "753053e923f30c9f0bc4405cf13ebda6"
      const u8 led_use_port[] = {

          IO_PORTA_01,

      };
      ```
      对于 **MAC** 地址，本例中一定要按照**三元组**里面的 **MAC** 地址传入到 `bt_mac_addr_set` 函数里

      设置 [**examples/AliGenie_socket.c**](./examples/AliGenie_socket.c)
      ```C
      void bt_ble_init(void)
      {
            u8 bt_addr[6] = {MAC_TO_LITTLE_ENDIAN(CUR_DEVICE_MAC_ADDR)};

            bt_mac_addr_set(bt_addr);

            mesh_setup(mesh_init);
      }
      ```

    - 编译工程并下载到目标板，接好串口，接好演示用 **LED** 灯，上电或者复位设备
    - `天猫精灵`连接到互联网上
      - 上电`天猫精灵`，**长按设备**上的**语音按键**，让设备进入待连接状态
      - 手机应用商店下载`天猫精灵 APP`，**APP** 上登陆个人中心
      - 打开手机 `WLAN`，将`天猫精灵`通过手机热点连接到互联网上

      ![AliGenie_connect][gif_AliGenie_connect]

    - 通过`天猫精灵`进行配网和控制
      - 配网对话

      >用户：“天猫精灵，搜索设备”

      >天猫精灵：“发现一个智能插座，是否连接”

      >用户：“连接”

      >天猫精灵：“连接成功。。。 。。。”

      - 语音控制 **插座命令** (可通过 [IoT开放平台][6] 添加自定义语音命令)

        **命令** | **效果**
        :-:|:-:
        `天猫精灵，打开插座` | 开发板上 **LED** 灯打开
        `天猫精灵，关闭插座` | 开发板上 **LED** 灯关闭

  - #### 代码解读
    - 配网

      关键在于如何设置在`天猫精灵`开发者网站申请下来的**三元组**
      - `天猫精灵`开发者网站申请三元组，并填到下面文件相应宏定义处

        例如申请到的三元组如下：

        **Product ID(十进制)** | **Device Secret** | **Mac地址**
        :-:|:-:|:-:
        12623 | 753053e923f30c9f0bc4405cf13ebda6 | 28fa7a42bf0d

        则按下面规则填写，**MAC** 前要加上 **0x**，**Secret** 要用 **双引号** 包住

        设置 [**examples/AliGenie_socket.c**](./examples/AliGenie_socket.c)
        ```C
        //< 三元组(本例以个人名义申请的插座类三元组)
        #define CUR_DEVICE_MAC_ADDR             0x28fa7a42bf0d
        #define PRODUCT_ID                      12623
        #define DEVICE_SECRET                   "753053e923f30c9f0bc4405cf13ebda6"
        ```

      - 建立 **Element** 和 **Model**

        按照 [插座软件规范][5]，要建立一个 **element**，两个**model**
        **Element** | **Model** | **属性名称**
        :-:|:-:|:-:
        Primary | Generic OnOff Server(0x1000) | 开关
        Primary | Vendor Model(0x01A80000) | 故障上报/定时控制开关

        相应代码操作如下：

        结构体`elements`注册了一个**primary element = SIG root_models + Vendor_server_models**

        结构体`root_models` **= Cfg_Server + Generic_OnOff_Server**

        结构体`vendor_server_models` **= Vendor_Client_Model + Vendor_Server_Model**

        ```C
        //< Basic_Cfg_Server + Generic_OnOff_Server
        static struct bt_mesh_model root_models[] = {
            BT_MESH_MODEL_CFG_SRV(&cfg_srv), 
            BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, gen_onoff_srv_op, &gen_onoff_pub_srv, &onoff_state[0]),
            // BT_MESH_MODEL(...) // add new sig model
        };

        //< Vendor_Client + Vendor_Server
        static struct bt_mesh_model vendor_server_models[] = {
            BT_MESH_MODEL_VND(BT_COMP_ID_LF, BT_MESH_VENDOR_MODEL_ID_CLI, NULL, NULL, NULL),
            BT_MESH_MODEL_VND(BT_COMP_ID_LF, BT_MESH_VENDOR_MODEL_ID_SRV, vendor_srv_op, NULL, &onoff_state[0]),
            // BT_MESH_MODEL_VND(...) // add new vendor model
        };

        //< Only primary element
        static struct bt_mesh_elem elements[] = {
            BT_MESH_ELEM(0, root_models, vendor_server_models),  // primary element
            // BT_MESH_ELEM(...) // add second element
        };
        ```

    - 用户数据处理
      - **SIG Generic OnOff Server**回调

        结构体 `root_models` 里的 **Generic_OnOff_Server** 注册了回调 `gen_onoff_srv_op` 来对用户数据进行处理

        当收到 `BT_MESH_MODEL_OP_GEN_ONOFF_GET` 等注册消息时，就会调用 `gen_onoff_get` 等对应的回调函数进行用户数据处理
        ```C
        static const struct bt_mesh_model_op gen_onoff_srv_op[] = {
            { BT_MESH_MODEL_OP_GEN_ONOFF_GET, 0, gen_onoff_get },
            { BT_MESH_MODEL_OP_GEN_ONOFF_SET, 2, gen_onoff_set },
            { BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK, 2, gen_onoff_set_unack },
            BT_MESH_MODEL_OP_END,
        };
        ```

      - **Vendor Model**回调

        结构体 `vendor_srv_op` 里的 **Vendor_Server_Model** 注册了回调 `vendor_srv_op` 来对用户数据进行处理

        当收到 `VENDOR_MSG_ATTR_GET` 等注册消息时，就会调用 `vendor_attr_get` 等对应的回调函数进行用户数据处理
        ```C
        static const struct bt_mesh_model_op vendor_srv_op[] = {
            { VENDOR_MSG_ATTR_GET, ACCESS_OP_SIZE, vendor_attr_get },
            { VENDOR_MSG_ATTR_SET, ACCESS_OP_SIZE, vendor_attr_set },
            BT_MESH_MODEL_OP_END,
        };
        ```

  >### SIG AliGenie Fan
  ---
  - #### 简介
    该实例按照阿里巴巴 [IoT开放平台][3] 关于 [天猫精灵蓝牙mesh软件基础规范][4]，根据 `硬件品类规范` 描述自己为一个 [风扇][7]，通过 `天猫精灵` 语音输入进行发现连接(配网)和控制设备
  - #### 实际操作
    - 基本配置

      使用 **USB DP** 作为串口 **debug** 脚，波特率为 **1000000**

      使用 **PB7** 控制 **LED** 灯(模拟风扇的开和关的操作)

      设备名称为`AG-Socket`

      **三元组**（`MAC 地址`、`ProductID`、`Secret`）在天猫精灵开发者网站申请

      设置 [**api/model_api.h**](./api/model_api.h)
      ```C
      #define CONFIG_MESH_MODEL                   SIG_MESH_ALIGENIE_FAN
      ```
      设置 [**board/xxxx/board_xxxx_demo_cfg.h**](./board/bd29/board_ac630x_demo_cfg.h)
      ```C
      #define TCFG_UART0_TX_PORT                  IO_PORT_DP
      #define TCFG_UART0_BAUDRATE                 1000000
      ```
      设置 [**examples/AliGenie_socket.c**](./examples/AliGenie_socket.c)
      ```C
      #define BLE_DEV_NAME                        'A', 'G', '-', 'F', 'a', 'n'
      //< 三元组(本例以个人名义申请的风扇类三元组)
      #define CUR_DEVICE_MAC_ADDR                 0x27fa7af002a0
      #define PRODUCT_ID                          7809508
      #define DEVICE_SECRET                       "d2729d5f3898079fa7b697c76a7bfe8e"
      const u8 led_use_port[] = {

          IO_PORTB_07,

      };
      ```
      对于 **MAC** 地址，本例中一定要按照**三元组**里面的 **MAC** 地址传入到 `bt_mac_addr_set` 函数里

      设置 [**examples/AliGenie_fan.c**](./examples/AliGenie_fan.c)
      ```C
      void bt_ble_init(void)
      {
            u8 bt_addr[6] = {MAC_TO_LITTLE_ENDIAN(CUR_DEVICE_MAC_ADDR)};

            bt_mac_addr_set(bt_addr);

            mesh_setup(mesh_init);
      }
      ```

    - 编译工程并下载到目标板，接好串口，接好演示用 **LED** 灯，上电或者复位设备
    - `天猫精灵`连接到互联网上
      - 上电`天猫精灵`，**长按设备**上的**语音按键**，让设备进入待连接状态
      - 手机应用商店下载`天猫精灵 APP`，**APP** 上登陆个人中心
      - 打开手机 `WLAN`，将`天猫精灵`通过手机热点连接到互联网上

      ![AliGenie_connect][gif_AliGenie_connect]

    - 通过`天猫精灵`进行配网和控制
      - 配网对话

      >用户：“天猫精灵，发现设备”

      >天猫精灵：“已为您发现风扇，是否需要连接”

      >用户：“连接”

      >天猫精灵：“连接成功。。。 。。。”

      - 语音控制 **风扇命令** (可通过 [IoT开放平台][6] 添加自定义语音命令)

        **命令** | **效果**
        :-:|:-:
        `天猫精灵，打开风扇` | 开发板上 **LED** 灯打开
        `天猫精灵，风扇调到3档` | 开发板上 **LED** 灯亮度改变
        `天猫精灵，关闭风扇` | 开发板上 **LED** 灯关闭

  - #### 代码解读
    - 配网

      关键在于如何设置在`天猫精灵`开发者网站申请下来的**三元组**
      - `天猫精灵`开发者网站申请三元组，并填到下面文件相应宏定义处

        例如申请到的三元组如下：

        **Product ID(十进制)** | **Device Secret** | **Mac地址**
        :-:|:-:|:-:
        7809508 | d2729d5f3898079fa7b697c76a7bfe8e | 27fa7af002a0

        则按下面规则填写，**MAC** 前要加上 **0x**，**Secret** 要用 **双引号** 包住

        设置 [**examples/AliGenie_fan.c**](./examples/AliGenie_fan.c)
        ```C
        //< 三元组(本例以个人名义申请的风扇类三元组)
        #define CUR_DEVICE_MAC_ADDR             0x27fa7af002a0
        #define PRODUCT_ID                      7809508
        #define DEVICE_SECRET                   "d2729d5f3898079fa7b697c76a7bfe8e"
        ```

    - 建立 **Element** 和 **Model**

        按照 [风扇软件规范][7]，要建立一个 **element**，两个**model**
        **Element** | **Model** | **属性名称**
        :-:|:-:|:-:
        Primary | Generic OnOff Server(0x1000) | 开关
        Primary | Vendor Model(0x01A80000) | 故障上报/定时控制风扇开关/调整风扇风速

        相应代码操作如下：

        结构体`elements`注册了一个**primary element = SIG root_models + Vendor_server_models**

        结构体`root_models` **= Cfg_Server + Generic_OnOff_Server**

        结构体`vendor_server_models` **= Vendor_Client_Model + Vendor_Server_Model**

        ```C
        //< Basic_Cfg_Server + Generic_OnOff_Server
        static struct bt_mesh_model root_models[] = {
               BT_MESH_MODEL_CFG_SRV(&cfg_srv), 
               BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, gen_onoff_srv_op, &gen_onoff_pub_srv, &onoff_state[0]),
            // BT_MESH_MODEL(...) // add new sig model
        };

        //< Vendor_Client + Vendor_Server
        static struct bt_mesh_model vendor_server_models[] = {
              BT_MESH_MODEL_VND(BT_COMP_ID_LF, BT_MESH_VENDOR_MODEL_ID_CLI, NULL, NULL, NULL),
              BT_MESH_MODEL_VND(BT_COMP_ID_LF, BT_MESH_VENDOR_MODEL_ID_SRV, vendor_srv_op, NULL, &onoff_state[0]),
           // BT_MESH_MODEL_VND(...) // add new vendor model
        };

        //< Only primary element
        static struct bt_mesh_elem elements[] = {
               BT_MESH_ELEM(0, root_models, vendor_server_models),  // primary element
               // BT_MESH_ELEM(...) // add second element
        };
        ```

    - 用户数据处理
      - **SIG Generic OnOff Server**回调

         结构体 `root_models` 里的 **Generic_OnOff_Server** 注册了回调 `gen_onoff_srv_op` 来对用户数据进行处理

         当收到 `BT_MESH_MODEL_OP_GEN_ONOFF_GET` 等注册消息时，就会调用 `gen_onoff_get` 等对应的回调函数进行用户数据处理
         ```C
         static const struct bt_mesh_model_op gen_onoff_srv_op[] = {
               { BT_MESH_MODEL_OP_GEN_ONOFF_GET, 0, gen_onoff_get },
               { BT_MESH_MODEL_OP_GEN_ONOFF_SET, 2, gen_onoff_set },
               { BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK, 2, gen_onoff_set_unack },
               BT_MESH_MODEL_OP_END,
         };
         ```

      - **Vendor Model**回调

        结构体 `vendor_srv_op` 里的 **Vendor_Server_Model** 注册了回调 `vendor_srv_op` 来对用户数据进行处理

        当收到 `VENDOR_MSG_ATTR_GET` 等注册消息时，就会调用 `vendor_attr_get` 等对应的回调函数进行用户数据处理
        ```C
        static const struct bt_mesh_model_op vendor_srv_op[] = {
               { VENDOR_MSG_ATTR_GET, ACCESS_OP_SIZE, vendor_attr_get },
               { VENDOR_MSG_ATTR_SET, ACCESS_OP_SIZE, vendor_attr_set },
               BT_MESH_MODEL_OP_END,
        };
        ```
      - **vendor_attr_set**函数控制风扇档位

        从接收到的消息解析出来的 `level` 为风扇档位

        该程序设定的档位数最大为5，各档位风速为最大风速的 `level` 除以5
        ```c
        case ATTR_TYPE_WIND_SPEED: {
        u7 level = buffer_pull_u8_from_head(buf);
        struct __fan_speed fan_speed = {
            .Opcode = buffer_head_init(VENDOR_MSG_ATTR_STATUS),
            .TID = tid,
            .Attr_Type = Attr_Type,
            .level = level,
        };

        if (level > 5) {
            printf("max level = 5\n");
            level = 5;
        }
            printf("fan level set to %d\r\n", level);

            set_led_duty(level * 1999);

            vendor_attr_status_send(model, ctx, &fan_speed, sizeof(fan_speed));
        }
        break;
        ```

    >### SIG AliGenie Light
    ---
  - #### 简介
    该实例按照阿里巴巴 [IoT开放平台][3] 关于 [天猫精灵蓝牙mesh软件基础规范][4]，根据 `硬件品类规范` 描述自己为一个 [灯][8]，通过 `天猫精灵` 语音输入进行发现连接(配网)和控制设备
  - #### 实际操作
    - 基本配置

    使用 **USB DP** 作为串口 **debug** 脚，波特率为 **1000000**

    使用 **PA1** 控制 **LED** 灯(模拟灯的开和关的操作)

    设备名称为`AG-Socket`

    **三元组**（`MAC 地址`、`ProductID`、`Secret`）在天猫精灵开发者网站申请

    设置 [**api/model_api.h**](./api/model_api.h)
    ```C
    #define CONFIG_MESH_MODEL                   SIG_MESH_ALIGENIE_LIGHT
    ```
    设置 [**board/xxxx/board_xxxx_demo_cfg.h**](./board/bd29/board_ac630x_demo_cfg.h)
    ```C
    #define TCFG_UART0_TX_PORT                  IO_PORT_DP
    #define TCFG_UART0_BAUDRATE                 1000000
    ```
    设置 [**examples/AliGenie_socket.c**](./examples/AliGenie_socket.c)
    ```C
    #define BLE_DEV_NAME                        'A', 'G', '-', 'L', 'i', 'g', 'h', 't'
    //< 三元组(本例以个人名义申请的灯类三元组)
    #define CUR_DEVICE_MAC_ADDR                 0x18146c110001
    #define PRODUCT_ID                          7218909
    #define DEVICE_SECRET                       "aab00b61998063e62f98ff04c9a787d4"
    const u8 led_use_port[] = {

        IO_PORTB_07,

    };
    ```
    对于 **MAC** 地址，本例中一定要按照**三元组**里面的 **MAC** 地址传入到 `bt_mac_addr_set` 函数里

    设置 [**examples/AliGenie_light.c**](./examples/AliGenie_light.c)
    ```C
    void bt_ble_init(void)
    {
          u8 bt_addr[6] = {MAC_TO_LITTLE_ENDIAN(CUR_DEVICE_MAC_ADDR)};

          bt_mac_addr_set(bt_addr);

          mesh_setup(mesh_init);
    }
    ```

    - 编译工程并下载到目标板，接好串口，接好演示用 **LED** 灯，上电或者复位设备
    - `天猫精灵`连接到互联网上
      - 上电`天猫精灵`，**长按设备**上的**语音按键**，让设备进入待连接状态
      - 手机应用商店下载`天猫精灵 APP`，**APP** 上登陆个人中心
      - 打开手机 `WLAN`，将`天猫精灵`通过手机热点连接到互联网上

      ![AliGenie_connect][gif_AliGenie_connect]
    - 通过`天猫精灵`进行配网和控制
      - 配网对话

      >用户：“天猫精灵，发现设备”

      >天猫精灵：“发现了智能灯，是否需要连接”

      >用户：“连接”

      >天猫精灵：“连接成功。。。 。。。”

      - 语音控制 **灯命令** (可通过 [IoT开放平台][6] 添加自定义语音命令)

        **命令** | **效果**
        :-:|:-:
        `天猫精灵，开灯` | 开发板上 **LED** 灯打开
        `天猫精灵，灯的亮度调到50` | 开发板上 **LED** 灯亮度改变
        `天猫精灵，关灯` | 开发板上 **LED** 灯关闭
        `天猫精灵，五分钟后开灯` | 开发板上 **LED** 灯在五分钟后打开

  - #### 代码解读
    - 配网

      关键在于如何设置在`天猫精灵`开发者网站申请下来的**三元组**
      - `天猫精灵`开发者网站申请三元组，并填到下面文件相应宏定义处

        例如申请到的三元组如下：

        **Product ID(十进制)** | **Device Secret** | **Mac地址**
        :-:|:-:|:-:
        7809508 | d2729d5f3898079fa7b697c76a7bfe8e | 27fa7af002a0

        则按下面规则填写，**MAC** 前要加上 **0x**，**Secret** 要用 **双引号** 包住

        设置 [**examples/AliGenie_light.c**](./examples/AliGenie_light.c)
        ```C
        //< 三元组(本例以个人名义申请的灯类三元组)
        #define CUR_DEVICE_MAC_ADDR             0x18146c110001
        #define PRODUCT_ID                      7218909
        #define DEVICE_SECRET                   "aab00b61998063e62f98ff04c9a787d4"
        ```

    - 建立 **Element** 和 **Model**

        按照 [风扇软件规范][7]，要建立一个 **element**，两个**model**
        **Element** | **Model** | **属性名称**
        :-:|:-:|:-:
        Primary | Generic OnOff Server(0x1000) | 开关
        Primary |  Light Lightness server(0x1300) | 灯的亮度调整
        Primary | Vendor Model(0x01A80000) | 故障上报/定时控制灯

        相应代码操作如下：

        结构体`elements`注册了一个**primary element = SIG root_models + Vendor_server_models**

        结构体`root_models` **= Cfg_Server + Generic_OnOff_Server**

        结构体`vendor_server_models` **= Vendor_Client_Model + Vendor_Server_Model**

        ```C
        //< Basic_Cfg_Server + Light_Lightness_server + Generic_OnOff_Server
        static struct bt_mesh_model root_models[] = {
               BT_MESH_MODEL_CFG_SRV(&cfg_srv), 
               BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, gen_onoff_srv_op, &gen_onoff_pub_srv, &onoff_state[0]),
               BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_SRV, light_lightness_srv_op, &gen_onoff_pub_srv, &light),
            // BT_MESH_MODEL(...) // add new sig model
        };

        //< Vendor_Client + Vendor_Server
        static struct bt_mesh_model vendor_server_models[] = {
              BT_MESH_MODEL_VND(BT_COMP_ID_LF, BT_MESH_VENDOR_MODEL_ID_CLI, NULL, NULL, NULL),
              BT_MESH_MODEL_VND(BT_COMP_ID_LF, BT_MESH_VENDOR_MODEL_ID_SRV, vendor_srv_op, NULL, &onoff_state[0]),
           // BT_MESH_MODEL_VND(...) // add new vendor model
        };

        //< Only primary element
        static struct bt_mesh_elem elements[] = {
               BT_MESH_ELEM(0, root_models, vendor_server_models),  // primary element
               // BT_MESH_ELEM(...) // add second element
        };
        ```

    - 用户数据处理
      - **SIG Generic OnOff Server**回调

        结构体 `root_models` 里的 **Generic_OnOff_Server** 注册了回调 `gen_onoff_srv_op` 来对用户数据进行处理

        当收到 `BT_MESH_MODEL_OP_GEN_ONOFF_GET` 等注册消息时，就会调用 `gen_onoff_get` 等对应的回调函数进行用户数据处理
        ```C
        static const struct bt_mesh_model_op gen_onoff_srv_op[] = {
              { BT_MESH_MODEL_OP_GEN_ONOFF_GET, 0, gen_onoff_get },
              { BT_MESH_MODEL_OP_GEN_ONOFF_SET, 2, gen_onoff_set },
              { BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK, 2, gen_onoff_set_unack },
              BT_MESH_MODEL_OP_END,
         };
         ```
    - ** Light Lightness server**回调

      结构体 `root_models` 里的 **Light_Lightness_server** 注册了回调 `light_lightness_srv_op` 来对用户数据进行处理

      当收到 `BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_GET` 等注册消息时，就会调用 `lightness_get` 等对应的回调函数进行用户数据处理

      ```c
      const struct bt_mesh_model_op light_lightness_srv_op[] = {
        { BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_GET, 				0, lightness_get},
        { BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_SET, 				0, lightness_set},
        { BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_SET_UNACK, 			0, lightness_set_unack},
        BT_MESH_MODEL_OP_END,
      };
      ```
    - **Vendor Model**回调

      结构体 `vendor_srv_op` 里的 **Vendor_Server_Model** 注册了回调 `vendor_srv_op` 来对用户数据进行处理

      当收到 `VENDOR_MSG_ATTR_GET` 等注册消息时，就会调用 `vendor_attr_get` 等对应的回调函数进行用户数据处理
      ```C
      static const struct bt_mesh_model_op vendor_srv_op[] = {
             { VENDOR_MSG_ATTR_GET, ACCESS_OP_SIZE, vendor_attr_get },
             { VENDOR_MSG_ATTR_SET, ACCESS_OP_SIZE, vendor_attr_set },
             BT_MESH_MODEL_OP_END,
      };
      ```
