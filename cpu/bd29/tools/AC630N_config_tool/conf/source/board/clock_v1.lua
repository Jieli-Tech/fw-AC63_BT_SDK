

require("const-def")
require("typeids")

clock_module = {}


local MOUDLE_COMMENT_NAME = "//                                  时钟配置                                       //"

local comment_begin = cfg:i32("clock注释开始", 0)
local clock_comment_begin_htext = module_output_comment(comment_begin, MOUDLE_COMMENT_NAME);

--print(MOUDLE_COMMENT_NAME);
local SYS_CLK_SRC = cfg:enumMap("系统时钟源配置", 
	{
		[0] = "SYS_CLOCK_INPUT_RC", 
		[1] = "SYS_CLOCK_INPUT_BT_OSC", 
		[2] = "SYS_CLOCK_INPUT_RTOSCH", 
		[3] = "SYS_CLOCK_INPUT_RTOSCL", 
		[4] = "SYS_CLOCK_INPUT_PAT", 
		[5] = "SYS_CLOCK_INPUT_PLL_BT_OSC", 
		[6] = "SYS_CLOCK_INPUT_PLL_RTOSCH", 
		[7] = "SYS_CLOCK_INPUT_PLL_PAT"
	}
)

---------------------------------[[ clock 输出列表]]---------------------------------
local SYS_CLK_SRC_OUTPUT_TABLE = {
    [0] = "SYS_CLOCK_INPUT_RC", 
    [1] = "SYS_CLOCK_INPUT_BT_OSC", 
    [2] = "SYS_CLOCK_INPUT_RTOSCH", 
    [3] = "SYS_CLOCK_INPUT_RTOSCL", 
    [4] = "SYS_CLOCK_INPUT_PAT", 
    [5] = "SYS_CLOCK_INPUT_PLL_BT_OSC",
    [6] = "SYS_CLOCK_INPUT_PLL_RTOSCH", 
    [7] = "SYS_CLOCK_INPUT_PLL_PAT"
}

--[[
//==========================================================================================//
//                                  配置项: CLOCK_CFG                                       //
//==========================================================================================//
--]]


--[[=============================== 配置子项1-0: clock_en ================================--]]
local clock_en = cfg:enum("时钟配置使能开关:", ENABLE_SWITCH, 0)
clock_en:setOSize(1)
clock_en:setTextOut(
	function (ty) 
		if (ty == 3) then
		--	return "#define TCFG_CLOCK_ENABLE  " .. clock_en.val .. "\n" 
		end
	end
)

-- 子项显示
local clock_en_view = cfg:hBox {
        cfg:stLabel(clock_en.name),
        cfg:enumView(clock_en),
        cfg:stSpacer(),
};


--[[=============================== 配置子项1-1: sys_clk_src ================================--]]
local sys_clk_src = cfg:enum("系统时钟源选择:", SYS_CLK_SRC, 5)
sys_clk_src:setOSize(1)
sys_clk_src:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_CLOCK_SYS_SRC" .. TAB_TABLE[5] .. SYS_CLK_SRC_OUTPUT_TABLE[sys_clk_src.val] .. NLINE_TABLE[1]
		end
	end
)
sys_clk_src:addDeps{clock_en}
sys_clk_src:setDepChangeHook(function ()
    if (clock_en.val == 0) then
        sys_clk_src:setShow(false);
    else
        sys_clk_src:setShow(true);
    end
end);

-- 子项显示
local sys_clk_src_view = cfg:hBox {
        cfg:stLabel(sys_clk_src.name),
        cfg:enumView(sys_clk_src),
        cfg:stSpacer(),
};



--[[=============================== 配置子项1-2: sys_clk ================================--]]
local sys_clk = cfg:i32("系统频率配置:", 24*1000000)
sys_clk:setOSize(4)
sys_clk:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_CLOCK_SYS_HZ" .. TAB_TABLE[5]  .. sys_clk.val .. NLINE_TABLE[1]
		end
	end
)
sys_clk:addDeps{clock_en}
sys_clk:setDepChangeHook(function ()
    if (clock_en.val == 0) then
        sys_clk:setShow(false);
    else
        sys_clk:setShow(true);
    end
end);

-- 子项显示
local sys_clk_view = cfg:hBox {
        cfg:stLabel(sys_clk.name),
        cfg:inputView(sys_clk),
        cfg:stSpacer(),
};



--[[=============================== 配置子项1-3: sys_clk ================================--]]
local osc_clk = cfg:i32("晶振频率配置:", 24*1000000)
osc_clk:setOSize(4)
osc_clk:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_CLOCK_OSC_HZ" .. TAB_TABLE[5]  .. osc_clk.val .. NLINE_TABLE[2]
		end
	end
)
osc_clk:addDeps{clock_en}
osc_clk:setDepChangeHook(function ()
    if (clock_en.val == 0) then
        osc_clk:setShow(false);
    else
        osc_clk:setShow(true);
    end
end);

-- 子项显示
local osc_clk_view = cfg:hBox {
        cfg:stLabel(osc_clk.name),
        cfg:inputView(osc_clk),
        cfg:stSpacer(),
};


--[[===================================================================================
==================================== 模块返回汇总 =====================================
====================================================================================--]]
-- A. 输出htext
local clock_output_htext_table = {};
insert_item_to_list(clock_output_htext_table, clock_comment_begin_htext);
--insert_item_to_list(clock_output_htext_table, clock_en);
insert_item_to_list(clock_output_htext_table, sys_clk_src);
insert_item_to_list(clock_output_htext_table, sys_clk);
insert_item_to_list(clock_output_htext_table, osc_clk);

insert_list_to_list(board_output_text_tabs, clock_output_htext_table);
if enable_moudles["clock"] == false then
    return;
end
-- B. 输出ctext：无

-- C. 输出bin：无

-- E. 默认值
local clock_cfg_default_table = {};
insert_item_to_list(clock_cfg_default_table, clock_en);
insert_item_to_list(clock_cfg_default_table, sys_clk_src);
insert_item_to_list(clock_cfg_default_table, sys_clk);
insert_item_to_list(clock_cfg_default_table, osc_clk);

-- D. 显示
local clock_group_view_list = {}
if open_by_program == "create" then
	insert_item_to_list(clock_group_view_list, clock_en_view);
end
insert_item_to_list(clock_group_view_list, sys_clk_src_view);
insert_item_to_list(clock_group_view_list, sys_clk_view);
insert_item_to_list(clock_group_view_list, osc_clk_view);

local clock_group_view = cfg:stGroup("",
        cfg:vBox(clock_group_view_list));

local clock_default_button_view = cfg:stButton(" 时钟配置恢复默认值 ", reset_to_default(clock_cfg_default_table));

local clock_view_list = cfg:vBox {
        clock_group_view,
        clock_default_button_view,
};

local clock_view = {"系统时钟配置",
        clock_view_list,
};

if open_by_program == "create" then
    insert_item_to_list(board_view_tabs, clock_view);
end

-- F. bindGroup：无



