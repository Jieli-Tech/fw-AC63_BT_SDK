// *INDENT-OFF*
#include "app_config.h"

/* =================  BR30 SDK memory ========================
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
#include "sdk_used_list.c"
);

UPDATA_SIZE     = 0x80;
UPDATA_BEG      = _MASK_MEM_BEGIN - UPDATA_SIZE;

#ifdef CONFIG_BR30_C_VERSION
UPDATA_BREDR_BASE_BEG = 0x2c600;
#else
UPDATA_BREDR_BASE_BEG = 0x2c000;
#endif

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

//============ About EQ coeff RAM ================
//internal EQ priv ram
EQ_PRIV_COEFF_BASE  = 0x02D860;
EQ_PRIV_SECTION_NUM = 10;
EQ_PRIV_COEFF_END   = EQ_PRIV_COEFF_BASE + 4 * EQ_PRIV_SECTION_NUM * 14;
//internal EQ cpu ram

/* #if (defined(EQ_SECTION_MAX) && (EQ_SECTION_MAX > 10)) */
/* EQ_SECTION_NUM = EQ_SECTION_MAX - EQ_PRIV_SECTION_NUM; */
/* #else */
/* EQ_SECTION_NUM = 0; */
/* #endif */

//=============== About BT RAM ===================
//CONFIG_BT_RX_BUFF_SIZE = (1024 * 18);

MEMORY
{
#if (USE_SDFILE_NEW)
	code0(rx)    	  : ORIGIN =  0x1E00120 ,    LENGTH = CONFIG_FLASH_SIZE
#else
	code0(rx)    	  : ORIGIN =  0x1E00020,    LENGTH = CONFIG_FLASH_SIZE
#endif
	ram0(rwx)         : ORIGIN = RAM0_BEG,  LENGTH = RAM0_SIZE
}


ENTRY(_start)

SECTIONS
{
    . = ORIGIN(code0);
    .text ALIGN(4):
    {
        PROVIDE(text_rodata_begin = .);

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

        *(.classic_tws_const)
        *(.classic_tws_code)

		*(.LOG_TAG_CONST*)
        *(.rodata*)

		. = ALIGN(4); // must at tail, make rom_code size align 4
        PROVIDE(text_rodata_end = .);

        clock_critical_handler_begin = .;
        KEEP(*(.clock_critical_txt))
        clock_critical_handler_end = .;

        gsensor_dev_begin = .;
        KEEP(*(.gsensor_dev))
        gsensor_dev_end = .;

		//mouse sensor dev begin
		. = ALIGN(4);
		OMSensor_dev_begin = .;
		KEEP(*(.omsensor_dev))
		OMSensor_dev_end = .;

        fm_dev_begin = .;
        KEEP(*(.fm_dev))
        fm_dev_end = .;

        fm_emitter_dev_begin = .;
        KEEP(*(.fm_emitter_dev))
        fm_emitter_dev_end = .;

        storage_device_begin = .;
        KEEP(*(.storage_device))
        storage_device_end = .;

		. = ALIGN(4);
		ai_server_device_begin = .;
		KEEP(*(.ai_server_device))
		ai_server_device_end = .;


		/********maskrom arithmetic ****/
        *(.opcore_table_maskrom)
        *(.bfilt_table_maskroom)
        *(.opcore_maskrom)
        *(.bfilt_code)
        *(.bfilt_const)
		/********maskrom arithmetic end****/

		. = ALIGN(4);
		#include "media/cpu/br30/audio_lib_text.ld"


		. = ALIGN(4);
        __VERSION_BEGIN = .;
        KEEP(*(.sys.version))
        __VERSION_END = .;

        *(.noop_version)

		. = ALIGN(4);

		*(.media.audio*)
		*(.media.mixer*)
		*(.media.media*)

		*( .wtg_dec_code )
		*( .wtg_dec_const)
		*( .wtg_dec_sparse_code)

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

	.irq_stack ALIGN(32):
    {
        _cpu0_sstack_begin = .;
        PROVIDE(cpu0_sstack_begin = .);
        *(.stack)
        _cpu0_sstack_end = .;
        PROVIDE(cpu0_sstack_end = .);
    	_stack_end = . ;
		. = ALIGN(4);

    } > ram0

	.data ALIGN(32):
	{
		//cpu start
        . = ALIGN(4);
        *(.data_magic)

		*(.audio_dec_data)
		. = ALIGN(4);
		#include "media/audio_lib_data.ld"
		. = ALIGN(4);

		*(.wav_data)
		*(.wav_dec_data)
		*(.wav_mem)
		*(.wav_ctrl_mem)
		. = ALIGN(4);

        *(.data*)

		/* . = ALIGN(4); */
		/* EQ_COEFF_BASE = . ; */
		/* . = EQ_COEFF_BASE + 4 * EQ_SECTION_NUM * 14; */

        . = ALIGN(32);
		#include "btstack/btstack_lib_data.ld"
		. = ALIGN(4);
		#include "system/system_lib_data.ld"
		. = ALIGN(4);
	} > ram0

	.bss ALIGN(32):
    {
        . = ALIGN(4);
		#include "btstack/btstack_lib_bss.ld"
        . = ALIGN(4);
		#include "system/system_lib_bss.ld"
        . = ALIGN(4);
        *(.bss)
        . = ALIGN(4);
		#include "media/audio_lib_bss.ld"
        . = ALIGN(4);

		*(.wav_bss)
		*(.wav_dec_bss)
        . = ALIGN(4);

		*(.audio_dec_bss)
        *(.os_bss)
        *(.volatile_ram)
		*(.btstack_pool)

            *(.usb_ep0)
            *(.usb_msd_dma)
		*(.audio_buf)
        *(.src_filt)
        *(.src_dma)
        *(.mem_heap)
		*(.memp_memory_x)

        . = ALIGN(4);
		*(.non_volatile_ram)

        . = ALIGN(32);

    } > ram0

	.data_code ALIGN(32):
	{
		data_code_pc_limit_begin = .;
		*(.flushinv_icache)
        *(.cache)
        *(.os_critical_code)
        *(.volatile_ram_code)
	  	 *(.media.aec.text)
		*(.os_code)
		*(.os_str)

        *(.fat_data_code)

    	audio_sync_code_begin = .;
        *(.audio_sync_code)
    	audio_sync_code_end = .;

		. = ALIGN(4);
        _SPI_CODE_START = . ;
        *(.spi_code)
		. = ALIGN(4);
        _SPI_CODE_END = . ;
		. = ALIGN(4);

        data_code_pc_limit_end = .;
	} > ram0

	.moveable_slot ALIGN(4):
	{
	    *(movable.slot.*)

	} >ram0

#ifdef CONFIG_BR30_C_VERSION
	.heap ALIGN(32):
	{
        _HEAP_BEGIN = .;
        . = ALIGN(0x1FC00);
        _HEAP_END = .;
	} >ram0
#endif

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

        .overlay_opus
        {
            *(.opus_mem)
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
#if 0
		.overlay_wav
		{
			*(.wav_mem)
			*(.wav_ctrl_mem)
		}
#endif
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
            *(.usb_audio_play_dma)
            *(.usb_audio_rec_dma)
            *(.uac_rx)
            *(.mass_storage)

            *(.usb_hid_dma)
            *(.usb_iso_dma)
            *(.uac_var)
            *(.usb_config_var)
		}

    } > ram0


	__report_overlay_end = .;

#ifdef CONFIG_CODE_BANK_ENABLE
   //加个空段防止下面的OVERLAY地址不对
    .ram0_empty ALIGN(4) :
	{
        . = . + 4;
    } > ram0

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
            . = ALIGN(4);
        }
		.overlay_bank3
		{
            *(.bank.code.3*)
            *(.bank.const.3*)
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
#endif

#ifndef CONFIG_BR30_C_VERSION
	_HEAP_BEGIN = . ;
	_HEAP_END = RAM0_END;
#endif

}

#include "btctrler/port/br30/btctler_lib.ld"
#include "update/update.ld"
#include "driver/cpu/br30/driver_lib.ld"
//================== Section Info Export ====================//
text_begin  = ADDR(.text);
text_size   = SIZEOF(.text);
text_end    = text_begin + text_size;

bss_begin = ADDR(.bss);
bss_size  = SIZEOF(.bss);
bss_end    = bss_begin + bss_size;

data_addr = ADDR(.data);
data_begin = text_begin + text_size;
data_size =  SIZEOF(.data);

moveable_slot_addr = ADDR(.moveable_slot);
moveable_slot_begin = data_begin + data_size;
moveable_slot_size =  SIZEOF(.moveable_slot);

data_code_addr = ADDR(.data_code);
data_code_begin = moveable_slot_begin + moveable_slot_size;
data_code_size =  SIZEOF(.data_code);


//================ OVERLAY Code Info Export ==================//
aec_addr = ADDR(.overlay_aec);
aec_begin = data_code_begin + data_code_size;
aec_size =  SIZEOF(.overlay_aec);

aac_addr = ADDR(.overlay_aac);
aac_begin = aec_begin + aec_size;
aac_size =  SIZEOF(.overlay_aac);

/*moveable_addr = ADDR(.overlay_moveable) ;
moveable_size = SIZEOF(.overlay_moveable) ;*/
//===================== HEAP Info Export =====================//
PROVIDE(HEAP_BEGIN = _HEAP_BEGIN);
PROVIDE(HEAP_END = _HEAP_END);
_MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN;
PROVIDE(MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN);

ASSERT(MALLOC_SIZE  >= 0x8000, "heap space too small !")

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

BR30_PHY_RAM_SIZE = PHY_RAM_SIZE;
BR30_SDK_RAM_SIZE = report_mmu_tlb_size + \
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

