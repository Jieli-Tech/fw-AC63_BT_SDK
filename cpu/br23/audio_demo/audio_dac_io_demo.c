/*
 ****************************************************************
 *							AUDIO_DAC_IO_DEMO
 * File  : audio_dac_io_demo.c
 * By    :
 * Notes : dac做普通io使用的demo
 * Usage : 在app_main()中调用 audio_dac_io_test()即可测试该模块
 *
 ****************************************************************
 */
#include "media/includes.h"
#include "app_config.h"
#include "audio_config.h"
#include "asm/includes.h"
#define DAC_IO_L 0x1
#define DAC_IO_R 0x2
#define DAC_IO_LR 0x3

#if (TCFG_AUDIO_DAC_ENABLE || TCFG_AUDIO_ENABLE)

extern struct audio_dac_hdl dac_hdl;
extern struct dac_platform_data dac_data;
extern s16 read_capless_DTB(void);
extern void _audio_dac_trim_hook(u8 pos);

__attribute__((aligned(4))) s16 dac_io_buff[16];
__attribute__((aligned(4))) s16 dac_temp_buff[4];

/* 将数据写进dac */
static int audio_dac_io_write(struct audio_dac_hdl *dac, s16 *data, u32 len)
{
    u16 swp = 0;
    u32 free_points = 0;
    u32 points = len / 2 / dac->channel;
    s16 *wptr = NULL;
    u32 wlen = 0;
    free_points = JL_AUDIO->DAC_SWN;
    swp = JL_AUDIO->DAC_SWP;
    if (swp + free_points > JL_AUDIO->DAC_LEN) {
        free_points = JL_AUDIO->DAC_LEN - swp;
    }
    if (points > free_points) {
        points = free_points;
    }
    wptr = (s16 *)(JL_AUDIO->DAC_ADR + swp * 2 * dac->channel);
    wlen = points * 2 * dac->channel;
    memcpy(wptr, data, wlen);
    JL_AUDIO->DAC_SWN = points;
    __asm_csync();
    return wlen;
}

/* 打开dac_io模块 */
void audio_dac_io_open(void)
{
    audio_dac_init(&dac_hdl, &dac_data);
    s16 dacr32 = read_capless_DTB();
    audio_dac_set_capless_DTB(&dac_hdl, dacr32);
    audio_dac_set_buff(&dac_hdl, dac_io_buff, sizeof(dac_io_buff));

    struct audio_dac_trim dac_trim;
    int len = syscfg_read(CFG_DAC_TRIM_INFO, (void *)&dac_trim, sizeof(dac_trim));
    if (len != sizeof(dac_trim) || dac_trim.left == 0 || dac_trim.right == 0) {
        audio_dac_do_trim(&dac_hdl, &dac_trim, 0);
        syscfg_write(CFG_DAC_TRIM_INFO, (void *)&dac_trim, sizeof(dac_trim));
    } else {
        _audio_dac_trim_hook(5);
    }
    audio_dac_set_trim_value(&dac_hdl, &dac_trim);
    audio_dac_set_delay_time(&dac_hdl, 1, 2);

    audio_dac_set_sample_rate(&dac_hdl, 48000);
    audio_dac_start(&dac_hdl);

    JL_AUDIO->DAC_CON |= BIT(6);
    JL_AUDIO->DAC_CON |= BIT(5);
    os_time_dly(5);

    audio_dac_ch_analog_gain_set(&dac_hdl, 0x3, 31);
    audio_dac_ch_digital_gain_set(&dac_hdl, 0x3, 16384);

    JL_AUDIO->DAC_CON &= ~(BIT(12) | BIT(13) | BIT(14) | BIT(15));
    printf(">> dac_hdl.channel:%d\n", dac_hdl.channel);
}

/* 设置dac_io的输出以及峰峰值 */
void audio_dac_io_set(u8 ch, u8 value)
{
    int i = 0;
    for (i = 0; i < dac_hdl.channel; i++) {
        if (ch & BIT(i)) {
            /* printf("i = %d, into ch BIT\n", i); */
            dac_temp_buff[i] = value ? (32767) : (-32768); //vpp
        }
        /* printf("> io[%d] = %d\n", i, dac_temp_buff[i]); */
    }
    audio_dac_io_write(&dac_hdl, dac_temp_buff, dac_hdl.channel * 2);
}

/* dac_io使用示例 */
void audio_dac_io_test(void)
{
    static bool value = 0;
    audio_dac_io_open();
    while (1) {
        value ^= 1;
        audio_dac_io_set(DAC_IO_LR, value);
        os_time_dly(1);
    }
}

#endif

