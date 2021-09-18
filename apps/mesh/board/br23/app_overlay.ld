
	OVERLAY : NOCROSSREFS AT(0x200000) SUBALIGN(4)
    {

		.overlay_aec
		{
			LONG(0xFFFFFFFF);
			/* *(.aec_bss_id) */
			o_aec_end = .;

			*(.aec_mem)
            *(.msbc_enc)
		}
		.overlay_mp3
		{
#ifdef CONFIG_MP3_WMA_LIB_SPECIAL
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
			*(.mp3_code)
#endif
			LONG(0xFFFFFFFF);
			/* *(.mp3_bss_id) */
			o_mp3_end = .;

			*(.mp3_mem)
			*(.mp3_ctrl_mem)
			*(.mp3pick_mem)
			*(.mp3pick_ctrl_mem)
		}
		.overlay_wma
		{
#ifdef CONFIG_MP3_WMA_LIB_SPECIAL
			*(.wma_code)
			*(.wma_const)
			*(.wma_dec_code)
			*(.wma_dec_const)
			/* *(.wma_dec_data) */
			/* *(.wma_dec_bss) */

			/* *(.wma_bss) */
			/* *(.wma_data) */
#endif
			LONG(0xFFFFFFFF);
			/* *(.wma_bss_id) */
			o_wma_end = .;

			*(.wma_mem)
			*(.wma_ctrl_mem)
			*(.wmapick_mem)
			*(.wmapick_ctrl_mem)
		}
		.overlay_wav
		{
						LONG(0xFFFFFFFF);
			/* *(.wav_bss_id) */

			*(.wav_mem)
			*(.wav_ctrl_mem)
		}

		.overlay_ape
        {
            *(.ape_mem)
            *(.ape_ctrl_mem)

			*(.ape_dec_sparse_code)
			*(.ape_dec_sparse_const)

			*(.ape_dec_code)
			*(.ape_dec_const)
			*(.ape_dec_data)
			*(.ape_dec_bss)

			LONG(0xFFFFFFFF);
			/* *(.ape_bss_id) */

			*(.ape_bss)
			*(.ape_data)
			*(.ape_const)
			*(.ape_code)

		}
		.overlay_flac
        {
			*(.flac_dec_sparse_code)
			*(.flac_dec_sparse_const)

			*(.flac_dec_code)
			*(.flac_dec_const)
			*(.flac_dec_data)

			*(.flac_data)
			*(.flac_const)
			*(.flac_code)

			LONG(0xFFFFFFFF);
			/* *(.flac_bss_id) */

            *(.flac_mem)
            *(.flac_ctrl_mem)
			*(.flac_dec_bss)
			*(.flac_bss)
		}

		.overlay_noise_reduction
        {
			LONG(0xFFFFFFFF);
			*(.noise_reduction)
		}

		.overlay_m4a
        {
			*(.m4a_dec_code)
			*(.m4a_dec_ff_const)
			*(.m4a_dec_const)
			*(.m4a_dec_data)

			*(.m4a_data)
			*(.m4a_const)
			*(.m4a_code)
			*(.m4apick_mem)
			*(.m4apick_ctrl_mem)

			LONG(0xFFFFFFFF);
			/* *(.m4a_bss_id) */

            *(.m4a_mem)
            *(.m4a_ctrl_mem)
			*(.m4a_dec_bss)
			*(.m4a_bss)
			*(.aac_ctrl_mem)
			*(.aac_bss)

			*(.alac_ctrl_mem)
			*(.alac_bss)
		}


		.overlay_amr
        {
			*(.amr_dec_sparse_code)
			*(.amr_dec_sparse_const)
			*(.amr_dec_code)

			*(.amr_dec_data)

			LONG(0xFFFFFFFF);
			/* *(.amr_bss_id) */

            *(.amr_mem)
            *(.amr_ctrl_mem)
			*(.amr_dec_bss)
		}
		.overlay_dts
        {
			*(.dts_dec_code)
			*(.dts_dec_data)

			*(.dts_data)
			*(.dts_const)
			*(.dts_code)

			LONG(0xFFFFFFFF);
			/* *(.dts_bss_id) */

            *(.dts_mem)
            *(.dts_ctrl_mem)
			*(.dts_dec_bss)
			*(.dts_bss)

		}



		.overlay_fm
		{
			LONG(0xFFFFFFFF);
			/* *(.fm_bss_id) */
			o_fm_end = .;

			*(.fm_mem)
			*(.linein_pcm_mem)
		}
        .overlay_pc
		{
#if (!USB_MEM_NO_USE_OVERLAY_EN)
            *(.usb_audio_play_dma)
            *(.usb_audio_rec_dma)
            *(.uac_rx)
            *(.mass_storage)

            *(.usb_msd_dma)
            *(.usb_hid_dma)
            *(.usb_iso_dma)
            *(.usb_cdc_dma)
            *(.uac_var)
            *(.usb_config_var)
            *(.cdc_var)
#endif
		}
    } > ram0




