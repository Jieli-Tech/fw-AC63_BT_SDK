
        btstack_bss_start = .;
        *(.bt_stack_bss)
        *(.ble_db_bss)
        *(.ble_sm_bss)
        *(.ble_att_bss)
        *(.ble_gatt_bss)
        *(.btstack_pool)

		/*mesh*/
        BTSTACK_LE_HOST_MESH_BSS_START = .;
        *(.ble_mesh_bss)
        *(.ble_mesh_tinycrypt_bss)
        BTSTACK_LE_HOST_MESH_BSS_SIZE = ABSOLUTE(. - BTSTACK_LE_HOST_MESH_BSS_START);
        
        . = ALIGN(4);
        _fmna_bss_begin = .;
        *(.fmna_stack_bss)
        *(.fmna_plaform_bss)
        *(.fmna_crypto_bss)
        _fmna_bss_end = .;
        btstack_fmna_bss_size   = _fmna_bss_end - _fmna_bss_begin;
        . = ALIGN(4);

        btstack_bss_end = .;

