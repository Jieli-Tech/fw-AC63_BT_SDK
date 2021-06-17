
	. = ALIGN(4);
	audio_sync_bss_begin = .;
        *(.audio_sync_bss)
	audio_sync_bss_end = .;
	. = ALIGN(4);

    *(.sbc_msbc_bss)
    *(.cvsd_bss)
    *(.aac_bss)
	*(.sbc_eng_bss)
	*(.bt_audioplc_bss)
    *(.lc3_decoder_bss)
    *(.wtgv2_bss)
    *(.wtgv2dec_bss)

    *(.anc_bss)
    *(.anc_user_bss)
    *(.anc_box_bss)
    *(.anc_btspp_bss)
    *(.anc_debug_bss)
