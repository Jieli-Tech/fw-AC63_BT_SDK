#ifndef AUDIO_ENCODER_H
#define AUDIO_ENCODER_H

#include "generic/typedef.h"
#include "generic/list.h"
#include "generic/circular_buf.h"
#include "media/audio_base.h"



enum {
    AUDIO_ENC_EVENT_CURR_TIME = 0x20,
    AUDIO_ENC_EVENT_END,
    AUDIO_ENC_EVENT_ERR,
    AUDIO_ENC_EVENT_SUSPEND,
    AUDIO_ENC_EVENT_RESUME,
};


struct audio_encoder_task {
    struct list_head head;
    const char *name;
};


struct audio_encoder;


struct audio_enc_input {
    int (*fget)(struct audio_encoder *, s16 **frame, u16 frame_len);
    void (*fput)(struct audio_encoder *, s16 *frame);
};

struct audio_enc_handler {
    int (*enc_probe)(struct audio_encoder *);
    int (*enc_output)(struct audio_encoder *, u8 *data, int len);
    int (*enc_post)(struct audio_encoder *);
    int (*enc_close)(struct audio_encoder *);//add by wuxu 20200221
};

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
        const struct audio_encoder_ops ops sec(.audio_encoder)

extern const struct audio_encoder_ops audio_encoder_begin[];
extern const struct audio_encoder_ops audio_encoder_end[];

#define list_for_each_audio_encoder(p) \
    for (p = audio_encoder_begin; p < audio_encoder_end; p++)


#define AUDIO_ENC_OS_MUTEX_EN     1

#if (AUDIO_ENC_OS_MUTEX_EN)
#include "os/os_api.h"
#endif//AUDIO_ENC_OS_MUTEX_EN

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

enum {
    AUDIO_ENCODER_IOCTRL_CMD_GET_HEAD_INFO = 0x0,
    AUDIO_ENCODER_IOCTRL_CMD_GET_TIME,
};

// 创建编码任务
int audio_encoder_task_create(struct audio_encoder_task *task, const char *name);

// 删除编码任务
int audio_encoder_task_del(struct audio_encoder_task *task);

// 激活编码任务链表中的所有编码
int audio_encoder_resume_all(struct audio_encoder_task *task);

// 获取编码输出buf
int audio_encoder_get_output_buff(void *_enc, s16 **buf);

// 编码输出
int audio_encoder_put_output_buff(void *_enc, void *buff, int len);

// 获取帧数据
int audio_encoder_get_frame(void *_enc, s16 **frame, u16 len);

// 数据帧使用完
void audio_encoder_put_frame(void *_enc, s16 *frame);

// 打开一个编码通道
int audio_encoder_open(struct audio_encoder *enc, const struct audio_enc_input *input,
                       struct audio_encoder_task *task);

// 获取编码器格式信息
int audio_encoder_get_fmt(struct audio_encoder *enc, struct audio_fmt **fmt);

// 设置编码的格式信息（编码器没有打开时，尝试打开编码器）
int audio_encoder_set_fmt(struct audio_encoder *enc, struct audio_fmt *fmt);

// 设置编码处理handler
void audio_encoder_set_handler(struct audio_encoder *enc, const struct audio_enc_handler *handler);


// 设置编码事件回调
void audio_encoder_set_event_handler(struct audio_encoder *enc,
                                     void (*handler)(struct audio_encoder *, int, int *), u32 maigc);

// 设置编码输出buf
void audio_encoder_set_output_buffs(struct audio_encoder *enc, s16 *buffs,
                                    u16 buff_size, u8 buff_num);

// 启动编码
int audio_encoder_start(struct audio_encoder *enc);

// 停止编码
int audio_encoder_stop(struct audio_encoder *enc);

// 编码挂起
int audio_encoder_suspend(struct audio_encoder *enc, int timeout_ms);

// 编码挂起激活
int audio_encoder_resume(struct audio_encoder *enc);

// 关闭编码
int audio_encoder_close(struct audio_encoder *enc);

// 编码器控制
int audio_encoder_ioctrl(struct audio_encoder *enc, int argc, ...);


#endif

