#include "app_config.h"

#if TCFG_USER_TWS_ENABLE
tws_local_media_sync
tws_afh_sync
tws_ci_data
tws_acl_data_sync
tws_event_sync
tws_conn_sync
tws_lmp_slot_sync
tws_low_latency
tws_media_sync
tws_power_balance
tws_sync_call
tws_tx_sync
tws_link_sync
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

sbc_decoder
msbc_decoder
sbc_hwaccel
cvsd_decoder
pcm_decoder

//mty_decoder

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

#if TCFG_DEC_G729_ENABLE
g729_decoder
#endif

#if TCFG_DEC_AAC_ENABLE
aac_decoder
#endif

#if TCFG_DEC_G726_ENABLE
g726_decoder
#endif

#if TCFG_DEC_MIDI_ENABLE
midi_decoder
#endif

#if TCFG_DEC_WAV_ENABLE
wav_decoder
#endif



