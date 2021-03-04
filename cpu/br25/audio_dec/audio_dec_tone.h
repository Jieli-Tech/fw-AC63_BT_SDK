

#ifndef _AUDIO_DEC_TONE_H_
#define _AUDIO_DEC_TONE_H_

#include "asm/includes.h"
#include "media/includes.h"
#include "media/file_decoder.h"
#include "system/includes.h"
#include "media/audio_decoder.h"
#include "application/audio_dec_app.h"
#include "tone_player.h"

// 提示音解码结束缘由
#define TONE_DEC_STOP_NOR				0 // 正常播放关闭
#define TONE_DEC_STOP_BY_OTHER_PLAY		1 // 被其他播放打断关闭

/*
 * 用于保护file_list播放
 * 原理是在当前播放结束后先启动一个空的解码占住res，
 * 避免播放file_list下一个时插入了其他解码
 */
#define TONE_DEC_PROTECT_LIST_PLAY		1


/*
 * 提示音file_list解码结构体
 * 依次播放"char **file_list"变量中定义的文件，直到file_list[n]为NULL
 * 如 file_list[0]=123.*; file_list[1]=456.*; file_list[2]=NULL;
 * 支持文件名解码、正弦波数组参数解码（需要实现get_sine()接口获取参数）
 * 正弦波数组播放格式示例：file_list[n]=DEFAULT_SINE_TONE(SINE_WTONE_NORAML);
 * 支持循环播放，循环播放需要指定播放起始（参数是代表循环次数）和结束，
 * 如：file_list[n]=TONE_REPEAT_BEGIN(-1); file_list[n+1]=456.*; file_list[n+2]=TONE_REPEAT_END();
 * 当需要使用外部自定义数据流时，可以通过实现*stream_handler接口来处理
 */
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

/*
 * 提示音解码结构体
 * 可以实现多个tone_dec_list_handle播放，通过结构体中的链表连接
 * 当解码中有正弦波数组播放时，必须要实现*get_sine接口
 */
struct tone_dec_handle {
    struct list_head head;	// 链表头
    struct audio_dec_sine_app_hdl *dec_sin;		// 文件播放句柄
    struct audio_dec_file_app_hdl *dec_file;	// sine播放句柄
    struct tone_dec_list_handle *cur_list;		// 当前播放list
    struct sin_param *(*get_sine)(u8 id, u8 *num);	// 按序列号获取sine数组
    OS_MUTEX mutex;		// 互斥
};

/*----------------------------------------------------------------------------*/
/**@brief    创建提示音播放句柄
   @param
   @return   提示音句柄
   @note
*/
/*----------------------------------------------------------------------------*/
struct tone_dec_handle *tone_dec_create(void);

/*----------------------------------------------------------------------------*/
/**@brief    设置sine数组获取回调
   @param    *dec: 提示音句柄
   @param    *get_sine: sine数组获取
   @return
   @note     当解码中有正弦波数组播放时，必须设置该接口
*/
/*----------------------------------------------------------------------------*/
void tone_dec_set_sin_get_hdl(struct tone_dec_handle *dec, struct sin_param * (*get_sine)(u8 id, u8 *num));

/*----------------------------------------------------------------------------*/
/**@brief    创建提示音播放list句柄
   @param    *dec: 提示音句柄
   @param    **file_list: 文件名
   @param    preemption: 打断标记
   @param    *evt_handler: 事件回调接口
   @param    *evt_priv: 事件回调私有句柄
   @param    *stream_handler: tone数据流设置回调
   @param    *stream_priv: tone数据流设置回调私有句柄
   @return   list句柄
   @note
*/
/*----------------------------------------------------------------------------*/
struct tone_dec_list_handle *tone_dec_list_create(struct tone_dec_handle *dec,
        const char **file_list,
        u8 preemption,
        void (*evt_handler)(void *priv, int flag),
        void *evt_priv,
        void (*stream_handler)(void *priv, int event, struct audio_dec_app_hdl *app_dec),
        void *stream_priv);

/*----------------------------------------------------------------------------*/
/**@brief    提示音list开始播放
   @param    *dec: 提示音句柄
   @param    *dec_list: list句柄
   @return   true: 成功
   @return   false: 成功
   @note     当前没有播放，马上开始播放。当前有播放，挂载到链表后面等待播放
*/
/*----------------------------------------------------------------------------*/
int tone_dec_list_add_play(struct tone_dec_handle *dec, struct tone_dec_list_handle *dec_list);

/*----------------------------------------------------------------------------*/
/**@brief    提示音播放停止
   @param    **ppdec: 提示音句柄
   @param    push_event: 普通提示音是否推送消息
   @param    end_flag: 结束类型
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void tone_dec_stop(struct tone_dec_handle **ppdec,
                   u8 push_event,
                   u8 end_flag);

/*----------------------------------------------------------------------------*/
/**@brief    指定提示音播放停止
   @param    **ppdec: 提示音句柄
   @param    push_event: 普通提示音是否推送消息
   @param    end_flag: 结束类型
   @return
   @note     如果该提示音正在播，停止播放并且播放下一个。如果不在播放，只从链表中删除
*/
/*----------------------------------------------------------------------------*/
void tone_dec_stop_spec_file(struct tone_dec_handle **ppdec,
                             char *file_name,
                             u8 push_event,
                             u8 end_flag);


#endif /*_AUDIO_DEC_TONE_H_*/

