#ifndef	_LEDC_H_
#define _LEDC_H_
#include "typedef.h"


typedef enum {
    t_21ns,
    t_42ns,
    t_63ns,
    t_125ns,
    t_250ns,
    t_500ns,
    t_1us,
    t_2us,
    t_4us,
} t_unit_enum;

struct ledc_platform_data {
    u8 index;           //控制器号
    u8 port;            //输出引脚
    u8 idle_level;      //当前帧的空闲电平，0：低电平， 1：高电平
    u8 out_inv;         //起始电平，0：高电平开始， 1：低电平开始
    u8 bit_inv;         //取数据时高低位镜像，0：不镜像，1：8位镜像，2:16位镜像，3:32位镜像
    t_unit_enum t_unit; //时间单位
    u8 t1h_cnt;         //1码的高电平时间 = t1h_cnt * t_unit;
    u8 t1l_cnt;         //1码的低电平时间 = t1l_cnt * t_unit;
    u8 t0h_cnt;         //0码的高电平时间 = t0h_cnt * t_unit;
    u8 t0l_cnt;         //0码的低电平时间 = t0l_cnt * t_unit;
    u32 t_rest_cnt;     //复位信号时间 = t_rest_cnt * t_unit;
    void (*cbfun)(void);//中断回调函数
};


#define LEDC_PLATFORM_DATA_BEGIN(data) \
	const struct ledc_platform_data data = {

#define LEDC_PLATFORM_DATA_END()  \
};

#endif

