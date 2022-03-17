#ifndef _COMMPROC_H_
#define _COMMPROC_H_

#include "generic/typedef.h"

/*降噪版本定义*/
#define ANS_V100	0xA1
#define ANS_V200	0xA2

//aec_cfg:
typedef struct __AEC_CONFIG {
    u8 mic_again;			//DAC增益,default:3(0~14)
    u8 dac_again;			//MIC增益,default:22(0~31)
    u8 aec_mode;        	//AEC模式,default:advance(diable(0), reduce(1), advance(2))
    u8 ul_eq_en;        	//上行EQ使能,default:enable(disable(0), enable(1))
    /*AGC*/
    float ndt_fade_in;  	//单端讲话淡入步进default: 1.3f(0.1 ~ 5 dB)
    float ndt_fade_out;  	//单端讲话淡出步进default: 0.7f(0.1 ~ 5 dB)
    float dt_fade_in;  		//双端讲话淡入步进default: 1.3f(0.1 ~ 5 dB)
    float dt_fade_out;  	//双端讲话淡出步进default: 0.7f(0.1 ~ 5 dB)
    float ndt_max_gain;   	//单端讲话放大上限,default: 12.f(0 ~ 24 dB)
    float ndt_min_gain;   	//单端讲话放大下限,default: 0.f(-20 ~ 24 dB)
    float ndt_speech_thr;   //单端讲话放大阈值,default: -50.f(-70 ~ -40 dB)
    float dt_max_gain;   	//双端讲话放大上限,default: 12.f(0 ~ 24 dB)
    float dt_min_gain;   	//双端讲话放大下限,default: 0.f(-20 ~ 24 dB)
    float dt_speech_thr;    //双端讲话放大阈值,default: -40.f(-70 ~ -40 dB)
    float echo_present_thr; //单端双端讲话阈值,default:-70.f(-70 ~ -40 dB)
    /*AEC*/
    float aec_dt_aggress;   //原音回音追踪等级, default: 1.0f(1 ~ 5)
    float aec_refengthr;    //进入回音消除参考值, default: -70.0f(-90 ~ -60 dB)
    /*ES*/
    float es_aggress_factor;//回音前级动态压制,越小越强,default: -3.0f(-1 ~ -5)
    float es_min_suppress;	//回音后级静态压制,越大越强,default: 4.f(0 ~ 10)
    /*ANS*/
    float ans_aggress;    	//噪声前级动态压制,越大越强default: 1.25f(1 ~ 2)
    float ans_suppress;    	//噪声后级静态压制,越小越强default: 0.04f(0 ~ 1)
} _GNU_PACKED_ AEC_CONFIG;

struct aec_s_attr {
    u8 agc_en: 1;				//AGC使能配置
    u8 ul_eq_en: 1;				//上行EQ使能配置
    u8 wideband: 1;				//宽带/窄带配置
    u8 toggle: 1;				//bypass配置
    u8 wn_en: 1;				//舒适噪声使能配置
    u8 dly_est : 1;				//延时估计使能
    u8 reserved: 2;				//保留位

    u8 output_way;				//输出方式配置,0:dac  1:fm_tx
    u8 ANS_mode;				//降噪等级配置
    u8 fm_tx_start;				//fm发射同步标志
    u8 far_noise_gate;			//参考数据噪声门限
    u8 dst_delay;				//延时估计目标延时
    u8 EnableBit;				//使能模块
    u8 packet_dump;				//数据丢掉配置
    u8 SimplexTail;				//单工模式连续清0帧数
    u8 aec_tail_length;			//AEC复杂等级:2~10,default：5
    u16 hw_delay_offset;		//硬件延时补偿
    u16 wn_gain;				//舒适噪声增益
    int SimplexThr;				//单工模式单工阈值
    /*AGC Parameters*/
    int AGC_echo_look_ahead;    	//in ms
    int AGC_echo_hold;          	//in ms
    float AGC_NDT_fade_in_step; 	//in dB
    float AGC_NDT_fade_out_step; 	//in dB
    float AGC_NDT_max_gain;     	//in dB
    float AGC_NDT_min_gain;     	//in dB
    float AGC_NDT_speech_thr;   	//in dB
    float AGC_DT_fade_in_step;  	//in dB
    float AGC_DT_fade_out_step; 	//in dB
    float AGC_DT_max_gain;      	//in dB
    float AGC_DT_min_gain;      	//in dB
    float AGC_DT_speech_thr;    	//in dB
    float AGC_echo_present_thr; 	//In dB
    /*AEC Parameters*/
    float AEC_RefEngThr;
    float AEC_DT_AggressiveFactor;
    /*NLP Parameters*/
    float ES_AggressFactor;
    float ES_MinSuppress;
    float ES_Unconverge_OverDrive;
    /*ANS Parameters*/
    float ANS_AggressFactor;
    float ANS_MinSuppress;
    float ANS_NoiseLevel;
    /*Callback Functions*/
    int (*aec_probe)(s16 *dat, u16 len);	//算法预处理回调函数
    int (*aec_post)(s16 *dat, u16 len);		//算法后处理回调函数
    int (*aec_update)(u8 aec_mode);			//动态参数调节回调函数
    int (*output_handle)(s16 *dat, u16 len);//输出回调函数

    /*Extended-Parameters*/
    u16 ref_sr;

    /*DNS Parameters*/
    float DNS_GainFloor;
    float DNS_OverDrive;
    float DNS_Loudness;
};

/*
*********************************************************************
*                  			AEC Init
* Description: AEC模块初始化
* Arguments  : attr	AEC模块参数
* Return	 : 0 成功 其他 失败
* Note(s)    : None.
*********************************************************************
*/
s32 aec_init(struct aec_s_attr *attr);

/*
*********************************************************************
*                  				AEC Exit
* Description: AEC退出
* Arguments  : None.
* Return	 : 0 成功 其他 失败
* Note(s)    : None.
*********************************************************************
*/
s32 aec_exit(void);

/*
*********************************************************************
*                  			AEC Input
* Description: AEC源数据输入
* Arguments  : dat	参考数据
*			   len	参考数据长度
* Return	 : 0 成功 其他 失败
* Note(s)    : None.
*********************************************************************
*/
s32 aec_fill_in_data(void *dat, u16 len);
int aec_fill_in_ref_data(void *dat, u16 len);

/*
*********************************************************************
*                  			AEC Reference
* Description: AEC参考数据输入
* Arguments  : dat	参考数据
*			   len	参考数据长度
* Return	 : 0 成功 其他 失败
* Note(s)    : None.
*********************************************************************
*/
s32 aec_fill_ref_data(void *dat, u16 len);

/*
*********************************************************************
*                  			AEC Toggle
* Description: AEC bypass设置
* Arguments  : toggle	bypass开关
* Return	 : None.
* Note(s)    : 该接口用于切换算法开关，1为正常流程，0为bypass，数据
* 			   不经算法处理
*********************************************************************
*/
void aec_toggle(u8 toggle);

/*
*********************************************************************
*                  			AEC CFG Updata
* Description: AEC模块配置动态更新
* Arguments  : cfg AEC模块参数
* Return	 : 0 成功 其他 失败
* Note(s)    : 该接口用于使用过程，动态修改模块参数
*********************************************************************
*/
int aec_cfg_update(AEC_CONFIG *cfg);

/*
*********************************************************************
*                  			AEC Reboot
* Description: AEC模块重新启动
* Arguments  : enablebit 重新启动使能的模块
* Return	 : 0 成功 其他 失败
* Note(s)    : 该接口用于重新启动AEC模块
*********************************************************************
*/
int aec_reboot(u8 enablebit);

/*
*********************************************************************
*                  			Audio CVP IOCTL
* Description: CVP功能配置
* Arguments  : cmd 		操作命令
*		       value 	操作数
*		       priv 	操作内存地址
* Return	 : 0 成功 其他 失败
* Note(s)    : (1)比如动态开关降噪NS模块:
*				audio_cvp_ioctl(CVP_NS_SWITCH,1,NULL);	//降噪关
*				audio_cvp_ioctl(CVP_NS_SWITCH,0,NULL);  //降噪开
*********************************************************************
*/
enum {
    CVP_AEC_SWITCH = 1,
    CVP_NLP_SWITCH,
    CVP_NS_SWITCH,
    CVP_AGC_SWITCH,
};
int aec_ioctl(int cmd, int value, void *priv);

int sms_tde_init(struct aec_s_attr *attr);
int sms_tde_exit();
int sms_tde_fill_in_data(u8 *dat, u16 len);
int sms_tde_fill_ref_data(u8 *dat, u16 len);
void sms_tde_toggle(u8 toggle);

#endif/*_COMMPROC_H_*/
