
		. = ALIGN(4);
        system_data_start = .;

		_video_subdev_begin = .;
		PROVIDE(video_subdev_begin = .);
		KEEP(*(.video_subdev.0))
		KEEP(*(.video_subdev.1))
		KEEP(*(.video_subdev.2))
		KEEP(*(.video_subdev.3))
		KEEP(*(.video_subdev.4))
		KEEP(*(.video_subdev.5))
		_video_subdev_end = .;
		PROVIDE(video_subdev_end = .);

		_audio_subdev_begin = .;
		PROVIDE(audio_subdev_begin = .);
		KEEP(*(.audio_subdev.0))
		KEEP(*(.audio_subdev.1))
		KEEP(*(.audio_subdev.2))
		KEEP(*(.audio_subdev.3))
		_audio_subdev_end = .;
		PROVIDE(audio_subdev_end = .);

		_iic_device_begin = .;
		PROVIDE(iic_device_begin = .);
		KEEP(*(.iic))
		_iic_device_end = .;
		PROVIDE(iic_device_end = .);

        _avin_spi_device_begin = .;
		PROVIDE(avin_spi_device_begin = .);
		KEEP(*(.sw_spi))
		_avin_spi_device_end = .;
		PROVIDE(avin_spi_device_end = .);

		_video_dev_begin = .;
		PROVIDE(video_dev_begin = .);
		KEEP(*(.video_device))
		_video_dev_end = .;
		PROVIDE(video_dev_end = .);

		_key_driver_ops_begin = .;
		PROVIDE(key_driver_ops_begin = .);
		KEEP(*(.key_driver_ops))
		_key_driver_ops_end = .;
		PROVIDE(key_driver_ops_end = .);

		_touch_driver_begin = .;
		PROVIDE(touch_driver_begin = .);
		KEEP(*(.touch_driver))
		_touch_driver_end = .;
		PROVIDE(touch_driver_end = .);

		_static_hi_timer_begin = .;
		PROVIDE(static_hi_timer_begin = .);
		KEEP(*(.hi_timer))
		_static_hi_timer_end = .;
		PROVIDE(static_hi_timer_end = .);

		_sys_cpu_timer_begin = .;
		PROVIDE(sys_cpu_timer_begin = .);
		KEEP(*(.sys_cpu_timer))
		_sys_cpu_timer_end = .;
		PROVIDE(sys_cpu_timer_end = .);

	    _sys_config_begin = .;
	    PROVIDE(sys_config_begin = .);
	    KEEP(*(.sys_cfg))
	    _sys_config_end = .;
	    PROVIDE(sys_config_end = .);

	    _sys_fat_begin = .;
	    PROVIDE(sys_fat_begin = .);
	    KEEP(*(.fs_fat))
	    _sys_fat_end = .;
	    PROVIDE(sys_fat_end = .);

		_app_begin = .;
		PROVIDE(app_begin = .);
		KEEP(*(.app))
		_app_end = .;
		PROVIDE(app_end = .);

		_os_begin = .;
		PROVIDE(os_begin = .);
/*		
 	*(.os_code)
		   *(.os_const)
*/		
		   *(.os_str)
		   *(.os_data)
		_os_end = .;
		PROVIDE(os_end = .);

	    *(.crypto_ecdh_data)
	    *(.crypto_data)

	    *(.mem_data)
        *(.os_port_data)

	    *(.uECC_data)
	    *(.ECDH_sample_data)

         __movable_slot_start = .;
         *(movable.slot.*);
         __movable_slot_end = .;

        system_data_end = .;


