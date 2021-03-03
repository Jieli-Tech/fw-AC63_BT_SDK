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

        *(.text*)
        bank_stub_start = .;
        *(.bank.stub.*)
        bank_stub_size = . - bank_stub_start;

        *(.LOG_TAG_CONST*)
        *(.rodata*)

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

		. = ALIGN(4); // must at tail, make rom_code size align 4

        clock_critical_handler_begin = .;
        KEEP(*(.clock_critical_txt))
        clock_critical_handler_end = .;
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


		/********maskrom arithmetic ****/
        *(.bfilt_code)
        *(.bfilt_table_maskroom)
       /********maskrom arithmetic end****/

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
		*(.mp2_encode_code)
		*(.mp2_encode_const)
		*(.mp2_encode_sparse_code)
		*(.mp2_encode_sparse_code)
		. = ALIGN(4);
		*(.mp3_enc_code)
		*(.mp3_enc_const)
		*(.mp3_enc_sparse_code)
		*(.mp3_enc_sparse_const)
		. = ALIGN(4);
		*(.adpcm_encode_code)
		*(.adpcm_encode_const)
		*(.adpcm_encode_sparse_code)
		*(.adpcm_encode_sparse_const)
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
        *(.data*)

        . = ALIGN(4);
		#include "btstack/btstack_lib_data.ld"
        . = ALIGN(4);
		#include "system/system_lib_data.ld"
		. = ALIGN(4);

        . = ALIGN(4);
		EQ_COEFF_BASE = . ;
		. = EQ_COEFF_BASE + 4 * EQ_SECTION_NUM * 14;
        . = ALIGN(4);


	  } > ram0

    .bss ALIGN(32):
    {
        *(.usb_h_dma)   //由于usb有个bug，会导致dma写的数据超出预设的buf，最长可能写超1k，为了避免死机，所以usb dma buffer后面放一些其他模块的buff来避免死机
        *(.usb_ep0)
        *(.dec_mix_buff)
        *(.sd0_var)
        *(.sd1_var)
        *(.dac_buff)
		. = ALIGN(4);
		#include "btstack/btstack_lib_bss.ld"
        . = ALIGN(4);
		#include "system/system_lib_bss.ld"
        . = ALIGN(4);

        *(.bss)
        . = ALIGN(4);
        *(.dts_dec_bss)
        *(.dts_bss)
        . = ALIGN(4);
        *(.m4a_dec_bss)
        *(.m4a_bss)
        . = ALIGN(4);
        *(COMMON)
        *(.volatile_ram)
		*(.audio_play_dma)

        . = ALIGN(4);
        *(.src_filt)
        *(.src_dma)

        . = ALIGN(4);
		*(.non_volatile_ram)
		. = ALIGN(32);
#if TCFG_VIR_UDISK_ENABLE == 1
        *(.usb_audio_play_dma)
        *(.usb_audio_rec_dma)
        *(.uac_rx)
        *(.mass_storage)
        *(.usb_msd_dma)
        *(.usb_hid_dma)
        *(.usb_iso_dma)
        *(.uac_var)
        *(.usb_config_var)
        . = ALIGN(32);
#endif

    } > ram0


	data_code_limit_begin = .;
	.data_code ALIGN(32):
	{
    	data_code_begin = .;
        *(.common*)
        . = ALIGN(4);

		media_code_begin = .;
        *(.media.*.text)
		media_code_end = .;
        media_code_size = media_code_end - media_code_begin;
        . = ALIGN(4);

		*(.flushinv_icache)
        *(.os_critical_code)
        *(.os_rewrite_code)
        *(.volatile_ram_code)
        *(.chargebox_code)

        *(.fat_data_code)
		. = ALIGN(4);
        _SPI_CODE_START = . ;
        *(.spi_code)
		. = ALIGN(4);
        _SPI_CODE_END = . ;

#if  (TCFG_LED7_RUN_RAM)
		. = ALIGN(4);
        *(.gpio_ram)
		. = ALIGN(4);
        *(.LED_code)
		. = ALIGN(4);
        *(.LED_const)
#endif
		. = ALIGN(4);
    	data_code_end = .;
	} > ram0



	overlay_begin = .;
	OVERLAY : NOCROSSREFS AT(0x200000) SUBALIGN(4)
    {
		.overlay_aec
		{
			*(.cvsd_data)
			*(.cvsd_const)
			*(.cvsd_code)

			KEEP(*(.aec_bss_id))
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
			KEEP(*(.mp3_bss_id))
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
			KEEP(*(.wma_bss_id))
			o_wma_end = .;

			*(.wma_mem)
			*(.wma_ctrl_mem)
			*(.wmapick_mem)
			*(.wmapick_ctrl_mem)
		}
		.overlay_wav
		{

			KEEP(*(.wav_bss_id))
			o_wav_end = .;

			*(.wav_dec_data)

			*(.wav_data)


			*(.wav_bss)
			*(.wav_dec_bss)
			*(.wav_mem)
			*(.wav_ctrl_mem)

		}
		.overlay_ape
        {
			KEEP(*(.ape_bss_id))
			o_ape_end = .;

            *(.ape_mem)
            *(.ape_ctrl_mem)
			*(.ape_dec_data)
			*(.ape_dec_bss)

			*(.ape_data)
		}
		.overlay_flac
        {
			KEEP(*(.flac_bss_id))
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
			KEEP(*(.m4a_bss_id))
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
			KEEP(*(.amr_bss_id))
			o_amr_end = .;

            *(.amr_mem)
            *(.amr_ctrl_mem)
		}
		.overlay_dts
        {
			KEEP(*(.dts_bss_id))
			o_dts_end = .;

            *(.dts_mem)
            *(.dts_ctrl_mem)
			*(.dts_dec_data)
			*(.dts_data)
		}

		.overlay_fm
		{
			*(.fm_code)
			KEEP(*(.fm_bss_id))
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

#include "update/update.ld"
#include "media/media.ld"
#include "driver/cpu/br25/driver_lib.ld"
#include "btctrler/port/br25/btctler_lib.ld"

//================== Section Info Export ====================//
text_begin  = ADDR(.text);
text_size   = SIZEOF(.text);
text_end    = ADDR(.text) + SIZEOF(.text);

bss_begin = ADDR(.bss);
bss_size  = SIZEOF(.bss);
bss_end   = bss_begin + bss_size;

data_addr = ADDR(.data);
data_begin = text_begin + text_size;
data_size =  SIZEOF(.data);

data_code_addr = ADDR(.data_code);
data_code_begin = data_begin + data_size;
data_code_size = SIZEOF(.data_code);

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
PROVIDE(HEAP_BEGIN = _HEAP_BEGIN);
PROVIDE(HEAP_END = _HEAP_END);
_MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN;
PROVIDE(MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN);


