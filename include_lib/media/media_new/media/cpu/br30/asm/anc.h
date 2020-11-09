#ifndef _ANC_H_
#define _ANC_H_

#include "generic/typedef.h"
#include "system/task.h"
#include "anc_btspp.h"

#define ANC_COEFF_NUM	(488 + 100)
#define ANC_COEFF_SIZE	(ANC_COEFF_NUM << 2)
enum {
    ANCDB_ERR_CRC = 1,
    ANCDB_ERR_TAG,
};

typedef enum {
    ANC_OFF = 1,		/*关闭模式*/
    ANC_ON,				/*降噪模式*/
    ANC_TRANSPARENCY,	/*通透模式*/
    ANC_TRAIN,			/*训练模式*/
} ANC_mode_t;

typedef enum {
    TRANS_LPF_1K,		//LPF 1K
    TRANS_LPF_2K,		//LPF 2K
    TRANS_LPF_3K,		//LPF 3K
    TRANS_LPF_4K,		//LPF 4K
    TRANS_LPF_5K,		//LPF 5K
    TRANS_LPF_6K,		//LPF 6K
    TRANS_LPF_7K,		//LPF 7K
    TRANS_LPF_8K,		//LPF 8K
} ANC_trans_lpf_t;		//通透模式低通滤波器组

typedef enum {
    TRANS_HPF_1K,		//HPF 1K
    TRANS_HPF_1_5K,		//HPF 1.5K
    TRANS_HPF_2K,		//HPF 2K
    TRANS_HPF_2_5K,		//HPF 2.5K
} ANC_trans_hpf_t;		//通透模式高通滤波器组

typedef enum {
    TRANS_ERR_TIMEOUT = 1,	//训练超时
    TRANS_ERR_MICERR,		//训练中MIC异常
    TRANS_ERR_FORCE_EXIT	//强制退出训练
} ANC_trans_errmsg_t;

#define ANC_MANA_UART  	   0
#define ANC_AUTO_UART 	   1
#define ANC_AUTO_BT_SPP    2

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
    u8 fz_fir_en; //FZ补偿滤波器使能
    u8 anc_fade_en;//ANC淡入淡出使能
    u8 filter_order;//ANC滤波器阶数
    u8 anc_sr;	//ANC采样率
    u16 anc_fade_gain;//ANC淡入淡出增益
    s16 fz_gain;  //FZ补偿增益
    int train_err;//训练结果 0:成功 other:失败
    int anc_gain;//ANC增益
    s32 *fz_coeff;//FZ补偿滤波器表
    s32 *coeff;//降噪模式滤波器表
    s16 *ffwz_nch_coeff;//通透模式低通滤波器表
    s16 *ffwz_lpf_coeff;//通透模式高通滤波器表
    anc_train_para_t train_para;//训练参数结构体
    void (*train_callback)(u8, u8);
    void (*pow_callback)(u8, u8);
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
#define ANC_DB_LEN_MAX		(1024 * 8)
#define ANC_FILT_ORDER_MAX	3
#define ANC_WZ_COEFF_SIZE	(488 * ANC_FILT_ORDER_MAX)
#define ANC_FZ_COEFF_SIZE	100
#else
#define ANC_DB_LEN_MAX		(1024 * 4)
#define ANC_FILT_ORDER_MAX	1
#define ANC_WZ_COEFF_SIZE	(488 * ANC_FILT_ORDER_MAX)
#define ANC_FZ_COEFF_SIZE	100
#endif/*CONFIG_ANC_30C_ENABLE*/
typedef struct {
    int wz_coeff[ANC_WZ_COEFF_SIZE];
    int fz_coeff[ANC_FZ_COEFF_SIZE];
} anc_coeff_t;

/*ANCIF配置区增益gain*/
typedef struct {
    u8 dac_gain;			//dac增益,default:8,range:[0:15]
    u8 ref_mic_gain;		//参考mic增益default:6,range[0:19]
    u8 err_mic_gain;		//误差mic增益default:6,range[0:19]
    u8 reserved;
    int anc_gain;			//降噪模式增益default:-8096,range[-32768:32767]
    int transparency_gain;	//通透模式增益default:7096,range[-32768:32767]
} anc_gain_t;

int audio_anc_train(audio_anc_t *param, u8 en);

/*
 *Description 	: audio_anc_open
 *Arguements  	: param is audio anc param
 *Returns	 	: 0  open success
 *		   		: -EPERM 不支持ANC
 *		   		: -EINVAL 参数错误
 *Notes			:
 */
int audio_anc_open(audio_anc_t *param);

int audio_anc_close();

void audio_anc_gain(int gain);

void audio_anc_fade(int gain, u8 en);

int audio_anc_mode(audio_anc_t *param);

void audio_anc_test(audio_anc_t *param);

void anc_train_para_init(anc_train_para_t *para);

void audio_anc_fz_gain(u8 fz_en, s16 fz_gain);

/*ANC配置id*/
enum {
    ANC_DB_COEFF,	//ANC系数
    ANC_DB_GAIN,	//ANC增益
    ANC_DB_MAX,
};
/*根据配置id获取不同的anc配置*/
int *anc_db_get(u8 id, u32 len);
/*
 *gain:增益配置,没有的话传NULL
 *coeff:系数配置,没有的话传NULL
 */
int anc_db_put(anc_gain_t *gain, anc_coeff_t *coeff);

#endif/*_ANC_H_*/
