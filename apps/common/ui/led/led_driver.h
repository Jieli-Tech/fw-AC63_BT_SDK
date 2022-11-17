#ifndef	_LED_DRIVER_H_
#define _LED_DRIVER_H_

#if LED_IO_DRVIER

#include "led_api.h"
// #include "config.h"

extern const u8 volt_to_level[];



// 图标显示状态标志 根据收到的信号解析以后改变
// 配合bFlashIcon 来控制是否点亮和是否显示
typedef struct __LED_ICON_STATUS {
    bool log_deng;                  //LOG灯
    bool zuo_zhuan;                 //左转向灯
    bool you_zhuan;                 //又转向灯
    bool da_deng;                   //大灯
    bool lang_deng;                 //廊灯
    bool ready_deng;                //READY图标灯
    u8 dang_wei;                    //档位 指示灯
    u8 dian_liang;                  //电量格子 指示灯
    bool chong_dian;                //充电图标
    bool xun_hang;                  //巡航图标
    bool gu_zhang;                  //故障灯
    bool m_gu_zhang;                 //电机故障
    bool kong_zhi_gu_zhang;         //控制故障
    bool ba_shou_gu_zhang;          //把手故障
    bool km_h_deng;                 //km/h 图标
    bool volt_deng;                 //电压图标
    bool bai_fen_bi_deng;           //百分比图标
    bool li_cheng_deng;             //总里程KM图标
    bool soc_deng;                  //SOC图标
} _LED_ICON_STATUS;

void led_drvier_clear(void);
void led_driver_scan(void);
extern _LED_DRIVER_VAR led_drvier_var;
extern _LED_ICON_STATUS led_status_var;
extern const u8 LED_NUMBER[];
extern const u8 LED_LARGE_LETTER[];
extern const u8 LED_SMALL_LETTER[];


#define LED_A   BIT(0)
#define LED_B	BIT(1)
#define LED_C	BIT(2)
#define LED_D	BIT(3)
#define LED_E	BIT(4)
#define LED_F	BIT(5)
#define LED_G	BIT(6)
#define LED_H	BIT(7)

//for LED0
#define LED_PLAY	LED_A
#define LED_PAUSE	LED_B
#define LED_USB		LED_C
#define LED_SD		LED_D
#define LED_2POINT	LED_E
#define LED_MHZ		LED_F
#define LED_MP3		LED_G
#define LED_FM	    LED_H

#define LED_PORT0	BIT(0)
#define LED_PORT1	BIT(12)
#define LED_PORT2	BIT(11)
#define LED_PORT3	BIT(10)
#define LED_PORT4	BIT(6)
#define LED_PORT5	BIT(5)
#define LED_PORT6	BIT(4)

#define LED_PORT_ALL	(LED_PORT0 | LED_PORT1 | LED_PORT2 |LED_PORT3 |LED_PORT4 |LED_PORT5 |LED_PORT6)



#define     GROUP_CYCLE_LED_NUMBER               3
#define     CYCLE_LED_GROUP                      6
#define     BAT_LEVEL_LED_NUMBER                 3
#define     MAX_BAT_LEVEL                        5
#define     ICON_FLASH_PERIOD                    1000
#define     VABT_48V_LOW_LEVEL                   441
#define     VABT_60V_LOW_LEVEL                   540
#define     VABT_72V_LOW_LEVEL                   652
#define     VABT_48V_HIGH_LEVEL                  480
#define     VABT_60V_HIGH_LEVEL                  600
#define     VABT_72V_HIGH_LEVEL                  720


//状态图标序号  需要按照原理图跟ico_status 数组严格对应
//如果后面有新增加的图标建议加在后面 如果这些图标里面有些需要删掉的可以直接把数组里面对应IO改成0 0
// 图标编号必须和ico_status数组严格一一对应
enum ICON {
    LIU_SHUI_0 = 0,  //流水灯
    LIU_SHUI_1,
    LIU_SHUI_2,
    LIU_SHUI_3,
    LIU_SHUI_4,
    LIU_SHUI_5,
    LIU_SHUI_6,
    LIU_SHUI_7,
    LIU_SHUI_8,
    LIU_SHUI_9,
    LIU_SHUI_10,
    LIU_SHUI_11,
    LIU_SHUI_12,
    LIU_SHUI_13,
    LIU_SHUI_14,
    LIU_SHUI_15,
    LIU_SHUI_16,
    LIU_SHUI_17,

    DIAN_LIANG_0 = 18, //18电量格子 指示灯
    DIAN_LIANG_1,
    DIAN_LIANG_2,
    DIAN_LIANG_3,
    DIAN_LIANG_4,
    DIAN_LIANG_5,
    DIAN_LIANG_6,
    DIAN_LIANG_7,
    DIAN_LIANG_8,
    DIAN_LIANG_9,
    DIAN_LIANG_10, //28
    DIAN_LIANG_11,
    DIAN_LIANG_12,
    DIAN_LIANG_13,
    DIAN_LIANG_14,

    LOG_0,  //33LOG灯
    LOG_1,
    LOG_2,
    LOG_3,
    LOG_4,
    LOG_5,

    ZHUAN_XIANG_ZUO_0,//39左转向灯
    ZHUAN_XIANG_ZUO_1,//40左转向灯
    ZHUAN_XIANG_YOU_0,//41右转向灯
    ZHUAN_XIANG_YOU_1,//42右转向灯
    DA_DENG,           //43大灯
    SHI_LANG_DENG,     //44廊灯
    READY_0,           //45READY图标灯
    READY_1,           //46READY图标灯
    DANG_WEI_1,        //47档位 指示灯
    DANG_WEI_2,        //48档位 指示灯
    DANG_WEI_3,        //49档位 指示灯
    DANG_WEI_4,        //50档位 指示灯
    YOU_HU,        //51充电图标

    XUN_HANG,           //52巡航图标
    GU_ZHANG,           //53故障灯
    M_GU_ZHANG,         //54电机故障
    KONG_ZHI_GU_ZHANG,  //55控制故障
    BA_SHOU_GUZHANG,    //56把手故障

    KM_H,              //57km/h 图标
    FU_TE,               //58电压图标
    BAI_FEN_BI,            //59百分比图标
    KM,                 //60总里程KM图标
    SOC,                //61SOC图标
    XIAO_SHU_DIAN,      //里程小数点显示
};

#endif

#endif	/*	_LED_DRIVER_H_	*/


