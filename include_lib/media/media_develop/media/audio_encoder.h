#ifndef AUDIO_ENCODER_H
#define AUDIO_ENCODER_H

#include "generic/typedef.h"
#include "generic/list.h"
#include "generic/circular_buf.h"
#include "media/audio_base.h"


// 编码事件
enum {
    AUDIO_ENC_EVENT_CURR_TIME = 0x20,
    AUDIO_ENC_EVENT_END,
    AUDIO_ENC_EVENT_ERR,
    AUDIO_ENC_EVENT_SUSPEND,
    AUDIO_ENC_EVENT_RESUME,
};


/*
 * 编码任务结构体
 * 链接所有的编码，按list链表循环执行
 */
struct audio_encoder_task {
    struct list_head head;
    const char *name;
};


struct audio_encoder;


/*
 * 编码输入接口
 * 用于获取PCM数据
 */
struct audio_enc_input {
    int (*fget)(struct audio_encoder *, s16 **frame, u16 frame_len);
    void (*fput)(struct audio_encoder *, s16 *frame);
};

/*
 * 编码处理过程中的回调
 */
struct audio_enc_handler {
    int (*enc_probe)(struct audio_encoder *);
    int (*enc_output)(struct audio_encoder *, u8 *data, int len);
    int (*enc_post)(struct audio_encoder *);
    int (*enc_close)(struct audio_encoder *);//add by wuxu 20200221
};

/*
 * 编码器结构体
 * 所有挂载到编码的编码器都需要该结构体
 * 至少需要实现open/start/run/close等函数
 */
struct audio_encoder_ops {
    u32 coding_type;
    void *(*open)(void *priv);
    int (*start)(void *);
    int (*set_fmt)(void *, struct audio_fmt *fmt);
    int (*run)(void *);
    int (*stop)(void *);
    int (*close)(void *);
    int (*ioctrl)(void *, int argc, int argv[]);
};

#define REGISTER_AUDIO_ENCODER(ops) \
        const struct audio_encoder_ops ops SEC(.audio_encoder)

extern const struct audio_encoder_ops audio_encoder_begin[];
extern const struct audio_encoder_ops audio_encoder_end[];

#define list_for_each_audio_encoder(p) \
    for (p = audio_encoder_begin; p < audio_encoder_end; p++)


#define AUDIO_ENC_OS_MUTEX_EN     1

#if (AUDIO_ENC_OS_MUTEX_EN)
#include "os/os_api.h"
#endif//AUDIO_ENC_OS_MUTEX_EN

/*
 * 编码通道
 */
struct audio_encoder {
    struct list_head entry;				// 链表。用于编码任务中轮询处理
    struct audio_encoder_task *task;	// 编码任务
    struct audio_fmt fmt;				// 编码格式
    const char *evt_owner;				// 用于接受消息的任务名称
    const struct audio_enc_input *input;			// 编码输入接口
    const struct audio_encoder_ops *enc_ops;		// 编码器接口
    const struct audio_enc_handler *enc_handler;	// 编码处理回调
    void (*evt_handler)(struct audio_encoder *enc, int, int *);	// 事件回调
    void *enc_priv;		// 编码器对应句柄
    // u32 pend_timeout;
    s16 *pcm_frame;		// 编码的帧数据
    u16 pcm_remain;		// 帧数据剩余长度
    u16 pcm_len;		// 帧数据长度
    s16 *output_buffs;		// 输出用的buf
    u16 output_buff_size;	// 输出buf大小
    u8 output_buff_num;		// 输出buf数量
    u8 curr_output_buff;	// 当前用的输出buf
    // u8 output_channel;
    u8 state;		// 编码状态
    u8 err;		// 编码结束错误类型标记
    volatile u8 resume_flag;// 编码激活标记
    u32 magic;	// 事件回调的私有标记
#if (AUDIO_ENC_OS_MUTEX_EN)
    OS_MUTEX mutex;	// 互斥
#endif//AUDIO_ENC_OS_MUTEX_EN
};

// 编码器命令
enum {
    AUDIO_ENCODER_IOCTRL_CMD_GET_HEAD_INFO = 0x0,
    AUDIO_ENCODER_IOCTRL_CMD_GET_TIME,
    AUDIO_ENCODER_IOCTRL_CMD_GET_TMARK,
};

/*
*********************************************************************
*                  Audio Encoder Task Create
* Description: 创建编码任务
* Arguments  : *task	任务句柄
*              *name	任务名
* Return	 : 0		成功
* Note(s)    : None.
*********************************************************************
*/
int audio_encoder_task_create(struct audio_encoder_task *task, const char *name);

/*
*********************************************************************
*                  Audio Encoder Delete Task
* Description: 删除编码任务
* Arguments  : *task	任务句柄
* Return	 : 0		成功
* Note(s)    : None.
*********************************************************************
*/
int audio_encoder_task_del(struct audio_encoder_task *task);

/*
*********************************************************************
*                  Audio Encoder Resume All Encoder Channel
* Description: 激活编码任务链表中的所有编码
* Arguments  : *task	任务句柄
* Return	 : 0		成功
* Note(s)    : None.
*********************************************************************
*/
int audio_encoder_resume_all(struct audio_encoder_task *task);

/*
*********************************************************************
*                  Audio Encoder Get Output Buff
* Description: 获取编码输出buf
* Arguments  : *_enc 	编码句柄
*              **buf	输出buf
* Return	 : 输出buf长度
* Note(s)    : None.
*********************************************************************
*/
int audio_encoder_get_output_buff(void *_enc, s16 **buf);

/*
*********************************************************************
*                  Audio Encoder Put Output Buff
* Description: 编码输出
* Arguments  : *_enc 	编码句柄
*              **buf	输出buf
*              len		输出长度
* Return	 : 输出了多少数据
* Note(s)    : None.
*********************************************************************
*/
int audio_encoder_put_output_buff(void *_enc, void *buff, int len);

/*
*********************************************************************
*                  Audio Encoder Get Frame
* Description: 获取帧数据
* Arguments  : *_enc 	编码句柄
*              **frame	返回帧信息
*              frame_len	获取帧最大长度
* Return	 : 实际帧长
* Note(s)    : None.
*********************************************************************
*/
int audio_encoder_get_frame(void *_enc, s16 **frame, u16 len);

/*
*********************************************************************
*                  Audio Encoder Put Frame
* Description: 数据帧使用完
* Arguments  : *_enc 	编码句柄
*              *frame	帧信息
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_encoder_put_frame(void *_enc, s16 *frame);

/*
*********************************************************************
*                  Audio Encoder Open Channel
* Description: 打开一个编码通道
* Arguments  : *enc 	编码句柄
*              *input	编码输入参数信息
*              *task	编码任务句柄
* Return	 : 0		成功
* Note(s)    : None.
*********************************************************************
*/
int audio_encoder_open(struct audio_encoder *enc, const struct audio_enc_input *input,
                       struct audio_encoder_task *task);

/*
*********************************************************************
*                  Audio Encoder Get Format
* Description: 获取编码器格式信息
* Arguments  : *enc 	编码句柄
*              **fmt	获取到的格式信息
* Return	 : 0		成功
* Note(s)    : None.
*********************************************************************
*/
int audio_encoder_get_fmt(struct audio_encoder *enc, struct audio_fmt **fmt);

/*
*********************************************************************
*                  Audio Encoder Set Format
* Description: 设置编码的格式信息
* Arguments  : *enc 	编码句柄
*              *fmt		格式信息
* Return	 : 0		成功
* Note(s)    : 编码器没有打开时，尝试打开编码器
*********************************************************************
*/
int audio_encoder_set_fmt(struct audio_encoder *enc, struct audio_fmt *fmt);

/*
*********************************************************************
*                  Audio Encoder Set Deal Handler
* Description: 设置编码处理handler
* Arguments  : *enc 	编码句柄
*              *handler	编码处理handler
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_encoder_set_handler(struct audio_encoder *enc, const struct audio_enc_handler *handler);


/*
*********************************************************************
*                  Audio Encoder Set Event Handler
* Description: 设置编码事件回调
* Arguments  : *enc 	编码句柄
*              *handler	回调函数
*              maigc	私有标识
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_encoder_set_event_handler(struct audio_encoder *enc,
                                     void (*handler)(struct audio_encoder *, int, int *), u32 maigc);

/*
*********************************************************************
*                  Audio Encoder Set Output Buff
* Description: 设置编码输出buf
* Arguments  : *enc 	编码句柄
*              *buffs	输出buf
*              buff_size	单个buf大小
*              buff_num		总共多少个buf
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_encoder_set_output_buffs(struct audio_encoder *enc, s16 *buffs,
                                    u16 buff_size, u8 buff_num);

/*
*********************************************************************
*                  Audio Encoder Start
* Description: 启动编码
* Arguments  : *enc 	编码句柄
* Return	 : 0		成功
* Note(s)    : 函数中调用编码器start()接口
*********************************************************************
*/
int audio_encoder_start(struct audio_encoder *enc);

/*
*********************************************************************
*                  Audio Encoder Stop
* Description: 停止编码
* Arguments  : *enc 	编码句柄
* Return	 : 0		成功
* Note(s)    : None.
*********************************************************************
*/
int audio_encoder_stop(struct audio_encoder *enc);

/*
*********************************************************************
*                  Audio Encoder Suspend
* Description: 编码挂起
* Arguments  : *enc 	编码句柄
* Return	 : 0		成功
* Note(s)    : None.
*********************************************************************
*/
int audio_encoder_suspend(struct audio_encoder *enc, int timeout_ms);

/*
*********************************************************************
*                  Audio Encoder Resume
* Description: 编码挂起激活
* Arguments  : *enc 	编码句柄
* Return	 : 0		成功
* Note(s)    : None.
*********************************************************************
*/
int audio_encoder_resume(struct audio_encoder *enc);

/*
*********************************************************************
*                  Audio Encoder Close
* Description: 关闭编码
* Arguments  : *enc 	编码句柄
* Return	 : 0		成功
* Note(s)    : None.
*********************************************************************
*/
int audio_encoder_close(struct audio_encoder *enc);

/*
*********************************************************************
*                  Audio Encoder Ioctrl
* Description: 编码器控制
* Arguments  : *enc 	编码句柄
*              argc		控制参数
* Return	 : -EINVAL	错误。其他	控制器返回
* Note(s)    : 函数中调用编码器ioctrl()接口
*********************************************************************
*/
int audio_encoder_ioctrl(struct audio_encoder *enc, int argc, ...);

/*
*********************************************************************
*                  Audio Encoder Get channel
* Description: 获取编码器声道配置参数
* Arguments  : *enc 	编码句柄
* Return	 : 声道数
* Note(s)    : None
*********************************************************************
*/
int audio_encoder_get_channel(struct audio_encoder *enc);
#endif

