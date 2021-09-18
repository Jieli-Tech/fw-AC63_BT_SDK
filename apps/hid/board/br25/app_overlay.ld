
	OVERLAY : NOCROSSREFS AT(0x200000) SUBALIGN(4)
    {
		.overlay_aec
		{
			*(.cvsd_data)
			*(.cvsd_const)
			*(.cvsd_code)

			LONG(0xFFFFFFFF);
			/* KEEP(*(.aec_bss_id)) */
			o_aec_end = .;

            *(.cvsd_codec)
			*(.aec_mem)
            *(.msbc_enc)
			*(.cvsd_bss)
#if (RECORDER_MIX_EN)
			*(.enc_file_mem)
#endif/*RECORDER_MIX_EN*/

#if TCFG_BLUETOOTH_BACK_MODE == 0
            . = ALIGN(4);
            *(.bd_base)

            *(.bredr_rxtx_bulk)
            acl_tx_pool = .;
            *(.bredr_tx_bulk)
#ifdef CONFIG_BT_TX_BUFF_SIZE
            acl_tx_pool_end = acl_tx_pool + CONFIG_BT_TX_BUFF_SIZE;
#else
            acl_tx_pool_end = acl_tx_pool;
#endif
            . = acl_tx_pool_end;

            acl_rx_pool = .;
            *(.bredr_rx_bulk)
#ifdef CONFIG_BT_RX_BUFF_SIZE
            acl_rx_pool_end = acl_rx_pool + CONFIG_BT_RX_BUFF_SIZE;
#else
            acl_rx_pool_end = acl_rx_pool;
#endif
            . = acl_rx_pool_end;


            tws_bulk_pool = .;
#ifdef CONFIG_TWS_BULK_POOL_SIZE
            tws_bulk_pool_end = tws_bulk_pool + CONFIG_TWS_BULK_POOL_SIZE;
#else
            tws_bulk_pool_end = tws_bulk_pool;
#endif
            . = tws_bulk_pool_end;
#endif
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
			/* KEEP(*(.mp3_bss_id)) */
			o_mp3_end = .;

			*(.mp3_mem)
			*(.mp3_ctrl_mem)
			*(.mp3pick_mem)
			*(.mp3pick_ctrl_mem)
		}
		.overlay_wma
		{
#ifdef CONFIG_MP3_WMA_LIB_SPECIAL
			*(.wma_dec_code)
			*(.wma_dec_const)
	   /*      *(.wma_dec_data) */
			/* *(.wma_dec_bss) */

			/* *(.wma_bss) */
			/* *(.wma_data) */
			*(.wma_const)
			*(.wma_code)
#endif
			LONG(0xFFFFFFFF);
			/* KEEP(*(.wma_bss_id)) */
			o_wma_end = .;

			*(.wma_mem)
			*(.wma_ctrl_mem)
			*(.wmapick_mem)
			*(.wmapick_ctrl_mem)
		}
		.overlay_wav
		{

			LONG(0xFFFFFFFF);
			/* KEEP(*(.wav_bss_id)) */
			o_wav_end = .;


			*(.wav_mem)
			*(.wav_ctrl_mem)

		}
		.overlay_ape
        {
			LONG(0xFFFFFFFF);
			/* KEEP(*(.ape_bss_id)) */
			o_ape_end = .;

            *(.ape_mem)
            *(.ape_ctrl_mem)
			*(.ape_dec_data)
			*(.ape_dec_bss)

			*(.ape_data)
		}
		.overlay_flac
        {
			LONG(0xFFFFFFFF);
			/* KEEP(*(.flac_bss_id)) */
			o_flac_end = .;

            *(.flac_mem)
            *(.flac_ctrl_mem)
			*(.flac_dec_data)
			*(.flac_dec_bss)

			*(.flac_bss)
			*(.flac_data)
		}
		.overlay_m4a
        {
			LONG(0xFFFFFFFF);
			/* KEEP(*(.m4a_bss_id)) */
			o_m4a_end = .;

            *(.m4a_mem)
            *(.m4a_ctrl_mem)
			*(.m4a_dec_data)
			*(.m4a_data)
			*(.m4apick_mem)
			*(.m4apick_ctrl_mem)

			*(.aac_ctrl_mem)
			*(.aac_bss)
			*(.aac_data)

			*(.alac_ctrl_mem)
			*(.alac_bss)
			*(.alac_data)
		}

		.overlay_amr
        {
			LONG(0xFFFFFFFF);
			/* KEEP(*(.amr_bss_id)) */
			o_amr_end = .;

            *(.amr_mem)
            *(.amr_ctrl_mem)
		}
		.overlay_dts
        {
			LONG(0xFFFFFFFF);
			/* KEEP(*(.dts_bss_id)) */
			o_dts_end = .;

            *(.dts_mem)
            *(.dts_ctrl_mem)
			*(.dts_dec_data)
			*(.dts_data)
		}

		.overlay_fm
		{
			*(.fm_code)
			LONG(0xFFFFFFFF);
			/* KEEP(*(.fm_bss_id)) */
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

