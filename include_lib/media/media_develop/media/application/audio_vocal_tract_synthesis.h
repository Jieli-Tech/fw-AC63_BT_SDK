
#ifndef _AUDIO_VOCAL_TRACT_API_H_
#define _AUDIO_VOCAL_TRACT_API_H_


#define FL_FR (BIT(0)|BIT(1))
#define RL_RR (BIT(2)|BIT(3))

struct audio_vocal_tract {
    struct list_head head;	// 链表头
    s16 *output;		// 输出buf
    u16 points;			// 输出buf总点数
    u16 remain_points;	// 输出剩余点数
    u32 process_len;	// 输出了多少
    u8  channel_num;	// 声道数
    u8  sample_sync;
    u16 sample_rate;	// 当前采样率
    volatile u8 active;	// 活动标记。1-正在运行

    struct audio_stream *stream;		// 音频流
    struct audio_stream_entry entry;	// 音频流入口，后级接mixer
};

struct audio_vocal_tract_ch {
    u32 start : 1;		// 启动标记。1-已经启动。输出时标记一次
    u32 open : 1;		// 打开标记。1-已经打开。输出标记为1，reset标记为0
    u32 pause : 1;		// 暂停标记。1-暂停
    u32 wait_resume : 1;
    u16 offset;			// 当前通道在输出buf中的偏移位置
    u32 synthesis_tar; //输入目标声道 FL:bit0, FR:bit1, RL:bit2, RR:bit3

    struct list_head list_entry;
    struct audio_vocal_tract *vocal_tract;
    struct audio_stream_entry entry;	// 音频流入口，通道后面不应该再接其他的音频流，最后由mixer合并后输出
};

// 打开一个vocal_tract
int audio_vocal_tract_open(struct audio_vocal_tract *vocal_tract, u32 len);

// 关闭vocal_tract
void audio_vocal_tract_close(struct audio_vocal_tract *vocal_tract);

// 设置输出buf
// void audio_vocal_tract_set_output_buf(struct audio_vocal_tract *vocal_tract, s16 *buf, u16 len);

// 设置输出声道数
void audio_vocal_tract_set_channel_num(struct audio_vocal_tract *vocal_tract, u8 channel_num);

void audio_vocal_tract_stream_resume(void *priv);


/* typedef struct _vocal_tract_open_parm { */
// u8 channel;//输入音频声道数
// u32 target_tract;//输入目标声道 FL:bit0, FR:bit1, RL:bit2, RR:bit3
/* } vocal_tract_open_parm; */


/* typedef struct _vocal_tract_hdl { */
// s16 *out_buf;
// u32 out_points;
// u32 out_total;

// vocal_tract_open_parm o_parm;
// struct audio_stream_entry entry;	// 音频流入口
// struct list_head tract_entry;
/* } vocal_tract_hdl; */
/*----------------------------------------------------------------------------*/
/**@brief   audio_vocal_tract_synthesis_open  声道合成打开
   @param    *_parm: 始化参数，详见结构体vocal_tract_open_parm
   @return   句柄
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_vocal_tract_synthesis_open(struct audio_vocal_tract_ch *ch, struct audio_vocal_tract *vocal_tract, u32 synthesis_tar);

/*----------------------------------------------------------------------------*/
/**@brief    audio_vocal_tract_synthesis_close 声道合成关闭处理
   @param    _hdl:句柄
   @return  0:成功  -1：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_vocal_tract_synthesis_close(struct audio_vocal_tract_ch *ch);
#endif

