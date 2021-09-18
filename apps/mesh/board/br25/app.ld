#include "app_config.h"

SECTIONS
{
    .text : ALIGN(4)
    {
        . = ALIGN(4);
        *(.dts_dec_const)
        *(.dts_const)
        *(.dts_dec_code)
        *(.dts_code)
        *(.dts_dec_sparse_code)
        *(.dts_dec_sparse_const)
        *(.dts_dec_ff_const)

        . = ALIGN(4);
        *(.m4a_const)
        *(.m4a_code)
        *(.m4a_dec_const)
        *(.m4a_dec_code)
        *(.m4a_dec_sparse_code)
        *(.m4a_dec_sparse_const)
		*(.m4a_dec_ff_const)

        . = ALIGN(4);
        *(.aac_const)
        *(.aac_code)
        *(.bt_aac_dec_eng_const)
        *(.bt_aac_dec_eng_code)
        *(.bt_aac_dec_core_code)
        *(.bt_aac_dec_core_sparse_code)

        . = ALIGN(4);
        *(.alac_const)
        *(.alac_code)

        . = ALIGN(4);
        gsensor_dev_begin = .;
        KEEP(*(.gsensor_dev))
        gsensor_dev_end = .;

		//mouse sensor dev begin
		. = ALIGN(4);
		OMSensor_dev_begin = .;
		KEEP(*(.omsensor_dev))
		OMSensor_dev_end = .;

		. = ALIGN(4);
        fm_dev_begin = .;
        KEEP(*(.fm_dev))
		fm_dev_end = .;

		. = ALIGN(4);
        fm_emitter_dev_begin = .;
        KEEP(*(.fm_emitter_dev))
        fm_emitter_dev_end = .;

		. = ALIGN(4);
        adapter_idev_begin = .;
        KEEP(*(.adapter_idev))
        adapter_idev_end = .;

		. = ALIGN(4);
        adapter_odev_begin = .;
        KEEP(*(.adapter_odev))
        adapter_odev_end = .;

		. = ALIGN(4);
        adapter_enc_begin = .;
        KEEP(*(.adapter_enc))
        adapter_enc_end = .;

		. = ALIGN(4);
        adapter_decoder_begin = .;
        KEEP(*(.adapter_decoder))
        adapter_decoder_end = .;

		. = ALIGN(4);
        adapter_encoder_begin = .;
        KEEP(*(.adapter_encoder))
        adapter_encoder_end = .;


#if (!TCFG_CODE_RUN_RAM_FM_MODE)
 	    *(.usr_timer_const)
		*(.usr_timer_code)
    	*(.timer_const)
		*(.timer_code)
    	*(.cbuf_const)
		*(.cbuf_code)
		*(.fm_data_code)
		*(.fm_data_const)
		. = ALIGN(4);
#endif

		/* . = ALIGN(4); */
        /* storage_device_begin = .; */
        /* KEEP(*(.storage_device)) */
        /* storage_device_end = .; */
#if  (!TCFG_LED7_RUN_RAM)
		. = ALIGN(4);
        *(.gpio_ram)
		. = ALIGN(4);
        *(.LED_code)
		. = ALIGN(4);
        *(.LED_const)
#endif

		. = ALIGN(4);
        *(.wav_dec_sparse_code)
        *(.wav_dec_sparse_const)
        *(.wav_dec_code)
        *(.wav_dec_const)
        *(.wav_const)
        *(.wav_code)
        . = ALIGN(4);
        *(.amr_const)
        *(.amr_code)
        *(.amr_dec_const)
        *(.amr_dec_code)
        *(.amr_dec_sparse_code)
        *(.amr_dec_sparse_const)
        *(.amr_dec_ff_const)
		. = ALIGN(4);
        *(.ape_dec_sparse_code)
        *(.ape_dec_sparse_const)
        *(.ape_dec_code)
        *(.ape_dec_const)
        *(.ape_bss)
        *(.ape_const)
        *(.ape_code)
        . = ALIGN(4);
        *(.flac_dec_sparse_code)
        *(.flac_dec_sparse_const)
        *(.flac_dec_code)
        *(.flac_dec_const)
        *(.flac_const)
        *(.flac_code)
        . = ALIGN(4);
    	*(.alac_dec_code)
        . = ALIGN(4);
    } > code0

    .data : ALIGN(4)
    {
        . = ALIGN(4);
        *(.data*)
#if TCFG_DEC_WAV_ENABLE
           *(.wav_dec_data)

			*(.wav_data)
#endif
        . = ALIGN(4);
    } > ram0

    .bss (NOLOAD) :ALIGN(4)
    {
        *(.usb_h_dma)   //由于usb有个bug，会导致dma写的数据超出预设的buf，最长可能写超1k，为了避免死机，所以usb dma buffer后面放一些其他模块的buff来避免死机
        *(.usb_ep0)
        *(.dec_mix_buff)
        *(.sd0_var)
        *(.sd1_var)
        *(.dac_buff)
        . = ALIGN(4);
#if TCFG_DEC_WAV_ENABLE
     	*(.wav_bss)
		*(.wav_dec_bss)
#endif
        *(.bss)
        . = ALIGN(4);
        *(.dts_dec_bss)
        *(.dts_bss)
        . = ALIGN(4);
        *(.m4a_dec_bss)
        *(.m4a_bss)
        . = ALIGN(4);
        *(.src_filt)
        *(.src_dma)

        . = ALIGN(32);
#if (USB_MEM_NO_USE_OVERLAY_EN)
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
        . = ALIGN(32);
#endif
    } > ram0

	.data_code ALIGN(32):
	{
#if (TCFG_CODE_RUN_RAM_FM_MODE)
 	    *(.usr_timer_const)
		*(.usr_timer_code)
    	*(.timer_const)
		*(.timer_code)
    	*(.cbuf_const)
		*(.cbuf_code)
		*(.fm_data_code)
		*(.fm_data_const)
		. = ALIGN(4);
#endif


#if  (TCFG_LED7_RUN_RAM)
		. = ALIGN(4);
        *(.gpio_ram)
		. = ALIGN(4);
        *(.LED_code)
		. = ALIGN(4);
        *(.LED_const)
#endif
		. = ALIGN(4);
	} > ram0

}

