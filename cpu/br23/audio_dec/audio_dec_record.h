#ifndef _AUDIO_DEC_RECORD_H_
#define _AUDIO_DEC_RECORD_H_

#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "media/audio_decoder.h"

// 录音文件播放
int record_file_play(void);
// 指定路径播放录音文件
int record_file_play_by_path(char *path);
// 关闭录音文件播放
void record_file_close(void);
// 获取录音播放总事件
int record_file_get_total_time(void);
// 获取录音播放当前时间
int record_file_dec_get_cur_time(void);

#endif /*_AUDIO_DEC_RECORD_H_*/

