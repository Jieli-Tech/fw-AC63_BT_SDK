

SECTIONS
{
    .data : ALIGN(4)
    {
        media_data_start = .;

        . = ALIGN(4);
		EQ_COEFF_BASE = . ;
		. = EQ_COEFF_BASE + 4 * EQ_SECTION_NUM * 14;

        . = ALIGN(4);
        *(.aec_data)
        . = ALIGN(4);
        *(.res_data)
        . = ALIGN(4);
        *(.ns_data)
        . = ALIGN(4);
        *(.der_data)
        . = ALIGN(4);
        *(.nlp_data)
        . = ALIGN(4);
        *(.qmf_data)
        . = ALIGN(4);
        *(.fft_data)
        . = ALIGN(4);
		*(.bt_audioplc_data)
		*(.sms_data)

        *(.pcm_data)
		/* *(.cvsd_data) */
		*(.g729_data)
		*(.g726_data)
		*(.wtg_dec_data)
		*(.mp3_data)
		*(.mp3_dec_data)
		*(.msbc_data)
		*(.mty_data)
		*(.mp3tsy_dec_data)
		*(.sbc_data)
		*(.sine_data)
		/* *(.wav_data) */
		*(.wma_data)
		*(.wma_dec_data)
		/* *(.aac_data) */
		*(.amr_data)
		*(.midi_data)
		/* *(.ape_data) */
		*(.audio_decoder_data)
		/* *(.dts_data) */
		/* *(.flac_data) */
		/* *(.m4a_data) */
        *(.mp3_encode_data)
		*(.media_device_data)
		*(.audio_encoder_data)
		*(.mixer_data)
		*(.dec_server_data)
		*(.rec_server_data)
		*(.auto_mute_data)
		*(.plc_data)
		*(.wireless_sync_data)

		*(.resample_cal_data)
		*(.bt_compressor_data)
		*(.bt_crossOver_data)
		*(.bt_limiter_data)
		*(.bt_compressor_sparse_data)
		*(.bt_limiter_sparse_data)
		*(.bt_crossOver_sparse_data)

        . = ALIGN(4);
		*(.dec2tws_mem)

#ifdef CONFIG_CPU_BR23
        . = ALIGN(4);
		*(.sbc_eng_code)
        . = ALIGN(4);
#endif
		/* *(.crossOver_code) */
		/* *(.crossOver_const) */
		/* *(.bt_crossOver_const) */
		/* *(.bt_crossOver_code) */
		/* . = ALIGN(4); */

		/* *(.compressor_code) */
		/* *(.compressor_const) */
		/* *(.bt_compressor_const) */
		/* *(.bt_compressor_code) */
		/* . = ALIGN(4); */

		/* *(.limiter_code) */
		/* *(.limiter_const) */
		/* *(.bt_limiter_const) */
		/* *(.bt_limiter_code) */
		/* . = ALIGN(4); */

        media_data_end = .;

    } > ram0

    .bss (NOLOAD) :ALIGN(4)
    {
        media_bss_start = .;
        . = ALIGN(4);
		*(.aec_bss)
		*(.res_bss)
		*(.ns_bss)
		*(.der_bss)
		*(.nlp_bss)
		*(.qmf_bss)
		*(.bt_audioplc_bss)
		*(.fft_bss)
		*(.sms_bss)
        *(.pcm_bss)
		/* *(.cvsd_bss) */
		*(.g729_bss)
		*(.g726_bss)
		*(.wtg_dec_bss)
		*(.mp3_bss)
		*(.mp3_dec_bss)
		*(.msbc_bss)
		*(.mty_bss)
		*(.mp3tsy_dec_bss)
		*(.sbc_bss)
		*(.sine_bss)
		/* *(.wav_bss) */
		*(.wma_bss)
		*(.wma_dec_bss)
		/* *(.aac_bss) */
		*(.amr_bss)
		*(.midi_bss)
		/* *(.ape_bss) */
		*(.audio_decoder_bss)
		/* *(.dts_bss) */
		/* *(.flac_bss) */
		/* *(.m4a_bss) */
        *(.mp3_encode_bss)
		*(.media_device_bss)
		*(.audio_encoder_bss)
		*(.mixer_bss)
		*(.dec_server_bss)
		*(.rec_server_bss)
		*(.auto_mute_bss)
		*(.plc_bss)
		*(.wireless_sync_bss)

		*(.audio_buf)

		*(.resample_cal_bss)
		*(.bt_crossOver_bss)
		*(.bt_compressor_bss)
		*(.bt_limiter_bss)
		*(.bt_limiter_sparse_bss)
		*(.bt_crossOver_sparse_bss)
		*(.bt_compressor_sparse_bss)
        media_bss_end = .;
    } > ram0

#ifdef CONFIG_CPU_BR25
	.data_code ALIGN(32):
	{
        media_data_code_start = .;
        . = ALIGN(4);
		*(.sbc_eng_code)
        . = ALIGN(4);
        media_data_code_end = .;

	} > ram0
#endif

    .text : ALIGN(4)
    {
		. = ALIGN(4);
        media_text_start = .;

		*(.aec_code)
		*(.aec_const)
		*(.der_code)
		*(.der_const)
		*(.nlp_code)
		*(.nlp_const)
		*(.qmf_code)
		*(.qmf_const)
		*(.bt_audioplc_sparse_code)
		*(.bt_audioplc_code)
		*(.bt_audioplc_const)

		. = ALIGN(4);
		*(.res_code)
		*(.res_const)

		. = ALIGN(4);
		*(.ns_code)
		*(.ns_const)
		*(.opcore_maskrom)

		. = ALIGN(4);
        *(.pcm_code)
        *(.pcm_const)
		/* *(.cvsd_code) */
		/* *(.cvsd_const) */
		*(.g729_code)
		*(.g729_const)
		*(.g726_code)
		*(.g726_const)

		*(.wtg_dec_code)
		*(.wtg_dec_const)
		*(.wtg_dec_sparse_code)
		*(.wtg_dec_sparse_const)
        *(.bfilt_code)
        . = ALIGN(4);
#ifndef CONFIG_MP3_WMA_LIB_SPECIAL
		*(.mp3_decstream_const)
			*(.mp3_decstream_code)
			*(.mp3_decstream_sparse_code)
			*(.mp3_decstream_sparse_const)

			*(.mp3_dec_sparse_code)
			*(.mp3_dec_sparse_const)

			*(.mp3_dec_code)
			*(.mp3_dec_const)
			/* *(.mp3_dec_data) */
			/* *(.mp3_dec_bss) */

			/* *(.mp3_bss) */
			/* *(.mp3_data) */
			*(.mp3_const)

#if SOUNDCARD_ENABLE
#else
			*(.mp3_code)
#endif
#endif

		*(.msbc_code)
		*(.msbc_const)
		*(.mty_code)
		*(.mty_const)
		*(.mp3tsy_dec_code)
		*(.mp3tsy_dec_const)
		*(.mp3tsy_dec_sparse_code)
		*(.mp3tsy_dec_sparse_const)
		*(.sbc_code)
		*(.sbc_const)
		*(.sine_code)
		*(.sine_const)
		/* *(.wav_code) */
		/* *(.wav_const) */

#ifndef CONFIG_MP3_WMA_LIB_SPECIAL
		*(.wma_code)
		*(.wma_const)
		*(.wma_dec_code)
		*(.wma_dec_const)
#endif
		*(.wma_dec_sparse_code)
		*(.wma_dec_sparse_const)
		*(.wma_decstream_const)
		*(.wma_decstream_code)
		*(.wma_decstream_sparse_code)
		*(.wma_decstream_sparse_const)
		/* *(.aac_code) */
		/* *(.aac_const) */
		*(.amr_code)
		*(.amr_const)
		*(.midi_code)
		*(.midi_const)

		/* *(.ape_code) */
		/* *(.ape_const) */
		*(.audio_decoder_code)
		*(.audio_decoder_const)
		/* *(.cvsd_code) */
		/* *(.cvsd_const) */
		/* *(.dts_code) */
		/* *(.dts_const) */
		/* *(.flac_code) */
		/* *(.flac_const) */
		/* *(.m4a_code) */
		/* *(.m4a_const) */
        *(.mp3_encode_code)
        *(.mp3_encode_const)
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
		*(.fft_code)
		*(.fft_const)


		*(.mp3_enc_code)
		*(.mp3_enc_const)
		*(.mp3_enc_sparse_code)
		*(.mp3_enc_sparse_const)
		. = ALIGN(4);
		*(.mp2_encode_code)
		*(.mp2_encode_const)
		*(.mp2_encode_sparse_code)
		*(.mp2_encode_sparse_const)
		. = ALIGN(4);
		*(.adpcm_encode_code)
		*(.adpcm_encode_const)
		*(.adpcm_encode_sparse_code)
		*(.adpcm_encode_sparse_const)
		. = ALIGN(4);


		*(.m4a_dec_sparse_code)
		*(.m4a_dec_sparse_const)

	 	*(.aac_dec_sparse_code)
		*(.aac_dec_sparse_const)
		*(.bt_aac_dec_core_const)

		*(.amr_dec_const)


		*(.dts_dec_ff_const)
		*(.dts_dec_sparse_code)
		*(.dts_dec_sparse_const)

		*(.res_sparse_code)
		*(.ns_sparse_code )
		*(.aec_sparse_code)
		*(.nlp_sparse_code)
		*(.der_sparse_code)
		*(.qmf_sparse_code)

		*(.sms_const);
		*(.sms_sparse_code);
		*(.sms_code);


		*(.resample_cal_code)
		*(.resample_cal_sparse_code)
		*(.resample_cal_const)

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

		*(.audio_codec_code)

		. = ALIGN(4);
    	_audio_decoder_begin = .;
    	PROVIDE(audio_decoder_begin = .);
    	    KEEP(*(.audio_decoder))
    	_audio_decoder_end = .;
    	PROVIDE(audio_decoder_end = .);

    	_audio_encoder_begin = .;
    	PROVIDE(audio_encoder_begin = .);
    	    KEEP(*(.audio_encoder))
    	_audio_encoder_end = .;
    	PROVIDE(audio_encoder_end = .);

    	_audio_package_begin = .;
    	PROVIDE(audio_package_begin = .);
    	    KEEP(*(.audio_package))
    	_audio_package_end = .;
    	PROVIDE(audio_package_end = .);

    	_audio_dev_begin = .;
    	PROVIDE(audio_dev_begin = .);
    	    KEEP(*(.audio_device))
    	_audio_dev_end = .;
    	PROVIDE(audio_dev_end = .);

    	_audio_hwaccel_begin = .;
    	PROVIDE(audio_hwaccel_begin = .);
    	    KEEP(*(.audio_hwaccel))
    	_audio_hwaccel_end = .;
    	PROVIDE(audio_hwaccel_end = .);

		. = ALIGN(4);
    	media_code_begin = .;
    	    *(.media.*.text)

		. = ALIGN(4);
		*(.crossOver_code)
		*(.crossOver_const)
		*(.bt_crossOver_const)
		*(.bt_crossOver_code)
		. = ALIGN(4);

		*(.compressor_code)
		*(.compressor_const)
		*(.bt_compressor_const)
		*(.bt_compressor_code)
		. = ALIGN(4);

		*(.limiter_code)
		*(.limiter_const)
		*(.bt_limiter_const)
		*(.bt_limiter_code)
		. = ALIGN(4);
		*(.lib_pitchshift_code)
		*(.lib_pitchshift_const)
		. = ALIGN(4);
		*(.sur_cal_const)
		*(.sur_cal_code)
		. = ALIGN(4);
		*(.reverb_cal_sparse_code)
#if (SOUNDCARD_ENABLE == 0)
		*(.mix_ram_code)
        . = ALIGN(4);
		*(.dvol_ram_code)
        . = ALIGN(4);
		*(.reverb_cal_const)
  . = ALIGN(4);
			*(.reverb_cal_code)
. = ALIGN(4);
		*(.stream_ram_code)

#endif
	
		

        . = ALIGN(4);

    	media_code_end = .;
		. = ALIGN(4);
		media_code_size = media_code_end - media_code_begin;

		. = ALIGN(4);
        media_text_end = .;

    } > code0
}

        media_code_total_size = media_text_end - media_text_start;





