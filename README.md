# fw-AC63_BT_SDK

中文 | [EN](./README-en.md)

AC63 系列通用蓝牙SDK 固件程序

本仓库包含SDK release 版本代码，线下线上支持同步发布，并且引用了其他开源项目（如Zephyr RTOS）.

本工程提供的例子，需要结合对应命名规则的库文件(lib.a) 和对应的子仓库进行编译.

快速开始
------------

欢迎使用杰理开源项目，在开始进入项目之前，请详细阅读SDK 介绍文档，
从而获得对杰理系列芯片和SDK 的大概认识，并且可以通过快速开始介绍来进行开发.

工具链
------------

关于如何获取`杰理工具链` 和 如何进行环境搭建，请阅读以下内容：

* 编译工具 ：请安装杰理编译工具来搭建起编译环境, [下载链接](https://pan.baidu.com/s/1f5pK7ZaBNnvbflD-7R22zA) 提取码: `ukgx`

* USB 升级工具 : 在开发完成后，需要使用杰理烧写工具将对应的`hex`文件烧录到目标板，进行开发调试, 关于如何获取工具请进入申请 [链接](https://item.taobao.com/item.htm?spm=a1z10.1-c-s.w4004-22883854875.5.504d246bXKwyeH&id=620295020803) 并详细阅读对应的[文档](.Stuff)


介绍文档
------------

* 芯片简介 : [SoC 数据手册扼要](./doc)

* SDK 版本信息 : [SDK 历史版本](doc/AC630N_bt_data_transfer_sdk_发布版本信息.pdf)

* SDK 介绍文档 : [SDK 快速开始简介](./doc/AC630N_bt_data_transfer_sdk介绍.pdf)

编译工程
-------------
请选择以下一个工程进行编译，下列目录包含了便于开发的工程文件：

* 蓝牙应用 : [SPP_LE](./apps/spp_and_le)

* 蓝牙应用 : [HID](./apps/hid)

* 蓝牙应用 : [Mesh](./apps/mesh)

SDK 同时支持Codeblock 和 Make 编译环境，请确保编译前已经搭建好编译环境，

* Codeblock 编译 : 进入对应的工程目录并找到后缀为 `.cbp` 的文件, 双击打开便可进行编译.

* Makefile 编译 : `apps/app_cfg` 开始编译之前，需要先选择好目标应用并编辑保存, 请双击 `make_prompt` 并输入 `make`

  `before build the project make sure the USB updater is connect and enter the update mode correctly`

蓝牙官方认证
-------------

经典蓝牙LMP / 低功耗蓝牙Link Layer 层和Host 协议栈均支持蓝牙5.0 和 5.1版本实现

* Core v5.0 [QDID 134104](https://launchstudio.bluetooth.com/ListingDetails/88799)

* Core v5.1 [QDID 136145](https://launchstudio.bluetooth.com/ListingDetails/91371)


硬件环境
-------------

* 开发评估板 ：开发板申请入口[链接](https://shop321455197.taobao.com/?spm=a230r.7195193.1997079397.2.2a6d391d3n5udo)

* 生产烧写工具 : 为量产和裸片烧写而设计, 申请入口 [连接](https://item.taobao.com/item.htm?spm=a1z10.1-c-s.w4004-22883854875.8.504d246bXKwyeH&id=620941819219) 并仔细阅读相关 [文档](./doc/stuff/烧写器使用说明文档.pdf)

* 无线测试盒 : 为空中升级/射频标定/快速产品测试而设计, 申请入口 [链接](https://item.taobao.com/item.htm?spm=a1z10.1-c-s.w4004-22883854875.10.504d246bXKwyeH&id=620942507511), 阅读[文档](./doc/stuff/AC690x_1T2测试盒使用说明.pdf) 获取更多详细信息.


免责声明
------------

AC63_BT_SDK 支持AC63 系列芯片开发.
AC63 系列芯片支持了通用蓝牙常见应用，可以作为开发，评估，样品，甚至量产使用，对应SDK 版本见tag v0.3.0
