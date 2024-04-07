
[tag download]:https://github.com/Jieli-Tech/fw-AC63_BT_SDK/tags
[tag_badgen]:https://img.shields.io/github/v/tag/Jieli-Tech/fw-AC63_BT_SDK?style=plastic&logo=bluetooth&labelColor=ffffff&color=informational&label=Tag&logoColor=blue

# fw-AC63_BT_SDK   [![tag][tag_badgen]][tag download]

[中文](./README.md) | EN

firmware for Generic bluetooth SDK（AC63 series）

This repository contains the Jieli source code additions to open
source projects (Zephyr RTOS).
It must be combined with lib.a and the repositories that use the same
naming convention to build the provided samples and to use the additional
subsystems and libraries.

Getting Started
------------

Welcome to JL open source! See the `Introduction to SDK` for a high-level overview,
and the documentation's `Getting Started Guide` to start developing.


Toolchain
------------

How to get the `JL Toolchain` and setup the build enviroment,see below

* Complie Tool ：install the JL complie tool to setup the build enviroment, [download link](https://doc.zh-jieli.com/Tools/zh-cn/dev_tools/dev_env/index.html) 

* USB updater : program flash tool to download the `hex` file to the target board, please accquire the tool form the [link](https://item.taobao.com/item.htm?spm=a1z10.1-c-s.w4004-22883854875.5.504d246bXKwyeH&id=620295020803) and check the related configuration and [document](https://doc.zh-jieli.com/AC63/zh-cn/master/getting_started/project_download/INI_config.html)


Documentation
------------

* Chipset brief : [SoC datasheet](https://doc.zh-jieli.com/vue/#/docs/ac63), [download link](./doc/datasheet)

* SDK Version: [SDK History](https://doc.zh-jieli.com/AC63/zh-cn/master/other/version/index.html)

* SDK introduction : [SDK quick start guide](https://doc.zh-jieli.com/AC63/zh-cn/master/index.html)

* SDK architure : [SDK module architure ](./doc/architure)

Build
-------------
Select a project to build. The following folders contains buildable projects:

* APP_Bluetooth : [SPP_LE](./apps/spp_and_le), usage: data transfer, centeral devices, boardcast devices, beacon, FindMy, multi-link, Dongle(usb / bt). [document](https://doc.zh-jieli.com/AC63/zh-cn/master/module_demo/spple/index.html)

* APP_Bluetooth : [HID](./apps/hid), usage: remote control, keyboard, mouse, game box, Voice remote control. [document](https://doc.zh-jieli.com/AC63/zh-cn/master/module_demo/hid/index.html)

* APP_Bluetooth : [Mesh](./apps/mesh), usage: Mesh nodes. [document](https://doc.zh-jieli.com/AC63/zh-cn/master/module_demo/mesh/index.html)

See Tags for the released versions.

Comming Soon：

* APP_Bluetooth ：`IoT (ipv6 / 6lowpan)`

* 2.4G_APP : `Vendor Wireless`

SDK support Codeblock & Make to build to project,make sure you already setup the enviroment

* Codeblock build : enter the project directory and find the `.cbp`,double click and build.

* Makefile build : double click the `tools/make_prompt.bat` and excute `make target`(see `makfile`)

  `before build the project make sure the USB updater is connect and enter the update mode correctly`

* Bluetooth OTA : [OTA](https://doc.zh-jieli.com/AC63/zh-cn/master/module_demo/ota/index.html) , usage: Single/Double bank bluetooth update.

Certification
-------------

Bluetooth Classic LMP /LE Link Layer protocol stack implementing Bluetooth 5.0/5.1/5.4 specification

* Core v5.0 [QDID 134104](https://launchstudio.bluetooth.com/ListingDetails/88799)

* Core v5.1 [QDID 136145](https://launchstudio.bluetooth.com/ListingDetails/91371)

* Core v5.4 [QDID 222830](https://launchstudio.bluetooth.com/ListingDetails/193923)


Hardware
-------------

* EV Board ：[link](https://shop321455197.taobao.com/?spm=a230r.7195193.1997079397.2.2a6d391d3n5udo)

* Production Tool : massive prodution and program the SoC, please accquire the tool from the [link](https://item.taobao.com/item.htm?spm=a1z10.1-c-s.w4004-22883854875.8.504d246bXKwyeH&id=620941819219) and check the releated [doc](./doc/stuff/烧写器使用说明文档.pdf)

* Wireless Tester : Over the air update/RF Calibration/Fast production test, please accuire the tool from the [link](https://item.taobao.com/item.htm?spm=a1z10.1-c-s.w4004-22883854875.10.504d246bXKwyeH&id=620942507511),check [doc](https://doc.zh-jieli.com/Tools/zh-cn/mass_prod_tools/testbox_1tuo2/index.html) for more infomation.


Community
--------------

* [Dingtalk Group] ID: `3375034077`

* [Q&A](./doc/FAQ)

Disclaimer
------------

AC630N_BT_SDK supports development with AC63 series devices.
AC63 Series devices (which are pre-production) and Bluetooth Mesh protocols are supported for development in tag v0.5.0 for prototyping and evaluation.
Support for production and deployment in end products is coming soon.
