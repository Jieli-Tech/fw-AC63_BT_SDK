/*****************************************************************
>file name : audio_codec_clock.c
>create time : Thu 03 Jun 2021 09:36:12 AM CST
*****************************************************************/
#define LOG_TAG         "[CODEC_CLK]"
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#include "debug.h"
#include "app_config.h"
#include "system/includes.h"
#include "audio_codec_clock.h"

#define AUDIO_CODING_ALL        (0xffffffff)
#define MAX_CODING_TYPE_NUM     5 //可根据模式的解码格式需求扩展
#define AUDIO_CODEC_BASE_CLK    SYS_96M

static LIST_HEAD(codec_clock_head);

struct audio_codec_clock {
    u32 coding_type;
    u32 clk;
};

struct audio_codec_clk_context {
    u8 mode;
    struct audio_codec_clock params;
    struct list_head entry;
};


const struct audio_codec_clock audio_clock[AUDIO_MAX_MODE][MAX_CODING_TYPE_NUM] = {
    {
        //A2DP_MODE
        {
            AUDIO_CODING_SBC,
#if (TCFG_BT_MUSIC_EQ_ENABLE && ((EQ_SECTION_MAX >= 10) && AUDIO_OUT_EFFECT_ENABLE)) || (AUDIO_VBASS_CONFIG || AUDIO_SURROUND_CONFIG)
            SYS_64M,
#elif (TCFG_BT_MUSIC_EQ_ENABLE && ((EQ_SECTION_MAX >= 10) || AUDIO_OUT_EFFECT_ENABLE))
#if A2DP_AUDIO_PLC_ENABLE
            SYS_64M,
#else
            SYS_48M,
#endif
#else
#if (TCFG_AUDIO_ANC_ENABLE)
            SYS_24M,
#else
            SYS_48M,
#endif
#endif
        },
        {
            AUDIO_CODING_AAC,
#if (TCFG_BT_MUSIC_EQ_ENABLE && ((EQ_SECTION_MAX >= 10) || AUDIO_OUT_EFFECT_ENABLE)) | (AUDIO_VBASS_CONFIG || AUDIO_SURROUND_CONFIG)
            SYS_64M,
#else
            SYS_48M,
#endif
        }
    },

    {
        //ESCO MODE
        {AUDIO_CODING_MSBC, 0},
        {AUDIO_CODING_CVSD, 0},
    },

    {
        //TONE MODE
        {AUDIO_CODING_AAC, SYS_64M},
        {AUDIO_CODING_WAV | AUDIO_CODING_MP3, 96 * 1000000L},
        {AUDIO_CODING_ALL, 0},
    },
    {
        //MIC2PCM MODE
        {AUDIO_CODING_PCM, SYS_48M},
    },
    {
        //KWS MODE
        {AUDIO_CODING_ALL, SYS_48M},
    },
    //TODO
};

int audio_codec_clock_set(u8 mode, u32 coding_type, u8 preemption)
{
    struct audio_codec_clk_context *ctx = (struct audio_codec_clk_context *)zalloc(sizeof(struct audio_codec_clk_context));
    u32 new_clk = 0;

    if (mode >= MAX_CODING_TYPE_NUM) {
        log_error("Not support this mode : %d", mode);
        return -EINVAL;
    }

    const struct audio_codec_clock *params = audio_clock[mode];
    int i = 0;

    for (i = 0; i < MAX_CODING_TYPE_NUM; i++) {
        if (params[i].coding_type & coding_type) {
            goto match_clock;
        }
    }

    log_error("Not found right coding type : %d", coding_type);
    return -EINVAL;

match_clock:
    new_clk = params[i].clk ? params[i].clk : clk_get("sys");
    if (!preemption) {
        struct audio_codec_clk_context *ctx1;
        if (!list_empty(&codec_clock_head)) {
            ctx1 = list_first_entry(&codec_clock_head, struct audio_codec_clk_context, entry);
            if (ctx1 && new_clk <= ctx1->params.clk) {
                /*叠加解码的时钟不可小于在播放的音频时钟*/
                new_clk = ctx1->params.clk + 12 * 1000000;
            }
        }
    }
    ctx->mode = mode;
    ctx->params.coding_type = coding_type;
    ctx->params.clk = new_clk;
    clk_set("sys", new_clk);
    list_add(&ctx->entry, &codec_clock_head);

    return 0;
}

void audio_codec_clock_del(u8 mode)
{
    struct audio_codec_clk_context *ctx;
    u32 next_clk = 0;

    list_for_each_entry(ctx, &codec_clock_head, entry) {
        if (ctx->mode == mode) {
            goto clock_del;
        }
    }

    return;
clock_del:
    list_del(&ctx->entry);
    free(ctx);

    if (!list_empty(&codec_clock_head)) {
        ctx = list_first_entry(&codec_clock_head, struct audio_codec_clk_context, entry);
        if (ctx) {
            next_clk = ctx->params.clk;
        }
    }

    if (!next_clk) {
        next_clk = AUDIO_CODEC_BASE_CLK;
    }
    clk_set("sys", next_clk);
}
