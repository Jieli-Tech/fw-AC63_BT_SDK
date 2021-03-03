// *INDENT-OFF*
#include "app_config.h"

/*
|   updata      |
|_______________|
|    ram1       |
|_______________|
|    TLB        |
|_______________|
|   isr base    |   (HW fixed)
|_______________|
|    ram0       |
|_______________|
|    DCACHE     |
|_______________|
*/




RAM1_LIMIT_L    = 0x2C000;
RAM1_LIMIT_H    = 0x30000;


/********************************************/
/*           0x2FF80 ~ 0x30000              */
/********************************************/
UPDATA_SIZE     = 0x80;
UPDATA_BEG      = RAM1_LIMIT_H - UPDATA_SIZE;

RAM1_BEGIN      = RAM1_LIMIT_L;
RAM1_END        = RAM1_LIMIT_H - UPDATA_SIZE;
RAM1_SIZE       = RAM1_END - RAM1_BEGIN;


#ifdef CONFIG_PSRAM_ENABLE
RAM_LIMIT_L     = 0x04000;
#else
RAM_LIMIT_L     = 0x00000;
#endif

RAM_LIMIT_H     = 0x2C000;
/********************************************/
/*           0x2BF00 ~ 0x2C000              */
/********************************************/
/* ISR_BASE        = _IRQ_MEM_ADDR; */
ISR_SIZE        = 0x100;
ISR_BASE        = RAM_LIMIT_H - ISR_SIZE;

/********************************************/
/*           0x00000 ~ 0x2BF00              */
/********************************************/
RAM_BEGIN       = RAM_LIMIT_L;
RAM_END         = RAM_LIMIT_H - ISR_SIZE;
RAM_SIZE        = RAM_END - RAM_BEGIN;


BANK_SIZE   = 5*1024;


PSRAM_BEG      = 0x800000;
PSRAM_SIZE     = 2M;


CODE_BEG   = 0X1E000C0;

UPDATA_BREDR_BASE_BEG = 0xF9000;

#if MIC_EFFECT_EQ_EN
MIC_EFFECT_EQ_SECTION = 8;
#else
MIC_EFFECT_EQ_SECTION = 3;
#endif

#if (EQ_SECTION_MAX > 10)
EQ_SECTION_NUM = EQ_SECTION_MAX+MIC_EFFECT_EQ_SECTION;
#else
EQ_SECTION_NUM = 10+MIC_EFFECT_EQ_SECTION;
#endif

MEMORY
{
    psram(rwx)        : ORIGIN =  PSRAM_BEG , LENGTH = PSRAM_SIZE
#if (USE_SDFILE_NEW)
	code0(rx)    	  : ORIGIN =  0x1E00120,    LENGTH = CONFIG_FLASH_SIZE
#else
	code0(rx)    	  : ORIGIN =  0x1E00020,    LENGTH = CONFIG_FLASH_SIZE
#endif
	ram0(rwx)         : ORIGIN =  RAM_BEGIN  , LENGTH = RAM_SIZE
    ram1(rwx)         : ORIGIN =  RAM1_BEGIN , LENGTH = RAM1_SIZE
}

#include "maskrom_stubs.ld"

EXTERN(
#include "sdk_used_list.c"
);

ENTRY(_start)

SECTIONS
{
/********************************************/
    . =ORIGIN(psram);
    .psram_text ALIGN(4):
    {
#ifdef CONFIG_PSRAM_ENABLE
        *(.text*)
        *(.LOG_TAG_CONST*)
        *(.rodata*)
#endif
    } > psram

    .psram ALIGN(32):
    {
#ifdef CONFIG_PSRAM_ENABLE
        *(.bss)
        *(COMMON)
#endif
    } > psram

    . = ORIGIN(code0);
    .text ALIGN(4):
    {
        text_code_begin = .;

        PROVIDE(text_rodata_begin = .);

        *(.startup.text)

        bank_stub_start = .;
		*(.bank.stub.*)
		bank_stub_size = . - bank_stub_start;

        *(.text*)
        *(.LOG_TAG_CONST*)
        *(.rodata*)

		*(.aac_data)
		*(.aac_const)
		*(.aac_code)

		*(.alac_data)
		*(.alac_const)
		*(.alac_code)
		*(.alac_dec_code)

		*(.bt_aac_dec_eng_const)
		*(.bt_aac_dec_eng_code)
		*(.bt_aac_dec_core_code)
		*(.bt_aac_dec_core_sparse_code)

		*(.dts_dec_const)

#ifndef TCFG_GPIO_RAM_ENABLE
        *(.gpio_ram)
        *(.LED_code)
        *(.LED_const)
#endif

		*(.cvsd_data)
		*(.cvsd_const)
		*(.cvsd_code)

		. = ALIGN(4);
        gsensor_dev_begin = .;
        KEEP(*(.gsensor_dev))
        gsensor_dev_end = .;

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

		/********maskrom arithmetic ****/
        *(.opcore_table_maskrom)
        *(.bfilt_table_maskroom)
        *(.opcore_maskrom)
        *(.bfilt_code)
        *(.bfilt_const)

		. = ALIGN(4);
		#include "btctrler/btctler_lib_text.ld"
		. = ALIGN(4);
		#include "btstack/btstack_lib_text.ld"
		. = ALIGN(4);
		#include "system/system_lib_text.ld"
        . = ALIGN(4);
        #include "ui/ui/ui.ld"
        . = ALIGN(4);

		. = ALIGN(4);
	    update_target_begin = .;
	    PROVIDE(update_target_begin = .);
	    KEEP(*(.update_target))
	    update_target_end = .;
	    PROVIDE(update_target_end = .);
		. = ALIGN(4);

        text_code_end = .;

    } >code0


    . = ORIGIN(ram0);

  	_data_code_begin = . ;


#ifdef CONFIG_CODE_BANK_ENABLE

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
    } > ram0
	bank_code_run_end_addr = .;

	common_code_run_addr = bank_code_run_addr + BANK_SIZE;
	ASSERT(bank_code_run_end_addr <= common_code_run_addr, "bank overflow!")

	. = common_code_run_addr;

   	.common ALIGN(4):
	{
        *(.common*)
        . = ALIGN(4);
    } > ram0
#else

	bank_code_run_addr = .;
	common_code_run_addr = .;
#endif

        //cpu start
   	.data ALIGN(4):
	  {
		/// 放在data 里面的code 必须放在这个位置保护起来

        *(.data_magic)
        . = ALIGN(4);

        *(.flushinv_icache)
        *(.volatile_ram_code)
        *(.os_critical_code)
        *(.chargebox_code)

 		*(.os_code)
	    *(.os_const)


        *(.ui_ram)

        *(.fat_data_code)
#ifdef TCFG_GPIO_RAM_ENABLE
        *(.gpio_ram)
        *(.LED_code)
        *(.LED_const)
#endif

#if TCFG_FM_INSIDE_ENABLE
        *(.fm_code)
#endif

        . = ALIGN(4);

#if (SOUNDCARD_ENABLE)
		*(.mix_ram_code)
        . = ALIGN(4);
		*(.dvol_ram_code)
        . = ALIGN(4);
		*(.reverb_cal_const)
 . = ALIGN(4);
			*(.reverb_cal_code)

        . = ALIGN(4);
		*(.stream_ram_code)
        . = ALIGN(4);
			*(.mp3_code)
        . = ALIGN(4);

#endif
        . = ALIGN(4);

    	_data_code_end = . ;

    	_cpu_store_begin = . ;
		. = ALIGN(4);
        *(.data*)

		. = ALIGN(4);
		dec_board_param_mem_begin = .;
		*(.dec_board_param_mem)
		dec_board_param_mem_end = .;


        . = ALIGN(32);
		#include "btstack/btstack_lib_data.ld"
        . = ALIGN(4);
		#include "btctrler/btctler_lib_data.ld"
        . = ALIGN(4);
		#include "system/system_lib_data.ld"
		. = ALIGN(4);

	  } > ram0

    .irq_stack ALIGN(32) :
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
		*(.boot_info)

    } > ram0

    .bss ALIGN(32) :
    {
        *(.usb_h_dma)   //由于usb有个bug，会导致dma写的数据超出预设的buf，最长可能写超1k，为了避免死机，所以usb dma buffer后面放一些其他模块的buff来避免死机
        *(.usb_ep0)
        *(.dec_mix_buff)
        *(.sd0_var)
        *(.sd1_var)
        *(.dac_buff)

        *(.bss)
        *(COMMON)

        *(.volatile_ram)
		#include "btctrler/btctler_lib_bss.ld"
		#include "btstack/btstack_lib_bss.ld"
		#include "system/system_lib_bss.ld"

		. = (( . + 31) / 32 * 32);

		. = ALIGN(4);
    } > ram0
    //cpu end
    _cpu_store_end = . ;

    _prp_store_begin = . ;
    .prp_bss ALIGN(32) :
    {
        //bt
        //
        //sbc
        //
        //audio
		. = (( . + 31) / 32 * 32);
    } > ram0
    _prp_store_end = . ;

    .bss ALIGN(32) :
	{
		NVRAM_DATA_START = .;
		*(.non_volatile_ram)
		NVRAM_DATA_SIZE = ABSOLUTE(. - NVRAM_DATA_START);
		. = ALIGN(4);
		NVRAM_END = .;
		_nv_pre_begin = . ;

        *(.src_filt)
        *(.src_dma)
		. = ALIGN(4);

    } > ram0

	overlay_begin = .;
	OVERLAY : NOCROSSREFS AT(0x200000) SUBALIGN(4)
    {

		.overlay_aec
		{

			*(.aec_bss_id)
			o_aec_end = .;

            *(.cvsd_codec)
			*(.aec_mem)
            *(.msbc_enc)
			*(.cvsd_bss)
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
			*(.mp3_bss_id)
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
			*(.wma_bss_id)
			o_wma_end = .;

			*(.wma_mem)
			*(.wma_ctrl_mem)
			*(.wmapick_mem)
			*(.wmapick_ctrl_mem)
		}
		.overlay_wav
		{
			*(.wav_dec_sparse_code)
			*(.wav_dec_sparse_const)

			*(.wav_dec_code)
			*(.wav_dec_const)
			*(.wav_dec_data)

			*(.wav_data)
			*(.wav_const)
			*(.wav_code)


			*(.wav_bss_id)

			*(.wav_bss)
			*(.wav_dec_bss)
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

			*(.ape_bss_id)

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

			*(.flac_bss_id)

            *(.flac_mem)
            *(.flac_ctrl_mem)
			*(.flac_dec_bss)
			*(.flac_bss)
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

			*(.m4a_bss_id)

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

			*(.amr_bss_id)

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

			*(.dts_bss_id)

            *(.dts_mem)
            *(.dts_ctrl_mem)
			*(.dts_dec_bss)
			*(.dts_bss)

		}



		.overlay_fm
		{
			*(.fm_bss_id)
			o_fm_end = .;

			*(.fm_mem)
			*(.linein_pcm_mem)
		}
        .overlay_pc
		{
#if TCFG_VIR_UDISK_ENABLE == 0
            *(.usb_audio_play_dma)
            *(.usb_audio_rec_dma)
            *(.uac_rx)
            *(.mass_storage)

            *(.usb_msd_dma)
            *(.usb_hid_dma)
            *(.usb_iso_dma)
            *(.uac_var)
            *(.usb_config_var)
#endif
		}
    } > ram0

	overlay_end = .;

	ASSERT(overlay_end <= overlay_begin + 64k, "overlay overflow 64k!")

    RAM_USED = .;


	. =ORIGIN(ram1);
    //TLB 起始需要16K 对齐；
    .mmu_tlb ALIGN(0x4000):
    {
        *(.mmu_tlb_segment);
    } > ram1


    .bss1 ALIGN(32) :
	{
    } > ram1
    RAM1_USED = .;


/********************************************/
    /*
    . =ORIGIN(ram0);
    .ram0_data ALIGN(4):
    {
        _VM_CODE_START = . ;
        *(.vm)
        _VM_CODE_END = . ;
        *(.flushinv_icache)

		. = ALIGN(4); // must at tail, make ram0_data size align 4
    } > ram0
    */

/********************************************/
}


#include "update/update.ld"
#include "media/media.ld"
#include "driver/cpu/br23/driver_lib.ld"

text_begin  = ADDR(.text) ;
text_size   = SIZEOF(.text) ;
text_end    = ADDR(.text) + SIZEOF(.text) ;

bss_begin = ADDR(.bss) ;
bss_size  = SIZEOF(.bss);

bss1_begin = ADDR(.bss1) ;
bss1_size  = SIZEOF(.bss1);

data_addr = ADDR(.data)  ;
data_begin = text_begin + text_size;
data_size =  SIZEOF(.data) ;

psram_vaddr = ADDR(.psram_text)  ;
psram_laddr = text_begin + text_size + data_size;
psram_text_size =  SIZEOF(.psram_text) ;

bank_code_load_addr = data_begin + data_size;


//================ OVERLAY Code Info Export ==================//

aec_addr = ADDR(.overlay_aec);
aec_begin = text_begin + text_size + data_size;
aec_size =  o_aec_end - aec_addr;
/* aec_size =  SIZEOF(.overlay_aec); */

wav_addr = ADDR(.overlay_wav);
wav_begin = aec_begin + aec_size;
wav_size =  SIZEOF(.overlay_wav);

ape_addr = ADDR(.overlay_ape);
ape_begin = wav_begin + wav_size;
ape_size =  SIZEOF(.overlay_ape);

flac_addr = ADDR(.overlay_flac);
flac_begin = ape_begin + ape_size;
flac_size =  SIZEOF(.overlay_flac);

m4a_addr = ADDR(.overlay_m4a);
m4a_begin = flac_begin + flac_size;
m4a_size =  SIZEOF(.overlay_m4a);

amr_addr = ADDR(.overlay_amr);
amr_begin = m4a_begin + m4a_size;
amr_size =  SIZEOF(.overlay_amr);

dts_addr = ADDR(.overlay_dts);
dts_begin = amr_begin + amr_size;
dts_size =  SIZEOF(.overlay_dts);

fm_addr = ADDR(.overlay_fm);
fm_begin = dts_begin + dts_size;
fm_size =  o_fm_end - fm_addr;
/* fm_size =  SIZEOF(.overlay_fm); */

#ifdef CONFIG_MP3_WMA_LIB_SPECIAL
mp3_addr = ADDR(.overlay_mp3);
mp3_begin = fm_begin + fm_size;
mp3_size =  o_mp3_end - mp3_addr;

wma_addr = ADDR(.overlay_wma);
wma_begin = mp3_begin + mp3_size;
wma_size = o_wma_end - wma_addr;

#endif


_HEAP_BEGIN = RAM_USED;
PROVIDE(HEAP_BEGIN = RAM_USED);

_HEAP_END = RAM_END;
PROVIDE(HEAP_END = RAM_END);

_HEAP_SIZE = HEAP_END - HEAP_BEGIN;
PROVIDE(HEAP_SIZE = HEAP_END - HEAP_BEGIN);

_HEAP1_BEGIN = RAM1_USED;
PROVIDE(HEAP1_BEGIN = RAM1_USED);

_HEAP1_END = RAM1_END;
PROVIDE(HEAP1_END = RAM1_END);

_HEAP1_SIZE = HEAP1_END - HEAP1_BEGIN;
PROVIDE(HEAP1_SIZE = HEAP1_END - HEAP1_BEGIN);

_MALLOC_SIZE = HEAP_SIZE + HEAP1_SIZE;
PROVIDE(MALLOC_SIZE = HEAP_SIZE + HEAP1_SIZE);

