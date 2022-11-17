/*
 *****************************************************************
 *
 * Audio 变采样使用demo
 *
 *****************************************************************
 */
#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "app_config.h"
#include "audio_config.h"
#include "audio_decode.h"
#include "app_main.h"
#include "Resample_api.h"
/* #include "audio_dec.h" */
/* #include "clock_cfg.h" */


static RS_PARA_STRUCT  rs_para_obj;
static RS_STUCT_API   *test_rs_api;

static s16 sin_48k[48] = {
    0, 2139, 4240, 6270, 8192, 9974, 11585, 12998,
    14189, 15137, 15826, 16244, 16384, 16244, 15826, 15137,
    14189, 12998, 11585, 9974, 8192, 6270, 4240, 2139,
    0, -2139, -4240, -6270, -8192, -9974, -11585, -12998,
    -14189, -15137, -15826, -16244, -16384, -16244, -15826, -15137,
    -14189, -12998, -11585, -9974, -8192, -6270, -4240, -2139
};

static s16 output_buf[96] = {0};



void audio_resample_demo(void)
{
    rs_para_obj.nch = 2; //双声道
    rs_para_obj.new_insample = 48000; //输入采样率
    rs_para_obj.new_outsample = 32000;//输出采样率
    test_rs_api = get_rsfast_context();
    s32 rs_bufsize = test_rs_api->need_buf();
    s16 *rs_buf = malloc(rs_bufsize);
    test_rs_api->open(rs_buf, &rs_para_obj);
    s16 *inbuf =  sin_48k;  //输入数据的buffer
    s16 *obuf =  output_buf; //输出数据的buffer
    u32 len = sizeof(sin_48k) / 2; //输入数据的长度
    u32 rdlen = test_rs_api->run(rs_buf, inbuf, len, obuf);    //len是n个s16，数据在 inbuf，返回rdlen个s16 是输出数据的长度,输出数据在obuf
    put_buf(obuf, rdlen); //把输出数据打印出来
}
