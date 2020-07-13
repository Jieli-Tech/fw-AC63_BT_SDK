

require("const-def")
require("typeids")



lowpower_module = {}

local MOUDLE_COMMENT_NAME = "//                                  低功耗配置                                     //"

local comment_begin = cfg:i32("lowerpower注释开始", 0)
local lowpower_comment_begin_htext = module_output_comment(comment_begin, MOUDLE_COMMENT_NAME);

--print(MOUDLE_COMMENT_NAME);
local POWER_MODE_SEL = cfg:enumMap("电源模式选择", 
	{
        [0] = "PWR_NO_CHANGE",
        [1] = "PWR_LDO33",
		[2] = "PWR_LDO15",
		[3] = "PWR_DCDC15",
	}
)

local VDDIOM = cfg:enumMap("VDDIOM",
	{
		[0] = "VDDIOM_VOL_20V", 
		[1] = "VDDIOM_VOL_22V", 
		[2] = "VDDIOM_VOL_24V", 
		[3] = "VDDIOM_VOL_26V", 
		[4] = "VDDIOM_VOL_28V", 
		[5] = "VDDIOM_VOL_30V", 
		[6] = "VDDIOM_VOL_32V", 
		[7] = "VDDIOM_VOL_36V", 
	}
)

local VDDIOW = cfg:enumMap("VDDIOW", 
	{
		[0] = "VDDIOW_VOL_21V",
		[1] = "VDDIOW_VOL_24V",
		[2] = "VDDIOW_VOL_28V",
		[3] = "VDDIOW_VOL_32V",
	}
)


local BTOSC_DISABLE = cfg:enumMap("BTOSC_DISABLE",
	{
		[0] = "晶振不保持",
		[1] = "晶振保持",
	}
)

local LOWPOWER_MODE_SEL = cfg:enumMap("低功耗模式选择", 
	{
		[0] = "不使用低功耗模式", 
		[4] = "使用睡眠模式",
	}
)

---------------------------------[[ clock 输出列表]]---------------------------------
local LOWPOWER_POWER_SEL_OUTPUT_TABLE = {
    [0] = "PWR_NO_CHANGE",
    [1] = "PWR_LDO33",
    [2] = "PWR_LDO15",
	[3] = "PWR_DCDC15",
}

local VDDIOM_LEVEL_OUTPUT_TABLE = {
    [0] = "VDDIOM_VOL_20V",
    [1] = "VDDIOM_VOL_22V", 
    [2] = "VDDIOM_VOL_24V", 
    [3] = "VDDIOM_VOL_26V", 
    [4] = "VDDIOM_VOL_28V", 
    [5] = "VDDIOM_VOL_30V", 
    [6] = "VDDIOM_VOL_32V",
    [7] = "VDDIOM_VOL_36V",
}

local VDDIOW_LEVEL_OUTPUT_TABLE = {
    [0] = "VDDIOW_VOL_21V",
    [1] = "VDDIOW_VOL_24V",
    [2] = "VDDIOW_VOL_28V",
    [3] = "VDDIOW_VOL_32V",
}



local LOWPOWER_MODE_OUTPUT_TABLE = {
    [0] = "0    //不使用低功耗模式",
    [4] = "SLEEP_EN",
}




--[[
//==========================================================================================//
//                                  配置项: LOWPOWER_CFG                                    //
//==========================================================================================//
--]]

--[[=============================== 配置子项1-0: lowpower_en ================================--]]
local lowpower_en = cfg:enum("低功耗配置使能开关:", ENABLE_SWITCH, 0)
lowpower_en:setOSize(1)
lowpower_en:setTextOut(
	function (ty) 
		if (ty == 3) then
		--	return "#define TCFG_LOWPOWER_ENABLE  " .. lowpower_en.val .. "\n" 
		end
	end
)

-- 子项显示
local lowpower_en_view = cfg:hBox {
        cfg:stLabel(lowpower_en.name),
        cfg:enumView(lowpower_en),
        cfg:stSpacer(),
};

--[[=============================== 配置子项1-1: power_mode_sel ================================--]]
local power_sel = cfg:enum("电源模式选择:", POWER_MODE_SEL, 2)
power_sel:setOSize(1)
power_sel:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_LOWPOWER_POWER_SEL" .. TAB_TABLE[4] .. LOWPOWER_POWER_SEL_OUTPUT_TABLE[power_sel.val] .. NLINE_TABLE[1]
		end
	end
)
power_sel:addDeps{lowpower_en}
power_sel:setDepChangeHook(function ()
    if (lowpower_en.val == 0) then
        power_sel:setShow(false);
    else
        power_sel:setShow(true);
    end
end);

-- 子项显示
local power_mode_sel_view = cfg:hBox {
        cfg:stLabel(power_sel.name),
        cfg:enumView(power_sel),
        cfg:stSpacer(),
};

--[[
local vddio_mode_sel = cfg:enum("电源模式选择:", VDDIOM, 6)
vddio_mode_sel:setOSize(1)
vddio_mode_sel:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_LOWPOWER_VDDIOM_LEVEL" .. TAB_TABLE[3] .. LOWPOWER_VDDIOM_LEVEL_OUTPUT_TABLE[vddio_mode_sel.val] .. NLINE_TABLE[1]
		end
	end
)
vddio_mode_sel:addDeps{lowpower_en}
vddio_mode_sel:setDepChangeHook(function ()
    if (lowpower_en.val == 0) then
        vddio_mode_sel:setShow(false);
    else
        vddio_mode_sel:setShow(true);
    end
end);
--]]


--[[=============================== 配置子项1-2: btosc_disable ================================--]]
local btosc_disable = cfg:enum("低功耗模式晶振保持:", BTOSC_DISABLE, 0)
btosc_disable:setOSize(1)
btosc_disable:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_LOWPOWER_BTOSC_DISABLE" ..TAB_TABLE[3]  .. btosc_disable.val .. NLINE_TABLE[1]
		end
	end
)
btosc_disable:addDeps{lowpower_en}
btosc_disable:setDepChangeHook(function ()
    if (lowpower_en.val == 0) then
        btosc_disable:setShow(false);
    else
        btosc_disable:setShow(true);
    end
end);

-- 子项显示
local btosc_disable_view = cfg:hBox {
        cfg:stLabel(btosc_disable.name),
        cfg:enumView(btosc_disable),
        cfg:stSpacer(),
};



--[[=============================== 配置子项1-2: lowpower_sel ================================--]]
local lowpower_sel = cfg:enum("低功耗模式选择:", LOWPOWER_MODE_SEL, 4)
lowpower_sel:setOSize(1)
lowpower_sel:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_LOWPOWER_LOWPOWER_SEL" .. TAB_TABLE[3]  .. LOWPOWER_MODE_OUTPUT_TABLE[lowpower_sel.val] .. NLINE_TABLE[1]
		end
	end
)
lowpower_sel:addDeps{lowpower_en}
lowpower_sel:setDepChangeHook(function ()
    if (lowpower_en.val == 0) then
        lowpower_sel:setShow(false);
    else
        lowpower_sel:setShow(true);
    end
end);

-- 子项显示
local lowpower_sel_view = cfg:hBox {
        cfg:stLabel(lowpower_sel.name),
        cfg:enumView(lowpower_sel),
        cfg:stSpacer(),
};


--[[=============================== 配置子项1-3: lp_vddiom_level ================================--]]
local lp_vddiom_level = cfg:enum("VDDIOM 电源配置:", VDDIOM, 5)
lp_vddiom_level:setOSize(1)
lp_vddiom_level:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_LOWPOWER_VDDIOM_LEVEL" .. TAB_TABLE[3] .. VDDIOM_LEVEL_OUTPUT_TABLE[lp_vddiom_level.val] .. NLINE_TABLE[1]
		end
	end
)
lp_vddiom_level:addDeps{lowpower_en}
lp_vddiom_level:setDepChangeHook(function ()
    if (lowpower_en.val == 0) then
        lp_vddiom_level:setShow(false);
    else
        lp_vddiom_level:setShow(true);
    end
end);

-- 子项显示
local lp_vddiom_level_view = cfg:hBox {
        cfg:stLabel(lp_vddiom_level.name),
        cfg:enumView(lp_vddiom_level),
        cfg:stSpacer(),
};



--[[=============================== 配置子项1-4: lp_vddiow_level ================================--]]
local lp_vddiow_level = cfg:enum("VDDIOW 电源配置:", VDDIOW, 2)
lp_vddiow_level:setOSize(1)
lp_vddiow_level:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_LOWPOWER_VDDIOW_LEVEL" .. TAB_TABLE[3]  .. VDDIOW_LEVEL_OUTPUT_TABLE[lp_vddiow_level.val] .. NLINE_TABLE[2]
		end
	end
)
lp_vddiow_level:addDeps{lowpower_en}
lp_vddiow_level:setDepChangeHook(function ()
    if (lowpower_en.val == 0) then
        lp_vddiow_level:setShow(false);
    else
        lp_vddiow_level:setShow(true);
    end
end);

-- 子项显示
local lp_vddiow_level_view = cfg:hBox {
        cfg:stLabel(lp_vddiow_level.name),
        cfg:enumView(lp_vddiow_level),
        cfg:stSpacer(),
};

--[[===================================================================================
==================================== 模块返回汇总 =====================================
====================================================================================--]]
-- A. 输出htext
local lowpower_output_htext = {};
insert_item_to_list(lowpower_output_htext, lowpower_comment_begin_htext);
--insert_item_to_list(lowpower_output_htext, lowpower_en);
insert_item_to_list(lowpower_output_htext, power_sel);
insert_item_to_list(lowpower_output_htext, btosc_disable);
insert_item_to_list(lowpower_output_htext, lowpower_sel);
insert_item_to_list(lowpower_output_htext, lp_vddiom_level);
insert_item_to_list(lowpower_output_htext, lp_vddiow_level);
if enable_moudles["lowpower"] == false then
    return;
end
insert_list_to_list(board_output_text_tabs, lowpower_output_htext);

-- B. 输出ctext：无

-- C. 输出bin：无

-- E. 默认值
local lowpower_cfg_default_table = {};

if open_by_program == "create" then
	insert_item_to_list(lowpower_cfg_default_table, lowpower_en);
end
insert_item_to_list(lowpower_cfg_default_table, power_sel);
insert_item_to_list(lowpower_cfg_default_table, btosc_disable);
insert_item_to_list(lowpower_cfg_default_table, lowpower_sel);
insert_item_to_list(lowpower_cfg_default_table, lp_vddiom_level);
insert_item_to_list(lowpower_cfg_default_table, lp_vddiow_level);

local lowpower_default_button_view = cfg:stButton(" 低功耗配置恢复默认值 ", reset_to_default(lowpower_cfg_default_table));

-- D. 显示
local lowpower_group_view = cfg:stGroup("",
        cfg:vBox {
            lowpower_en_view,
            power_mode_sel_view,
            btosc_disable_view,
            lowpower_sel_view,
            lp_vddiom_level_view,
            lp_vddiow_level_view,
        }
);

local lowpower_view_list = cfg:vBox {
        lowpower_group_view,
        lowpower_default_button_view,
};

local lowpower_view = {"低功耗配置",
        lowpower_view_list,
};

if open_by_program == "create" then
    insert_item_to_list(board_view_tabs, lowpower_view);
end

-- F. bindGroup：无



