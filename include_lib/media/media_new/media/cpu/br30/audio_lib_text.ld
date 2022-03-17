
		. = ALIGN(4);
        media_text_start = .;

		. = ALIGN(4);
        *(.dns_16k_data)
        *(.dns_8k_data)
		*(.jlsp_const)
		*(.jlsp_code)
        /*
		*(.aec_code)
		*(.aec_const)

		. = ALIGN(4);
		*(.res_code)
		*(.res_const)

		. = ALIGN(4);
		*(.ns_code)
		*(.ns_const)

		*(.nlp_code)
		*(.nlp_const)
		*(.der_code)
		*(.der_const)
		*(.qmf_code)
		*(.qmf_const)
		*(.fft_code)
		*(.fft_const)
        */

    	anc_code_begin = .;
		*(.anc_code)
		*(.anc_const)
		*(.anc_user_code)
		*(.anc_user_const)
		*(.anc_box_code)
		*(.anc_box_const)
		*(.anc_btspp_code)
		*(.anc_btspp_const)
		*(.anc_debug_code)
		*(.anc_debug_const)
    	anc_code_end = .;
		anc_code_size = anc_code_end - anc_code_begin;

		*(.ns_sparse_code )
		*(.aec_sparse_code)
		*(.nlp_sparse_code)
		*(.der_sparse_code)
		*(.qmf_sparse_code)
		*(.bt_audioplc_code)
		*(.bt_audioplc_sparse_code)
		*(.bt_audioplc_const)
		*(.dms_sparse_code)
	    *(.bark_const)

		*(.sms_const);
		*(.sms_sparse_code);
		*(.sms_code);

		. = ALIGN(4);
        *(.pcm_code)
        *(.pcm_const)
        *(.cvsd_code)
        *(.cvsd_const)
        *(.g729_code)
        *(.g729_const)
		*(.wtgv2_code)
		*(.wtgv2_const)
		*(.wtgv2dec_code)
		*(.wtgv2dec_const)
		*(.wtgv2dec_str)
		*(.wtg_decv2_sparse_code)
        *(.mp3_code)
        *(.mp3_const)
        *(.mp3_dec_code)
        *(.mp3_dec_const)
        *(.mp3_dec_sparse_code)

        *(.mp3_decstream_code)
        *(.mp3_decstream_const)
        *(.mp3_decstream_sparse_code)

        *(.adpcm_encode_sparse_code)
        *(.adpcm_encode_code)
        *(.adpcm_encode_const) 
        /**(.bfilt_code)*/
		*(.msbc_code)
		*(.msbc_const)
		/* *(.mty_code) */
		/* *(.mty_const) */
		*(.sbc_code)
		*(.sbc_const)
 *(.sbc_eng_code)
 *(.sbc_eng_const)
*(.sbc_encoder_code)
		*(.sine_code)
		*(.sine_const)
        *(.wav_code)
        *(.wav_const)
		*(.wav_dec_code)
		*(.wav_dec_const)
		*(.wav_dec_sparse_code)
		*(.wav_dec_sparse_const)
        *(.wma_code)
        *(.wma_const)
		*(.wma_dec_code)
		*(.wma_dec_const)
		*(.wma_dec_sparse_code)
		*(.wma_dec_sparse_const)
        *(.wma_decstream_code)
        *(.wma_decstream_const)
        *(.wma_decstream_sparse_code)
#ifdef CONFIG_MOVABLE_ENABLE
    	audio_sync_code_begin = .;
        *(.audio_sync_code)
    	audio_sync_code_end = .;
#endif

        *(.aac_code)
        *(.aac_const)
		*(.amr_code)
		*(.amr_const)
		/* *(.ape_code) */
		/* *(.ape_const) */
		*(.audio_decoder_code)
		*(.audio_decoder_const)
        *(.audio_track_const)
        *(.audio_track_code)
		/* *(.cvsd_code) */
		/* *(.cvsd_const) */
		/* *(.dts_code) */
		/* *(.dts_const) */
		/* *(.flac_code) */
		/* *(.flac_const) */
		/* *(.m4a_code) */
		/* *(.m4a_const) */
		*(.media_device_code)
		*(.media_device_const)
		*(.audio_encoder_code)
		*(.audio_encoder_const)
		*(.mixer_code)
		*(.mixer_const)
		*(.dec_server_code)
		*(.dec_server_const)
		*(.rec_server_code)
		*(.rec_server_const)
		*(.auto_mute_code)
		*(.auto_mute_const)
		*(.plc_code)
		*(.plc_const)
		*(.wireless_sync_code)
		*(.wireless_sync_const)
		*(.sbc_eng_code)
		*(.sbc_eng_const)

		*(.audio_codec_code)

		*(.bt_compressor_sparse_const)
		*(.bt_compressor_sparse_code)
		*(.compressor_sparse_code)
		*(.compressor_sparse_const)

		*(.bt_limiter_sparse_const)
		*(.bt_limiter_sparse_code)
		*(.limiter_sparse_code)
		*(.limiter_sparse_const)

		*(.bt_crossOver_sparse_const)
		*(.bt_crossOver_sparse_code)
		*(.crossOver_sparse_code)
		*(.crossOver_sparse_const)

		. = ALIGN(4);
    	_audio_decoder_begin = .;
    	PROVIDE(audio_decoder_begin = .);
    	    *(.audio_decoder)
    	_audio_decoder_end = .;
    	PROVIDE(audio_decoder_end = .);

    	_audio_encoder_begin = .;
    	PROVIDE(audio_encoder_begin = .);
    	    *(.audio_encoder)
    	_audio_encoder_end = .;
    	PROVIDE(audio_encoder_end = .);

    	_audio_package_begin = .;
    	PROVIDE(audio_package_begin = .);
    	    *(.audio_package)
    	_audio_package_end = .;
    	PROVIDE(audio_package_end = .);

    	_audio_dev_begin = .;
    	PROVIDE(audio_dev_begin = .);
    	    *(.audio_device)
    	_audio_dev_end = .;
    	PROVIDE(audio_dev_end = .);

    	_audio_hwaccel_begin = .;
    	PROVIDE(audio_hwaccel_begin = .);
    	    *(.audio_hwaccel)
    	_audio_hwaccel_end = .;
    	PROVIDE(audio_hwaccel_end = .);

		. = ALIGN(4);
    	media_code_begin = .;
    	    *(.media.*.text)
		. = ALIGN(4);
		*(.compressor_code)
		*(.compressor_const)
		*(.bt_compressor_const)
		*(.bt_compressor_code)

		*(.crossOver_code)
		*(.crossOver_const)
		*(.bt_crossOver_const)
		*(.bt_crossOver_code)

		*(.limiter_code)
		*(.limiter_const)
		*(.bt_limiter_const)
		*(.bt_limiter_code)
       	*(.drc_const)
		*(.drc_code)
		*(.drc_sparse_code)
		*(.drc_sparse_const)
	    . = ALIGN(4);
		*(.sur_cal_const)
		*(.sur_cal_code)
	    . = ALIGN(4);
	    *(.lib_pitchshift_code)
	    *(.lib_pitchshift_const)
	    . = ALIGN(4);
        *(.lc3_decoder_code)
        *(.lc3_decoder_const)
#if (TCFG_ENC_LC3_ENABLE || TCFG_DEC_LC3_ENABLE)
		*( .lc3_codec_c_sparse_code)
		*(.lc3_codec_d_code)
        	/* *(.lc3_codec_c_const) */
		/* *(.lc3_codec_c_code) */
#endif


        *(.lf_audioplc_sparse_code)
        *(.lf_audioplc_code)
        *(.lf_audioplc_const)
	    . = ALIGN(4);
        *(.speex_code)
	    *(.opus_code)
	    *(.opus_encoder_code)
	    *(.speex_encoder_code)
	    *(.resample_fastcal_sparse_code)
            *(.resample_fastcal_code)
            *(.resample_fastcal_const)
	    . = ALIGN(4);

    	media_code_end = .;
		. = ALIGN(4);
		media_code_size = media_code_end - media_code_begin;

		. = ALIGN(4);
        media_text_end = .;
