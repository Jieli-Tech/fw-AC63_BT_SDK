#ifndef _CHARGE_H_
#define _CHARGE_H_

#include "typedef.h"
#include "device.h"

/*------充满电电压选择 3.962V-4.611V-------*/
#define CHARGE_FULL_V_3962		0
#define CHARGE_FULL_V_4002		1
#define CHARGE_FULL_V_4044		2
#define CHARGE_FULL_V_4086		3
#define CHARGE_FULL_V_4130		4
#define CHARGE_FULL_V_4175		5
#define CHARGE_FULL_V_4222		6
#define CHARGE_FULL_V_4270		7
#define CHARGE_FULL_V_4308		8
#define CHARGE_FULL_V_4349		9
#define CHARGE_FULL_V_4391		10
#define CHARGE_FULL_V_4434		11
#define CHARGE_FULL_V_4472		12
#define CHARGE_FULL_V_4517		13
#define CHARGE_FULL_V_4564		14
#define CHARGE_FULL_V_4611		15
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
	恒流：20-220mA
*/
#define CHARGE_mA_20			0
#define CHARGE_mA_30			1
#define CHARGE_mA_40			2
#define CHARGE_mA_50			3
#define CHARGE_mA_60			4
#define CHARGE_mA_70			5
#define CHARGE_mA_80			6
#define CHARGE_mA_90			7
#define CHARGE_mA_100			8
#define CHARGE_mA_110			9
#define CHARGE_mA_120			10
#define CHARGE_mA_140			11
#define CHARGE_mA_160			12
#define CHARGE_mA_180			13
#define CHARGE_mA_200			14
#define CHARGE_mA_220			15

/*
 	充电口下拉选择
	电阻 50k ~ 200k
*/
#define CHARGE_PULLDOWN_50K     0
#define CHARGE_PULLDOWN_100K    1
#define CHARGE_PULLDOWN_150K    2
#define CHARGE_PULLDOWN_200K    3


#define CHARGE_CCVOL_V			300		//最低充电电流档转向用户设置充电电流档的电压转换点(AC693X无涓流充电，电池电压低时采用最低电流档，电池电压大于设置的值时采用用户设置的充电电流档)

#define DEVICE_EVENT_FROM_CHARGE	(('C' << 24) | ('H' << 16) | ('G' << 8) | '\0')

struct charge_platform_data {
    u8 charge_en;	        //内置充电使能
    u8 charge_poweron_en;	//开机充电使能
    u8 charge_full_V;	    //充满电电压大小
    u8 charge_full_mA;	    //充满电电流大小
    u8 charge_mA;	        //充电电流大小
    u8 ldo5v_pulldown_en;   //下拉使能位
    u8 ldo5v_pulldown_lvl;	//ldo5v的下拉电阻配置项,若充电舱需要更大的负载才能检测到插入时，请将该变量置为对应阻值
    u8 ldo5v_pulldown_keep; //下拉电阻在softoff时是否保持,ldo5v_pulldown_en=1时有效
    u16 ldo5v_off_filter;	//ldo5v拔出过滤值，过滤时间 = (filter*2 + 20)ms,ldoin<0.6V且时间大于过滤时间才认为拔出,对于充满直接从5V掉到0V的充电仓，该值必须设置成0，对于充满由5V先掉到0V之后再升压到xV的充电仓，需要根据实际情况设置该值大小
    u16 ldo5v_on_filter;    //ldo5v>vbat插入过滤值,电压的过滤时间 = (filter*2)ms
    u16 ldo5v_keep_filter;  //1V<ldo5v<vbat维持电压过滤值,过滤时间= (filter*2)ms
    u16 charge_full_filter; //充满过滤值,连续检测充满信号恒为1才认为充满,过滤时间 = (filter*2)ms
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


extern void set_charge_event_flag(u8 flag);
extern void set_charge_online_flag(u8 flag);
extern u8 get_charge_online_flag(void);
extern u8 get_charge_poweron_en(void);
extern void charge_start(void);
extern void charge_close(void);
extern u8 get_charge_mA_config(void);
extern void set_charge_mA(u8 charge_mA);
extern u8 get_ldo5v_pulldown_en(void);
extern u8 get_ldo5v_pulldown_res(void);
extern u8 get_ldo5v_online_hw(void);
extern u8 get_lvcmp_det(void);
extern void charge_check_and_set_pinr(u8 mode);
extern u16 get_charge_full_value(void);
extern void charge_module_stop(void);
extern void charge_module_restart(void);
extern void ldoin_wakeup_isr(void);
extern void charge_wakeup_isr(void);
extern int charge_init(const struct dev_node *node, void *arg);
extern const struct device_operations charge_dev_ops;
extern void charge_set_ldo5v_detect_stop(u8 stop);

#endif    //_CHARGE_H_
