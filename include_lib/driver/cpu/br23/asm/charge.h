#ifndef _CHARGE_H_
#define _CHARGE_H_

#include "typedef.h"

/*------充满电电压选择 3.869V-4.567V-------*/
#define CHARGE_FULL_V_3869		0
#define CHARGE_FULL_V_3907		1
#define CHARGE_FULL_V_3946		2
#define CHARGE_FULL_V_3985		3
#define CHARGE_FULL_V_4026		4
#define CHARGE_FULL_V_4068		5
#define CHARGE_FULL_V_4122		6
#define CHARGE_FULL_V_4157		7
#define CHARGE_FULL_V_4202		8
#define CHARGE_FULL_V_4249		9
#define CHARGE_FULL_V_4295		10
#define CHARGE_FULL_V_4350		11
#define CHARGE_FULL_V_4398		12
#define CHARGE_FULL_V_4452		13
#define CHARGE_FULL_V_4509		14
#define CHARGE_FULL_V_4567		15
#define CHARGE_FULL_V_MAX       16
/*****************************************/

/*充满电电流选择 2mA-30mA*/
#define CHARGE_FULL_mA_2		0
#define CHARGE_FULL_mA_5		1
#define CHARGE_FULL_mA_7	 	2
#define CHARGE_FULL_mA_10		3
#define CHARGE_FULL_mA_15		4
#define CHARGE_FULL_mA_20		5
#define CHARGE_FULL_mA_25		6
#define CHARGE_FULL_mA_30		7

/*
 	充电电流选择
	恒流：20-320mA
*/
#define CHARGE_mA_20			0
#define CHARGE_mA_40			1
#define CHARGE_mA_60			2
#define CHARGE_mA_80			3
#define CHARGE_mA_100			4
#define CHARGE_mA_120			5
#define CHARGE_mA_140			6
#define CHARGE_mA_160			7
#define CHARGE_mA_180			8
#define CHARGE_mA_200			9
#define CHARGE_mA_220			10
#define CHARGE_mA_240			11
#define CHARGE_mA_260			12
#define CHARGE_mA_280			13
#define CHARGE_mA_300			14
#define CHARGE_mA_320			15

#define CHARGE_CCVOL_V			300		//最低充电电流档转向用户设置充电电流档的电压转换点(AC693X无涓流充电，电池电压低时采用最低电流档，电池电压大于设置的值时采用用户设置的充电电流档)

#define DEVICE_EVENT_FROM_CHARGE	(('C' << 24) | ('H' << 16) | ('G' << 8) | '\0')


struct charge_platform_data {
    u8 charge_en;	//内置充电使能
    u8 charge_poweron_en;	//开机充电使能
    u8 charge_full_V;	//充满电电压大小
    u8 charge_full_mA;	//充满电电流大小
    u8 charge_mA;	//充电电流大小
    u8 ldo5v_pulldown_en;	//ldo5v的100K下拉电阻使能,若充电舱需要更大的负载才能检测到插入时，请将该变量置1
    u16 ldo5v_off_filter;	//ldo5v拔出过滤值，过滤时间 = (filter*2 + 20)ms,ldoin<0.6V且时间大于过滤时间才认为拔出,对于充满直接从5V掉到0V的充电仓，该值必须设置成0，对于充满由5V先掉到0V之后再升压到xV的充电仓，需要根据实际情况设置该值大小
};

#define CHARGE_PLATFORM_DATA_BEGIN(data) \
		struct charge_platform_data data  = {

#define CHARGE_PLATFORM_DATA_END()  \
};


enum {
    CHARGE_FULL_33V = 0,	//充满标记位
    TERMA_33V,				//模拟测试信号
    VBGOK_33V,				//模拟测试信号
    CICHARGE_33V,			//涓流转恒流信号
};

enum {
    CHARGE_EVENT_CHARGE_START,
    CHARGE_EVENT_CHARGE_CLOSE,
    CHARGE_EVENT_CHARGE_FULL,
    CHARGE_EVENT_LDO5V_KEEP,
    CHARGE_EVENT_LDO5V_IN,
    CHARGE_EVENT_LDO5V_OFF,
    CHARGE_EVENT_USB_CHARGE_IN,
    CHARGE_EVENT_USB_CHARGE_OFF,
};


extern void set_charge_online_flag(u8 flag);
extern u8 get_charge_online_flag(void);
extern u8 get_charge_poweron_en(void);
extern void charge_start(void);
extern void charge_close(void);
extern u8 get_charge_mA_config(void);
extern void set_charge_mA(u8 charge_mA);
extern u8 get_ldo5v_pulldown_en(void);
extern u8 get_ldo5v_online_hw(void);
extern u8 get_lvcmp_det(void);
extern void charge_check_and_set_pinr(u8 mode);
extern u16 get_charge_full_value(void);
int charge_api_init(void *arg);
extern const struct device_operations charge_dev_ops;

#endif    //_CHARGE_H_
