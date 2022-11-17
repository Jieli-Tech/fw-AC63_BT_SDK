#ifndef _ANC_H_
#define _ANC_H_

#include "generic/typedef.h"
#include "system/task.h"
#include "media/audio_def.h"

enum {
    ANCDB_ERR_CRC = 1,
    ANCDB_ERR_TAG,
    ANCDB_ERR_PARAM,
    ANCDB_ERR_STATE,
};

typedef enum {
    ANC_OFF = 1,		/*关闭模式*/
    ANC_ON,				/*降噪模式*/
    ANC_TRANSPARENCY,	/*通透模式*/
    ANC_BYPASS,			/*训练模式*/
    ANC_TRAIN,			/*训练模式*/
    ANC_TRANS_TRAIN,	/*通透训练模式*/
} ANC_mode_t;

/*ANC状态*/
enum {
    ANC_STA_CLOSE = 0,
    ANC_STA_INIT,
    ANC_STA_OPEN,
};

typedef enum {
    TRANS_FILTER_CLOSE = 0,
    TRANS_LPF_1K,		//LPF 1K
    TRANS_LPF_2K,		//LPF 2K
    TRANS_LPF_3K,		//LPF 3K
    TRANS_LPF_4K,		//LPF 4K

    TRANS_LPF_5K,		//LPF 5K
    TRANS_LPF_6K,		//LPF 6K
    TRANS_LPF_7K,		//LPF 7K
    TRANS_LPF_8K,		//LPF 8K
    TRANS_HPF_0_2K,		//HPF 0.2K

    TRANS_HPF_0_2_5K,	//HPF 0.25K
    TRANS_HPF_0_3K,		//HPF 0.3K
    TRANS_HPF_0_3_5K,	//HPF 0.35K
    TRANS_HPF_0_4K,		//HPF 0.4K
    TRANS_HPF_0_5K,		//HPF 0.5K

    TRANS_HPF_0_6K,		//HPF 0.6K
    TRANS_HPF_0_7K,		//HPF 0.7K
    TRANS_HPF_0_8K,		//HPF 0.8K
    TRANS_HPF_1K,		//HPF 1K
    TRANS_HPF_1_5K,		//HPF 1.5K

    TRANS_HPF_1_8K,		//HPF 1.8K
    TRANS_HPF_2K,		//HPF 2K
    TRANS_HPF_2_5K,		//HPF 2.5K
    TRANS_NOTCH_1K,		//NOTCH 1K
    TRANS_NOTCH_4_5K,	//NOTCH 4.5K

} ANC_trans_filter_t;		//通透轻量滤波器组

typedef enum {
    TRANS_ERR_TIMEOUT = 1,	//训练超时
    TRANS_ERR_MICERR,		//训练中MIC异常
    TRANS_ERR_FORCE_EXIT	//强制退出训练
} ANC_trans_errmsg_t;

#define A_MIC0					BIT(0)		//模拟MIC0	PA1 PA2
#define A_MIC1        			BIT(1)		//模拟MIC1	PB7 PB8
#define D_MIC0       			BIT(2)		//数字MIC0
#define D_MIC1       		 	BIT(3)		//数字MIC1

#define ANC_MANA_UART  	   0
#define ANC_AUTO_UART 	   1
#define ANC_AUTO_BT_SPP    2

typedef struct {
    int fb0sz_dly[10];
    // int sz_dly[10];
    // int fz_dly[10];
    // int ffwz_dly[10];
    int fflms_dly[10];
    // int fb1sz_dly[10];
    // int fbwz_dly[10];
    // int fblms_dly[10];
} audio_anc_dly_t;

typedef struct {
    u8  mode;			//当前训练步骤
    u8  train_busy;		//训练繁忙位 0-空闲 1-训练中 2-训练结束测试中
    u8	train_step;		//训练系数
    u8  ret_step;		//自适应训练系数值
    u8  auto_ts_en;		//自动步进微调使能位
    u8  noise_level;	//静音训练白噪等级，等级越高，白噪声音越小。与DAC 模拟增益设置成反比
    /*
         DAC GAIN <10    noise_level 1;
     10< DAC GAIN <12    noise_level 2;
         DAC GAIN >12    noise_level 3;
     */
    u8  enablebit; 			//训练模式使能标志位 前馈|混合馈|通透
    u8 tool_time_flag;		//工具修改时间标志
    u8	mic_dma_export_en;	//从ANC DAM拿数，会消耗4*4K
    u8 	only_mute_train_en;	//只跑静音训练保持sz,fz系数
    u16 timerout_id;		//训练超时定时器ID
    s16 fb0_gain;
    s16 fb0sz_dly_en;
    u16 fb0sz_dly_num;
    int sz_gain;			//静音训练sz 增益
    u32 sz_lower_thr;		//误差MIC下限能量阈值
    u32 fz_lower_thr;   	//参考MIC下限能量阈值
    u32 sz_noise_thr;		//误差MIC底噪阈值
    u32 fz_noise_thr;   	//参考MIC底噪阈值
    u32 sz_adaptive_thr;	//误差MIC自适应收敛阈值
    u32 fz_adaptive_thr;	//参考MIC自适应收敛阈值
    u32 wz_train_thr;		//噪声训练阈值
    u32 non_adaptive_time;  //非自适应收敛时间
    u32 sz_adaptive_time;   //误差MIC自适应收敛时间
    u32 fz_adaptive_time;   //参考MIC自适应收敛时间
    u32 wz_train_time; 		//噪声训练时间
} anc_train_para_t;

typedef struct {
    u8 mic_errmsg;
    u8 status;
    u32 dat[4];  //ff_num/ff_dat/fb_num/fb_dat
    u32 pow;
    u32 temp_pow;
    u32 mute_pow;
} anc_ack_msg_t;
typedef struct {
    u8 start;//ANC状态
    u8 mode; //ANC模式
    u8 dac_gain_l;//dac左声道增益
    u8 dac_gain_r;//dac右声道增益
    u8 ref_mic_gain;//参考mic
    u8 err_mic_gain;//误差mic
    u8 train_way; //训练方式
    u8 trans_hpf_en; //通透模式高通使能
    u8 trans_lpf_en; //通透模式低通使能
    u8 trans_hpf_sel;
    u8 trans_lpf_sel;
    u8 fz_fir_en; //FZ补偿滤波器使能
    u8 anc_fade_en;//ANC淡入淡出使能
    u8 mic_swap_en;//交换MIC0 与MIC1 的数据
    u8 filter_order;
    u8 mic_type;//ANC 硬件MIC类型
    u8 tool_enablebit;		//工具打开的ANC功能
    u8 trans_advance_mode;	//高级通透模式使能
    u8 trans_order;			//通透模式滤波器阶数
    u8 trans_howling_flag;	//啸叫抑制算法标志
    u8 online_busy;			//在线调试繁忙标志位
    u16 anc_fade_gain;//ANC淡入淡出增益
    s16 fz_gain;  //FZ补偿增益
    s16 trans_fz_gain;  //通透FZ补偿增益
    int train_err;//训练结果 0:成功 other:失败
    int anc_gain;//ANCFF增益
    int anc_fbgain;//ANCFB增益
    u32 sample_rate;//ANC采样率
    u32 trans_sample_rate;	//通透模式采样率
    int coeff_size;//ANC 滤波器总长度
    int wz_coeff_size;//ANC 滤波器长度
    s32 *sz_coeff;//FZ补偿滤波器表
    s32 *fz_coeff;//FZ补偿滤波器表
    s32 *trans_fz_coeff;//FZ补偿滤波器表
    s32 *coeff;//降噪模式滤波器表
    s32 *trans_coeff;//通透模式训练滤波器表
    u32 trans_aim_pow;
    s16 *ffwz_lpf_coeff;//通透模式低通滤波器表
    s16 *ffwz_hpf_coeff;//通透模式高通滤波器表
#ifdef CONFIG_ANC_30C_ENABLE
    audio_anc_dly_t anc_dly;
#endif
    anc_train_para_t train_para;//训练参数结构体
    void (*train_callback)(u8, u8);
    void (*pow_callback)(anc_ack_msg_t *msg_t, u8 setp);
} audio_anc_t;

#define ANC_DB_HEAD_LEN		20/*ANC配置区数据头长度*/
#define ANCIF_GAIN_TAG_01	"ANCGAIN01"
#define ANCIF_COEFF_TAG_01	"ANCCOEF01"
#define ANCIF_GAIN_TAG		ANCIF_GAIN_TAG_01	//当前增益配置版本
#define ANCIF_COEFF_TAG		ANCIF_COEFF_TAG_01	//当前系数配置版本
#define ANCIF_HEADER_LEN	10
typedef struct {
    u32 total_len;	//4 后面所有数据加起来长度
    u16 group_crc;	//6 group_type开始做的CRC，更新数据后需要对应更新CRC
    u16 group_type;	//8
    u16 group_len;  //10 header开始到末尾的长度
    char header[ANCIF_HEADER_LEN];//20
    int coeff[0];
} anc_db_head_t;

/*ANCIF配置区滤波器系数coeff*/
#ifdef CONFIG_ANC_30C_ENABLE
#define ANC_DB_LEN_MAX		(1024 * 16)
#define ANC_FILT_ORDER_MAX			2
#define ANC_WZ_COEFF_SIZE	(488 * ANC_FILT_ORDER_MAX)
#define ANC_SZ_COEFF_SIZE	100
#define ANC_FZ_COEFF_SIZE	100
typedef struct {
    int sz_coeff[ANC_SZ_COEFF_SIZE];
    int fz_coeff[ANC_FZ_COEFF_SIZE];
    int wz_coeff[];		//这里表示wz_coeff+trans_fz_coeff+trans_coeff
} anc_coeff_t;

#else		//30B
#define ANC_DB_LEN_MAX		(1024 * 8)
#define ANC_FILT_ORDER_MAX	1
#define ANC_WZ_COEFF_SIZE	(488 * ANC_FILT_ORDER_MAX)
#define ANC_FZ_COEFF_SIZE	100
typedef struct {
    int wz_coeff[ANC_WZ_COEFF_SIZE];
    int fz_coeff[ANC_FZ_COEFF_SIZE];
    int trans_coeff[];		//trans_coeff
} anc_coeff_t;

#endif/*CONFIG_ANC_30C_ENABLE*/

/*ANCIF配置区增益gain*/
typedef struct {
    u8 dac_gain;			//dac增益,default:8,range:[0:15]
    u8 ref_mic_gain;		//参考mic增益default:6,range[0:19]
    u8 err_mic_gain;		//误差mic增益default:6,range[0:19]
    u8 order;				//滤波器阶数default:1,range[1:4]
    u8 trans_hpf_sel;		//通透轻量滤波器1default:TRANS_HPF_1K,range[0:19]
    u8 trans_lpf_sel;		//通透轻量滤波器2default:TRANS_LPF_4K,range[0:19]
    u8 trans_order;			//通透模式滤波器阶数default:1,range[1:4]
    u8 trans_advance_mode;	//高级通透模式使能default:0,range[0-3]
    int anc_gain;			//降噪模式增益default:-8096,range[-32768:32767]
    int transparency_gain;	//通透模式增益default:7096,range[-32768:32767]
    int fb_gain; 			//降噪模式增益default:-8096,range[-32768:32767]
    u32 sample_rate;		//采样率default:48000,range[12000,24000,48000,96000]
    u32 trans_sample_rate;	//通透模式采样率default:48000range[12000,24000,48000,96000]
} anc_gain_t;

int audio_anc_train(audio_anc_t *param, u8 en);

/*
 *Description 	: audio_anc_run
 *Arguements  	: param is audio anc param
 *Returns	 	: 0  open success
 *		   		: -EPERM 不支持ANC
 *		   		: -EINVAL 参数错误
 *Notes			:
 */
int audio_anc_run(audio_anc_t *param, u8 anc_state);

int audio_anc_close();

void audio_anc_gain(int ffwz_gain, int fbwz_gain);

void audio_anc_fade(int gain, u8 en);

void audio_anc_test(audio_anc_t *param);

void anc_train_para_init(anc_train_para_t *para);

void audio_anc_fz_gain(u8 fz_en, s16 fz_gain);

void anc_coeff_online_update(audio_anc_t *param, u8 reset_en);

int anc_coeff_size_count(audio_anc_t *param);

/*ANC配置id*/
enum {
    ANC_DB_COEFF,	//ANC系数
    ANC_DB_GAIN,	//ANC增益
    ANC_DB_MAX,
};

typedef struct {
    u8 state;		//ANC配置区状态
    u16 total_size;	//ANC配置区总大小
} anc_db_t;

/*anc配置区初始化*/
int anc_db_init(anc_db_t *db);
/*根据配置id获取不同的anc配置*/
int *anc_db_get(u8 id, u32 len);
/*
 *gain:增益配置,没有的话传NULL
 *coeff:系数配置,没有的话传NULL
 */
int anc_db_put(audio_anc_t *param, anc_gain_t *gain, anc_coeff_t *coeff);

#endif/*_ANC_H_*/
