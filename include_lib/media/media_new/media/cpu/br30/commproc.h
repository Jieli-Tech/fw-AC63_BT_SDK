#ifndef _COMMPROC_H_
#define _COMMPROC_H_

#include "generic/typedef.h"
//#include "media/audio_dev.h"

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
    u8 agc_en: 1;
    u8 ul_eq_en: 1;
    u8 wideband: 1;
    u8 toggle: 1;
    u8 wn_en: 1;
    u8 dly_est : 1;
    u8 reserved: 2;

    u8 dst_delay;/*延时估计目标延时*/
    u8 EnableBit;
    u8 packet_dump;
    u8 SimplexTail;
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
    int AGC_echo_look_ahead;    	//in ms
    int AGC_echo_hold;          	// in ms
    u16 hw_delay_offset;/*dac hardware delay offset*/
    u16 wn_gain;/*white_noise gain*/
    int SimplexThr;
    float ES_AggressFactor;
    float ES_MinSuppress;
    float ES_Unconverge_OverDrive;
    float ANS_AggressFactor;
    float ANS_MinSuppress;
    float ANS_NoiseLevel;
    int (*aec_probe)(s16 *dat, u16 len);
    int (*aec_post)(s16 *dat, u16 len);
    int (*aec_update)(u8 aec_mode);
    int (*output_handle)(s16 *dat, u16 len);
    u8 output_way;    // 0:dac  1:fm
    u8 ANS_mode;
    u8 fm_tx_start;
    u8 far_noise_gate;//default:10

    float AEC_RefEngThr;
    float AEC_DT_AggressiveFactor;
    //float AES_AggressFactor;
    //float AES_RefEngThr;

};

s32 aec_init(struct aec_s_attr *attr);
s32 aec_exit();
u32 aec_output_read(s16 *buf, u16 npoint);
s32 aec_fill_in_data(void *dat, u16 len);
int aec_fill_in_ref_data(void *dat, u16 len);
s32 aec_fill_ref_data(void *dat, u16 len);
void aec_toggle(u8 toggle);
int aec_cfg_update(AEC_CONFIG *cfg);
int aec_reboot(u8 enablebit);

#endif
