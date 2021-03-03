#ifndef __LOCALTWS_H_
#define __LOCALTWS_H_

#include "application/audio_localtws.h"
#include "media/localtws_decoder.h"

#define LOCALTWS_ENC_FLAG_STREAM		BIT(0)	// 数据源是流数据


// localtws检测是否使能
int localtws_check_enable(void);

// localtws蓝牙事件处理
int localtws_bt_event_deal(struct bt_event *evt);

// 打开localtws编码
int localtws_enc_api_open(struct audio_fmt *pfmt, u32 flag);
// 关闭localtws编码
void localtws_enc_api_close(void);
// localtws编码写入
int localtws_enc_api_write(s16 *data, int len);

// localtws设置等待a2dp状态
void localtws_set_wait_a2dp_start(u8 flag);

// localtws启动（活动设备主动调用）
void localtws_start(struct audio_fmt *pfmt);
// localtws停止（活动设备主动调用）
void localtws_stop(void);

// 打开localtws解码
int localtws_dec_open(u32 value);
// 关闭localtws解码
int localtws_dec_close(u8 drop_frame_start);
// localtws已经打开
u8 localtws_dec_is_open(void);
// localtws解码激活
void localtws_dec_resume(void);
// localtws抛弃数据
int localtws_media_dat_abandon(void);
// localtws暂停
void localtws_dec_pause(void);
// localtws已经开始解码
int localtws_dec_out_is_start(void);

// localtws暂停控制
void localtws_decoder_pause(u8 pause);


#endif /*__LOCALTWS_H_*/

