#ifndef __LP_TOUCH_KEY_EPD_H__
#define __LP_TOUCH_KEY_EPD_H__

enum {
    EPD_IN = 0,
    EPD_OUT = 1,
    EPD_STATE_NO_CHANCE
};

enum {
    EPD_REF_VALLEY = 0,
    EPD_REF_PEAK = 1,
};

#define  EPD_HIST_N_MAX 256
#define  EPD_HISTORY_N_MAX 20
#define  EPD_IIR_ALPHA_Q 10
#define  EPD_IIR_Q 5

typedef struct {
    short N;
    short min;
    short scale;
    short decay_frames;
    short decay_count;
    short hist[EPD_HIST_N_MAX];
} HistTracker;

void HistTracker_init(HistTracker *h, int N, int min, int scale, int decay_frames);
void HistTracker_update(HistTracker *h, int d);
void HistTracker_reset(HistTracker *h);
void HistTracker_decay(HistTracker *h);
int HistTracker_get_peak(HistTracker *h);

typedef struct {
    int prev;
    int alpha;
    int one_minus_alpha;
} IirFilter;

void IirFilter_init(IirFilter *h, int alpha);
void IirFilter_reset(IirFilter *h);
int IirFilter_update(IirFilter *h, int d);
int IirFilter_get_value(IirFilter *h);
int IirFilter_prev_is_None(IirFilter *h);

typedef struct {
    IirFilter iir;
    short stable_nframes;
    short up_th;
    short low_th;
    short slope_interval;
    short slope_th;
    short frame_count;
    short history_size;
    short history_index;
    short history_valid_len;
    short history[EPD_HISTORY_N_MAX];
} PeakTracker;

void PeakTracker_init(PeakTracker *h, int alpha, int stable_nframes, int up_th, int low_th, int slope_interval, int slope_th);
int PeakTracker_update(PeakTracker *h, int y);
void  PeakTracker_reset(PeakTracker *h);
int  PeakTracker_get_value(PeakTracker *h);

typedef struct {
    short epd_startup_skip_nframes;// = 5  # 开启的时候跳过几帧数据

    short epd_startup_stable_nframes;// = self.epd_startup_skip_nframes + 20  # 开启的时候采集初始数据的帧数

    short epd_peak_skip_nframes;// = 10  # 从入耳到出耳时，跳过一些数据不去跟踪计算
    short epd_valley_skip_nframes;// = 50  # 从出耳到入耳时，跳过一些数据不去跟踪计算

    short epd_peak_ref_transit_nframes;// = 100  # 从入耳到出耳状态，阈值计算时参考 波谷值来计算的帧数
    short epd_valley_ref_transit_nframes;// = 500  # 从出耳到入耳状态，阈值计算时参考 波峰值来计算的帧数

    short epd_peak_tracker_stable_nframes;// = 10  # 波峰跟踪参数
    short epd_peak_up_th;// = 10  # 波峰跟踪参数
    short epd_peak_low_th;// = -20  # 波峰跟踪参数
    short epd_peak_slope_interval;// = 5  # 波峰计算斜率的间隔
    short epd_peak_slope_th;// = -6  # 波峰跟踪参数

    short epd_peak_iir_coeff;// = 0.005  # 波峰跟踪时iir滤波器的系数

    short epd_up_th_iir_coeff;// = 0.005  # 对阈值做平滑的系数
    short epd_down_th_iir_coeff;// = 0.005  # 对阈值做平滑的系数
    short epd_hist_N;// = 160  # 波谷跟踪直方图的点数
    short epd_hist_min;// = 4300  # 波谷跟踪直方图的起始点
    short epd_hist_scale;// = 4  # 直方图统计下采样
    short epd_hist_decay_frames;// = 500  # 直方图跟新的间隔
    short epd_hist_update_interval;// = 60 #波谷值多少帧获取一次
    short epd_up_th_ref_peak;// = -80  # 相对于波峰的高阈值
    short epd_down_th_ref_peak;// = -150  # 相对于波峰的低阈值
    short epd_up_th_ref_valley;// = 220  # 相对于波谷的高阈值
    short epd_down_th_ref_valley;// = 120  # 相对于波谷的低阈值
} EPD_Parameters;

typedef struct {
    EPD_Parameters p;
    HistTracker hist_tracker;
    PeakTracker peak_tracker;
    IirFilter down_th_iir;
    IirFilter up_th_iir;
    short state;
    short ref;
    short cur_state_frame_count;
    short frame_count;

    short tracking_raw;
    short tracking_peak;
    short tracking_valley;
    short tracking_up_th;
    short tracking_down_th;

    short hist_update_frame_count;

} EarphoneDetection;

void EarphoneDetection_get_default_params(EPD_Parameters *p);
void EarphoneDetection_init(EarphoneDetection *h, EPD_Parameters *p);
int EarphoneDetection_process(EarphoneDetection *h, int y);

void lp_touch_key_epd_init();
u8 lp_touch_key_epd_update_res(int res);

#endif /* #ifndef __LP_TOUCH_KEY_EPD_H__ */
