/*****************************************************************
>file name : include_lib/media/includes.h
>author : lichao
>create time : Mon 26 Nov 2018 07:47:14 PM CST
*****************************************************************/
#ifndef __MEDIA_INCLUDES_H__
#define __MEDIA_INCLUDES_H__

#ifdef CONFIG_AUDIO_ONCHIP

#include "media/audio_decoder.h"
#include "media/audio_encoder.h"
#include "media/mixer.h"
#include "media/automute.h"
#include "media/audio_stream.h"
#include "effectrs_sync.h"
#include "audio_resample.h"

#include "asm/cpu_includes.h"
#include "application/eq_func_define.h"
/*
#include "asm/dac.h"
#include "asm/audio_adc.h"
#if (defined CONFIG_CPU_BR26 || \
     defined CONFIG_CPU_BR23 || \
     defined CONFIG_CPU_BR21 || \
     defined CONFIG_CPU_BR25 || \
	 defined CONFIG_CPU_BR30)
#include "asm/audio_src.h"
#endif
*/

#if defined CONFIG_CPU_BR23 || \
    defined CONFIG_CPU_BR25
#include "asm/audio_spdif.h"
#endif

#endif/*CONFIG_AUDIO_ONCHIP*/
#endif
