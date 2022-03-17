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
#ifndef CONFIG_LARGE_PROGRAM_ENABLE
	code0(rx)    	  : ORIGIN =  0x1E00120,    LENGTH = CONFIG_FLASH_SIZE
#else /* #ifdef CONFIG_LARGE_PROGRAM_ENABLE */
	code0(rx)    	  : ORIGIN =  0x1000120,    LENGTH = CONFIG_FLASH_SIZE
#endif /* #ifdef CONFIG_LARGE_PROGRAM_ENABLE */
#else /* #if (USE_SDFILE_NEW) */
	code0(rx)    	  : ORIGIN =  0x1E00020,    LENGTH = CONFIG_FLASH_SIZE
#endif /* #if (USE_SDFILE_NEW) */
	ram0(rwx)         : ORIGIN =  RAM_BEGIN  , LENGTH = RAM_SIZE
    ram1(rwx)         : ORIGIN =  RAM1_BEGIN , LENGTH = RAM1_SIZE
}

#include "maskrom_stubs.ld"

EXTERN(
_start
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

		*(.aac_const)
		*(.aac_code)

		*(.alac_const)
		*(.alac_code)
		*(.alac_dec_code)

		*(.bt_aac_dec_eng_const)
		*(.bt_aac_dec_eng_code)
		*(.bt_aac_dec_core_code)
		*(.bt_aac_dec_core_sparse_code)

		*(.dts_dec_const)
        *(.fat_data_code_ex)

#if  (!TCFG_LED7_RUN_RAM)
        *(.gpio_ram)
        *(.LED_code)
        *(.LED_const)
		. = ALIGN(4);
#endif


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
		*(.cvsd_const)
		*(.cvsd_code)
		. = ALIGN(4);

		. = ALIGN(4);
        KEEP(*(.rec_const))

		. = ALIGN(4);
        KEEP(*(.bark_const))

		. = ALIGN(4);
        KEEP(*(.rec_sparse_code))

		. = ALIGN(4);
        KEEP(*(.rec_code))


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
		#include "media/cpu/br23/media_lib_text.ld"

		. = ALIGN(4);
        __VERSION_BEGIN = .;
        KEEP(*(.sys.version))
        __VERSION_END = .;
        *(.noop_version)

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
        *(.chargebox_code)
        *(.os_critical_code)
        *(.chargebox_code)

 		*(.os_code)
	    *(.os_const)


        *(.ui_ram)

        *(.fat_data_code)

#if (TCFG_ENC_LC3_ENABLE || TCFG_DEC_LC3_ENABLE)
		*(.lc3_codec_ari_c_code)
		*(.lc3_codec_e_code)
		*(.lc3_codec_c_const)
		*(.lc3_codec_c_code)
#endif

#if  (TCFG_LED7_RUN_RAM)
        *(.gpio_ram)
        *(.LED_code)
        *(.LED_const)
		. = ALIGN(4);
#endif

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

#if TCFG_FM_INSIDE_ENABLE
        *(.fm_code)
#endif
		. = ALIGN(4);
		*(.rec_data)
		. = ALIGN(4);
		*(.rec_bss)
		. = ALIGN(4);

        . = ALIGN(4);
		#include "media/cpu/br23/media_lib_data_text.ld"
        . = ALIGN(4);

    	_data_code_end = . ;

    	_cpu_store_begin = . ;

        . = ALIGN(32);
		#include "btstack/btstack_lib_data.ld"
        . = ALIGN(4);
		#include "btctrler/btctler_lib_data.ld"
        . = ALIGN(4);
		#include "system/system_lib_data.ld"
		. = ALIGN(4);
		#include "media/cpu/br23/media_lib_data.ld"
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
        *(.bss)
        *(COMMON)
		*(.cvsd_bss)

        *(.volatile_ram)
		#include "btctrler/btctler_lib_bss.ld"
		#include "btstack/btstack_lib_bss.ld"
		#include "system/system_lib_bss.ld"
		#include "media/cpu/br23/media_lib_bss.ld"

		. = ALIGN(4);
    } > ram0

	.vir_timer ALIGN(32):
    {
        *(.vir_rtc)
	} > ram0

    //cpu end
    _cpu_store_end = . ;

    .bss ALIGN(32) :
	{
		NVRAM_DATA_START = .;
		*(.non_volatile_ram)
		NVRAM_DATA_SIZE = ABSOLUTE(. - NVRAM_DATA_START);
		. = ALIGN(4);
		NVRAM_END = .;
		_nv_pre_begin = . ;

		. = ALIGN(4);

    } > ram0

	overlay_begin = .;
    #include "app_overlay.ld"
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

#include "app.ld"
#include "update/update.ld"
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

ASSERT(RAM_USED > bss_begin,"RAM_USED < bss_begin");
ASSERT(RAM_USED > data_addr,"RAM_USED < data_addr");
ASSERT(RAM_USED > overlay_begin,"RAM_USED < overlay_begin");

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

