

#ifndef _AUDIO_DEC_TONE_H_
#define _AUDIO_DEC_TONE_H_

#include "asm/includes.h"
#include "media/includes.h"
#include "media/file_decoder.h"
#include "system/includes.h"
#include "media/audio_decoder.h"
#include "application/audio_dec_app.h"
#include "tone_player.h"

#define TONE_DEC_STOP_NOR				0 // 正常播放关闭
#define TONE_DEC_STOP_BY_OTHER_PLAY		1 // 被其他播放打断关闭

#define TONE_DEC_PROTECT_LIST_PLAY		1 // 保护链表播放

struct tone_dec_list_handle {
    struct list_head list_entry;	// 链表
    u8 preemption : 1;		// 打断
    u8 idx;					// 循环播放序号
    u8 repeat_begin;		// 循环播放起始序号
    u8 dec_ok_cnt;			// 正常播放计数
    u16 loop;				// 循环播放次数
    int sync_confirm_time;
    char **file_list;		// 文件名
    const char *evt_owner;				// 事件接受任务
    void (*evt_handler)(void *priv, int flag);	// 事件回调
    void *evt_priv;						// 事件回调私有句柄
    void (*stream_handler)(void *priv, int event, struct audio_dec_app_hdl *);	// 数据流设置回调
    void *stream_priv;						// 数据流设置回调私有句柄
#if TONE_DEC_PROTECT_LIST_PLAY
    void *list_protect;
#endif
};

struct tone_dec_handle {
    struct list_head head;	// 链表头
    struct audio_dec_sine_app_hdl *dec_sin;		// 文件播放句柄
    struct audio_dec_file_app_hdl *dec_file;	// sine播放句柄
    struct tone_dec_list_handle *cur_list;		// 当前播放list
    struct sin_param *(*get_sine)(u8 id, u8 *num);	// 按序列号获取sine数组
    OS_MUTEX mutex;		// 互斥
};

// 创建提示音播放句柄
struct tone_dec_handle *tone_dec_create(void);

// 设置sine数组获取回调
void tone_dec_set_sin_get_hdl(struct tone_dec_handle *dec, struct sin_param * (*get_sine)(u8 id, u8 *num));

// 创建提示音播放list句柄
struct tone_dec_list_handle *tone_dec_list_create(struct tone_dec_handle *dec,
        const char **file_list,
        u8 preemption,
        void (*evt_handler)(void *priv, int flag),
        void *evt_priv,
        void (*stream_handler)(void *priv, int event, struct audio_dec_app_hdl *app_dec),
        void *stream_priv);

// 提示音list开始播放
int tone_dec_list_add_play(struct tone_dec_handle *dec, struct tone_dec_list_handle *dec_list);

// 提示音播放停止
void tone_dec_stop(struct tone_dec_handle **ppdec,
                   u8 push_event,
                   u8 end_flag);

// 指定提示音播放停止
void tone_dec_stop_spec_file(struct tone_dec_handle **ppdec,
                             char *file_name,
                             u8 push_event,
                             u8 end_flag);


#endif /*_AUDIO_DEC_TONE_H_*/

