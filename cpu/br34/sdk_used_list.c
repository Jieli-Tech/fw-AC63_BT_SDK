#include "app_config.h"

#if TCFG_USER_TWS_ENABLE
tws_acl_data_sync
tws_event_sync
tws_conn_sync
tws_lmp_slot_sync
tws_media_sync
tws_sync_call
tws_link_sync

#ifndef CONFIG_256K_FLASH
tws_local_media_sync
tws_afh_sync
#if TCFG_EQ_ONLINE_ENABLE
tws_ci_data
#endif /* #if TCFG_EQ_ONLINE_ENABLE */
tws_power_balance
#endif

#endif

#if CONFIG_FATFS_ENABLE
fat_vfs_ops
#endif

#if VFS_ENABLE
sdfile_vfs_ops
#endif

#if TCFG_NORFLASH_DEV_ENABLE && VFS_ENABLE
nor_fs_vfs_ops
nor_sdfile_vfs_ops
#endif

#if FLASH_INSIDE_REC_ENABLE
inside_nor_fs_vfs_ops
#endif


#if 0
sbc_decoder
msbc_decoder
sbc_hwaccel
#else
sbc_hw_decoder
msbc_hw_decoder
#endif
cvsd_decoder
pcm_decoder

#if TCFG_DEC_MTY_ENABLE
mty_decoder
#endif

#if TCFG_DEC_MP3_ENABLE
mp3_decoder
#if TCFG_DEC2TWS_ENABLE
mp3pick_decoder
#endif
#endif

#if TCFG_DEC_WMA_ENABLE
wma_decoder
#if TCFG_DEC2TWS_ENABLE
wmapick_decoder
#endif
#endif

#if TCFG_DEC_FLAC_ENABLE
flac_decoder
#endif

#if TCFG_DEC_APE_ENABLE
ape_decoder
#endif

#if TCFG_DEC_M4A_ENABLE
m4a_decoder
#endif

#if TCFG_DEC_AMR_ENABLE
amr_decoder
#endif

#if TCFG_DEC_DTS_ENABLE
dts_decoder
#endif

#if TCFG_DEC_G729_ENABLE || TCFG_BT_SUPPORT_G729
g729_decoder
#endif

#if TCFG_DEC_AAC_ENABLE || TCFG_BT_SUPPORT_AAC
aac_decoder
#endif

#if TCFG_DEC_G726_ENABLE
g726_decoder
#endif

#if TCFG_DEC_WTGV2_ENABLE
wtgv2_decoder
#endif

#if TCFG_DEC_SPEEX_ENABLE
speex_decoder
#endif

#if TCFG_DEC_OPUS_ENABLE
opus_decoder
#endif

#if TCFG_DEC_MIDI_ENABLE
midi_decoder
#endif

#if (TCFG_DEC_WAV_ENABLE || TCFG_WAV_TONE_MIX_ENABLE)
wav_decoder
#endif


cvsd_encoder
msbc_hw_encoder

#if TCFG_ENC_OPUS_ENABLE
opus_encoder
#endif

#if TCFG_ENC_SPEEX_ENABLE
speex_encoder
#endif


#if TCFG_ENC_SBC_ENABLE
sbc_encoder
#endif


#if TCFG_DEC_LC3_ENABLE
lc3_decoder
#endif

#if TCFG_ENC_LC3_ENABLE
lc3_encoder
#endif

#if TCFG_ENC_ADPCM_ENABLE
adpcm_encoder
#endif



#if 0       //deep_sleep
bt
tmr
uart
os
#endif

