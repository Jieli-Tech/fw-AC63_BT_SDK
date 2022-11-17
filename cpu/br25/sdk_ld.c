// *INDENT-OFF*
#include "app_config.h"

/* =================  BR25 SDK memory ========================
 _______________ ___ 0x32000(136K)
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
|_______________|0x10000 RAM_LIMIT_L
|   Reserved    |
|_______________|0
 =========================================================== */

#include "maskrom_stubs.ld"

EXTERN(
_start
#include "sdk_used_list.c"
);

UPDATA_SIZE     = 0x80;
UPDATA_BEG      = _MASK_MEM_BEGIN - UPDATA_SIZE;
UPDATA_BREDR_BASE_BEG = 0xf9000; //depend on loader code & data

RAM_LIMIT_L     = 0x10000;
RAM_LIMIT_H     = UPDATA_BEG;
PHY_RAM_SIZE    = RAM_LIMIT_H - RAM_LIMIT_L;

//from mask export
ISR_BASE       = _IRQ_MEM_ADDR;
ROM_RAM_SIZE   = _MASK_MEM_SIZE;
ROM_RAM_BEG    = _MASK_MEM_BEGIN;


RAM_BEGIN       = RAM_LIMIT_L;
RAM_END         = RAM_LIMIT_H;
RAM_SIZE        = RAM_END - RAM_BEGIN;

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

//=============== About BT RAM ===================
//CONFIG_BT_RX_BUFF_SIZE = (1024 * 18);


MEMORY
{
	code0(rx)    	  : ORIGIN =  0x1E00120,  LENGTH = CONFIG_FLASH_SIZE
	ram0(rwx)         : ORIGIN =  RAM_BEGIN, LENGTH = RAM_SIZE
}


ENTRY(_start)

SECTIONS
{
	. = ORIGIN(code0);
    .text ALIGN(4):
    {
        PROVIDE(flash_code_begin = .);

        *(.startup.text)

        bank_stub_start = .;
        *(.bank.stub.*)
        bank_stub_size = . - bank_stub_start;

		. = ALIGN(4); // must at tail, make rom_code size align 4

        clock_critical_handler_begin = .;
        KEEP(*(.clock_critical_txt))
        clock_critical_handler_end = .;

        chargestore_handler_begin = .;
        KEEP(*(.chargestore_callback_txt))
        chargestore_handler_end = .;

		/********maskrom arithmetic ****/
        *(.bfilt_code)
        *(.bfilt_table_maskroom)
       /********maskrom arithmetic end****/

		. = ALIGN(4);
        __VERSION_BEGIN = .;
        KEEP(*(.sys.version))
        __VERSION_END = .;
        *(.noop_version)

        . = ALIGN(4);

        *(.tech_lib.aec.text)

		. = ALIGN(4);
		#include "btstack/btstack_lib_text.ld"
		. = ALIGN(4);
		#include "system/system_lib_text.ld"

		. = ALIGN(4);
	    update_target_begin = .;
	    PROVIDE(update_target_begin = .);
	    KEEP(*(.update_target))
	    update_target_end = .;
	    PROVIDE(update_target_end = .);
		. = ALIGN(4);

        . = ALIGN(4);
		*(.ui_ram)
		. = ALIGN(4);
        #include "ui/ui/ui.ld"
		. = ALIGN(4);
		#include "media/cpu/br25/media_lib_text.ld"
		. = ALIGN(4);

        *(.text*)
        *(.LOG_TAG_CONST*)
        *(.rodata*)
        *(.fat_data_code_ex)
        *(.fat_data_code)
        . = ALIGN(4);

        PROVIDE(flash_code_end = flash_code_begin + 1M);

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
        . = ALIGN(4);
        *(.data_magic)

        . = ALIGN(32);

        . = ALIGN(4);

		#include "btstack/btstack_lib_data.ld"
        . = ALIGN(4);
		#include "system/system_lib_data.ld"
		. = ALIGN(4);
		#include "media/cpu/br25/media_lib_data.ld"

        . = ALIGN(4);
		EQ_COEFF_BASE = . ;
		. = EQ_COEFF_BASE + 4 * EQ_SECTION_NUM * 14;
        . = ALIGN(4);


	} > ram0

    .bss ALIGN(32):
    {
		. = ALIGN(4);
		#include "btstack/btstack_lib_bss.ld"
        . = ALIGN(4);
		#include "system/system_lib_bss.ld"
        . = ALIGN(4);
		#include "media/cpu/br25/media_lib_bss.ld"
        . = ALIGN(4);

        *(COMMON)
        *(.volatile_ram)
		*(.audio_play_dma)

        . = ALIGN(4);
		*(.non_volatile_ram)
		. = ALIGN(32);

    } > ram0


	data_code_limit_begin = .;
	.data_code ALIGN(32):
	{
    	data_code_begin = .;
        *(.common*)
        . = ALIGN(4);

		#include "media/cpu/br25/media_lib_data_text.ld"

		*(.flushinv_icache)
        *(.os_critical_code)
        *(.os_rewrite_code)
        *(.volatile_ram_code)
        *(.chargebox_code)

		. = ALIGN(4);
        _SPI_CODE_START = . ;
        *(.spi_code)
		. = ALIGN(4);
        _SPI_CODE_END = . ;

		. = ALIGN(4);
    	data_code_end = .;
	} > ram0

	.vir_timer ALIGN(32):
    {
        *(.vir_rtc)
    } > ram0

	overlay_begin = .;
    #include "app_overlay.ld"

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
#endif /* #ifdef CONFIG_CODE_BANK_ENABLE */
	data_code_limit_end = .;

	_HEAP_BEGIN = .;
	_HEAP_END = RAM_END;


}

#include "app.ld"
#include "update/update.ld"
#include "driver/cpu/br25/driver_lib.ld"
#include "btctrler/port/br25/btctler_lib.ld"

//================== Section Info Export ====================//
text_begin  = ADDR(.text);
text_size   = SIZEOF(.text);
text_end    = ADDR(.text) + SIZEOF(.text);
ASSERT((text_size % 4) == 0,"!!! text_size Not Align 4 Bytes !!!");

bss_begin = ADDR(.bss);
bss_size  = SIZEOF(.bss);
bss_end   = bss_begin + bss_size;
ASSERT((bss_size % 4) == 0,"!!! bss_size Not Align 4 Bytes !!!");

data_addr = ADDR(.data);
data_begin = text_begin + text_size;
data_size =  SIZEOF(.data);
ASSERT((data_size % 4) == 0,"!!! data_size Not Align 4 Bytes !!!");

data_code_addr = ADDR(.data_code);
data_code_begin = data_begin + data_size;
data_code_size = SIZEOF(.data_code);
ASSERT((data_code_size % 4) == 0,"!!! data_code_size Not Align 4 Bytes !!!");

//================ OVERLAY Code Info Export ==================//


aec_addr = ADDR(.overlay_aec);
aec_begin = data_code_begin + data_code_size;
aec_size =  o_aec_end - aec_addr;
/* aec_size =  SIZEOF(.overlay_aec); */

wav_addr = ADDR(.overlay_wav);
wav_begin = aec_begin + aec_size;
wav_size = o_wav_end - wav_addr;

ape_addr = ADDR(.overlay_ape);
ape_begin = wav_begin + wav_size;
ape_size =  o_ape_end - ape_addr;

flac_addr = ADDR(.overlay_flac);
flac_begin = ape_begin + ape_size;
flac_size = o_flac_end - flac_addr;

m4a_addr = ADDR(.overlay_m4a);
m4a_begin = flac_begin + flac_size;
m4a_size = o_m4a_end - m4a_addr;

amr_addr = ADDR(.overlay_amr);
amr_begin = m4a_begin + m4a_size;
amr_size = o_amr_end - amr_addr;

dts_addr = ADDR(.overlay_dts);
dts_begin = amr_begin + amr_size;
dts_size = o_dts_end - dts_addr;

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

overlay_code_end_addr = wma_begin + wma_size;
#else
overlay_code_end_addr = fm_begin + fm_size;
#endif


//================ Bank Code Info Export ==================//
bank_code_load_addr = overlay_code_end_addr;

//===================== HEAP Info Export =====================//
ASSERT(_HEAP_BEGIN > bss_begin,"_HEAP_BEGIN < bss_begin");
ASSERT(_HEAP_BEGIN > data_addr,"_HEAP_BEGIN < data_addr");
ASSERT(_HEAP_BEGIN > overlay_begin,"_HEAP_BEGIN < overlay_begin");
ASSERT(_HEAP_BEGIN > data_code_addr,"_HEAP_BEGIN < data_code_addr");

PROVIDE(HEAP_BEGIN = _HEAP_BEGIN);
PROVIDE(HEAP_END = _HEAP_END);
_MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN;
PROVIDE(MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN);


