#ifndef _COMMPROC_DMS_H_
#define _COMMPROC_DMS_H_

#include "generic/typedef.h"

/*降噪版本定义*/
#define DMS_V100    0xA1
#define DMS_V200    0xA2

/*DMS输出选择*/
typedef enum {
    DMS_OUTPUT_SEL_DEFAULT = 0,	/*默认输出：dms处理后的数据*/
    DMS_OUTPUT_SEL_MASTER,		/*主mic原始数据*/
    DMS_OUTPUT_SEL_SLAVE,		/*副mic原始数据*/
} CVP_OUTPUT_ENUM;

//dms_cfg:
typedef struct {
    u8 mic_again;			//DAC增益,default:3(0~14)
    u8 dac_again;			//MIC增益,default:22(0~31)
    u8 enable_module;       //使能模块
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
    /*aec*/
    int aec_process_maxfrequency;	//default:8000,range[3000:8000]
    int aec_process_minfrequency;	//default:0,range[0:1000]
    int af_length;					//default:128 range[128:256]
    /*nlp*/
    int nlp_process_maxfrequency;	//default:8000,range[3000:8000]
    int nlp_process_minfrequency;	//default:0,range[0:1000]
    float overdrive;				//default:1,range[0:30]
    /*ans*/
    float aggressfactor;			//default:1.25,range[1:2]
    float minsuppress;				//default:0.04,range[0.01:0.1]
    float init_noise_lvl;			//default:-75db,range[-100:-30]
    /*enc*/
    int enc_process_maxfreq;		//default:8000,range[3000:8000]
    int enc_process_minfreq;		//default:0,range[0:1000]
    int sir_maxfreq;				//default:3000,range[1000:8000]
    float mic_distance;				//default:0.015,range[0.035:0.015]
    float target_signal_degradation;//default:1,range[0:1]
    float enc_aggressfactor;		//default:4.f,range[0:4]
    float enc_minsuppress;			//default:0.09f,range[0:0.1]
    /*common*/
    float global_minsuppress;		//default:0.0,range[0.0:0.09]
} _GNU_PACKED_ AEC_DMS_CONFIG;

struct dms_attr {
    u8 ul_eq_en: 1;
    u8 wideband: 1;
    u8 wn_en: 1;
    u8 dly_est : 1;
    u8 aptfilt_only: 1;
    u8 reserved: 3;

    u8 dst_delay;/*延时估计目标延时*/
    u8 EnableBit;
    u8 packet_dump;
    u8 SimplexTail;
    u8 output_sel;/*dms output选择*/
    u16 hw_delay_offset;/*dac hardware delay offset*/
    u16 wn_gain;/*white_noise gain*/
    /*AGC*/
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
    /*AEC*/
    int aec_process_maxfrequency;	//default:8000,range[3000:8000]
    int aec_process_minfrequency;	//default:0,range[0:1000]
    int af_length;					//default:128 range[128:256]
    /*NLP*/
    int nlp_process_maxfrequency;	//default:8000,range[3000:8000]
    int nlp_process_minfrequency;	//default:0,range[0:1000]
    float overdrive;				//default:1,range[0:30]
    /*ANS*/
    float aggressfactor;			//default:1.25,range[1:2]
    float minsuppress;				//default:0.04,range[0.01:0.1]
    float init_noise_lvl;			//default:-75db,range[-100:-30]
    /*ENC*/
    int enc_process_maxfreq;		//default:8000,range[3000:8000]
    int enc_process_minfreq;		//default:0,range[0:1000]
    int sir_maxfreq;				//default:3000,range[1000:8000]
    float mic_distance;				//default:0.015,range[0.035:0.015]
    float target_signal_degradation;//default:1,range[0:1]
    float enc_aggressfactor;		//default:4.f,range[0:4]
    float enc_minsuppress;			//default:0.09f,range[0:0.1]
    /*BCS*/
    int bone_process_maxfreq;
    int bone_process_minfreq;
    float bone_init_noise_lvl;
    int Bone_AEC_Process_MaxFrequency;
    int Bone_AEC_Process_MinFrequency;
    /*common*/
    float global_minsuppress;		//default:0.0,range[0.0:0.09]
    /*data handle*/
    int (*aec_probe)(s16 *dat, u16 len);
    int (*aec_post)(s16 *dat, u16 len);
    int (*aec_update)(u8 EnableBit);
    int (*output_handle)(s16 *dat, u16 len);

    /*flexible enc*/
    int flexible_af_length;          //default:512 range[128、256、512、1024]
    int sir_minfreq;                 //default:100,range[0:8000]
    int SIR_mean_MaxFrequency;       //default:1000,range[0:8000]
    int SIR_mean_MinFrequency;       //default:100,range[0:8000]
    float ENC_CoheFlgMax_gamma;      //default:0.5f,range[0:1]
    float coheXD_thr;                //default:0.5f,range[0:1]
    float Disconverge_ERLE_Thr;      //default:-6.f,range[-20:5]

    /*WN*/
    float wn_detect_time;           //in second
    float wn_detect_time_ratio_thr; //0-1
    float wn_detect_thr;            //0-1
    float wn_minsuppress;           //0-1

    /*Extended-Parameters*/
    u16 ref_sr;
};

s32 aec_dms_init(struct dms_attr *attr);
s32 aec_dms_exit();
s32 aec_dms_fill_in_data(void *dat, u16 len);
int aec_dms_fill_in_ref_data(void *dat, u16 len);
s32 aec_dms_fill_ref_data(void *dat, u16 len);
void aec_dms_toggle(u8 toggle);
int aec_dms_cfg_update(AEC_DMS_CONFIG *cfg);
int aec_dms_reboot(u8 enablebit);

s32 aec_dms_flexible_init(struct dms_attr *attr);
s32 aec_dms_flexible_exit();
s32 aec_dms_flexible_fill_in_data(void *dat, u16 len);
int aec_dms_flexible_fill_in_ref_data(void *dat, u16 len);
s32 aec_dms_flexible_fill_ref_data(void *dat, u16 len);
void aec_dms_flexible_toggle(u8 toggle);
int aec_dms_flexible_cfg_update(AEC_DMS_CONFIG *cfg);
int aec_dms_flexible_reboot(u8 enablebit);

/*
*********************************************************************
*                  			Audio CVP IOCTL
* Description: CVP功能配置
* Arguments  : cmd 		操作命令
*		       value 	操作数
*		       priv 	操作内存地址
* Return	 : 0 成功 其他 失败
* Note(s)    : (1)比如动态开关降噪NS模块:
*				aec_dms_ioctl(CVP_NS_SWITCH,1,NULL);	//降噪关
*				aec_dms_ioctl(CVP_NS_SWITCH,0,NULL);  //降噪开
*********************************************************************
*/
enum {
    CVP_AEC_SWITCH = 1,
    CVP_NLP_SWITCH,
    CVP_NS_SWITCH,
    CVP_AGC_SWITCH,
    CVP_ENC_SWITCH,
};
int aec_dms_ioctl(int cmd, int value, void *priv);

#endif/*_COMMPROC_DMS_H_*/
