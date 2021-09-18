#include "app_config.h"

SECTIONS
{
    .text : ALIGN(4)
    {
		. = ALIGN(4);
        gsensor_dev_begin = .;
        KEEP(*(.gsensor_dev))
        gsensor_dev_end = .;

		. = ALIGN(4);
        codec_dev_begin = .;
        KEEP(*(.codec_dev))
        codec_dev_end = .;

		. = ALIGN(4);
        hrsensor_dev_begin = .;
        KEEP(*(.hrsensor_dev))
        hrsensor_dev_end = .;

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

		/* . = ALIGN(4); */
        /* storage_device_begin = .; */
        /* KEEP(*(.storage_device)) */
        /* storage_device_end = .; */

		/* . = ALIGN(4); */
		/* ui_main_begin = .; */
		/* KEEP(*(.ui_main)) */
		/* ui_main_end = .; */
		/* . = ALIGN(4); */

        chargeIc_dev_begin = .;
        KEEP(*(.chargeIc_dev))
            chargeIc_dev_end = .;
        . = ALIGN(4);

#if TCFG_DEC_WAV_ENABLE
        	*(.wav_dec_sparse_code)
			*(.wav_dec_sparse_const)

			*(.wav_dec_code)
			*(.wav_dec_const)
	        *(.wav_const)
			*(.wav_code)
#endif

		. = ALIGN(4);
        *(.text*)
        *(.LOG_TAG_CONST*)
        *(.rodata*)
		. = ALIGN(4);
    } > code0

    .data : ALIGN(4)
    {
        *(.data*)

		*(.cvsd_data)

		. = ALIGN(4);
		dec_board_param_mem_begin = .;
		KEEP(*(.dec_board_param_mem))
		dec_board_param_mem_end = .;

        . = ALIGN(4);
		*(.sbc_eng_code)
        . = ALIGN(4);

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

        *(.src_filt)
        *(.src_dma)

		. = ALIGN(4);
#if TCFG_DEC_WAV_ENABLE
     	*(.wav_bss)
		*(.wav_dec_bss)
#endif
		. = (( . + 31) / 32 * 32);

		. = ALIGN(4);
    } > ram0

}

