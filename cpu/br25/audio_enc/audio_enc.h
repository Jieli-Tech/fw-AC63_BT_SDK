#ifndef _AUDIO_ENC_H_
#define _AUDIO_ENC_H_

#include "generic/typedef.h"
#include "media/includes.h"

#ifndef CONFIG_LITE_AUDIO
#include "mic_effect.h"
#include "loud_speaker.h"
#endif/*CONFIG_LITE_AUDIO*/


struct record_file_fmt {
    u8  gain;//增益配置
    u8  channel;//声道数，1:单声道， 2：双声道
    u16 sample_rate;//采样率
    u32 bit_rate;//码率
    u32 coding_type;//编码格式
    char *dev;//录音设备盘符
    char *folder;//录音文件夹
    char *filename;//录音文件名
    u32 cut_head_time;//录音文件去头时间,单位ms
    u32 cut_tail_time;//录音文件去尾时间,单位ms
    u32 limit_size;//录音文件大小最小限制， 单位byte
    u8  source;//录音输入源
    void (*err_callback)(void);
};


int esco_enc_open(u32 coding_type, u8 frame_len);
void esco_enc_close();

enum enc_source {
    ENCODE_SOURCE_MIX = 0x0,
    ENCODE_SOURCE_MIC,
    ENCODE_SOURCE_LINE0_LR,
    ENCODE_SOURCE_LINE1_LR,
    ENCODE_SOURCE_LINE2_LR,
    ENCODE_SOURCE_USER,
};

u32 recorder_get_encoding_time();
int recorder_is_encoding(void);
void recorder_device_offline_check(char *logo);
void recorder_encode_stop(void);
void recorder_pcm2file_write_pcm_ex(s16 *data, int len);
int mixer_recorder_encoding(void);
int mixer_recorder_start(void);
void mixer_recorder_stop(void);
int recorder_encode_start(struct record_file_fmt *f);
int recorder_userdata_to_enc(s16 *data, int len);

int audio_encoder_task_open(void);
void audio_encoder_task_close(void);

int audio_mic_open(struct adc_mic_ch *mic, u16 sample_rate, u8 gain);
void audio_mic_add_output(struct audio_adc_output_hdl *output);
void audio_mic_start(struct adc_mic_ch *mic);
void audio_mic_close(struct adc_mic_ch *mic, struct audio_adc_output_hdl *output);
void audio_mic_set_gain(struct adc_mic_ch *mic, u8 gain);
//////////////////////////////////////////////////////////////////
#include "audio_enc_file.h"
#include "audio_enc_recoder.h"
#include "audio_recorder_mix.h"

#endif/*_AUDIO_ENC_H_*/
