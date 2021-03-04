
local MOUDLE_COMMENT_NAME = "//                                  LED 配置                                       //"

local comment_begin = cfg:i32("pwmled注释开始", 0);
local pwmled_comment_begin_htext = module_output_comment(comment_begin, MOUDLE_COMMENT_NAME);

--print(MOUDLE_COMMENT_NAME);
--[[
//==========================================================================================//
//                                  配置项: PWM_LED_CFG                                     //
//==========================================================================================//
--]]
local LED_LIGHT_LEVEL = cfg:enumMap("LED 亮度",
    {
        [1] = " 1 级亮度(最暗)",
        [2] = " 2 级亮度",
        [3] = " 3 级亮度",
        [4] = " 4 级亮度",
        [5] = " 5 级亮度",
        [6] = " 6 级亮度",
        [7] = " 7 级亮度",
        [8] = " 8 级亮度",
        [9] = " 9 级亮度",
        [10] = " 10 级亮度(最亮)",
    }
);

local LED_FAST_FLASH_FREQUENCY = cfg:enumMap("LED 快闪闪烁频率",
    {
        [1] = " 每 100 毫秒闪烁 1 次",
        [2] = " 每 200 毫秒闪烁 1 次",
        [3] = " 每 300 毫秒闪烁 1 次",
        [4] = " 每 400 毫秒闪烁 1 次",
    }
);

local LED_SLOW_FLASH_FREQUENCY = cfg:enumMap("LED 慢闪闪烁频率",
    {
        [1] = " 每 0.5 秒闪烁 1 次",
        [2] = " 每 1 秒闪烁 1 次",
        [3] = " 每 1.5 秒闪烁 1 次",
        [4] = " 每 2 秒闪烁 1 次",
        [5] = " 每 2.5 秒闪烁 1 次",
        [6] = " 每 3 秒闪烁 1 次",
        [7] = " 每 3.5 秒闪烁 1 次",
        [8] = " 每 4 秒闪烁 1 次",
    }
);


local function add_depend_show(dep_cfg, new_cfg)
    new_cfg:addDeps{dep_cfg};
    new_cfg:setDepChangeHook(function() new_cfg:setShow(dep_cfg.val ~= 0); end);
end;
--[[=============================== 配置子项0: pwmled_en ================================--]]
local pwmled_en = cfg:enum("LED灯使能开关:", ENABLE_SWITCH, 0)
pwmled_en:setOSize(1);
pwmled_en:setTextOut(
	function (ty) 
		if (ty == 3) then
            if enable_moudles["pwmled"] == false then
			    return "#define TCFG_PWMLED_ENABLE" .. TAB_TABLE[5] .. ENABLE_SWITCH_TABLE[0] .. NLINE_TABLE[2]
            end
			return "#define TCFG_PWMLED_ENABLE" .. TAB_TABLE[5] .. ENABLE_SWITCH_TABLE[pwmled_en.val] .. NLINE_TABLE[2]
		end
	end
);

-- 子项显示
local pwmled_en_group_view = cfg:stGroup("",
        cfg:hBox {
            cfg:stLabel(pwmled_en.name),
            cfg:enumView(pwmled_en),
            cfg:stSpacer(),
        }
);



--[[=============================== 配置子项1: pwmled_port_sel ================================--]]
local pwmled_port_sel = cfg:enum("LED 灯引脚选择:", PORTS, 22);
pwmled_port_sel:setOSize(1);
pwmled_port_sel:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_PWMLED_PORT" .. TAB_TABLE[5] ..  PORTS_TABLE[pwmled_port_sel.val] .. NLINE_TABLE[1]
		end
	end
);
add_depend_show(pwmled_en, pwmled_port_sel);

-- 子项显示
local pwmled_port_sel_group_view = cfg:stGroup("",
        cfg:hBox {
            cfg:stLabel(pwmled_port_sel.name),
            cfg:enumView(pwmled_port_sel),
            cfg:stSpacer(),
        }
);


--[[=============================== 配置子项2: 红灯亮度设置 ================================--]]
local red_led_light_sel = cfg:enum("LED 红灯亮度设置: ", LED_LIGHT_LEVEL, 3);
red_led_light_sel:setOSize(1);
red_led_light_sel:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_REDLED_LIGHT" .. TAB_TABLE[5] .. red_led_light_sel.val .. NLINE_TABLE[1]
		end
	end
);
add_depend_show(pwmled_en, red_led_light_sel);

-- 子项显示
local red_led_light_sel_hbox_view = cfg:hBox {
            cfg:stLabel(red_led_light_sel.name),
            cfg:enumView(red_led_light_sel),
            cfg:stSpacer(),
};

--[[=============================== 配置子项3: 蓝灯亮度设置 ================================--]]
local blue_led_light_sel = cfg:enum("LED 蓝灯亮度设置: ", LED_LIGHT_LEVEL, 3);
blue_led_light_sel:setOSize(1);
blue_led_light_sel:setTextOut(
	function (ty)
		if (ty == 3) then
			return "#define TCFG_BLUELED_LIGHT" .. TAB_TABLE[5] .. blue_led_light_sel.val .. NLINE_TABLE[1]
		end
	end
);
add_depend_show(pwmled_en, blue_led_light_sel);

-- 子项显示
local blue_led_light_sel_hbox_view = cfg:hBox {
            cfg:stLabel(blue_led_light_sel.name),
            cfg:enumView(blue_led_light_sel),
            cfg:stSpacer(),
};


--[[=============================== 配置子项4: 快闪频率设置 ================================--]]
local led_fast_flash_freq_sel = cfg:enum("LED 单独快闪频率设置: ", LED_FAST_FLASH_FREQUENCY, 3);
led_fast_flash_freq_sel:setOSize(1);
led_fast_flash_freq_sel:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_SINGLE_FAST_FLASH_FREQ" .. TAB_TABLE[3] .. led_fast_flash_freq_sel.val .. NLINE_TABLE[1]
		end
	end
);
add_depend_show(pwmled_en, led_fast_flash_freq_sel);

-- 子项显示
local led_fast_flash_freq_sel_hbox_view = cfg:hBox {
            cfg:stLabel(led_fast_flash_freq_sel.name),
            cfg:enumView(led_fast_flash_freq_sel),
            cfg:stSpacer(),
};

--[[=============================== 配置子项5: 慢闪频率设置 ================================--]]
local led_slow_flash_freq_sel = cfg:enum("LED 单独慢闪频率设置: ", LED_SLOW_FLASH_FREQUENCY, 3);
led_slow_flash_freq_sel:setOSize(1);
led_slow_flash_freq_sel:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_SINGLE_SLOW_FLASH_FREQ" .. TAB_TABLE[3] .. led_slow_flash_freq_sel.val .. NLINE_TABLE[1]
		end
	end
);
add_depend_show(pwmled_en, led_slow_flash_freq_sel);

-- 子项显示
local led_slow_flash_freq_sel_hbox_view = cfg:hBox {
            cfg:stLabel(led_slow_flash_freq_sel.name),
            cfg:enumView(led_slow_flash_freq_sel),
            cfg:stSpacer(),
};


--[[=============================== 配置子项5: LED交替快闪频率设置 ================================--]]
local double_led_fast_flash_freq_sel = cfg:enum("LED 交替快闪频率设置: ", LED_FAST_FLASH_FREQUENCY, 3);
double_led_fast_flash_freq_sel:setOSize(1);
double_led_fast_flash_freq_sel:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_DOUBLE_FAST_FLASH_FREQ" .. TAB_TABLE[3] .. double_led_fast_flash_freq_sel.val .. NLINE_TABLE[2]
		end
	end
);
add_depend_show(pwmled_en, double_led_fast_flash_freq_sel);

-- 子项显示
local double_led_fast_flash_freq_sel_hbox_view = cfg:hBox {
            cfg:stLabel(double_led_fast_flash_freq_sel.name),
            cfg:enumView(double_led_fast_flash_freq_sel),
            cfg:stSpacer(),
};

--[[=============================== 配置子项6: LED交替慢闪频率设置 ================================--]]
local double_led_slow_flash_freq_sel = cfg:enum("LED 交替慢闪频率设置: ", LED_SLOW_FLASH_FREQUENCY, 3);
double_led_slow_flash_freq_sel:setOSize(1);
double_led_slow_flash_freq_sel:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_DOUBLE_SLOW_FLASH_FREQ" .. TAB_TABLE[3] .. double_led_slow_flash_freq_sel.val .. NLINE_TABLE[1]
		end
	end
);
add_depend_show(pwmled_en, double_led_slow_flash_freq_sel);

-- 子项显示
local double_led_slow_flash_freq_sel_hbox_view = cfg:hBox {
            cfg:stLabel(double_led_slow_flash_freq_sel.name),
            cfg:enumView(double_led_slow_flash_freq_sel),
            cfg:stSpacer(),
};

-- 子项显示
local pwmled_para_sel_group_view = cfg:stGroup("",
        cfg:vBox {
            red_led_light_sel_hbox_view,
            blue_led_light_sel_hbox_view,
            led_slow_flash_freq_sel_hbox_view,
            led_fast_flash_freq_sel_hbox_view,
            double_led_slow_flash_freq_sel_hbox_view,
            double_led_fast_flash_freq_sel_hbox_view,
        }
);


--[[===================================================================================
==================================== 模块返回汇总 =====================================
====================================================================================--]]
-- A. 输出htext
local pwmled_output_htext = {};
insert_item_to_list(pwmled_output_htext, pwmled_comment_begin_htext);
insert_item_to_list(pwmled_output_htext, pwmled_en);

if enable_moudles["pwmled"] == true then
    insert_item_to_list(pwmled_output_htext, pwmled_port_sel);
    insert_item_to_list(pwmled_output_htext, red_led_light_sel);
    insert_item_to_list(pwmled_output_htext, blue_led_light_sel);
    insert_item_to_list(pwmled_output_htext, led_slow_flash_freq_sel);
    insert_item_to_list(pwmled_output_htext, led_fast_flash_freq_sel);
    insert_item_to_list(pwmled_output_htext, double_led_slow_flash_freq_sel);
    insert_item_to_list(pwmled_output_htext, double_led_fast_flash_freq_sel);
end

insert_list_to_list(board_output_text_tabs, pwmled_output_htext);

if enable_moudles["pulse"] == false then
    return;
end

-- B. 输出ctext：无

-- C. 输出bin：
--[[============================== 输出到 bin 汇总 ================================--]]
local pwmled_output_bin_list = {
    pwmled_en,
    pwmled_port_sel,
    red_led_light_sel,
    blue_led_light_sel,
    led_slow_flash_freq_sel,
    led_fast_flash_freq_sel,
    double_led_slow_flash_freq_sel,
    double_led_fast_flash_freq_sel,
};

local pwmled_output_bin = cfg:group("PWM_LED_CFG",
    BIN_ONLY_CFG["HW_CFG"].pwmled.id,
    1,
    pwmled_output_bin_list
);

insert_item_to_list(board_output_bin_tabs, pwmled_output_bin);

-- E. 默认值
local pwmled_cfg_default_table = {};

if open_by_program == "create" then
	insert_item_to_list(pwmled_cfg_default_table, pwmled_en);
end

insert_item_to_list(pwmled_cfg_default_table, pwmled_port_sel);
insert_item_to_list(pwmled_cfg_default_table, red_led_light_sel);
insert_item_to_list(pwmled_cfg_default_table, blue_led_light_sel);
insert_item_to_list(pwmled_cfg_default_table, led_slow_flash_freq_sel);
insert_item_to_list(pwmled_cfg_default_table, led_fast_flash_freq_sel);
insert_item_to_list(pwmled_cfg_default_table, double_led_slow_flash_freq_sel);
insert_item_to_list(pwmled_cfg_default_table, double_led_fast_flash_freq_sel);
local pwmled_default_button_view = cfg:stButton(" LED 配置恢复默认值 ", reset_to_default(pwmled_cfg_default_table));

-- D. 显示
local pwmled_group_view_list = {}

if open_by_program == "create" then
	insert_item_to_list(pwmled_group_view_list, pwmled_en_group_view);
end

insert_item_to_list(pwmled_group_view_list, pwmled_port_sel_group_view);
insert_item_to_list(pwmled_group_view_list, pwmled_para_sel_group_view);

local pwmled_group_view_list = cfg:stGroup("",
    cfg:stHScroll(
		cfg:vBox(pwmled_group_view_list)
	)
);

local pwmled_view_list = cfg:vBox {
    pwmled_group_view_list,
	pwmled_default_button_view,
};

local pwmled_view = {"LED灯配置",
    pwmled_view_list,
};

insert_item_to_list(board_view_tabs, pwmled_view);

-- F. bindGroup：无
