[icon_build]:./../../doc/stuff/build_passing.svg
[pic_client_flowsheet]:./../../doc/stuff/client_flowsheet.jpg
[pic_server_flowsheet]:./../../doc/stuff/server_flowsheet.jpg
[gif_OnOff_client]:./../../doc/Generic_On_Off_Client.gif
[gif_OnOff_server]:./../../doc/Generic_On_Off_Server.gif
[icon_download]:https://api.bintray.com/packages/nordic/android/no.nordicsemi.android%3Amesh/images/download.svg
[download_address]:https://github.com/NordicSemiconductor/Android-nRF-Mesh-Library/releases/download/v2.4.1/nRF.Mesh.-.V.-.2.4.1.apk

[1]:https://www.bluetooth.com/specifications/mesh-specifications/
[2]:https://www.bluetooth.com/specifications/bluetooth-core-specification/

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
    - #### [应用实例选择](./README.md#应用实例选择-1)
    - #### [Mesh 配置](./README.md#Mesh-配置-1)
    - #### [Board 配置](./README.md#Board-配置-1)
-  ### [应用实例](./README.md#应用实例-1)
    - #### [SIG Generic OnOff Client](./README.md#SIG-Generic-OnOff-Client-1)
    - #### [SIG Generic OnOff Server](./README.md#SIG-Generic-OnOff-Server-1)

## Start
> :book: 目录文件结构 :book:
```java
▾ mesh/
    ▾ api/
        feature_correct.h
        mesh_config_common.c
        model_api.c
        model_api.h
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
        tmall_genie_vendor.c
        vendor_client.c
        vendor_server.c
```

  >### 应用实例选择
  ---
  - 在 [**api/model_api.h**](./api/model_api.h) 下，通过配置`CONFIG_MESH_MODEL`选择相应例子，SDK提供了5个应用实例。默认选择`SIG_MESH_GENERIC_ONOFF_CLIENT`，即位于 [**examples/generic_onoff_client.c**](./examples/generic_onoff_client.c) 下的例子。
```C
//< Detail in "MshMDLv1.0.1"
#define SIG_MESH_GENERIC_ONOFF_CLIENT       0 // examples/generic_onoff_client.c
#define SIG_MESH_GENERIC_ONOFF_SERVER       1 // examples/generic_onoff_server.c
#define SIG_MESH_VENDOR_CLIENT              2 // examples/vendor_client.c
#define SIG_MESH_VENDOR_SERVER              3 // examples/vendor_server.c
#define SIG_MESH_TMALL_GENIE_VENDOR         4 // examples/tmall_genie_vendor.c
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

      设备名称为`OnOff_cli`，MAC地址为`11:22:33:44:55:66`

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

      设备名称为`OnOff_srv`，MAC地址为`22:22:33:44:55:66`

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
