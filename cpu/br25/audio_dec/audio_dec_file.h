
#ifndef _AUDIO_DEC_FILE_H_
#define _AUDIO_DEC_FILE_H_

#include "asm/includes.h"
#include "media/includes.h"
#include "media/file_decoder.h"
#include "system/includes.h"
#include "media/audio_decoder.h"
#include "app_config.h"
#include "music/music_decrypt.h"
#include "music/music_id3.h"

#include "application/audio_pitchspeed.h"
#include "application/audio_surround.h"
#include "application/audio_vbass.h"


#define FILE_DEC_REPEAT_EN			0 // 无缝循环播放

#define FILE_DEC_AB_REPEAT_EN		0 // AB点复读

enum {
    FILE_DEC_STREAM_CLOSE = 0,
    FILE_DEC_STREAM_OPEN,
};

struct file_dec_hdl {
    struct audio_stream *stream;	// 音频流
    struct file_decoder file_dec;	// file解码句柄
    struct audio_res_wait wait;		// 资源等待句柄
    struct audio_mixer_ch mix_ch;	// 叠加句柄
    struct audio_eq_drc *eq_drc;//eq drc句柄
    struct audio_dec_breakpoint *dec_bp; // 断点
    s_pitchspeed_hdl *p_pitchspeed_hdl; // 变速变调句柄
    surround_hdl *surround;         //环绕音效句柄
    vbass_hdl *vbass;               //虚拟低音句柄



    u32 id;					// 唯一标识符，随机值
    void *file;				// 文件句柄
    u32 pick_flag : 1;		// 挑出数据帧发送（如MP3等)。不是输出pcm，后级不能接任何音效处理等
    u32 pcm_enc_flag : 1;	// pcm压缩成数据帧发送（如WAV等）
    u32 read_err : 1;		// 读数出错
    u32 ab_repeat_status : 3;	// AB复读状态

#if TCFG_DEC_DECRYPT_ENABLE
    CIPHER mply_cipher;		// 解密播放
#endif
#if TCFG_DEC_ID3_V1_ENABLE
    MP3_ID3_OBJ *p_mp3_id3_v1;	// id3_v1信息
#endif
#if TCFG_DEC_ID3_V2_ENABLE
    MP3_ID3_OBJ *p_mp3_id3_v2;	// id3_v2信息
#endif

#if FILE_DEC_REPEAT_EN
    u8 repeat_num;			// 无缝循环次数
    struct fixphase_repair_obj repair_buf;	// 无缝循环句柄
#endif

    struct audio_dec_breakpoint *bp;	// 断点信息

    void *evt_priv;			// 事件回调私有参数
    void (*evt_cb)(void *, int argc, int *argv);	// 事件回调句柄

    void (*stream_handler)(void *priv, int event, struct file_dec_hdl *);	// 数据流设置回调
    void *stream_priv;						// 数据流设置回调私有句柄

};



struct file_decoder *file_dec_get_file_decoder_hdl(void);

#define file_dec_is_stop()				file_decoder_is_stop(file_dec_get_file_decoder_hdl())
#define file_dec_is_play()				file_decoder_is_play(file_dec_get_file_decoder_hdl())
#define file_dec_is_pause()				file_decoder_is_pause(file_dec_get_file_decoder_hdl())
#define file_dec_pp()				    file_decoder_pp(file_dec_get_file_decoder_hdl())
#define file_dec_FF(x)					file_decoder_FF(file_dec_get_file_decoder_hdl(),x)
#define file_dec_FR(x)					file_decoder_FR(file_dec_get_file_decoder_hdl(),x)
#define file_dec_get_breakpoint(x)		file_decoder_get_breakpoint(file_dec_get_file_decoder_hdl(),x)
#define file_dec_get_total_time()		file_decoder_get_total_time(file_dec_get_file_decoder_hdl())
#define file_dec_get_cur_time()			file_decoder_get_cur_time(file_dec_get_file_decoder_hdl())
#define file_dec_get_decoder_type()		file_decoder_get_decoder_type(file_dec_get_file_decoder_hdl())

// 创建一个文件解码
int file_dec_create(void *priv, void (*handler)(void *, int argc, int *argv));
// 打开文件解码
int file_dec_open(void *file, struct audio_dec_breakpoint *bp);
// 关闭文件解码
void file_dec_close();

// 文件解码重新开始
int file_dec_restart(int id);
// 推送文件解码重新开始命令
int file_dec_push_restart(void);
// 获取file_dec状态
int file_dec_get_status(void);

// 设置解码数据流设置回调接口
void file_dec_set_stream_set_hdl(struct file_dec_hdl *dec,
                                 void (*stream_handler)(void *priv, int event, struct file_dec_hdl *),
                                 void *stream_priv);
// 获取文件解码hdl
void *get_file_dec_hdl();

#if (FILE_DEC_AB_REPEAT_EN)
int file_dec_ab_repeat_switch(void);
int file_dec_ab_repeat_close(void);
#else
#define file_dec_ab_repeat_switch()
#define file_dec_ab_repeat_close()
#endif/*FILE_DEC_AB_REPEAT_EN*/

#endif /*TCFG_APP_MUSIC_EN*/

