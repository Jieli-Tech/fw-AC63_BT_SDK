
	. = ALIGN(4);
	audio_sync_bss_begin = .;
        *(.audio_sync_bss)
	audio_sync_bss_end = .;
	. = ALIGN(4);

    *(.cvsd_bss)
    *(.aac_bss)
	*(.sbc_eng_bss)
	*(.bt_audioplc_bss)
