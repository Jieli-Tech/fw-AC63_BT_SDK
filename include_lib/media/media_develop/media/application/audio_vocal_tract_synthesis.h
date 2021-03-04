
#ifndef _AUDIO_VOCAL_TRACT_API_H_
#define _AUDIO_VOCAL_TRACT_API_H_

#include "system/includes.h"
#include "media/includes.h"

#define FL_FR (BIT(0)|BIT(1))
#define RL_RR (BIT(2)|BIT(3))

struct audio_vocal_tract {
    struct list_head head;		// 链表头
    s16 *output;				// 输出buf
    u16 points;			        // 输出buf总点数
    u16 remain_points;	        // 输出剩余点数
    u32 process_len;	        // 输出了多少
    u8  channel_num;	        // 声道数
    u8  sample_sync;            //记录同步模块句柄
    u32 sample_rate;	        // 当前采样率
    volatile u8 active;	        // 活动标记。1-正在运行

    struct audio_stream *stream;		// 音频流
    struct audio_stream_entry entry;	// 音频流入口，后级接mixer
};

struct audio_vocal_tract_ch {
    u32 start : 1;		        // 启动标记。1-已经启动。输出时标记一次
    u32 open : 1;		        // 打开标记。1-已经打开。输出标记为1，reset标记为0
    u32 pause : 1;		        // 暂停标记。1-暂停
    u32 wait_resume : 1;
    u16 offset;			        // 当前通道在输出buf中的偏移位置
    u32 synthesis_tar;          //输入目标声道 FL:bit0, FR:bit1, RL:bit2, RR:bit3

    struct list_head list_entry;
    struct audio_vocal_tract *vocal_tract;
    struct audio_stream_entry entry;	// 音频流入口，通道后面不应该再接其他的音频流，最后由mixer合并后输出
};

/*----------------------------------------------------------------------------*/
/**@brief   打开声道合并
   @param   *vocal_tract: 句柄
   @param   len:声道合并buf长度
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_vocal_tract_open(struct audio_vocal_tract *vocal_tract, u32 len);

/*----------------------------------------------------------------------------*/
/**@brief   关闭声道合并
   @param   *vocal_tract: 句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void audio_vocal_tract_close(struct audio_vocal_tract *vocal_tract);


/*----------------------------------------------------------------------------*/
/**@brief   声道合并的通道数
   @param   *vocal_tract: 句柄
   @param   channel_num:输出通道数，支持（四声道）
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void audio_vocal_tract_set_channel_num(struct audio_vocal_tract *vocal_tract, u8 channel_num);

/*----------------------------------------------------------------------------*/
/**@brief   声道合并数据流激活
   @param   *priv: 句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void audio_vocal_tract_stream_resume(void *priv);


/*----------------------------------------------------------------------------*/
/**@brief   声道合并输入源通道打开
   @param    *ch: 源通道句柄
   @param    *vocal_tract:声道合并句柄
   @param    synthesis_tar:输出的目标声道  FL_FR、RL_RR
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_vocal_tract_synthesis_open(struct audio_vocal_tract_ch *ch, struct audio_vocal_tract *vocal_tract, u32 synthesis_tar);

/*----------------------------------------------------------------------------*/
/**@brief    声道合并输入源通道关闭
   @param    *ch:声道输入源句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_vocal_tract_synthesis_close(struct audio_vocal_tract_ch *ch);
#endif

