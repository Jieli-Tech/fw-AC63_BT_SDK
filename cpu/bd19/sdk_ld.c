// *INDENT-OFF*
#include "app_config.h"

/* =================  BR34 SDK memory ========================
 _______________ ___ 0x2C000(176K)
|   isr base    |
|_______________|___ _IRQ_MEM_ADDR(size = 0x100)
|rom export ram |
|_______________|
|    update     |
|_______________|___ RAM_LIMIT_H
|     HEAP      |
|_______________|___ data_code_pc_limit_H
| audio overlay |
|_______________|
|   data_code   |
|_______________|___ data_code_pc_limit_L
|     bss       |
|_______________|
|     data      |
|_______________|
|   irq_stack   |
|_______________|
|   boot info   |
|_______________|
|     TLB       |
|_______________|0 RAM_LIMIT_L

 =========================================================== */

#include "maskrom_stubs.ld"

EXTERN(
_start
#include "sdk_used_list.c"
);

//============ About RAM ================

UPDATA_SIZE     = 0x80;
UPDATA_BEG      = _MASK_MEM_BEGIN - UPDATA_SIZE;
UPDATA_BREDR_BASE_BEG = 0xeb60;

RAM_LIMIT_L     = 0;
RAM_LIMIT_H     = UPDATA_BEG;
PHY_RAM_SIZE    = RAM_LIMIT_H - RAM_LIMIT_L;

//from mask export
ISR_BASE       = _IRQ_MEM_ADDR;
ROM_RAM_SIZE   = _MASK_MEM_SIZE;
ROM_RAM_BEG    = _MASK_MEM_BEGIN;

RAM0_BEG 		= RAM_LIMIT_L;
RAM0_END 		= RAM_LIMIT_H;
RAM0_SIZE 		= RAM0_END - RAM0_BEG;


//=============== About BT RAM ===================
//CONFIG_BT_RX_BUFF_SIZE = (1024 * 18);


//============ About CODE ================
CODE_BEG = 0x1E00120;

MEMORY
{
	code0(rx)       : ORIGIN = CODE_BEG,        LENGTH = CONFIG_FLASH_SIZE
	ram0(rwx)       : ORIGIN = RAM0_BEG,        LENGTH = RAM0_SIZE
}


ENTRY(_start)

SECTIONS
{
    . = ORIGIN(code0);
    .text ALIGN(4):
    {
        PROVIDE(text_rodata_begin = .);

        *(.entry_text)
        *(.startup.text)

		*(.text)

		#include "btstack/btstack_lib_text.ld"
		#include "system/system_lib_text.ld"

		. = ALIGN(4);
	    update_target_begin = .;
	    PROVIDE(update_target_begin = .);
	    KEEP(*(.update_target))
	    update_target_end = .;
	    PROVIDE(update_target_end = .);
		. = ALIGN(4);

		*(.LOG_TAG_CONST*)
        *(.rodata*)
        *(.fat_data_code_ex)

		. = ALIGN(4); // must at tail, make rom_code size align 4
        PROVIDE(text_rodata_end = .);

        clock_critical_handler_begin = .;
        KEEP(*(.clock_critical_txt))
        clock_critical_handler_end = .;

        chargestore_handler_begin = .;
        KEEP(*(.chargestore_callback_txt))
        chargestore_handler_end = .;

        gsensor_dev_begin = .;
        KEEP(*(.gsensor_dev))
        gsensor_dev_end = .;

		. = ALIGN(4);
        //mouse sensor dev begin
        OMSensor_dev_begin = .;
        KEEP(*(.omsensor_dev))
        OMSensor_dev_end = .;

        storage_device_begin = .;
        KEEP(*(.storage_device))
        storage_device_end = .;

		/********maskrom arithmetic ****/
        *(.opcore_table_maskrom)
        *(.bfilt_table_maskroom)
        *(.opcore_maskrom)
        *(.bfilt_code)
        *(.bfilt_const)
		/********maskrom arithmetic end****/

        . = ALIGN(4);
		*(.audio_decoder_code)
		*(.audio_decoder_const)
		*(.audio_encoder_code)
		*(.audio_encoder_const)
		*(.audio_codec_code)
		. = ALIGN(4);
		*(.media.audio_decoder.text)
		*(.media.audio_encoder.text)
        . = ALIGN(4);
		*(.usbc_encoder_code)
		*(.usbc_decoder_code)
        *(.resample_fastcal_const)
        *(.resample_fastcal_code)
        *(.resample_fastcal_sparse_code)
#if (TCFG_ENC_USBC_ENABLE || TCFG_DEC_USBC_ENABLE)
		*(.usbc_codec_code)
		*(.usbc_codec_const)
#endif
        *(.adpcm_encode_sparse_code)
        *(.adpcm_encode_code)
        *(.adpcm_encode_const)
        *(.pcm_code)
        . = ALIGN(4);
		*(.wav_code)
		*(.wav_const)
		*(.wav_dec_sparse_code)
		*(.wav_dec_const)
		*(.wav_dec_code)
	    *(.wtgv2_code)
		*(.wtgv2_const)
		*(.wtgv2dec_code)
		*(.wtgv2dec_const)
		*(.wtgv2dec_str)
		*(.wtg_decv2_sparse_code)
		*(.lc3_decoder_code)
		*(.msbc_codec_const)
		*(.msbc_codec_code)
		*(.msbc_const)
		*(.msbc_code)
        *(.audio_pwm_code)
		*(.opus_code)
	    *(.opus_encoder_code)
		*(.opus_encoder_const)
		*(.opus_enc_const)
		*(.opus_enc_code)
#if (TCFG_ENC_LC3_ENABLE || TCFG_DEC_LC3_ENABLE)
		*(.lc3_codec_ari_c_const)
		*(.lc3_codec_c_const)
		*(.lc3_codec_c_sparse_code)
		*(.lc3_codec_d_code)
		*(.lc3_codec_c_code)
		*(.lc3_codec_e_code)
		*(.lc3_codec_ari_c_code)
		*(.lc3_codec_t16k_code)
#endif

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

        . = ALIGN(4);
        __VERSION_BEGIN = .;
        KEEP(*(.sys.version))
        __VERSION_END = .;

        *(.noop_version)


		. = ALIGN(32);
		m_code_addr = . ;
		*(.m.code*)
		*(.movable.code*)
			m_code_size = ABSOLUTE(. - m_code_addr) ;
		. = ALIGN(32);
	  } > code0

    . = ORIGIN(ram0);
	//TLB 起始需要16K 对齐；
    .mmu_tlb ALIGN(0x4000):
    {
        *(.mmu_tlb_segment);
    } > ram0

	.boot_info ALIGN(32):
	{
		*(.boot_info)
        . = ALIGN(32);
	} > ram0


	.data ALIGN(32):
	{
		//cpu start
        *(.data*)

        . = ALIGN(32);
		#include "btstack/btstack_lib_data.ld"
		. = ALIGN(4);
		#include "system/system_lib_data.ld"
		. = ALIGN(4);
		*(.wav_data)
		*(.usbc_encoder_data)
		*(.usbc_decoder_data)
        *(.resample_fastcal_data)
	    *(.wtgv2_data)
		*(.wtgv2dec_data)
		*(.opus_encoder_data)
        *(.opus_enc_data)
#if (TCFG_ENC_LC3_ENABLE || TCFG_DEC_LC3_ENABLE)
		*(.lc3_codec_ari_c_data)
		*(.lc3_codec_c_data)
#endif
        *(.msbc_data)
        *(.msbc_codec_data)
		. = ALIGN(4);
#if (TCFG_ENC_LC3_ENABLE || TCFG_DEC_LC3_ENABLE)
		*(.lc3_codec_ari_c_data)
		*(.lc3_codec_c_data)
#endif
		. = ALIGN(4);
	} > ram0

	.irq_stack ALIGN(32):
    {
		*(.stack_magic)
        _cpu0_sstack_begin = .;
        PROVIDE(cpu0_sstack_begin = .);
        *(.stack)
        _cpu0_sstack_end = .;
        PROVIDE(cpu0_sstack_end = .);
    	_stack_end = . ;
		*(.stack_magic0)
		. = ALIGN(4);

    } > ram0

	.bss ALIGN(32):
    {
            *(.usb_audio_play_dma)
            *(.usb_audio_rec_dma)
            *(.uac_rx)
            *(.mass_storage)
            *(.usb_h_dma)

            *(.usb_ep0)
            *(.usb_msd_dma)
            *(.usb_hid_dma)
            *(.usb_iso_dma)
            *(.usb_cdc_dma)
            *(.uac_var)
            *(.usb_config_var)
            *(.cdc_var)
        . = ALIGN(4);
	        *(.wav_bss)
			*(.wtgv2_bss)
			*(.wtgv2dec_bss)
			*(.opus_encoder_bss)
			*(.opus_enc_bss)
            *(.resample_fastcal_bss)


#if (TCFG_ENC_LC3_ENABLE || TCFG_DEC_LC3_ENABLE)
	*(.lc3_codec_ari_c_bss)
	*(.lc3_codec_c_bss)
#endif
        *(.msbc_bss)
        *(.msbc_codec_bss)
        . = ALIGN(4);
		#include "btstack/btstack_lib_bss.ld"
        . = ALIGN(4);
		#include "system/system_lib_bss.ld"
        . = ALIGN(4);
        *(.bss)
        . = ALIGN(4);
        *(.os_bss)
        *(.volatile_ram)
		*(.btstack_pool)

        *(.mem_heap)
		*(.memp_memory_x)


        . = ALIGN(32);

    } > ram0

	.data_code ALIGN(32):
	{
		data_code_pc_limit_begin = .;
		*(.flushinv_icache)
        *(.cache)
        *(.os_critical_code)
        *(.volatile_ram_code)
        *(.chargebox_code)
		*(.os_code)
		*(.os_str)

        *(.fat_data_code)

		. = ALIGN(4);
        _SPI_CODE_START = . ;
        *(.spi_code)
		. = ALIGN(4);
        _SPI_CODE_END = . ;
		. = ALIGN(4);

	} > ram0

	/* .moveable_slot ALIGN(4): */
	/* { */
		/* *(movable.slot.*) */
	/* } >ram0 */
    .nv_bss ALIGN(32) :
	{
		NVRAM_DATA_START = .;
		*(.non_volatile_ram)
		NVRAM_DATA_SIZE = ABSOLUTE(. - NVRAM_DATA_START);
		. = ALIGN(4);
		NVRAM_END = .;
		_nv_pre_begin = . ;
    } > ram0

	__report_overlay_begin = .;
	OVERLAY : AT(0x200000) SUBALIGN(4)
    {
		.overlay_aec
		{
            . = ALIGN(4);
			aec_code_begin  = . ;
			*(.text._*)
			*(.data._*)
			*(.aec_code)
			*(.aec_const)
			*(.res_code)
			*(.res_const)
			*(.ns_code)
			*(.ns_const)
			*(.fft_code)
			*(.fft_const)
			*(.nlp_code)
			*(.nlp_const)
			*(.der_code)
			*(.der_const)
			*(.qmf_code)
			*(.qmf_const)
			*(.aec_data)
			*(.res_data)
			*(.ns_data)
			*(.nlp_data)
        	*(.der_data)
        	*(.qmf_data)
        	*(.fft_data)
			*(.dms_code)
			*(.dms_const)
			*(.dms_data)
            . = ALIGN(4);
			*(.aec_mux)
            . = ALIGN(4);
			aec_code_end = . ;
			aec_code_size = aec_code_end - aec_code_begin ;
		}

		.overlay_aac
		{
			. = ALIGN(4);
			aac_dec_code_begin = .;
			*(.bt_aac_dec_code)
            *(.bt_aac_dec_sparse_code)
			aac_dec_code_end = .;
			aac_dec_code_size  = aac_dec_code_end - aac_dec_code_begin ;

			. = ALIGN(4);
			bt_aac_dec_const_begin = .;
			*(.bt_aac_dec_const)
            *(.bt_aac_dec_sparse_const)
			. = ALIGN(4);
			bt_aac_dec_const_end = .;
			bt_aac_dec_const_size = bt_aac_dec_const_end -  bt_aac_dec_const_begin ;

			*(.bt_aac_dec_data)
			. = ALIGN(4);
		}

    } > ram0

   //加个空段防止下面的OVERLAY地址不对
    .ram0_empty0 ALIGN(4) :
	{
        . = . + 4;
    } > ram0

	//__report_overlay_begin = .;
	OVERLAY : AT(0x210000) SUBALIGN(4)
    {
		.overlay_aec_ram
		{
            . = ALIGN(4);
			*(.msbc_enc)
			*(.cvsd_codec)
			*(.aec_bss)
			*(.res_bss)
			*(.ns_bss)
			*(.nlp_bss)
        	*(.der_bss)
        	*(.qmf_bss)
        	*(.fft_bss)
			*(.aec_mem)
			*(.dms_bss)
		}

		.overlay_aac_ram
		{
            . = ALIGN(4);
			*(.bt_aac_dec_bss)

			. = ALIGN(4);
			*(.aac_mem)
			*(.aac_ctrl_mem)
			/* 		. += 0x5fe8 ; */
			/*		. += 0xef88 ; */
		}

		.overlay_mp3
		{
			*(.mp3_mem)
			*(.mp3_ctrl_mem)
			*(.mp3pick_mem)
			*(.mp3pick_ctrl_mem)
			*(.dec2tws_mem)
		}
		.overlay_wma
		{
			*(.wma_mem)
			*(.wma_ctrl_mem)
			*(.wmapick_mem)
			*(.wmapick_ctrl_mem)
		}
		.overlay_ape
        {
            *(.ape_mem)
            *(.ape_ctrl_mem)
		}
		.overlay_flac
        {
            *(.flac_mem)
            *(.flac_ctrl_mem)
		}
		.overlay_m4a
        {
            *(.m4a_mem)
            *(.m4a_ctrl_mem)
		}
		.overlay_amr
        {
            *(.amr_mem)
            *(.amr_ctrl_mem)
		}
		.overlay_dts
        {
            *(.dts_mem)
            *(.dts_ctrl_mem)
		}
		.overlay_fm
		{
			*(.fm_mem)
		}
        .overlay_pc
		{
		}
		/* .overlay_moveable */
		/* { */
			/* . += 0xa000 ; */
		/* } */
    } > ram0

#ifdef CONFIG_CODE_BANK_ENABLE
    //bank code addr
    bank_code_run_addr = .;

    OVERLAY : AT(0x300000) SUBALIGN(4)
    {
        .overlay_bank0
        {
            *(.bank.code.0*)
            *(.bank.const.0*)
            . = ALIGN(4);
        }
        .overlay_bank1
        {
            *(.bank.code.1*)
            *(.bank.const.1*)
            . = ALIGN(4);
        }
        .overlay_bank2
        {
            *(.bank.code.2*)
            *(.bank.const.2*)
            *(.bank.ecdh.*)
            . = ALIGN(4);
        }
        .overlay_bank3
        {
            *(.bank.code.3*)
            *(.bank.const.3*)
            *(.bank.enc.*)
            . = ALIGN(4);
        }
        .overlay_bank4
        {
            *(.bank.code.4*)
            *(.bank.const.4*)
            . = ALIGN(4);
        }
        .overlay_bank5
        {
            *(.bank.code.5*)
            *(.bank.const.5*)
            . = ALIGN(4);
        }
        .overlay_bank6
        {
            *(.bank.code.6*)
            *(.bank.const.6*)
            . = ALIGN(4);
        }
        .overlay_bank7
        {
            *(.bank.code.7*)
            *(.bank.const.7*)
            . = ALIGN(4);
        }
        .overlay_bank8
        {
            *(.bank.code.8*)
            *(.bank.const.8*)
            . = ALIGN(4);
        }
        .overlay_bank9
        {
            *(.bank.code.9*)
            *(.bank.const.9*)
            . = ALIGN(4);
        }
    } > ram0

    //common bank code addr
    common_code_run_addr = .;
    .common ALIGN(4):
    {
       *(.common*)
       . = ALIGN(4);
    } > ram0
#endif
    /* maskrom area */
    NVRAM_LIMIT = 0xc000 - 0x100 - 0x400;
   	_nv_pre_end = 0xc000 - 0x100 - 0x400;
    ASSERT(NVRAM_END <= NVRAM_LIMIT, "NVRAM space overflow!")

	data_code_pc_limit_end = .;
	__report_overlay_end = .;

	_HEAP_BEGIN = . ;
	_HEAP_END = RAM0_END;
}

#include "btctrler/port/bd19/btctler_lib.ld"
#include "update/update.ld"
#include "driver/cpu/bd19/driver_lib.ld"
//================== Section Info Export ====================//
text_begin  = ADDR(.text);
text_size   = SIZEOF(.text);
text_end    = text_begin + text_size;
ASSERT((text_size % 4) == 0,"!!! text_size Not Align 4 Bytes !!!");

bss_begin = ADDR(.bss);
bss_size  = SIZEOF(.bss);
bss_end    = bss_begin + bss_size;
ASSERT((bss_size % 4) == 0,"!!! bss_size Not Align 4 Bytes !!!");

nvbss_begin = NVRAM_DATA_START;
nvbss_size  = NVRAM_LIMIT - nvbss_begin;
ASSERT((nvbss_size % 4) == 0,"!!! nvbss_size Not Align 4 Bytes !!!");

data_addr = ADDR(.data);
data_begin = text_begin + text_size;
data_size =  SIZEOF(.data);
ASSERT((data_size % 4) == 0,"!!! data_size Not Align 4 Bytes !!!");

/* moveable_slot_addr = ADDR(.moveable_slot); */
/* moveable_slot_begin = data_begin + data_size; */
/* moveable_slot_size =  SIZEOF(.moveable_slot); */

data_code_addr = ADDR(.data_code);
data_code_begin = data_begin + data_size;
data_code_size =  SIZEOF(.data_code);
ASSERT((data_code_size % 4) == 0,"!!! data_code_size Not Align 4 Bytes !!!");

//================ OVERLAY Code Info Export ==================//
aec_addr = ADDR(.overlay_aec);
aec_begin = data_code_begin + data_code_size;
aec_size =  SIZEOF(.overlay_aec);

aac_addr = ADDR(.overlay_aac);
aac_begin = aec_begin + aec_size;
aac_size =  SIZEOF(.overlay_aac);


//================ BANK ==================//
bank_code_load_addr = aac_begin + aac_size;

/* moveable_addr = ADDR(.overlay_moveable) ; */
/* moveable_size = SIZEOF(.overlay_moveable) ; */
//===================== HEAP Info Export =====================//
PROVIDE(HEAP_BEGIN = _HEAP_BEGIN);
PROVIDE(HEAP_END = _HEAP_END);
_MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN;
PROVIDE(MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN);

/* ASSERT(MALLOC_SIZE  >= 0x8000, "heap space too small !") */

//============================================================//
//=== report section info begin:
//============================================================//
report_text_beign = ADDR(.text);
report_text_size = SIZEOF(.text);
report_text_end = report_text_beign + report_text_size;

report_mmu_tlb_begin = ADDR(.mmu_tlb);
report_mmu_tlb_size = SIZEOF(.mmu_tlb);
report_mmu_tlb_end = report_mmu_tlb_begin + report_mmu_tlb_size;

report_boot_info_begin = ADDR(.boot_info);
report_boot_info_size = SIZEOF(.boot_info);
report_boot_info_end = report_boot_info_begin + report_boot_info_size;

report_irq_stack_begin = ADDR(.irq_stack);
report_irq_stack_size = SIZEOF(.irq_stack);
report_irq_stack_end = report_irq_stack_begin + report_irq_stack_size;

report_data_begin = ADDR(.data);
report_data_size = SIZEOF(.data);
report_data_end = report_data_begin + report_data_size;

report_bss_begin = ADDR(.bss);
report_bss_size = SIZEOF(.bss);
report_bss_end = report_bss_begin + report_bss_size;

report_data_code_begin = ADDR(.data_code);
report_data_code_size = SIZEOF(.data_code);
report_data_code_end = report_data_code_begin + report_data_code_size;

report_overlay_begin = __report_overlay_begin;
report_overlay_size = __report_overlay_end - __report_overlay_begin;
report_overlay_end = __report_overlay_end;

report_heap_beign = _HEAP_BEGIN;
report_heap_size = _HEAP_END - _HEAP_BEGIN;
report_heap_end = _HEAP_END;

BD19_PHY_RAM_SIZE = PHY_RAM_SIZE;
BD19_SDK_RAM_SIZE = report_mmu_tlb_size + \
					report_boot_info_size + \
					report_irq_stack_size + \
					report_data_size + \
					report_bss_size + \
					report_overlay_size + \
					report_data_code_size + \
					report_heap_size;
//============================================================//
//=== report section info end
//============================================================//

