#if 1
#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "app_config.h"
#include "audio_config.h"
#include "audio_decode.h"
#include "app_main.h"
/* #include "audio_dec.h" */
/* #include "clock_cfg.h" */

struct demo_frame_decoder {
    u8 start;			// 解码开始
    u8 wait_resume;		// 需要激活
    int coding_type;	// 解码类型
    int sr;             //
    struct audio_decoder decoder;	// 解码器
    struct audio_res_wait wait;		// 资源等待句柄
    struct audio_mixer_ch mix_ch;	// 叠加句柄
    struct audio_stream *stream;	// 音频流
    struct audio_fmt *fmt;        //解码参数配置
#ifndef CONFIG_MEDIA_DEVELOP_ENABLE
    u8 remain;
    struct audio_src_handle *src_sync;
#endif
};

struct demo_frame_decoder *demo_frame_dec = NULL;
extern struct audio_mixer mixer;
extern struct audio_dac_hdl dac_hdl;
/* extern struct audio_decoder_task decode_task; */
struct audio_decoder_task decode_task;


int demo_frame_media_get_packet(u8 **frame);
void demo_frame_media_free_packet(void *_packet);
void *demo_frame_media_fetch_packet(int *len, void *prev_packet);
int demo_frame_media_get_packet_num(void);
extern void audio_pwm_open(void);

int demo_frame_dec_close(void);

struct demo_frame_media_rx_bulk {
    struct list_head entry;
    int data_len;
    u8 data[0];
};
//697n 耳机sdk 调用此解码demo时要注意时钟要大于等于48M，不然时钟容易不够跑
static u32 *demo_frame_media_buf = NULL;
static LIST_HEAD(demo_frame_media_head);
u16 demo_frame_test_tmr = 0;
static u32 demo_frame_cnt = 0;
#define DEMO_FRAME_TIME		5
#define DEC_DATA_POINTS	37//		42  //每次解码的字节数，要等于一帧编码数据的字节数
#define DEC_DATA_SAMPRATE			16000
/* #define DEC_DEMO_CLK                48*1000000L   //设置编码时候的时钟 */
static unsigned char enc_data[] = { //编码数据改下
//lc3编码后的数据
#if 0 //lc3编码后的数据
    0x28, 0x00, 0xce, 0xeb, 0xe5, 0xd6, 0xdd, 0x8b, 0xa4, 0xa4, 0x76, 0x87, 0x10, 0x7D, 0x2c, 0xaf,
    0xA0, 0x9d, 0xf4, 0x4b, 0xa6, 0x19, 0xe2, 0x89, 0xd5, 0x56, 0xaa, 0x64, 0xb4, 0xaa, 0x8f, 0x52,
    0x97, 0xba, 0xcf, 0x3a, 0x91, 0x73, 0xc4, 0x6c, 0xce, 0x4f, 0x28, 0x00, 0xe9, 0x08, 0xd8, 0xba,
    0x74, 0x83, 0xbc, 0xbc, 0xd7, 0xbf, 0x47, 0x92, 0xe1, 0x4d, 0x99, 0x24, 0xc6, 0x07, 0x30, 0x3c,
    0x40, 0xa0, 0x71, 0xf0, 0x56, 0xf0, 0xb6, 0x0f, 0x96, 0x8f, 0xfd, 0xc3, 0x35, 0x38, 0xc0, 0xdf,
    0x50, 0xe4,	0xba, 0x4f, 0x28, 0x00, 0xfb, 0x0c, 0xaf, 0x21, 0xdf, 0x89, 0x61, 0xc0, 0xe8, 0x82,
    0x8c, 0x93, 0xfa, 0x51, 0x9b, 0xa2, 0xc7, 0xee, 0xa5, 0x42, 0x1b, 0xaa, 0xaa, 0xaa, 0xb5, 0x54,
    0x69, 0x3b, 0x75, 0x6a, 0xc4, 0x38, 0xcf, 0xc0, 0x19, 0xe8, 0xa0, 0xe4, 0xb9, 0x4f, 0x28, 0x00,
    0xf9, 0xa5, 0x51, 0xef, 0x21, 0x4d, 0x48, 0x1f, 0xeb, 0x2b, 0x22, 0x37, 0x3a, 0xc3, 0x0b, 0xe1,
    0x78, 0x97, 0x73, 0x26, 0x90, 0xd4, 0xb4, 0xd6, 0xab, 0x5a, 0x2c, 0x66, 0x2e, 0xa2, 0xa4, 0x29,
    0x75, 0x86, 0xca, 0x0a, 0x20, 0xe4, 0xbb, 0x4f
#endif
#if 1  //USBC编码后的数据
    0xE6, 0x55, 0x44, 0x34, 0xDC, 0xCA, 0x9A, 0x72, 0x68, 0x6D, 0xB6,
    0x0D, 0x9B, 0x6C, 0x70, 0x86, 0xDC, 0x4B, 0x96, 0x34, 0xEE, 0x3D, 0x45, 0xEE, 0x2D, 0xCB, 0xAA,
    0x64, 0x76, 0xDB, 0x32, 0x1D, 0xB6, 0xC9, 0x68, 0x6D, 0xB0, 0xE6, 0x55, 0x44, 0x44, 0x82, 0x3A,
    0x44, 0x37, 0x25, 0x4D, 0x1C, 0x94, 0x36, 0xD5, 0x82, 0xCD, 0xB5, 0x1C, 0x43, 0x6D, 0x52, 0xEB,
    0x1A, 0x6B, 0x8E, 0xA2, 0xE7, 0x8A, 0xE5, 0xD4, 0x99, 0x3B, 0x6D, 0x4C, 0x8E, 0xDB, 0x50, 0xE6,
    0x55, 0x44, 0x44, 0x25, 0xC3, 0x6D, 0x60, 0x8E, 0x91, 0x0D, 0xC9, 0x53, 0x47, 0x25, 0x0D, 0xB5,
    0x60, 0xB3, 0x6D, 0x47, 0x10, 0xDB, 0x54, 0xBA, 0xC6, 0x9A, 0xE3, 0xA8, 0xB9, 0xE2, 0xB9, 0x75,
    0x26, 0x4E, 0xDB, 0x50, 0xE6, 0x55, 0x44, 0x44, 0x32, 0x3B, 0x6D, 0x49, 0x70, 0xDB, 0x58, 0x23,
    0xA4, 0x43, 0x72, 0x54, 0xD1, 0xC9, 0x43, 0x6D, 0x58, 0x2C, 0xDB, 0x51, 0xC4, 0x36, 0xD5, 0x2E,
    0xB1, 0xA6, 0xB8, 0xEA, 0x2E, 0x78, 0xAE, 0x5D, 0x40, 0xE6, 0x55, 0x44, 0x44, 0x99, 0x3B, 0x6D,
    0x4C, 0x8E, 0xDB, 0x52, 0x5C, 0x36, 0xD6, 0x08, 0xE9, 0x10, 0xDC, 0x95, 0x34, 0x72, 0x50, 0xDB,
    0x56, 0x0B, 0x36, 0xD4, 0x71, 0x0D, 0xB5, 0x4B, 0xAC, 0x69, 0xAE, 0x3A, 0x8B, 0x90, 0xE6, 0x54,
    0x44, 0x44, 0xE2, 0xB9, 0x75, 0x26, 0x4E, 0xD3, 0x53, 0x23, 0xB6, 0xD4, 0x97, 0x0D, 0xB5, 0x82,
    0x3A, 0x24, 0x37, 0x25, 0x45, 0x1C, 0x94, 0x36, 0xD5, 0x82, 0xCD, 0x35, 0x1C, 0x43, 0x6D, 0x52,
    0xEB, 0x22, 0x60, 0xE6, 0x55, 0x44, 0x44, 0xB8, 0xEA, 0x2E, 0x78, 0xAE, 0x5D, 0x49, 0x93, 0xB6,
    0xD4, 0xC8, 0xED, 0xB5, 0x25, 0xC3, 0x6D, 0x60, 0x8E, 0x91, 0x0D, 0xC9, 0x53, 0x47, 0x25, 0x0D,
    0xB5, 0x60, 0xB3, 0x6D, 0x47, 0x10, 0xDB, 0x50, 0xE6, 0x55, 0x44, 0x44, 0x4B, 0xAC, 0x69, 0xAE,
    0x3A, 0x8B, 0x9E, 0x2B, 0x97, 0x52, 0x64, 0xED, 0xB5, 0x32, 0x3B, 0x6D, 0x49, 0x70, 0xDB, 0x58,
    0x23, 0xA4, 0x43, 0x72, 0x54, 0xD1, 0xC9, 0x43, 0x6D, 0x58, 0x2C, 0xDB, 0x50, 0xE6, 0x55, 0x44,
    0x44, 0x1C, 0x43, 0x6D, 0x52, 0xEB, 0x1A, 0x6B, 0x8E, 0xA2, 0xE7, 0x8A, 0xE5, 0xD4, 0x99, 0x3B,
    0x6D, 0x4C, 0x8E, 0xDB, 0x52, 0x5C, 0x36, 0xD6, 0x08, 0xE9, 0x10, 0xDC, 0x95, 0x34, 0x72, 0x50,
    0xDB, 0x50, 0xE6, 0x55, 0x44, 0x44, 0x60, 0xB3, 0x6D, 0x47, 0x10, 0xDB, 0x54, 0xBA, 0xC6, 0x9A,
    0xE3, 0xA8, 0xB9, 0xE2, 0xB9, 0x75, 0x26, 0x4E, 0xDB, 0x53, 0x23, 0xB6, 0xD4, 0x97, 0x0D, 0xB5,
    0x82, 0x3A, 0x44, 0x37, 0x25, 0x4D, 0x10, 0xE6, 0x55, 0x44, 0x44, 0xC9, 0x43, 0x6D, 0x58, 0x2C,
    0xDB, 0x51, 0xC4, 0x36, 0xD5, 0x2E, 0xB1, 0xA6, 0xB8, 0xEA, 0x2E, 0x78, 0xAE, 0x5D, 0x49, 0x93,
    0xB6, 0xD4, 0xC8, 0xED, 0xB5, 0x25, 0xC3, 0x6D, 0x60, 0x8E, 0x91, 0x00
#endif
};

// 解码获取数据
static int demo_frame_dec_get_frame(struct audio_decoder *decoder, u8 **frame)
{
    struct demo_frame_decoder *dec = container_of(decoder, struct demo_frame_decoder, decoder);
    u8 *packet = NULL;
    int len = 0;

    // 获取数据
    len = demo_frame_media_get_packet(&packet);
    if (len <= 0) {
        // 失败
        putchar('X');
        dec->wait_resume = 1;
        return len;
    }

    *frame = packet;

    return len;
}

// 解码释放数据空间
static void demo_frame_dec_put_frame(struct audio_decoder *decoder, u8 *frame)
{
    struct demo_frame_decoder *dec = container_of(decoder, struct demo_frame_decoder, decoder);

    if (frame) {
        demo_frame_media_free_packet((void *)(frame));
    }
}

// 解码查询数据
static int demo_frame_dec_fetch_frame(struct audio_decoder *decoder, u8 **frame)
{
    struct demo_frame_decoder *dec = container_of(decoder, struct demo_frame_decoder, decoder);
    u8 *packet = NULL;
    int len = 0;
    u32 wait_timeout = 0;

    if (!dec->start) {
        wait_timeout = jiffies + msecs_to_jiffies(500);
    }

__retry_fetch:
    packet = demo_frame_media_fetch_packet(&len, NULL);
    if (packet) {
        *frame = packet;
    } else if (!dec->start) {
        // 解码启动前获取数据来做格式信息获取等
        if (time_before(jiffies, wait_timeout)) {
            os_time_dly(1);
            goto __retry_fetch;
        }
    }

    return len;
}

static const struct audio_dec_input demo_frame_input = {
    .coding_type = AUDIO_CODING_USBC,
    .data_type   = AUDIO_INPUT_FRAME,
    .ops = {
        .frame = {
            .fget = demo_frame_dec_get_frame,
            .fput = demo_frame_dec_put_frame,
            .ffetch = demo_frame_dec_fetch_frame,
        }
    }
};


// 解码预处理
static int demo_frame_dec_probe_handler(struct audio_decoder *decoder)
{
    struct demo_frame_decoder *dec = container_of(decoder, struct demo_frame_decoder, decoder);

    if (demo_frame_media_get_packet_num() < 1) {
        // 没有数据时返回负数，等有数据时激活解码
#if defined(CONFIG_MEDIA_DEVELOP_ENABLE)
        audio_decoder_suspend(decoder);
#else
        audio_decoder_suspend(decoder, 0);
#endif
        dec->wait_resume = 1;
        return -EINVAL;
    }
    return 0;
}

// 解码后处理
static int demo_frame_dec_post_handler(struct audio_decoder *decoder)
{
    return 0;
}

//解码后输出
#ifndef CONFIG_MEDIA_DEVELOP_ENABLE
static int demo_frame_dec_output_handler(struct audio_decoder *decoder, s16 *data, int len, void *priv)
{
    struct demo_frame_decoder *dec = container_of(decoder, struct demo_frame_decoder, decoder);
    char err = 0;
    int rlen = len;
    int wlen = 0;


    if (decoder == NULL) {
        r_printf("decoder NULL");
    }
    wdt_clear();
    printf("demo data len:%d \n", len);
    put_buf(data, len);
    return len;

}
#endif

static const struct audio_dec_handler demo_frame_dec_handler = {
    .dec_probe  = demo_frame_dec_probe_handler,
    .dec_post   = demo_frame_dec_post_handler,
#ifndef CONFIG_MEDIA_DEVELOP_ENABLE
    .dec_output = demo_frame_dec_output_handler,
#endif

};


// 解码释放
static void demo_frame_dec_release(void)
{
    // 删除解码资源等待
    audio_decoder_task_del_wait(&decode_task, &demo_frame_dec->wait);
    /* clock_remove(DEC_SBC_CLK); */
    // 释放空间
    local_irq_disable();
    free(demo_frame_dec);
    demo_frame_dec = NULL;
    local_irq_enable();
}

// 解码关闭
static void demo_frame_audio_res_close(void)
{
    if (demo_frame_dec->start == 0) {
        printf("demo_frame_dec->start == 0");
        return ;
    }

    // 关闭数据流节点
    demo_frame_dec->start = 0;
#if 0
    audio_decoder_close(&demo_frame_dec->decoder);
    audio_mixer_ch_close(&demo_frame_dec->mix_ch);

    // 先关闭各个节点，最后才close数据流
#if defined(CONFIG_MEDIA_DEVELOP_ENABLE)
    if (demo_frame_dec->stream) {
        audio_stream_close(demo_frame_dec->stream);
        demo_frame_dec->stream = NULL;
    }
#endif
    app_audio_state_exit(APP_AUDIO_STATE_MUSIC);
#endif
}

// 解码事件处理
static void demo_frame_dec_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_DEC_EVENT_END:
        printf("AUDIO_DEC_EVENT_END\n");
        demo_frame_dec_close();
        break;
    }
}

// 解码数据流激活
static void demo_frame_out_stream_resume(void *p)
{
    struct demo_frame_decoder *dec = (struct demo_frame_decoder *)p;

    audio_decoder_resume(&dec->decoder);
}

// 收到数据后的处理
void demo_frame_media_rx_notice_to_decode(void)
{
    if (demo_frame_dec && demo_frame_dec->start && demo_frame_dec->wait_resume) {
        demo_frame_dec->wait_resume = 0;
        audio_decoder_resume(&demo_frame_dec->decoder);
    }
}

// 解码start
static int demo_frame_dec_start(void)
{
    int err;
    struct audio_fmt *fmt;
    struct demo_frame_decoder *dec = demo_frame_dec;

    if (!demo_frame_dec) {
        return -EINVAL;
    }

    printf("demo_frame_dec_start: in\n");

    // 打开demo_frame解码
    err = audio_decoder_open(&dec->decoder, &demo_frame_input, &decode_task);
    if (err) {
        goto __err1;
    }

    // 设置运行句柄
    audio_decoder_set_handler(&dec->decoder, &demo_frame_dec_handler);
    struct audio_fmt f = {0};
    f.coding_type = dec->coding_type;
    dec->decoder.fmt.channel = dec->fmt->channel;
    dec->decoder.fmt.sample_rate = dec->sr;
    dec->decoder.fmt.bit_rate = dec->fmt->bit_rate;
    dec->decoder.fmt.frame_len = dec->fmt->frame_len;
    err = audio_decoder_set_fmt(&dec->decoder, &f);
    if (err) {
        goto __err2;
    }
    // 获取解码格式
    err = audio_decoder_get_fmt(&dec->decoder, &fmt);
    if (err) {
        goto __err2;
    }

    // 使能事件回调
    audio_decoder_set_event_handler(&dec->decoder, demo_frame_dec_event_handler, 0);

    fmt->sample_rate = dec->sr;
#if 0
#if defined(CONFIG_MEDIA_DEVELOP_ENABLE)
    // 设置输出声道类型
    audio_decoder_set_output_channel(&dec->decoder, audio_output_channel_type());
    // 配置mixer通道参数
    audio_mixer_ch_open_head(&dec->mix_ch, &mixer); // 挂载到mixer最前面
    audio_mixer_ch_set_src(&dec->mix_ch, 1, 0);
    audio_mixer_ch_set_no_wait(&dec->mix_ch, 1, 20); // 超时自动丢数
    /* audio_mixer_ch_set_sample_rate(&dec->mix_ch, fmt->sample_rate); */

    // 数据流串联
    struct audio_stream_entry *entries[8] = {NULL};
    u8 entry_cnt = 0;
    entries[entry_cnt++] = &dec->decoder.entry;
    // 添加自定义数据流节点等
    // 最后输出到mix数据流节点
    entries[entry_cnt++] = &dec->mix_ch.entry;
    // 创建数据流，把所有节点连接起来
    dec->stream = audio_stream_open(dec, demo_frame_out_stream_resume);
    audio_stream_add_list(dec->stream, entries, entry_cnt);

    // 设置音频输出类型
    audio_output_set_start_volume(APP_AUDIO_STATE_MUSIC);
#else
    // 设置输出声道类型
    enum audio_channel channel;
    int dac_output = audio_dac_get_channel(&dac_hdl);
    if (dac_output == DAC_OUTPUT_LR) {
        channel = AUDIO_CH_LR;
    } else if (dac_output == DAC_OUTPUT_MONO_L) {
        channel = AUDIO_CH_L;
    } else if (dac_output == DAC_OUTPUT_MONO_R) {
        channel = AUDIO_CH_R;
    } else {
        channel = AUDIO_CH_DIFF;
    }
    audio_decoder_set_output_channel(&dec->decoder, channel);
    audio_mixer_ch_open(&dec->mix_ch, &mixer);
    audio_mixer_ch_set_resume_handler(&dec->mix_ch, (void *)&dec->decoder, (void (*)(void *))audio_decoder_resume);
    audio_mixer_ch_set_sample_rate(&dec->mix_ch, fmt->sample_rate);
    app_audio_state_switch(APP_AUDIO_STATE_MUSIC, get_max_sys_vol());
    app_audio_set_volume(APP_AUDIO_STATE_MUSIC, 16, 1);
#endif
#endif
    // 开始解码
    dec->start = 1;
    err = audio_decoder_start(&dec->decoder);
    if (err) {
        goto __err3;
    }

    clk_set("sys", 96 * 1000000L);
    /* clock_set_cur(); */

    return 0;

__err3:
    dec->start = 0;

#if 0
    audio_mixer_ch_close(&dec->mix_ch);

    // 先关闭各个节点，最后才close数据流
#if defined(CONFIG_MEDIA_DEVELOP_ENABLE)
    if (dec->stream) {
        audio_stream_close(dec->stream);
        dec->stream = NULL;
    }
#endif

#endif

__err2:
    audio_decoder_close(&dec->decoder);
__err1:
    demo_frame_dec_release();

    return err;
}

// 解码资源等待回调
static int demo_frame_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;

    y_printf("demo_frame_wait_res_handler: %d\n", event);

    if (event == AUDIO_RES_GET) {
        // 可以开始解码
        err = demo_frame_dec_start();
    } else if (event == AUDIO_RES_PUT) {
        // 被打断
        if (demo_frame_dec->start) {
            demo_frame_audio_res_close();
        }
    }
    return err;
}

// 打开解码
int demo_frame_dec_open(int sr, u32 coding_type, struct audio_fmt *fmt) //采样率，解码类型，fmt里面配置除采样率和解码类型外的参数
{
    struct demo_frame_decoder *dec;

    if (demo_frame_dec) {
        return 0;
    }

    printf("demo_frame_dec_open \n");

    dec = zalloc(sizeof(*dec));
    ASSERT(dec);

    /* clock_add(DEC_SBC_CLK); */

    demo_frame_dec = dec;

    struct audio_fmt fmt_default = { //lc3解码需要配置这三个参数
        .channel = 1,
        .frame_len = 50,
        .bit_rate = 64000
    };

    if (fmt != NULL) {
        dec->fmt = fmt;
    } else {
        dec->fmt = &fmt_default;

    }
    dec->sr = sr;  //采样率
    dec->coding_type = coding_type;	// 解码类型
    dec->wait.priority = 4;		// 解码优先级
    dec->wait.preemption = 0;	// 不使能直接抢断解码
    dec->wait.snatch_same_prio = 1;	// 可抢断同优先级解码
    dec->wait.handler = demo_frame_wait_res_handler;
    audio_decoder_task_add_wait(&decode_task, &dec->wait);

    return 0;
}

// 关闭解码
int demo_frame_dec_close(void)
{
    if (!demo_frame_dec) {
        return 0;
    }

    if (demo_frame_dec->start) {
        demo_frame_audio_res_close();
    }
    demo_frame_dec_release();

    /* clock_set_cur(); */
    printf("demo_frame_dec_close: exit\n");
    return 1;
}


// 获取frame数据
int demo_frame_media_get_packet(u8 **frame)
{
    struct demo_frame_media_rx_bulk *p;
    local_irq_disable();
    if (demo_frame_media_head.next != &demo_frame_media_head) {
        p = list_entry((&demo_frame_media_head)->next, typeof(*p), entry);
        list_del(&p->entry);
        *frame = p->data;
        local_irq_enable();
        return p->data_len;
    }
    local_irq_enable();

    return 0;
}

// 释放frame数据
void demo_frame_media_free_packet(void *data)
{
    struct demo_frame_media_rx_bulk *rx = container_of(data, struct demo_frame_media_rx_bulk, data);

    local_irq_disable();
    list_del(&rx->entry);
    local_irq_enable();

    lbuf_free(rx);
}

// 检查frame数据
void *demo_frame_media_fetch_packet(int *len, void *prev_packet)
{
    struct demo_frame_media_rx_bulk *p;
    local_irq_disable();
    if (demo_frame_media_head.next != &demo_frame_media_head) {
        if (prev_packet) {
            p = container_of(prev_packet, struct demo_frame_media_rx_bulk, data);
            if (p->entry.next != &demo_frame_media_head) {
                p = list_entry(p->entry.next, typeof(*p), entry);
                *len = p->data_len;
                local_irq_enable();
                return p->data;
            }
        } else {
            p = list_entry((&demo_frame_media_head)->next, typeof(*p), entry);
            *len = p->data_len;
            local_irq_enable();
            return p->data;
        }
    }
    local_irq_enable();

    return NULL;
}

// 获取数据量
int demo_frame_media_get_packet_num(void)
{
    struct demo_frame_media_rx_bulk *p;
    u32 num = 0;
    local_irq_disable();
    list_for_each_entry(p, &demo_frame_media_head, entry) {
        num++;
    }
    local_irq_enable();
    return num;
}


/* __attribute__((weak)) */
/* void demo_frame_media_rx_notice_to_decode(void) */
/* { */
/* } */

// 用timer模拟填数
static void demo_frame_test_time_func(void *param)
{
    struct demo_frame_media_rx_bulk *p;
    while (1) {
        p = lbuf_alloc((struct lbuff_head *)demo_frame_media_buf, sizeof(*p) + DEC_DATA_POINTS);
        if (!p) {
            break;
        }
        u32 data_num = DEC_DATA_POINTS > sizeof(enc_data) ? sizeof(enc_data) : DEC_DATA_POINTS;
        // 填数
        p->data_len = data_num;
        memcpy(p->data, &enc_data[demo_frame_cnt * data_num], data_num);


        demo_frame_cnt ++;
        if (demo_frame_cnt >= sizeof(enc_data) / data_num) {


            demo_frame_cnt  = 0;
        }

        local_irq_disable();
        list_add_tail(&p->entry, &demo_frame_media_head);
        local_irq_enable();
        // 告诉上层有数据
        demo_frame_media_rx_notice_to_decode();
    }
}

// 模拟定时关闭
static void demo_frame_test_close(void *param)
{
    y_printf("%s,%d \n", __func__, __LINE__);
    if (demo_frame_test_tmr) {
        sys_timer_del(demo_frame_test_tmr);
        demo_frame_test_tmr = 0;
    }
    demo_frame_dec_close();
    local_irq_disable();
    list_del_init(&demo_frame_media_head);
    if (demo_frame_media_buf) {
        free(demo_frame_media_buf);
        demo_frame_media_buf = NULL;
    }
    local_irq_enable();
}

void demo_frame_test(void);
static int demo_frame_test_again(int param)
{
    demo_frame_test();
    return 0;
}

// 模拟定时关闭
static void demo_frame_test_to_close(void *param)
{
    demo_frame_test_close(NULL);
#if 1
    y_printf("demo_frame_sbc_test_to_close, again \n");
    int argv[3];
    argv[0] = (int)demo_frame_test_again;
    argv[1] = 1;
    argv[2] = (int)0;
    os_taskq_post_type("app_core", Q_CALLBACK, ARRAY_SIZE(argv), argv);
#endif
}

void demo_frame_test(void)
{
    printf("%s,%d \n", __func__, __LINE__);
    demo_frame_test_close(NULL);
    // 申请空间
    u32 buf_size = 2 * 1024;
    void *buf = malloc(buf_size);
    ASSERT(buf);
    // 初始化lbuf
    local_irq_disable();
    demo_frame_media_buf = buf;
    lbuf_init(demo_frame_media_buf, buf_size, 4, 0);
    local_irq_enable();
    demo_frame_test_time_func(NULL);
    // 用timer模拟填数
    demo_frame_test_tmr = sys_timer_add(NULL, demo_frame_test_time_func, DEMO_FRAME_TIME);
    y_printf("id:%d \n", demo_frame_test_tmr);
    // 模拟定时关闭
    /* sys_timeout_add(NULL, demo_frame_test_close, 10 * 1000); */
    // 启动解码
    demo_frame_dec_open(DEC_DATA_SAMPRATE, AUDIO_CODING_USBC, NULL); //创建一个解码
}

int audio_dec_init()
{
    int err;

    printf("audio_dec_init\n");


    tone_play_init();
    // 创建解码任务
    err = audio_decoder_task_create(&decode_task, "audio_dec");


    audio_pwm_open();


    return err;
}
#endif
