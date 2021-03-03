// *INDENT-OFF*
#include "app_config.h"

RAM_LIMIT_L     = 0x00000;
RAM_LIMIT_H     = 0x0C000;

/********************************************/
/*           0xbf00 ~ 0xc000                */
/********************************************/
/* ISR_BASE        = _IRQ_MEM_ADDR; */
ISR_SIZE        = 0x100;
ISR_BASE        = RAM_LIMIT_H - ISR_SIZE;

/********************************************/
/*           0xbb00 ~ 0xbf00              */
/********************************************/
/* ROM_RAM_BEG     = _MASK_MEM_BEGIN; */
ROM_RAM_SIZE    = 0x400;
ROM_RAM_BEGIN   = RAM_LIMIT_H - ROM_RAM_SIZE - ISR_SIZE;

/********************************************/
/*           0xba80 ~ 0xbb00                */
/********************************************/
UPDATA_SIZE     = 0x80;
UPDATA_BEG      = RAM_LIMIT_H - UPDATA_SIZE - ROM_RAM_SIZE - ISR_SIZE;

UPDATA_COPY_BEG = 0x1d000 - UPDATA_SIZE;

/********************************************/
/*           0x0800 ~ 0xba80                */
/********************************************/
RAM_BEGIN       = RAM_LIMIT_L;
RAM_END         = RAM_LIMIT_H - UPDATA_SIZE - ROM_RAM_SIZE - ISR_SIZE;
RAM_SIZE        = RAM_END - RAM_BEGIN;

UPDATA_BREDR_BASE_BEG = 0x1C000;

MEMORY
{
#if (USE_SDFILE_NEW)
#if (RCSP_UPDATE_EN)
	code0(rx)    	  : ORIGIN =  0x1E000E0, LENGTH = CONFIG_FLASH_SIZE
#else
	code0(rx)    	  : ORIGIN =  0x1E000C0,    LENGTH = CONFIG_FLASH_SIZE
#endif
#else
	code0(rx)    	  : ORIGIN =  0x1E00020,    LENGTH = CONFIG_FLASH_SIZE
#endif
	ram0(rwx)         : ORIGIN =  RAM_BEGIN,    LENGTH = RAM_SIZE
    irq_table(rw)     : ORIGIN =  0xbf00,       LENGTH = 0x100
}

#include "maskrom_stubs.ld"
#include "maskrom_stubs_os.ld"

EXTERN(
#include "sdk_used_list.c"
);

ENTRY(_start)

SECTIONS
{
/********************************************/
    . = ORIGIN(code0);
    .text ALIGN(4):
    {
        PROVIDE(text_rodata_begin = .);

        *(.startup.text)

        *(.text*)
        *(.rodata*)

        *(.LOG_TAG_CONST*)

		. = ALIGN(4);
        gsensor_dev_begin = .;
        KEEP(*(.gsensor_dev))
        gsensor_dev_end = .;

		. = ALIGN(4);
        //mouse sensor dev begin
        OMSensor_dev_begin = .;
        KEEP(*(.omsensor_dev))
        OMSensor_dev_end = .;

		. = ALIGN(4);
        storage_device_begin = .;
        KEEP(*(.storage_device))
        storage_device_end = .;

		. = ALIGN(4);
		#include "btctrler/btctler_lib_text.ld"
		. = ALIGN(4);
		#include "system/system_lib_text.ld"

		. = ALIGN(4);
	    update_target_begin = .;
	    PROVIDE(update_target_begin = .);
	    KEEP(*(.update_target))
	    update_target_end = .;
	    PROVIDE(update_target_end = .);
		. = ALIGN(4);

    } >code0

    . = ORIGIN(ram0);

    //TLB 起始0x0000
    .mmu_tlb ALIGN(0x4):
    {
        *(.mmu_tlb_segment);
    } > ram0

        //cpu start
   	.data ALIGN(4):
	  {
    	_data_code_begin = . ;
        . = ALIGN(4);
        *(.data_magic)

        . = ALIGN(4);
        *(.flushinv_icache)
        *(.volatile_ram_code)
        *(.os_critical_code)
		_os_begin = .;
		PROVIDE(os_begin = .);
		   *(.os_code)
		   *(.os_const)
		   *(.os_str)
		   *(.os_data)
		_os_end = .;
		PROVIDE(os_end = .);

        *(.fat_data_code)
        *(reboot.rodata)
        *(.non_volatile_ram_code)

    	_data_code_end = . ;

    	_cpu_store_begin = . ;

        . = ALIGN(4);
        *(.data*)

        . = ALIGN(4);
		#include "btctrler/btctler_lib_data.ld"
        . = ALIGN(4);
		#include "system/system_lib_data.ld"

	  } > ram0

    .irq_stack ALIGN(32) :
    {
		*(.stack_magic)
    	_stack_begin = . ;
		*(.stack)
    	_stack_end = . ;
		*(.stack_magic0)

		. = ALIGN(4);
		*(.boot_info)

    } > ram0

    .bss ALIGN(32) :
    {
        *(.bss)
        *(COMMON)
        *(.volatile_ram)
        *(.btstack_pool)
        *(.usb_dma_mem)

		#include "btctrler/btctler_lib_bss.ld"
		#include "system/system_lib_bss.ld"

		. = (( . + 31) / 32 * 32);
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


	OVERLAY : NOCROSSREFS AT(0x200000) SUBALIGN(4)
    {
		.overlay_aec
		{
			*(.aec_mem)
            *(.msbc_enc)
            *(.cvsd_codec)
		}
		.overlay_mp3
		{
			*(.mp3_mem)
			*(.mp3_ctrl_mem)
		}
		.overlay_wma
		{
			*(.wma_mem)
			*(.wma_ctrl_mem)
		}
		.overlay_wav
		{
			*(.wav_mem)
			*(.wav_ctrl_mem)
		}
		.overlay_pc
		{
			*(.usb_audio_play_dma)
            *(.usb_audio_rec_dma)
            *(.uac_rx)
            *(.mass_storage)

            *(.usb_ep0)
            *(.usb_msd_dma)
            *(.usb_hid_dma)
            *(.usb_iso_dma)
            *(.uac_var)
            *(.usb_config_var)
		}
    } > ram0

    .nv_bss ALIGN(32) :
	{
		NVRAM_DATA_START = .;
		*(.non_volatile_ram)
		NVRAM_DATA_SIZE = ABSOLUTE(. - NVRAM_DATA_START);
		. = ALIGN(4);
		NVRAM_END = .;
		_nv_pre_begin = . ;
    } > ram0

    /* maskrom area */
    NVRAM_LIMIT = 0xc000 - 0x100 - 0x400;
   	_nv_pre_end = 0xc000 - 0x100 - 0x400;
    ASSERT(NVRAM_END <= NVRAM_LIMIT, "NVRAM space overflow!")


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
/* #include "system/system.ld" */
#ifdef CONFIG_AUDIO_ONCHIP
#include "media/media.ld"
#endif/*CONFIG_AUDIO_ONCHIP*/
#include "btstack/btstack_lib.ld"
#include "driver/cpu/bd29/driver_lib.ld"

text_begin  = ADDR(.text) ;
text_size   = SIZEOF(.text) ;
text_end    = ADDR(.text) + SIZEOF(.text) ;

bss_begin = ADDR(.bss) ;
bss_size  = SIZEOF(.bss);

//nvbss_begin = ORIGIN(nvram);
nvbss_begin = NVRAM_DATA_START;
nvbss_size  = NVRAM_LIMIT - nvbss_begin;

data_addr = ADDR(.data)  ;
data_begin = text_begin + text_size;
data_size =  SIZEOF(.data) ;

_HEAP_BEGIN = NVRAM_END;
PROVIDE(HEAP_BEGIN = NVRAM_END);

_HEAP_END = NVRAM_LIMIT - UPDATA_SIZE - 1;
PROVIDE(HEAP_END = NVRAM_LIMIT - UPDATA_SIZE);

_MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN;
PROVIDE(MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN);


