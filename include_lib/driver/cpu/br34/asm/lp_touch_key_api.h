#ifndef _LP_TOUCH_KEY_API_
#define _LP_TOUCH_KEY_API_

#include "typedef.h"

#define CTMU_SHORT_CLICK_DELAY_TIME 	400 	//单击事件后等待下一次单击时间(ms)
#define CTMU_HOLD_CLICK_DELAY_TIME 		200 	//long事件产生后, 发hold事件间隔(ms)
#define CTMU_LONG_KEY_DELAY_TIME 		1000 	//从按下到产生long事件的时间(ms)

//长按开机时间:
#define CFG_M2P_CTMU_CH0_SOFTOFF_LONG_TIME 			5	//(n + 6) * 100(ms)

//======== CH0是否发送单击事件
#define CFG_EAROUT_NOTIFY_CH0_ONE_CLICK_EVNET 		1  //CH0是否发送单击事件

//======== 在耳外选择发送触摸通道类型
#define CFG_EAROUT_NOTIFY_CH0_ALL_EVENT				0  //在耳外发CH0所有消息
#define CFG_EAROUT_NO_NOTIFY_CH0_ALL_EVENT 			1  //在耳外不发CH0所有消息
#define CFG_EAROUT_NO_NOTIFY_CH0_CLICK_EVENT 		2  //在耳外只发CH0 LONG, HOLD, UP事件

#define CFG_EAROUT_NOTIFY_CH0_EVENT_SEL 			CFG_EAROUT_NOTIFY_CH0_ALL_EVENT

//触摸按键长按复位时间配置
#define CTMU_RESET_TIME_CONFIG			8000	//长按复位时间(ms), 配置为0关闭


struct ctmu_ch_cfg {
    u8 enable;
    u8 key_value;
    u8 port;
    u8 sensitivity;
};

struct lp_touch_key_platform_data {
    struct ctmu_ch_cfg ch0;
    struct ctmu_ch_cfg ch1;
};


typedef struct __LP_TOUCH_KEY_CONFIG {
    u8 cfg_en; //配置项有效标志0: 配置项无效, 使用软件默认配置, 1: 配置项有效, 使用配置工具配置值
    u8 touch_key_sensity_class;
    u8 earin_key_sensity_class;
} _GNU_PACKED_ LP_TOUCH_KEY_CONFIG;


/* =========== ctmu API ============= */
//ctmu 初始化
void lp_touch_key_init(const struct lp_touch_key_platform_data *config);
u8 lp_touch_key_power_on_status();


#endif /* #ifndef _LP_TOUCH_KEY_API_ */
