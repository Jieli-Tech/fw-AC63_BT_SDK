
local MOUDLE_COMMENT_NAME = "//                                  PLCNT配置                                      //"

local comment_begin = cfg:i32("pulse注释开始", 0)
local pulse_comment_begin_htext = module_output_comment(comment_begin, MOUDLE_COMMENT_NAME);

--print(MOUDLE_COMMENT_NAME);
---------------------------------[[ 触摸键 ]]---------------------------------
local PULSE_MODE = cfg:enumMap("工作模式", 
							{
								[0] = "触摸按键", 
								[1] = "波形检测", 
							}
						)

local PULSE_CLK_SRC = cfg:enumMap("时钟源选择", 
							{
								[0] = "OSC_CLK", 
								[1] = "MUX_IN_CLK", 
								[2] = "PLL_192M", 
								[3] = "PLL_240M", 
							}
						)

---------------------------------[[ plcnt 输出列表 ]]---------------------------------
local PLCNT_MODE_OUTPUT_TABLE = {
    [0] = "TOUCH_KEY",
    [1] = "SINGAL_MEASURE",
}

local PLCNT_CLK_OUTPUT_TABLE = {
    [0] = "PLCNT_OSC_CLK",
    [1] = "PLCNT_MUX_IN_CLK",
    [2] = "PLCNT_PLL_192M_CLK",
    [3] = "PLCNT_PLL_240M_CLK",
}


--[[
//==========================================================================================//
//                                  配置项: PULSE_CFG                                       //
//==========================================================================================//
--]]

--[[=============================== 配置子项1-0: pulse_en ================================--]]
local pulse_en = cfg:enum("触摸按键使能开关:", ENABLE_SWITCH, 0)
pulse_en:setOSize(1)
pulse_en:setTextOut(
	function (ty) 
		if (ty == 3) then
            if enable_moudles["pulse"] == false then
			    return "#define TCFG_PULSE_ENABLE" .. TAB_TABLE[5] .. ENABLE_SWITCH_TABLE[0] .. NLINE_TABLE[2]
            end
			return "#define TCFG_PULSE_ENABLE" .. TAB_TABLE[5] .. ENABLE_SWITCH_TABLE[pulse_en.val] .. NLINE_TABLE[2]
		end
	end
)

-- 子项显示
local pulse_en_view = cfg:hBox {
        cfg:stLabel(pulse_en.name),
        cfg:enumView(pulse_en),
        cfg:stSpacer(),
};



--[[=============================== 配置子项1-1: pulse_mode_sel ================================--]]
local pulse_mode_sel = cfg:enum("工作模式选择:", PULSE_MODE, 0)
pulse_mode_sel:setOSize(1)
pulse_mode_sel:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_PULSE_MODE_SEL" .. TAB_TABLE[5] .. PLCNT_MODE_OUTPUT_TABLE[pulse_mode_sel.val] .. NLINE_TABLE[1]
		end
	end
)
pulse_mode_sel:addDeps{pulse_en}
pulse_mode_sel:setDepChangeHook(function ()
    if (pulse_en.val == 0) then
        pulse_mode_sel:setShow(false);
    else
        pulse_mode_sel:setShow(true);
    end
end);

-- 子项显示
local pulse_mode_sel_view = cfg:hBox {
        cfg:stLabel(pulse_mode_sel.name),
        cfg:enumView(pulse_mode_sel),
        cfg:stSpacer(),
};



--[[=============================== 配置子项1-2: pulse_port_sel ================================--]]
local pulse_port = cfg:enum("工作模式选择:", PULSE_MODE, 0)
local pulse_port = cfg:enum("触摸键引脚选择:", PORTS, 0xFF)
pulse_port:setOSize(1)
pulse_port:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_PULSE_PORT" .. TAB_TABLE[6] .. PORTS_TABLE[pulse_port.val] .. NLINE_TABLE[1]
		end
	end
)
pulse_port:addDeps{pulse_en}
pulse_port:setDepChangeHook(function ()
    if (pulse_en.val == 0) then
        pulse_port:setShow(false);
    else
        pulse_port:setShow(true);
    end
end);

-- 子项显示
local pulse_port_sel_view = cfg:hBox {
        cfg:stLabel(pulse_port.name),
        cfg:enumView(pulse_port),
        cfg:stSpacer(),
};


--[[=============================== 配置子项1-3: pulse_clk_src_sel ================================--]]
local pulse_clk_src_sel = cfg:enum("触摸键时钟源选择:", PULSE_CLK_SRC, 0)
pulse_clk_src_sel:setOSize(1)
pulse_clk_src_sel:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_PULSE_CLK_SRC_SEL" .. TAB_TABLE[4] .. PLCNT_CLK_OUTPUT_TABLE[pulse_clk_src_sel.val] .. NLINE_TABLE[2]
		end
	end
)
pulse_clk_src_sel:addDeps{pulse_en}
pulse_clk_src_sel:setDepChangeHook(function ()
    if (pulse_en.val == 0) then
        pulse_clk_src_sel:setShow(false);
    else
        pulse_clk_src_sel:setShow(true);
    end
end);

-- 子项显示
local pulse_clk_src_sel_view = cfg:hBox {
        cfg:stLabel(pulse_clk_src_sel.name),
        cfg:enumView(pulse_clk_src_sel),
        cfg:stSpacer(),
};


--[[===================================================================================
==================================== 模块返回汇总 =====================================
====================================================================================--]]
-- A. 输出htext
local pulse_output_htext = {};
insert_item_to_list(pulse_output_htext, pulse_comment_begin_htext);
insert_item_to_list(pulse_output_htext, pulse_en);

if enable_moudles["pulse"] == true then
insert_item_to_list(pulse_output_htext, pulse_mode_sel);
insert_item_to_list(pulse_output_htext, pulse_port);
insert_item_to_list(pulse_output_htext, pulse_clk_src_sel);
end

insert_list_to_list(board_output_text_tabs, pulse_output_htext);

if enable_moudles["pulse"] == false then
    return;
end

-- B. 输出ctext：无
-- C. 输出bin：无

-- E. 默认值
local pulse_cfg_default_table = {};

if open_by_program == "create" then
	insert_item_to_list(pulse_cfg_default_table, pulse_en);
end

insert_item_to_list(pulse_cfg_default_table, pulse_mode_sel);
insert_item_to_list(pulse_cfg_default_table, pulse_port);
insert_item_to_list(pulse_cfg_default_table, pulse_clk_src_sel);

local pulse_default_button_view = cfg:stButton(" 触摸按键配置恢复默认值 ", reset_to_default(pulse_cfg_default_table));

-- D. 显示
local pulse_group_view_list = {}

if open_by_program == "create" then
	insert_item_to_list(pulse_group_view_list, pulse_en_view);
end

insert_item_to_list(pulse_group_view_list, pulse_mode_sel_view);
insert_item_to_list(pulse_group_view_list, pulse_port_sel_view);
insert_item_to_list(pulse_group_view_list, pulse_clk_src_sel_view);

local pulse_group_view = cfg:stGroup("",
        cfg:vBox (pulse_group_view_list)
);

local pulse_view_list = cfg:vBox {
        pulse_group_view,
        pulse_default_button_view,
};

local pulse_view = {"触摸按键配置",
        pulse_view_list,
};

if open_by_program == "create" then
    insert_item_to_list(board_view_tabs, pulse_view);
end

-- F. bindGroup：无

