
// *INDENT-OFF*
#include "app_config.h"

#ifdef __SHELL__
[配置项预留起始项一_注意该位置不要定义子配置项]
[配置项预留起始项二_注意该位置不要定义子配置项]
[配置项预留起始项三_注意该位置不要定义子配置项]
[配置项预留起始项四_注意该位置不要定义子配置项]
[配置项预留起始项五_注意该位置不要定义子配置项]


[EXTRA_CFG_PARAM]

#if CONFIG_DOUBLE_BANK_ENABLE
BR22_TWS_DB=YES;	//dual bank flash framework enable
FLASH_SIZE=1M;		//flash_size cfg
BR22_TWS_VERSION=0; //default fw version

DB_UPDATE_DATA=YES; //generate db_update_data.bin
SPECIAL_OPT=0;
FORCE_4K_ALIGN=YES; // force aligin with 4k bytes
#else
NEW_FLASH_FS=YES;	//enable single bank flash framework
#endif 				//CONFIG_DOUBLE_BANK_ENABLE


#ifdef CONFIG_BR30_C_VERSION
CHIP_NAME=AC699N;//8
#else
CHIP_NAME=AC897N;//8
#endif

ENTRY=0x1E00120;////程序入口地址
#ifdef CONFIG_BR30_C_VERSION
PID=AC699N;//长度16byte,示例：芯片封装_应用方向_方案名称
#else
PID=AC897N;//长度16byte,示例：芯片封装_应用方向_方案名称
#endif

VID=0.01;	


RESERVED_OPT=0;

DOWNLOAD_MODEL=usb;//
// DOWNLOAD_MODEL=SERIAL;//usb
// SERIAL_DEVICE_NAME=JlVirtualJtagSerial;
// SERIAL_BARD_RATE=1000000;
// SERIAL_CMD_OPT=11;
// SERIAL_CMD_RATE=100; //[n*10000]
// SERIAL_CMD_RES=0;
// SERIAL_INIT_BAUD_RATE=9600;
// LOADER_BAUD_RATE=1000000;
// LOADER_ASK_BAUD_RATE=1000000;
// BEFORE_LOADER_WAIT_TIME=100;
// SERIAL_SEND_KEY=yes;

// #####匹配的芯片版本,请勿随意改动 ######
[CHIP_VERSION]
#ifdef CONFIG_BR30_C_VERSION
SUPPORTED_LIST=C
#else
SUPPORTED_LIST=B,D,E,M,N,O,P
#endif

// #####################################################    UBOOT配置项，请勿随意调整顺序    ##################################################
[SYS_CFG_PARAM]
// #data_width[0 1 2 3 4] 3的时候uboot自动识别2或者4线
// #clk [0-255]
// #mode:
// #	  0 RD_OUTPUT,		 1 cmd 		 1 addr 
// #	  1 RD_I/O,   		 1 cmd 		 x addr
// #	  2 RD_I/O_CONTINUE] no_send_cmd x add
// #port:
// #	  0  优先选A端口  CS:PD3  CLK:PD0  D0:PD1  D1:PD2  D2:PB7  D3:PD5
// #	  1  优先选B端口  CS:PA13 CLK:PD0  D0:PD1  D1:PA14 D2:PA15 D3:PD5
SPI=2_3_0_0;	//data_clk_mode_port;
// OSC=btosc;
// OSC_FREQ=12MHz; #[24MHz 12MHz]
// SYS_CLK=24MHz;	#[48MHz 24MHz]
UTTX=PA05;//uboot串口tx
UTBD=1000000;//uboot串口波特率
// #UTRX=PB01;串口升级[PB00 PB05 PA05]
// #硬件复位可选择port口或LDOIN复位，LDOIN复位：死机进仓就可以复位
#if TCFG_LP_TOUCH_KEY_ENABLE
RESET=LDOIN_04_1;	//LDOIN口_入仓时间_有效电平（入仓时间有00、01、02、04、08、16六个值可选，单位为秒，当入仓时间为00时，则关闭长按复位。)
#else
RESET=PB01_08_0; //IO_TIME_LEVEL (TIME OF PRESSING 00、01、02、04、08、16 value，unit:second，if it configs to 00，this func will be closed.)
#endif

// 0:disable
// 1:PA9 PA10 
// 2:USB
// 3:PB1 PB2
// 4:PB6 PB7

// #sdtap=2;
psram=1;

VLVD=5;//VDDIO_LVD挡位，0: 1.8V   1: 1.9V   2: 2.0V   3: 2.1V   4: 2.2V   5: 2.3V   6: 2.4V   7: 2.5V

//#############################################################################################################################################



// ########flash空间使用配置区域###############################################
// #PDCTNAME:    产品名，对应此代码，用于标识产品，升级时可以选择匹配产品名
// #BOOT_FIRST:  1=代码更新后，提示APP是第一次启动；0=代码更新后，不提示
// #UPVR_CTL：   0：不允许高版本升级低版本   1：允许高版本升级低版本
// #XXXX_ADR:    区域起始地址	AUTO：由工具自动分配起始地址
// #XXXX_LEN:    区域长度		CODE_LEN：代码长度
// #XXXX_OPT:    区域操作属性
// #
// #
// #
// #操作符说明  OPT:
// #	0:  下载代码时擦除指定区域
// #	1:  下载代码时不操作指定区域
// #	2:  下载代码时给指定区域加上保护
// ############################################################################
[RESERVED_CONFIG]
BTIF_ADR=AUTO;
BTIF_LEN=0x1000;
BTIF_OPT=1;

#if (defined(CONFIG_APP_BT_ENABLE) || RCSP_UPDATE_EN) && !CONFIG_DOUBLE_BANK_ENABLE
EXIF_ADR=AUTO;
EXIF_LEN=0x1000;
EXIF_OPT=1;
#endif

// #WTIF_ADR=BEGIN_END;
// #WTIF_LEN=0x1000;
// #WTIF_OPT=1;

PRCT_ADR=0;
PRCT_LEN=CODE_LEN;
PRCT_OPT=2;

VM_ADR=0;
VM_LEN=4K;
VM_OPT=1;

#if TCFG_AUDIO_ANC_ENABLE
// #ANC配置区，如果不想ANC配置因为代码大小变化而改变位置，从而失效，需要手动指定(flash末尾8K位置)
// #4Mbit:0x7E000 8Mbit:0xFE000 16Mbit:0x1FE000
// #加载了anc_gains.bin或者anc_coeff.bin，则表示使用文件里面的配置
// #ANC增益配置保留区
ANCIF_FILE=anc_gains.bin;
ANCIF_ADR=0xFE000;
ANCIF_LEN=0x80;
ANCIF_OPT=1;
// #ANC系数配置保留区
// #ANCIF1_FILE=anc_coeff.bin;
ANCIF1_ADR=0xFE080;
ANCIF1_LEN=0xF80;
ANCIF1_OPT=1;
#endif

[BURNER_CONFIG]
SIZE=32;


[配置项预留结束项一_注意该位置不要定义子配置项]
[配置项预留结束项二_注意该位置不要定义子配置项]
[配置项预留结束项三_注意该位置不要定义子配置项]
[配置项预留结束项四_注意该位置不要定义子配置项]
[配置项预留结束项五_注意该位置不要定义子配置项]
#else

[EXTRA_CFG_PARAM]

#if CONFIG_DOUBLE_BANK_ENABLE
BR22_TWS_DB=YES;	//dual bank flash framework enable
FLASH_SIZE=1M;		//flash_size cfg
BR22_TWS_VERSION=0; //default fw version

DB_UPDATE_DATA=YES; //generate db_update_data.bin
SPECIAL_OPT=0;
FORCE_4K_ALIGN=YES; // force aligin with 4k bytes
#else
NEW_FLASH_FS=YES;	//enable single bank flash framework
#endif 				//CONFIG_DOUBLE_BANK_ENABLE


#ifdef CONFIG_BR30_C_VERSION
CHIP_NAME=AC699N;//8
#else
CHIP_NAME=AC897N;//8
#endif

ENTRY=0x1E00120;////程序入口地址
#ifdef CONFIG_BR30_C_VERSION
PID=AC699N;//长度16byte,示例：芯片封装_应用方向_方案名称
#else
PID=AC897N;//长度16byte,示例：芯片封装_应用方向_方案名称
#endif

VID=0.01;	


RESERVED_OPT=0;

DOWNLOAD_MODEL=usb;//
// DOWNLOAD_MODEL=SERIAL;//usb
// SERIAL_DEVICE_NAME=JlVirtualJtagSerial;
// SERIAL_BARD_RATE=1000000;
// SERIAL_CMD_OPT=11;
// SERIAL_CMD_RATE=100; //[n*10000]
// SERIAL_CMD_RES=0;
// SERIAL_INIT_BAUD_RATE=9600;
// LOADER_BAUD_RATE=1000000;
// LOADER_ASK_BAUD_RATE=1000000;
// BEFORE_LOADER_WAIT_TIME=100;
// SERIAL_SEND_KEY=yes;

// #####匹配的芯片版本,请勿随意改动 ######
[CHIP_VERSION]
#ifdef CONFIG_BR30_C_VERSION
SUPPORTED_LIST=C
#else
SUPPORTED_LIST=B,D,E,M,N,O,P
#endif

// #####################################################    UBOOT配置项，请勿随意调整顺序    ##################################################
[SYS_CFG_PARAM]
// #data_width[0 1 2 3 4] 3的时候uboot自动识别2或者4线
// #clk [0-255]
// #mode:
// #	  0 RD_OUTPUT,		 1 cmd 		 1 addr 
// #	  1 RD_I/O,   		 1 cmd 		 x addr
// #	  2 RD_I/O_CONTINUE] no_send_cmd x add
// #port:
// #	  0  优先选A端口  CS:PD3  CLK:PD0  D0:PD1  D1:PD2  D2:PB7  D3:PD5
// #	  1  优先选B端口  CS:PA13 CLK:PD0  D0:PD1  D1:PA14 D2:PA15 D3:PD5
SPI=2_3_0_0;	//data_clk_mode_port;
// OSC=btosc;
// OSC_FREQ=12MHz; #[24MHz 12MHz]
// SYS_CLK=24MHz;	#[48MHz 24MHz]
UTTX=PA05;//uboot串口tx
UTBD=1000000;//uboot串口波特率
// #UTRX=PB01;串口升级[PB00 PB05 PA05]
// #硬件复位可选择port口或LDOIN复位，LDOIN复位：死机进仓就可以复位
#if TCFG_LP_TOUCH_KEY_ENABLE
RESET=LDOIN_04_1;	//LDOIN口_入仓时间_有效电平（入仓时间有00、01、02、04、08、16六个值可选，单位为秒，当入仓时间为00时，则关闭长按复位。)
#else
RESET=PB01_08_0; //IO_TIME_LEVEL (TIME OF PRESSING 00、01、02、04、08、16 value，unit:second，if it configs to 00，this func will be closed.)
#endif

// 0:disable
// 1:PA9 PA10 
// 2:USB
// 3:PB1 PB2
// 4:PB6 PB7

// #sdtap=2;
psram=1;

VLVD=5;//VDDIO_LVD挡位，0: 1.8V   1: 1.9V   2: 2.0V   3: 2.1V   4: 2.2V   5: 2.3V   6: 2.4V   7: 2.5V

//#############################################################################################################################################



// ########flash空间使用配置区域###############################################
// #PDCTNAME:    产品名，对应此代码，用于标识产品，升级时可以选择匹配产品名
// #BOOT_FIRST:  1=代码更新后，提示APP是第一次启动；0=代码更新后，不提示
// #UPVR_CTL：   0：不允许高版本升级低版本   1：允许高版本升级低版本
// #XXXX_ADR:    区域起始地址	AUTO：由工具自动分配起始地址
// #XXXX_LEN:    区域长度		CODE_LEN：代码长度
// #XXXX_OPT:    区域操作属性
// #
// #
// #
// #操作符说明  OPT:
// #	0:  下载代码时擦除指定区域
// #	1:  下载代码时不操作指定区域
// #	2:  下载代码时给指定区域加上保护
// ############################################################################
[RESERVED_CONFIG]
BTIF_ADR=AUTO;
BTIF_LEN=0x1000;
BTIF_OPT=1;

#if CONFIG_APP_BT_ENABLE && !CONFIG_DOUBLE_BANK_ENABLE
EXIF_ADR=AUTO;
EXIF_LEN=0x1000;
EXIF_OPT=1;
#endif

// #WTIF_ADR=BEGIN_END;
// #WTIF_LEN=0x1000;
// #WTIF_OPT=1;

PRCT_ADR=0;
PRCT_LEN=CODE_LEN;
PRCT_OPT=2;

VM_ADR=0;
VM_LEN=4K;
VM_OPT=1;

#if TCFG_AUDIO_ANC_ENABLE
// #ANC配置区，如果不想ANC配置因为代码大小变化而改变位置，从而失效，需要手动指定(flash末尾8K位置)
// #4Mbit:0x7E000 8Mbit:0xFE000 16Mbit:0x1FE000
// #加载了anc_gains.bin或者anc_coeff.bin，则表示使用文件里面的配置
// #ANC增益配置保留区
ANCIF_FILE=anc_gains.bin;
ANCIF_ADR=0xFE000;
ANCIF_LEN=0x80;
ANCIF_OPT=1;
// #ANC系数配置保留区
// #ANCIF1_FILE=anc_coeff.bin;
ANCIF1_ADR=0xFE080;
ANCIF1_LEN=0xF80;
ANCIF1_OPT=1;
#endif

[BURNER_CONFIG]
SIZE=32;


#endif


