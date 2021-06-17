#ifndef _PLCNT_DRV_H_
#define _PLCNT_DRV_H_

//计数参考时钟选择, 频率越高, 精度越高
enum touch_key_clk {
    TOUCH_KEY_OSC_CLK = 0,
    TOUCH_KEY_MUX_IN_CLK,  //外部输入, ,一般不用, 保留
    TOUCH_KEY_PLL_192M_CLK,
    TOUCH_KEY_PLL_240M_CLK,
};

struct touch_key_port {
    u8 port; 			//触摸按键IO
    u8 key_value; 		//按键返回值
};

struct touch_key_platform_data {
    u8 num; 	//触摸按键个数;
    u8 clock; 	//触摸按键选择时钟;
    u8 change_gain; 	//变化放大倍数
    s16 press_cfg;  	//按下判决门限
    s16 release_cfg0; 	//释放判决门限0
    s16 release_cfg1; 	//释放判决门限1
    const struct touch_key_port *port_list; //触摸按键列表
};


#define TOUCH_KEY_CH_MAX		3

typedef struct _CTM_KEY_VAR {
    s32 touch_release_buf[TOUCH_KEY_CH_MAX]; 		//按键释放值滤波器buffer
    u16 touch_cnt_buf[TOUCH_KEY_CH_MAX];			//按键计数值滤波器buffer
    s16 FLT1CFG1;					//滤波器1配置参数1
    s16 FLT1CFG2;					//滤波器1配置参数2, 等于(-RELEASECFG0)<<FLT1CFG0
    s16 PRESSCFG;					//按下判决门限
    s16 RELEASECFG0;				//释放判决门限0
    s16 RELEASECFG1;				//释放判决门限1
    s8  FLT0CFG;					//滤波器0配置参数(0/1/2/3)
    s8  FLT1CFG0;					//滤波器1配置参数0
    u16 touch_key_state;			//按键状态标志，随时可能被中断改写，按键处理程序需要将此标志复制出来再行处理
    u8  touch_init_cnt[TOUCH_KEY_CH_MAX];				//初始化计数器，非0时进行初始化
} sCTM_KEY_VAR;

/* =========== pclcnt API ============= */
//plcnt 初始化
int plcnt_init(void *_data);

//获取plcnt按键状态
u8 get_plcnt_value(void);


#endif  /* _PLCNT_DRV_H_ */


