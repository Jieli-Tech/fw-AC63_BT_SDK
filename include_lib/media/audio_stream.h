#ifndef AUDIO_STREAM_H
#define AUDIO_STREAM_H

#include "generic/includes.h"
#include "media/audio_base.h"


#define AUDIO_STREAM_IOCTRL_CMD_CHECK_ACTIVE		(1)


struct audio_stream_entry;
struct audio_frame_copy;


struct audio_data_frame {
    u8 channel;
    // u8 coding_type;
    u16 stop : 1;
    u16 data_sync : 1;
    u16 no_subsequent : 1;	// 没有后续
    u32 sample_rate;
    u16 offset;             //数据偏移
    u16 data_len;
    s16 *data;
};


struct audio_stream {
    struct list_head input_cp_head;
    struct audio_data_frame output;
    struct audio_stream_entry *entry;
    void *priv;
    void (*resume)(void *priv);
    /*void (*suspend)(void *priv);*/
};


struct audio_stream_group {
    struct audio_stream_entry *entry;
};

struct audio_stream_entry {
    u8  pass_by;
    u16 offset;
    struct audio_stream *stream;
    struct audio_stream_entry *input;
    struct audio_stream_entry *output;
    struct audio_stream_entry *sibling;
    struct audio_stream_group *group;
    struct audio_frame_copy *frame_copy;
    int (*prob_handler)(struct audio_stream_entry *,  struct audio_data_frame *in);
    int (*data_handler)(struct audio_stream_entry *,  struct audio_data_frame *in,
                        struct audio_data_frame *out);
    void (*data_process_len)(struct audio_stream_entry *,  int len);
    void (*data_clear)(struct audio_stream_entry *);
    int (*ioctrl)(struct audio_stream_entry *, int cmd, int *param);
};

struct audio_frame_copy {
    struct list_head head;
    struct audio_data_frame frame;
    struct audio_stream_entry entry;
};



/*
 * resume: 模块唤醒数据流时的回调函数
 */
struct audio_stream *audio_stream_open(void *priv, void (*resume)(void *priv));

void audio_stream_add_first(struct audio_stream *stream,
                            struct audio_stream_entry *entry);

void audio_stream_add_head(struct audio_stream *stream,
                           struct audio_stream_entry *entry);

void audio_stream_add_tail(struct audio_stream *stream,
                           struct audio_stream_entry *entry);

void audio_stream_add_entry(struct audio_stream_entry *input,
                            struct audio_stream_entry *output);

void audio_stream_add_list(struct audio_stream *stream,
                           struct audio_stream_entry *entry[], int num);

void audio_stream_del_entry(struct audio_stream_entry *entry);


void audio_stream_del_list(struct audio_stream_entry *entry[], int num);


void audio_stream_group_add_entry(struct audio_stream_group *group,
                                  struct audio_stream_entry *entry);

void audio_stream_group_del_entry(struct audio_stream_group *group,
                                  struct audio_stream_entry *entry);

int audio_stream_group_entry_num(struct audio_stream_group *group);

/*
 * 数据输入接口
 */
int audio_stream_run(struct audio_stream_entry *from, struct audio_data_frame *);


/*
 * 模块要唤醒数据流传递时调用
 */
int audio_stream_resume(struct audio_stream_entry *entry);


void audio_stream_clear_from(struct audio_stream_entry *entry);

void audio_stream_clear(struct audio_stream *stream);

void audio_stream_clear_by_entry(struct audio_stream_entry *entry);


/*
 * 数据流活动检测。true 活动
 */
int audio_stream_check_active_from(struct audio_stream_entry *entry);


void audio_stream_close(struct audio_stream *);

#endif





















