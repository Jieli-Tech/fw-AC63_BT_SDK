#ifndef _CTMU_DRV_H_
#define _CTMU_DRV_H_

#include "typedef.h"

#define CTMU_KEY_CH_MAX		3

struct ctmu_key_port {
    u16 press_delta;    //按下判决的阈值
    u8 port; 			//触摸按键IO
    u8 key_value; 		//按键返回值
};

struct ctmu_touch_key_platform_data {
    u8 num; 	        //触摸按键个数
    const struct ctmu_key_port *port_list;
};

/* =========== ctmu API ============= */
//ctmu 初始化
int ctmu_init(void *_data);

//获取plcnt按键状态
u8 get_ctmu_value(void);


#endif /* #ifndef _CTMU_DRV_H_ */

