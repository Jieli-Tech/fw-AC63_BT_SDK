#ifndef __TM162X_H__
#define __TM162X_H__

#if LED_162X_EN

#define BYTE_NUM      (16)
#define FLASH_TIME    (2)
#define DELAY_TIME_NUM  3


#define  ICON_CTR(x,y)    icon_ctr(x,y)

typedef enum {
    LED_188_OFF = 0,
    LED_188_NORMAL,
    LED_188_LOW,
} LED_1688_CTR;



#define LIGHT_LEVEL_1 (0x8f)
#define LIGHT_LEVEL_2 (0x8a)
// #define LIGHT_LEVEL_3 (0x87)

typedef enum {
    NUMBER,
    STRING,
    NOTHING
} SHOW_TYPE;

typedef struct _led1629_VAR {
    signed char  bCoordinateX;
    u8  bFlashChar;
    u8  bFlashIcon;
    u8 bShowBuff[BYTE_NUM];     //显示内容buff
    u8 bshow_driver_buff[BYTE_NUM];   //

} LED1629_VAR;

extern LED1629_VAR led1629_var;
extern bool is_display_hold;
extern u8 timeout_cnt_main;
extern u8 disp_mode;
extern u8 sys_going_to_poweroff;

void tm162xin(u8 p);
void led1629_string(u8 *led_dis);
u8 led1629_char(u8 chardata);
void led1629_dispBuff_driverBuff(void);
void led1629_scan(void *p);
void led1629_init(void);
void led1629_ctr(LED_1688_CTR ctr);
void led_driver_init(void);

#define     LED_FLASH_TIME                       100
#define     GROUP_CYCLE_LED_NUMBER               3
#define     CYCLE_LED_GROUP                      6
#define     BAT_LEVEL_LED_NUMBER                 3
#define     MAX_BAT_LEVEL                        5
extern const u8 volt_to_level[MAX_BAT_LEVEL];


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
    YOU_HU,        	   //51油壶图标

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
    XIAO_SHU_DIAN,      //里程小数点
};

#endif
#endif

