
	. = ALIGN(4);
	audio_sync_bss_begin = .;
        *(.audio_sync_bss)
	audio_sync_bss_end = .;
	. = ALIGN(4);
	*(.jlsp_bss)
    *(.cvsd_bss)
    *(.aac_bss)
	*(.sbc_eng_bss)
	*(.bt_audioplc_bss)
    *(.lc3_decoder_bss)
#if (TCFG_ENC_LC3_ENABLE || TCFG_DEC_LC3_ENABLE)
	*(.lc3_codec_ari_c_bss)
	*(.lc3_codec_c_bss)
#endif
    *(.wtgv2_bss)
    *(.wtgv2dec_bss)

    *(.anc_bss)
    *(.anc_user_bss)
    *(.anc_box_bss)
    *(.anc_btspp_bss)
    *(.anc_debug_bss)
    *(.lf_audioplc_bss)
    	*(.audio_track_bss)
