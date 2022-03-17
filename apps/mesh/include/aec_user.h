#ifndef _AEC_USER_H_
#define _AEC_USER_H_

#include "generic/typedef.h"
#include "user_cfg.h"

/*兼容SMS和DMS*/
#if TCFG_AUDIO_DUAL_MIC_ENABLE
#include "commproc_dms.h"
#define aec_open		aec_dms_init
#define aec_close		aec_dms_exit
#define aec_in_data		aec_dms_fill_in_data
#define aec_in_data_ref	aec_dms_fill_in_ref_data
#define aec_ref_data	aec_dms_fill_ref_data
void aec_cfg_fill(AEC_DMS_CONFIG *cfg);
#else
#include "commproc.h"
#define aec_open		aec_init
#define aec_close		aec_exit
#define aec_in_data		aec_fill_in_data
#define aec_in_data_ref(...)
#define aec_ref_data	aec_fill_ref_data
void aec_cfg_fill(AEC_CONFIG *cfg);
#endif

#define AEC_DEBUG_ONLINE	0
#define AEC_READ_CONFIG		1

#define AEC_EN				BIT(0)
#define NLP_EN				BIT(1)
#define	ANS_EN				BIT(2)
#define	ENC_EN				BIT(3)
#define	AGC_EN				BIT(4)

/*aec module enable bit define*/
#define AEC_MODE_ADVANCE	(AEC_EN | NLP_EN | ANS_EN )
#define AEC_MODE_REDUCE		(NLP_EN | ANS_EN )

#define AEC_MODE_SIMPLEX	(ANS_EN)


extern const u8 CONST_AEC_SIMPLEX;

s8 aec_debug_online(void *buf, u16 size);
void aec_input_clear_enable(u8 enable);

int audio_aec_open(u16 sample_rate, s16 enablebit, int (*out_hdl)(s16 *data, u16 len));
int audio_aec_init(u16 sample_rate);
void audio_aec_close(void);
void audio_aec_inbuf(s16 *buf, u16 len);
void audio_aec_inbuf_ref(s16 *buf, u16 len);
void audio_aec_refbuf(s16 *buf, u16 len);
u8 audio_aec_status(void);
void audio_aec_reboot(u8 reduce);


extern struct aec_s_attr aec_param;
extern const u8 CONST_AEC_SIMPLEX;

struct aec_s_attr *aec_param_init(u16 sr);
s8 aec_debug_online(void *buf, u16 size);
void aec_cfg_fill(AEC_CONFIG *cfg);
void aec_input_clear_enable(u8 enable);


#endif/*_AEC_USER_H_*/
