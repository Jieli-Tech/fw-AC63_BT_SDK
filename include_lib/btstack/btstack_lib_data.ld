
        btstack_data_start = .;
        *(.bt_stack_data)
        *(.ble_db_data)
        *(.ble_sm_data)
        *(.ble_att_data)
        *(.ble_gatt_data)

		/*mesh*/
        BTSTACK_LE_HOST_MESH_DATA_START = .;
		. = (. +3) & ~ 3;
        _net_buf_pool_list = .;
        *(._net_buf_pool.static.*)

        *(.ble_mesh_data)
        *(.ble_mesh_tinycrypt_data)
        BTSTACK_LE_HOST_MESH_DATA_SIZE = ABSOLUTE(. - BTSTACK_LE_HOST_MESH_DATA_START);

        _fmna_data_begin = .;
        *(.fmna_stack_data)
        *(.fmna_plaform_data)
        *(.fmna_crypto_data)
        _fmna_data_end = .;
        btstack_fmna_data_size  = _fmna_data_end - _fmna_data_begin;
        . = ALIGN(4);


        btstack_data_end = .;


