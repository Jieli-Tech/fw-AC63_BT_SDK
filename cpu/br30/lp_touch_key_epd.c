#include "includes.h"
#include "lp_touch_key_epd.h"

/* #define SUPPORT_MS_EXTENSIONS */
/* #ifdef SUPPORT_MS_EXTENSIONS */
/* #pragma bss_seg(	".lp_touch_key_epd_bss") */
/* #pragma data_seg(	".lp_touch_key_epd_data") */
/* #pragma const_seg(	".lp_touch_key_epd_const") */
/* #pragma code_seg(	".lp_touch_key_epd_code") */
/* #endif */

#define     EPD_IO_DEBUG_0(i,x)       //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT &= ~BIT(x);}
#define     EPD_IO_DEBUG_1(i,x)       //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT |= BIT(x);}

static EarphoneDetection epd;

void HistTracker_init(HistTracker *h, int N, int min, int scale, int decay_frames)
{
    h->N = N;
    h->min = min;
    h->scale = scale;
    h->decay_frames = decay_frames;
    h->decay_count = 0;
    memset(h->hist, 0, sizeof(h->hist[0])*h->N);

    if (h->N > EPD_HIST_N_MAX) {
        printf("hist size is overflow\n");
    }
}

void HistTracker_update(HistTracker *h, int d)
{
    int q;

    //printf("u:%d\n", d);

    if (h->decay_count >= h->decay_frames) {
        HistTracker_decay(h);
    }
    q = (d - h->min) / h->scale;

    //printf("q:%d\n", q);

    if (q < h->N) {
        h->hist[q]++;
        h->decay_count++;
    }
}

void HistTracker_reset(HistTracker *h)
{
    h->decay_count = 0;
    memset(h->hist, 0, sizeof(h->hist[0])*h->N);
}

void HistTracker_decay(HistTracker *h)
{


    for (int i = 0; i < h->N; i++) {
        h->hist[i] = h->hist[i] / 2;
    }
    h->decay_count = 0;
}

int HistTracker_get_peak(HistTracker *h)
{

    /*for (int i = 0; i < h->N; i++)
    {
        printf("h(%d):%d\n", i, h->hist[i]);
    }*/

    int M = 0;
    int idx = 0;
    for (int i = 0; i < h->N; i++) {
        if (h->hist[i] > M) {
            M = h->hist[i];
            idx = i;
        }
    }
    idx = h->min + idx * h->scale;
    return idx;
}

void IirFilter_init(IirFilter *h, int alpha)
{
    h->alpha = alpha;
    h->one_minus_alpha = (1 << EPD_IIR_ALPHA_Q) - h->alpha;
    h->prev = -1;
}

void IirFilter_reset(IirFilter *h)
{
    h->prev = -1;
}

int IirFilter_update(IirFilter *h, int d)
{
    if (h->prev < 0) {
        h->prev = d * (1 << EPD_IIR_Q);
    } else {
        h->prev = (h->one_minus_alpha * h->prev + h->alpha * d * (1 << EPD_IIR_Q) + (1 << (EPD_IIR_ALPHA_Q - 1))) >> EPD_IIR_ALPHA_Q;
    }
    return IirFilter_get_value(h);
}

int IirFilter_get_value(IirFilter *h)
{
    return (h->prev + (1 << (EPD_IIR_Q - 1))) >> EPD_IIR_Q;
}


int IirFilter_prev_is_None(IirFilter *h)
{
    return h->prev == -1;
}

void PeakTracker_init(PeakTracker *h, int alpha, int stable_nframes, int up_th, int low_th, int slope_interval, int slope_th)
{
    IirFilter_init(&h->iir, alpha);
    h->stable_nframes = stable_nframes;
    h->up_th = up_th;
    h->low_th = low_th;
    h->slope_interval = slope_interval;
    h->slope_th = slope_th;
    h->history_size = h->slope_interval;
    h->frame_count = 0;
    h->history_index = 0;
    h->history_valid_len = 0;

    if (h->history_size > EPD_HISTORY_N_MAX) {
        printf("history_size is overflow\n");
    }
}

int PeakTracker_update(PeakTracker *h, int y)
{
    int deriv = 0;
    h->frame_count += 1;
    if (h->history_valid_len < h->history_size) {
        deriv = 0;
    } else {
        deriv = y - h->history[h->history_index];
    }
    if (h->frame_count < h->stable_nframes || IirFilter_prev_is_None(&h->iir)) {
        IirFilter_update(&h->iir, y);
    } else {
        if (deriv >= h->slope_th
            && y < IirFilter_get_value(&h->iir) + h->up_th
            && y > IirFilter_get_value(&h->iir) + h->low_th) {
            IirFilter_update(&h->iir, y);
        } else if (y >= IirFilter_get_value(&h->iir) + h->up_th) {
            PeakTracker_reset(h);
            IirFilter_update(&h->iir, y);
        }
    }

    h->history[h->history_index] = y;
    h->history_index++;
    h->history_valid_len++;
    if (h->history_index > h->history_size) {
        h->history_index = 0;
    }
    if (h->history_valid_len > h->history_size) {
        h->history_valid_len = h->history_size;
    }

    return IirFilter_get_value(&h->iir);
}

void PeakTracker_reset(PeakTracker *h)
{
    IirFilter_reset(&h->iir);
    h->history_valid_len = 0;
    h->history_index = 0;
}

int PeakTracker_get_value(PeakTracker *h)
{
    return IirFilter_get_value(&h->iir);
}

void EarphoneDetection_get_default_params(EPD_Parameters *p)
{
    p->epd_startup_skip_nframes = 5;// # 开启的时候跳过几帧数据

    p->epd_startup_stable_nframes = p->epd_startup_skip_nframes + 20;//# 开启的时候采集初始数据的帧数

    p->epd_peak_skip_nframes = 10;//# 从入耳到出耳时，跳过一些数据不去跟踪计算
    p->epd_valley_skip_nframes = 50;//# 从出耳到入耳时，跳过一些数据不去跟踪计算

    p->epd_peak_ref_transit_nframes = 100;//# 从入耳到出耳状态，阈值计算时参考 波谷值来计算的帧数
    p->epd_valley_ref_transit_nframes = 500;//# 从出耳到入耳状态，阈值计算时参考 波峰值来计算的帧数

    p->epd_peak_tracker_stable_nframes = 10;//# 波峰跟踪参数
    p->epd_peak_up_th = 10;//# 波峰跟踪参数
    p->epd_peak_low_th = -20;//# 波峰跟踪参数
    p->epd_peak_slope_interval = 5;//# 波峰计算斜率的间隔
    p->epd_peak_slope_th = -6;//# 波峰跟踪参数

    p->epd_peak_iir_coeff = 5;//# 波峰跟踪时iir滤波器的系数

    p->epd_up_th_iir_coeff = 5;//# 对阈值做平滑的系数
    p->epd_down_th_iir_coeff = 5;//# 对阈值做平滑的系数

    p->epd_hist_decay_frames = 500;//# 直方图跟新的间隔
    p->epd_hist_update_interval = 60;//#波谷值多少帧获取一次

    /////////////////////
    p->epd_hist_scale = 4;//# 直方图统计下采样
    p->epd_hist_N = 200;//# 波谷跟踪直方图的点数
    p->epd_hist_min = 5700;//# 波谷跟踪直方图的起始点

    p->epd_up_th_ref_peak = -50;//# 相对于波峰的高阈值
    p->epd_down_th_ref_peak = -100;//# 相对于波峰的低阈值

    p->epd_up_th_ref_valley = 80;//# 相对于波谷的高阈值
    p->epd_down_th_ref_valley = 60;//# 相对于波谷的低阈值
}

void EarphoneDetection_init(EarphoneDetection *h, EPD_Parameters *p)
{
    if (p != &h->p) {
        memcpy(&h->p, p, sizeof(EPD_Parameters));
    }
    HistTracker_init(&h->hist_tracker, p->epd_hist_N, p->epd_hist_min, p->epd_hist_scale, p->epd_hist_decay_frames);
    PeakTracker_init(&h->peak_tracker, p->epd_peak_iir_coeff, h->p.epd_peak_tracker_stable_nframes, h->p.epd_peak_up_th,
                     h->p.epd_peak_low_th, h->p.epd_peak_slope_interval, h->p.epd_peak_slope_th);
    IirFilter_init(&h->down_th_iir, p->epd_down_th_iir_coeff);
    IirFilter_init(&h->up_th_iir, p->epd_up_th_iir_coeff);

    h->state = EPD_OUT;
    h->ref = EPD_REF_PEAK;

    h->cur_state_frame_count = 0;
    h->frame_count = 0;

    h->tracking_raw = 0;
    h->tracking_peak = 0;
    h->tracking_valley = 0;
    h->tracking_up_th = 0;
    h->tracking_down_th = 0;
    h->hist_update_frame_count = 0;
}

int EarphoneDetection_process(EarphoneDetection *h, int y)
{
    int iir_y;
    h->frame_count++;
    if (h->frame_count < h->p.epd_startup_skip_nframes) {
        return h->state;
    }

    h->cur_state_frame_count++;
    if (h->frame_count < h->p.epd_startup_stable_nframes) {
        iir_y = PeakTracker_update(&h->peak_tracker, y);

        h->tracking_peak = iir_y;
        h->tracking_down_th = IirFilter_update(&h->down_th_iir, h->tracking_peak + h->p.epd_down_th_ref_peak);
        h->tracking_up_th = IirFilter_update(&h->up_th_iir, h->tracking_peak + h->p.epd_up_th_ref_peak);

        return h->state;
    }

    //decision
    if (h->state == EPD_OUT) {
        if (y < h->tracking_down_th) {
            h->state = EPD_IN;
            h->cur_state_frame_count = 0;
            PeakTracker_reset(&h->peak_tracker);
            HistTracker_reset(&h->hist_tracker);
        }
    } else {
        if (y > h->tracking_up_th) {
            h->state = EPD_OUT;
            h->cur_state_frame_count = 0;
            PeakTracker_reset(&h->peak_tracker);
            HistTracker_reset(&h->hist_tracker);
        }
    }

    //update trackings
    if (h->state == EPD_OUT) {
        if (h->cur_state_frame_count > h->p.epd_peak_skip_nframes) {
            iir_y = PeakTracker_update(&h->peak_tracker, y);

            if (h->ref != EPD_REF_PEAK) {
                if (h->cur_state_frame_count > h->p.epd_peak_ref_transit_nframes) {
                    h->ref = EPD_REF_PEAK;
                }
            }

            if (h->cur_state_frame_count > h->p.epd_peak_ref_transit_nframes || iir_y > h->tracking_peak) {
                if (h->ref == EPD_REF_PEAK) {
                    h->tracking_peak = iir_y;
                    h->tracking_down_th = IirFilter_update(&h->down_th_iir, h->tracking_peak + h->p.epd_down_th_ref_peak);
                    h->tracking_up_th = IirFilter_update(&h->up_th_iir, h->tracking_peak + h->p.epd_up_th_ref_peak);
                }
            }
        }
    } else {
        if (h->cur_state_frame_count > h->p.epd_valley_skip_nframes) {
            HistTracker_update(&h->hist_tracker, y);

            if (h->ref != EPD_REF_VALLEY) {
                if (h->cur_state_frame_count > h->p.epd_valley_ref_transit_nframes) {
                    h->ref = EPD_REF_VALLEY;
                    h->hist_update_frame_count = 0;
                }
            }

            if (h->cur_state_frame_count > h->p.epd_valley_ref_transit_nframes) {
                if (h->ref == EPD_REF_VALLEY) {
                    if (h->cur_state_frame_count == h->p.epd_valley_ref_transit_nframes + 1) {
                        h->hist_update_frame_count = 0;
                    }

                    if (h->hist_update_frame_count % h->p.epd_hist_update_interval == 0) {
                        h->hist_update_frame_count = 0;
                        h->tracking_valley = HistTracker_get_peak(&h->hist_tracker);
                    }
                    //printf("v:%d\n", h->tracking_valley);
                    h->tracking_down_th = IirFilter_update(&h->down_th_iir, h->tracking_valley + h->p.epd_down_th_ref_valley);
                    h->tracking_up_th = IirFilter_update(&h->up_th_iir, h->tracking_valley + h->p.epd_up_th_ref_valley);

                    h->hist_update_frame_count++;

                }
            }
        }

    }

    return h->state;
}

void lp_touch_key_epd_init()
{
    EarphoneDetection_get_default_params(&epd.p);

    EarphoneDetection_init(&epd, &epd.p);
}

u8 lp_touch_key_epd_update_res(int res)
{
    static u8 last_state = EPD_OUT;

    EPD_IO_DEBUG_1(A, 5);
    u8 state = EarphoneDetection_process(&epd, res);
    EPD_IO_DEBUG_0(A, 5);

    if (state != last_state) {
        last_state = state;
        return state;
    } else {
        return EPD_STATE_NO_CHANCE;
    }
}

