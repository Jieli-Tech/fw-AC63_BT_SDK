
		. = ALIGN(4);
        system_text_start = .;

		 _device_node_begin = .;
   		PROVIDE(device_node_begin = .);
   		     *(.device)
   		_device_node_end = .;
   		PROVIDE(device_node_end = .);

		config_target_begin = .;
   		PROVIDE(config_target_begin = .);
   		     *(.config_target)
		config_target_end = .;
   		PROVIDE(config_target_end = .);

	    system_code_begin = .;
	        *(.system.*.text)
	    system_code_end = .;
		. = ALIGN(4);
		system_code_size = system_code_end - system_code_begin;

		vfs_ops_begin = .;
			*(.vfs_operations)
		vfs_ops_end = .;

	    _lib_version_begin = .;
	    PROVIDE(lib_version_begin = .);
	        *(.lib_version)
	    _lib_version_end = .;
	    PROVIDE(lib_version_end = .);

		_initcall_begin = .;
		PROVIDE(initcall_begin = .);
			*(.initcall)
		_initcall_end = .;
		PROVIDE(initcall_end = .);

		_early_initcall_begin = .;
		PROVIDE(early_initcall_begin = .);
			*(.early.initcall)
		_early_initcall_end = .;
		PROVIDE(early_initcall_end = .);

		_late_initcall_begin = .;
		PROVIDE(late_initcall_begin = .);
		   *(.late.initcall)
		_late_initcall_end = .;
		PROVIDE(late_initcall_end = .);

		_platform_initcall_begin = .;
		PROVIDE(platform_initcall_begin = .);
		   *(.platform.initcall)
		_platform_initcall_end = .;
		PROVIDE(platform_initcall_end = .);

		_module_initcall_begin = .;
		PROVIDE(module_initcall_begin = .);
		   *(.module.initcall)
		_module_initcall_end = .;
		PROVIDE(module_initcall_end = .);

		_sys_event_handler_begin = .;
		PROVIDE(sys_event_handler_begin = .);
		   *(.sys_event.4.handler)
		   *(.sys_event.3.handler)
		   *(.sys_event.2.handler)
		   *(.sys_event.1.handler)
		   *(.sys_event.0.handler)
		_sys_event_handler_end = .;
		PROVIDE(sys_event_handler_end = .);

		_syscfg_arg_begin = .;
		PROVIDE(syscfg_arg_begin = .);
		   *(.syscfg.arg)
		_syscfg_arg_end = .;
		PROVIDE(syscfg_arg_end = .);

		_syscfg_handler_begin = .;
		PROVIDE(syscfg_handler_begin = .);
		   *(.syscfg.handler)
		_syscfg_handler_end = .;
		PROVIDE(syscfg_handler_end = .);

		_syscfg_ops_begin = .;
		PROVIDE(syscfg_ops_begin = .);
		   *(.syscfg.2.ops)
		   *(.syscfg.1.ops)
		   *(.syscfg.0.ops)
		_syscfg_ops_end = .;
		PROVIDE(syscfg_ops_end = .);

		_server_info_begin = .;
		PROVIDE(server_info_begin = .);
		   *(.server_info)
		_server_info_end = .;
		PROVIDE(server_info_end = .);

		_bus_device_begin = .;
		PROVIDE(bus_device_begin = .);
		   *(.bus_device)
		_bus_device_end = .;
		PROVIDE(bus_device_end = .);

	    _sys_power_hal_ops_begin = .;
	    PROVIDE(sys_power_hal_ops_begin = .);
	        *(.sys_power_hal_ops)
	    _sys_power_hal_ops_end = .;
	    PROVIDE(sys_power_hal_ops_end = .);


		. = ALIGN(4);
	    lp_target_begin = .;
	    PROVIDE(lp_target_begin = .);
	        *(.lp_target)
	    lp_target_end = .;
	    PROVIDE(lp_target_end = .);

		. = ALIGN(4);
        deepsleep_target_begin = .;
        PROVIDE(deepsleep_target_begin = .);
            *(.deepsleep_target)
        deepsleep_target_end = .;
        PROVIDE(deepsleep_target_end = .);

	    crypto_begin = .;
	    *(.crypto_ecdh_code)
	    *(.crypto_ecdh_const)

	    *(.crypto_bigint_code)
	    *(.crypto_bigint_const)

	    *(.crypto_code)
	    *(.crypto_const)

	    *(.ECDH_sample_code)
	    *(.ECDH_sample_const)

	    *(.uECC_code)
	    *(.uECC_const)

	    *(.hmac_code)
	    *(.hmac_const)

	    *(.hash_sample_code)
	    *(.hash_sample_const)

	    *(.aes_cmac_sample_code)
	    *(.aes_cmac_sample_const)
	    crypto_end = .;
	    crypto_size = . - crypto_begin;

        *(.mem_code)
        *(.mem_const)

        *(.os_port_code)
        *(.os_port_const)

        system_text_end = .;


