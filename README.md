# fw-AC63_BT_SDK

EN | [中文](./README-zh.md)

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

* Complie Tool ：install the JL complie tool to setup the build enviroment, [download link](https://pan.baidu.com/s/1f5pK7ZaBNnvbflD-7R22zA) code: `ukgx`

* USB updater : program flash tool to download the `hex` file to the target board, please accquire the tool form the [link](https://item.taobao.com/item.htm?spm=a1z10.1-c-s.w4004-22883854875.5.504d246bXKwyeH&id=620295020803) and check the related [doc](.Stuff)


Documentation
------------

* Chipset brief : [SoC datasheet](./doc)

* SDK introduction : [SDK quick start quide](./doc/)


Build
-------------
Select a project to build. The following folders contains buildable projects:

* APP_Bluetooth : [SPP_LE](./apps/spp_and_le)

* APP_Bluetooth : [HID](./apps/hid)

* APP_Bluetooth : [Mesh](./apps/mesh)

SDK support Codeblock & Make to build to project,make sure you already setup the enviroment 

* Codeblock build : enter the project directory and find the `.cbp`,double click and build.

* Makefile build : `apps/app_cfg` select the target you want to build,double click the `make_prompt` and excute `make`
    
  `before build the project make sure the USB updater is connect and enter the update mode correctly`

Certification
-------------

Bluetooth Classic LMP /LE Link Layer protocol stack implementing Bluetooth 5.0 specification

* [QDID 134104](https://launchstudio.bluetooth.com/ListingDetails/88799)


Hardware
-------------

* EV Board ：(https://shop321455197.taobao.com/?spm=a230r.7195193.1997079397.2.2a6d391d3n5udo)

* Production Tool : massive prodution and program the SoC, please accquire the tool from the [link](https://item.taobao.com/item.htm?spm=a1z10.1-c-s.w4004-22883854875.8.504d246bXKwyeH&id=620941819219) and check the releated [doc](./Stuff)

* Wireless Tester : Over the air update/RF Calibration/Fast production test, please accuire the tool from the [link](https://item.taobao.com/item.htm?spm=a1z10.1-c-s.w4004-22883854875.10.504d246bXKwyeH&id=620942507511),check [doc](./stuff) for more infomation.


Disclaimer
------------

AC630N_BT_SDK supports development with AC63 series devices.
AC63 Series devices (which are pre-production) and Bluetooth Mesh protocols are supported for development in v0.2.0 for prototyping and evaluation.
Support for production and deployment in end products is coming soon.

