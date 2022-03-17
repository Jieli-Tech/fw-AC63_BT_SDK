
	. = ALIGN(4);
	audio_sync_data_begin = .;
        *(.audio_sync_data)
	audio_sync_data_end = .;
	. = ALIGN(4);
	*(.jlsp_data)
    *(.sbc_data)
    *(.msbc_data)
    *(.cvsd_data)
    *(.aac_data)
	*(.sbc_eng_data)
*(.sbc_encoder_data)
	*(.bt_audioplc_data)
    *(.lc3_decoder_data)
#if (TCFG_ENC_LC3_ENABLE || TCFG_DEC_LC3_ENABLE)
		*(.lc3_codec_ari_c_const)
		*(.lc3_codec_ari_c_data)
		*(.lc3_codec_c_data)
#endif
   *(.wtgv2_data)
    *(.wtgv2dec_data)

    *(.anc_data)
    *(.anc_user_data)
    *(.anc_box_data)
    *(.anc_btspp_data)
    *(.anc_debug_data)

	*(.sms_data)
    *(.lf_audioplc_data)
    	*(.audio_track_data)
