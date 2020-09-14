
local MOUDLE_COMMENT_NAME = "//                                  IRFLT配置                                      //"

local comment_begin = cfg:i32("irflt注释开始", 0)
local irflt_comment_begin_htext = module_output_comment(comment_begin, MOUDLE_COMMENT_NAME);

--print(MOUDLE_COMMENT_NAME);

---------------------------------[[ 红外 ]]---------------------------------
local IRFLT_TIMER_SOURCE = cfg:enumMap("IRFLT_TIMER_SOURCE", 
									{
										[4] = "TIMER0", 
										[1] = "TIMER1", 
										[6] = "TIMER2", 
										[7] = "TIMER3"
									}
								)


--[[
//==========================================================================================//
//                                  配置项: IRFLT_CFG                                       //
//==========================================================================================//
--]]


--[[=============================== 配置子项1-0: irflt_en ================================--]]
local irflt_en = cfg:enum("红外模块使能开关:", ENABLE_SWITCH, 0)
irflt_en:setOSize(1)
irflt_en:setTextOut(
	function (ty) 
		if (ty == 3) then
            if enable_moudles["irflt"] == false then
			    return "#define TCFG_IRFLT_ENABLE" .. TAB_TABLE[5] .. ENABLE_SWITCH_TABLE[0] .. NLINE_TABLE[2]
            end
			return "#define TCFG_IRFLT_ENABLE" .. TAB_TABLE[5] .. ENABLE_SWITCH_TABLE[irflt_en.val] .. NLINE_TABLE[2]
		end
	end
)

-- 子项显示
local irflt_en_view = cfg:hBox {
        cfg:stLabel(irflt_en.name),
        cfg:enumView(irflt_en),
        cfg:stSpacer(),
};

--[[=============================== 配置子项1-1: irflt_port ================================--]]
local irflt_port = cfg:enum("红外模块检测引脚选择:", PORTS, 0xFF)
irflt_port:setOSize(1)
irflt_port:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_IRFLT_PORT" .. TAB_TABLE[6] .. PORTS_TABLE[irflt_port.val] .. NLINE_TABLE[1]
		end
	end
)
irflt_port:addDeps{irflt_en}
irflt_port:setDepChangeHook(function ()
    if (irflt_en.val == 0) then
        irflt_port:setShow(false);
    else
        irflt_port:setShow(true);
    end
end);

-- 子项显示
local irflt_port_view = cfg:hBox {
        cfg:stLabel(irflt_port.name),
        cfg:enumView(irflt_port),
        cfg:stSpacer(),
};


--[[=============================== 配置子项1-2: irflt_timer_src_sel ================================--]]
local irflt_timer_src_sel = cfg:enum("红外模块时钟源选择:", IRFLT_TIMER_SOURCE, 4)
irflt_timer_src_sel:setOSize(1)
irflt_timer_src_sel:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_IRFLT_TIMER_SRC_SEL" .. TAB_TABLE[3] .. irflt_timer_src_sel.val .. NLINE_TABLE[2]
		end
	end
)
irflt_timer_src_sel:addDeps{irflt_en}
irflt_timer_src_sel:setDepChangeHook(function ()
    if (irflt_en.val == 0) then
        irflt_timer_src_sel:setShow(false);
    else
        irflt_timer_src_sel:setShow(true);
    end
end);

-- 子项显示
local irflt_timer_src_sel_view = cfg:hBox {
        cfg:stLabel(irflt_timer_src_sel.name),
        cfg:enumView(irflt_timer_src_sel),
        cfg:stSpacer(),
};


--[[===================================================================================
==================================== 模块返回汇总 =====================================
====================================================================================--]]
-- A. 输出htext
local irflt_output_htext = {};
insert_item_to_list(irflt_output_htext, irflt_comment_begin_htext);
insert_item_to_list(irflt_output_htext, irflt_en);

if enable_moudles["irflt"] == true then
insert_item_to_list(irflt_output_htext, irflt_port);
insert_item_to_list(irflt_output_htext, irflt_timer_src_sel);
end

insert_list_to_list(board_output_text_tabs, irflt_output_htext);
if enable_moudles["irflt"] == false then
    return;
end

-- B. 输出ctext：无
-- C. 输出bin：无

-- E. 默认值
local irflt_cfg_default_table = {};
if open_by_program == "create" then
	insert_item_to_list(irflt_cfg_default_table, irflt_en);
end
insert_item_to_list(irflt_cfg_default_table, irflt_port);
insert_item_to_list(irflt_cfg_default_table, irflt_timer_src_sel);

-- D. 显示
local irflt_group_view_list = {}

if open_by_program == "create" then
	insert_item_to_list(irflt_group_view_list, irflt_en_view);
end

insert_item_to_list(irflt_group_view_list, irflt_port_view);
insert_item_to_list(irflt_group_view_list, irflt_timer_src_sel_view);

local irflt_group_view = cfg:stGroup("红外模块配置",
        cfg:vBox (irflt_group_view_list)
);

local irflt_default_button_view = cfg:stButton(" 红外配置恢复默认值 ", reset_to_default(irflt_cfg_default_table));
local irflt_view_list = cfg:vBox {
        irflt_group_view,
        irflt_default_button_view,
};

local irflt_view = {"红外模块配置",
        irflt_view_list,
};

if open_by_program == "create" then
    insert_item_to_list(board_view_tabs, irflt_view);
end

-- F. bindGroup：无





