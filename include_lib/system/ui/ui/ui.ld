   	    lcd_interface_begin = .;
        KEEP(*(.lcd_if_info))
        lcd_interface_end = .;

		ui_style_begin = .;
		KEEP(*(.ui_style))
		ui_style_end = .;

        elm_event_handler_begin_UPGRADE = .;
        KEEP(*(.elm_event_handler_UPGRADE))
		elm_event_handler_end_UPGRADE = .;

        elm_event_handler_begin_JL = .;
        KEEP(*(.elm_event_handler_JL))
		elm_event_handler_end_JL = .;

        elm_event_handler_begin_JL_01 = .;
        KEEP(*(.elm_event_handler_JL_01))
        elm_event_handler_end_JL_01 = .;

        elm_event_handler_begin_JL_02 = .;
        KEEP(*(.elm_event_handler_JL_02))
        elm_event_handler_end_JL_02 = .;

        elm_event_handler_begin_JL_03 = .;
        KEEP(*(.elm_event_handler_JL_03))
        elm_event_handler_end_JL_03 = .;

		control_event_handler_begin = .;
        KEEP(*(.control_event_handler))
		control_event_handler_end = .;

        control_ops_begin = .;
        KEEP(*(.control_ops))
        control_ops_end = .;

		vg_event_handler_begin = .;
           *(.vg_event_handler)
        vg_event_handler_end = .;

        on_show_map_begin = .;
           *(.on_show_map)
        on_show_map_end = .;

        on_click_map_begin = .;
           *(.on_click_map)
        on_click_map_end = .;

        on_touch_map_begin = .;
           *(.on_touch_map)
        on_touch_map_end = .;

        on_change_map_begin = .;
           *(.on_change_map)
        on_change_map_end = .;

        on_key_map_begin = .;
           *(.on_key_map)
        on_key_map_end = .;

		img_loader_begin = .;
		   *(.img_loader)
		img_loader_end = .;

		battery_notify_begin = .;
           *(.battery_notify)
        battery_notify_end = .;




