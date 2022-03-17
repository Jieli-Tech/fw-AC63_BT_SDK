/*****************************************************************
>file name : include_lib/media/audio_enc_server.h
>author : lichao
>create time : Mon 19 Nov 2018 08:45:49 PM CST
*****************************************************************/

#ifndef _AUDIO_ENCODE_SERVER_H_
#define _AUDIO_ENCODE_SERVER_H_

#include "media/avmodule.h"
#include "fs/fs.h"

#define AUDIO_REQ_REC                       0
#define AUDIO_REQ_FRAME                     1

#define AUDIO_STATE_REC_OPEN                0
#define AUDIO_STATE_REC_START               1
#define AUDIO_STATE_REC_PAUSE               2
#define AUDIO_STATE_REC_STOP                3
#define AUDIO_STATE_REC_MUTE                4
#define AUDIO_STATE_REC_UNMUTE              5

#define AUDIO_STATE_GET_FRAME               8
#define AUDIO_STATE_PUT_FRAME               9

/*
#define AUDIO_REC_TYPE_FILE                       0
#define AUDIO_REC_TYPE_BUFFER                     1
*/

struct audio_enc_info {
    u8 channel;
    u8 name_code; /* 0:ansi, 1:unicode_le, 2:unicode_be*/
    int sample_rate;
    int bit_rate;
};

struct audio_rec_vfs_ops {
    void *(*fopen)(const char *path, const char *mode);
    int (*fread)(void *file, void *buf, u32 len);
    int (*fwrite)(void *file, void *buf, u32 len);
    int (*fseek)(void *file, u32 offset, int seek_mode);
    int (*ftell)(void *file);
    int (*flen)(void *file);
    int (*fclose)(void *file);
};

struct audio_enc_buf_ops {
    void *(*alloc_frame)(void *priv, u32 len);
    void *(*alloc_free_space)(void *priv, u32 *len);
    void (*free_frame)(void *priv, void *data);
    int (*finish_frame)(void *priv, void *data, u32 len);
};


struct audio_pcm_data {
    //unsigned channel : 8;
    //unsigned sample_rate : 24;
    u8 channel;
    u32 sample_rate;
    void *data;
    int  len;
    int  finish_len;
};

struct audio_package_info {
    u8 channel;
    u32 sample_rate;
    u32 msec;
    u32 bit_rate;
    u32 frame_head_len;
    void *file;
    const struct audio_rec_vfs_ops *vfs_ops;
};

struct audio_encoder_info {
    u8 channel;
    u16 frame_len;
    u32 sample_rate;
    u32 msec;
    u32 bit_rate;
    u32 frame_head_len;
    void *priv;
    const struct audio_enc_buf_ops *buf_ops;
};

struct audio_package_ops {
    const char *name;
    void *(*open)(struct audio_package_info *info);
    int (*pack)(void *, void *data, int len);
    int (*get_info)(void *, struct audio_package_info *info);
    int (*close)(void *);
};

struct audio_encoder_ops {
    const char *name;
    void *(*open)(struct audio_encoder_info *info);
    int (*encode)(void *, struct audio_pcm_data *pcm);
    int (*close)(void *);
};

struct as_packet_ops {
    int (*put_packet)(void *priv, void *buff, int len);
};

struct as_audio_rec {
    u8 state;
    u8 channel;
    u8 volume;
    u32 sample_rate;
    u32 bit_rate;
    u32 msec;
    void *frame_buffer;
    u32 fb_size;
    void *dev_buffer;
    u32 db_size;
    FILE *file;
    u32 fsize;
    const char *format;
    const char *source;
    const struct audio_rec_vfs_ops *vfs_ops;
    void *priv;
    int packet_len;
    const struct as_packet_ops  *packet_ops;
    void *aec_attr;
};

struct as_audio_frame {
    u8 state;
    u8 noblock;
    u32 timeout;
    u32 msec;
    void *data;
    int len;
};

union audio_rec_req {
    struct as_audio_rec rec;
    struct as_audio_frame frame;
};

#define REGISTER_AUDIO_ENCODER(ops) \
        const struct audio_encoder_ops ops sec(.audio_encoder)

#define REGISTER_AUDIO_PACKAGE(ops) \
        const struct audio_package_ops ops sec(.audio_package)

extern const struct audio_encoder_ops audio_encoder_begin[];
extern const struct audio_encoder_ops audio_encoder_end[];

#define list_for_each_audio_encoder(p) \
    for (p = audio_encoder_begin; p < audio_encoder_end; p++)


extern const struct audio_package_ops audio_package_begin[];
extern const struct audio_package_ops audio_package_end[];

#define list_for_each_audio_package(p) \
    for (p = audio_package_begin; p < audio_package_end; p++)



#endif
