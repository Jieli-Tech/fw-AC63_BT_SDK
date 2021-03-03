#ifndef PCM_DECODER_H
#define PCM_DECODER_H


#include "media/includes.h"


struct pcm_decoder {
    u16 ch_num : 4;			// 声道数
    u16 output_ch_num : 4;	// 输出声道数
    u16 output_ch_type : 4;	// 输出声道类型
    u16 dec_no_out_sound : 1;	// 解码不直接输出声音（用于TWS转发）
    u16 sample_rate;	// 采样率
    struct audio_decoder decoder;	// 解码句柄
    void *read_priv;	// 数据获取私有句柄
    int (*read_data)(void *priv, void *buf, int len);	// 数据获取
};

// 打开pcm解码
int pcm_decoder_open(struct pcm_decoder *, struct audio_decoder_task *decode_task);
// 关闭pcm解码
void pcm_decoder_close(struct pcm_decoder *dec);
// 设置pcm解码事件回调接口
void pcm_decoder_set_event_handler(struct pcm_decoder *dec,
                                   void (*handler)(struct audio_decoder *, int, int *), u32 maigc);
// 设置pcm解码数据获取接口
void pcm_decoder_set_read_data(struct pcm_decoder *dec, int (*read_data)(void *priv, void *buf, int len), void *read_priv);
// 设置pcm解码数据处理接口
void pcm_decoder_set_data_handler(struct pcm_decoder *dec,
                                  int (*data_handler)(struct audio_stream_entry *,  struct audio_data_frame *in, struct audio_data_frame *out));

#endif /*PCM_DECODER_H*/

