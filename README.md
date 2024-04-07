
[tag download]:https://github.com/Jieli-Tech/fw-AC63_BT_SDK/tags
[tag_badgen]:https://img.shields.io/github/v/tag/Jieli-Tech/fw-AC63_BT_SDK?style=plastic&logo=bluetooth&labelColor=ffffff&color=informational&label=Tag&logoColor=blue

# fw-AC63_BT_SDK   [![tag][tag_badgen]][tag download]

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

* 编译工具 ：请安装杰理编译工具来搭建起编译环境, [下载链接](https://doc.zh-jieli.com/Tools/zh-cn/dev_tools/dev_env/index.html) 

* USB 升级工具 : 在开发完成后，需要使用杰理烧写工具将对应的`hex`文件烧录到目标板，进行开发调试, 关于如何获取工具请进入申请 [链接](https://item.taobao.com/item.htm?spm=a1z10.1-c-s.w4004-22883854875.5.504d246bXKwyeH&id=620295020803) 并详细阅读对应的[文档](https://doc.zh-jieli.com/Tools/zh-cn/dev_tools/forced_upgrade/index.html)，以及相关下载脚本[配置](https://doc.zh-jieli.com/AC63/zh-cn/master/getting_started/project_download/INI_config.html)

介绍文档
------------

* 芯片简介 : [SoC 数据手册扼要](https://doc.zh-jieli.com/vue/#/docs/ac63), [下载链接](./doc/datasheet)

* SDK 版本信息 : [SDK 历史版本](https://doc.zh-jieli.com/AC63/zh-cn/master/other/version/index.html)

* SDK 介绍文档 : [SDK 快速开始简介](https://doc.zh-jieli.com/AC63/zh-cn/master/index.html)

* SDK 结构文档 : [SDK 模块结构](./doc/architure)

编译工程
-------------
请选择以下一个工程进行编译，下列目录包含了便于开发的工程文件：

* 蓝牙应用 : [SPP_LE](./apps/spp_and_le), 适用领域：透传, 数传, 扫描设备, 广播设备, 信标, FindMy应用, 多机连接. Dongle(usb / bt). [文档链接](https://doc.zh-jieli.com/AC63/zh-cn/master/module_demo/spple/index.html)

* 蓝牙应用 : [HID](./apps/hid), 适用领域：遥控器, 自拍器, 键盘, 鼠标, 吃鸡王座, 语音遥控器. [文档链接](https://doc.zh-jieli.com/AC63/zh-cn/master/module_demo/hid/index.html)

* 蓝牙应用 : [Mesh](./apps/mesh), 适用领域：物联网节点, 天猫精灵接入, 自组网应用. [文档链接](https://doc.zh-jieli.com/AC63/zh-cn/master/module_demo/mesh/index.html)

已发布版本详见 标签(Tags)。

即将发布：

* 蓝牙应用 ：`IoT (ipv6 / 6lowpan)`

* 2.4G 应用 : `Vendor Wireless`

SDK 同时支持Codeblock 和 Make 编译环境，请确保编译前已经搭建好编译环境，

* Codeblock 编译 : 进入对应的工程目录并找到后缀为 `.cbp` 的文件, 双击打开便可进行编译.

* Makefile 编译 : 双击`tools/make_prompt.bat`，输入 `make target`（具体`target`的名字，参考`Makefile`开头的注释）

  `在编译下载代码前，请确保USB 升级工具正确连接并且进入编程模式`
  
* 蓝牙OTA : [OTA](https://doc.zh-jieli.com/AC63/zh-cn/master/module_demo/ota/index.html) , 适用领域：单备份，双备份蓝牙升级

蓝牙官方认证
-------------

经典蓝牙LMP / 低功耗蓝牙Link Layer 层和Host 协议栈均支持蓝牙5.0 、5.1和5.4版本实现

* Core v5.0 [QDID 134104](https://launchstudio.bluetooth.com/ListingDetails/88799)

* Core v5.1 [QDID 136145](https://launchstudio.bluetooth.com/ListingDetails/91371)

* Core v5.4 [QDID 222830](https://launchstudio.bluetooth.com/ListingDetails/193923)


硬件环境
-------------

* 开发评估板 ：开发板申请入口[链接](https://shop321455197.taobao.com/?spm=a230r.7195193.1997079397.2.2a6d391d3n5udo)

* 生产烧写工具 : 为量产和裸片烧写而设计, 申请入口 [链接](https://item.taobao.com/item.htm?spm=a1z10.1-c-s.w4004-22883854875.8.504d246bXKwyeH&id=620941819219) 并仔细阅读相关 [文档](https://doc.zh-jieli.com/Tools/zh-cn/mass_prod_tools/burner_1tuo2/index.html)

* 无线测试盒 : 为空中升级/射频标定/快速产品测试而设计, 申请入口 [链接](https://item.taobao.com/item.htm?spm=a1z10.1-c-s.w4004-22883854875.10.504d246bXKwyeH&id=620942507511), 阅读[文档](https://doc.zh-jieli.com/Tools/zh-cn/mass_prod_tools/testbox_1tuo2/index.html) 获取更多详细信息.


社区
--------------

* 技术交流群，钉钉群 ID: `3375034077`

* 常见问题集合[链接](./doc/FAQ)

免责声明
------------

AC63_BT_SDK 支持AC63 系列芯片开发.
AC63 系列芯片支持了通用蓝牙常见应用，可以作为开发，评估，样品，甚至量产使用，对应SDK 版本见tag 和 release
