
local MOUDLE_COMMENT_NAME = "//                                  充电参数配置                                   //"

local comment_begin = cfg:i32("charge注释开始", 0);
local charge_comment_begin_htext = module_output_comment(comment_begin, MOUDLE_COMMENT_NAME);

--print(MOUDLE_COMMENT_NAME);
local CHARGE_FULL_VOL = cfg:enumMap("充电满电压选择(3.869V ~ 4.567V):",
	{
		[0] = "3.869V", 
		[1] = "3.907V", 
		[2] = "3.946V",
		[3] = "3.985V",
		[4] = "4.026V",
		[5] = "4.068V",
		[6] = "4.122V",
		[7] = "4.157V",
		[8] = "4.202V",
		[9] = "4.249V",
		[10] = "4.295V",
		[11] = "4.350V",
		[12] = "4.398V",
		[13] = "4.452V",
		[14] = "4.509V",
		[15] = "4.567V"
	}
)

local CHARGE_FULL_VOL_OUPUT_TABLE = {
    [0] = "CHARGE_FULL_V_3869",
    [1] = "CHARGE_FULL_V_3907",
    [2] = "CHARGE_FULL_V_3946",
    [3] = "CHARGE_FULL_V_3985",
    [4] = "CHARGE_FULL_V_4026",
    [5] = "CHARGE_FULL_V_4068",
    [6] = "CHARGE_FULL_V_4122",
    [7] = "CHARGE_FULL_V_4157",
    [8] = "CHARGE_FULL_V_4202",
    [9] = "CHARGE_FULL_V_4249",
    [10] = "CHARGE_FULL_V_4295",
    [11] = "CHARGE_FULL_V_4350",
    [12] = "CHARGE_FULL_V_4398",
    [13] = "CHARGE_FULL_V_4452",
    [14] = "CHARGE_FULL_V_4509",
    [15] = "CHARGE_FULL_V_4567",
}


local CHARGE_FULL_MA = cfg:enumMap("充电满电流选择(2mA ~ 30mA):",
	{
		[0] = "2mA", 
		[1] = "5mA", 
		[2] = "7mA", 
		[3] = "10mA", 
		[4] = "15mA", 
		[5] = "20mA", 
		[6] = "25mA", 
		[7] = "30mA" 
	}
)

local CHARGE_FULL_MA_OUPUT_TABLE = {
    [0] = "CHARGE_FULL_mA_2",
    [1] = "CHARGE_FULL_mA_5",
    [2] = "CHARGE_FULL_mA_7",
    [3] = "CHARGE_FULL_mA_10",
    [4] = "CHARGE_FULL_mA_15",
    [5] = "CHARGE_FULL_mA_20",
    [6] = "CHARGE_FULL_mA_25",
    [7] = "CHARGE_FULL_mA_30",
}

local CHARGE_MA = cfg:enumMap("充电电流", 
	{
		[0] = "20mA", 
		[1] = "30mA", 
		[2] = "40mA", 
		[3] = "50mA", 
		[4] = "60mA", 
		[5] = "70mA", 
		[6] = "80mA", 
		[7] = "90mA", 
		[8] = "100mA", 
		[9] = "110mA", 
		[10] = "120mA", 
		[11] = "140mA", 
		[12] = "160mA", 
		[13] = "180mA", 
		[14] = "200mA", 
		[15] = "220mA" 
	}
)

--[[
//==========================================================================================//
//                                  配置项: CHARGE_CFG                                      //
//==========================================================================================//
--]]

--[[=============================== 配置子项1-0: charge_en ================================--]]
local en = cfg:enum("充电配置使能开关", ENABLE_SWITCH, 1)
en:setOSize(1)
en:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_CHARGE_ENABLE" .. TAB_TABLE[5] .. ENABLE_SWITCH_TABLE[en.val] .. NLINE_TABLE[1]
		end
	end
)

-- 子项显示
local charge_en_view = cfg:hBox {
        cfg:stLabel(en.name .. "："),
        cfg:enumView(en),
        cfg:stSpacer(),
};


--[[=============================== 配置子项1-1: poweron_en ================================--]]
local poweron_en = cfg:enum("开机充电使能开关", ENABLE_SWITCH, 0)
poweron_en:setOSize(1)
poweron_en:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_CHARGE_POWERON_ENABLE" .. TAB_TABLE[3] .. FUNCTION_SWITCH_TABLE[poweron_en.val] .. NLINE_TABLE[1]
		end
	end
)
poweron_en:addDeps{en}
poweron_en:setDepChangeHook(function ()
    if (en.val == 0) then
        poweron_en:setShow(false);
    else
        poweron_en:setShow(true);
    end
end);

-- 子项显示
local poweron_en_view = cfg:hBox {
        cfg:stLabel(poweron_en.name .. "："),
        cfg:enumView(poweron_en),
        cfg:stSpacer(),
};


--[[=============================== 配置子项1-2: off_poweron_en ================================--]]
local off_poweron_en = cfg:enum("充电拔出自动开机使能", ENABLE_SWITCH, 0)
off_poweron_en:setOSize(1)
off_poweron_en:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_CHARGE_OFF_POWERON_NE" .. TAB_TABLE[3] .. FUNCTION_SWITCH_TABLE[off_poweron_en.val] .. NLINE_TABLE[1]
		end
	end
)
off_poweron_en:addDeps{en}
off_poweron_en:setDepChangeHook(function ()
    if (en.val == 0) then
        off_poweron_en:setShow(false);
    else
        off_poweron_en:setShow(true);
    end
end);

-- 子项显示
local off_poweron_en_view = cfg:hBox {
        cfg:stLabel(off_poweron_en.name .. "："),
        cfg:enumView(off_poweron_en),
        cfg:stSpacer(),
};


--[[=============================== 配置子项1-3: full_v ================================--]]
local full_v = cfg:enum("充电满电压", CHARGE_FULL_VOL, 8)
full_v:setOSize(1)
full_v:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_CHARGE_FULL_V" .. TAB_TABLE[5] .. CHARGE_FULL_VOL_OUPUT_TABLE[full_v.val] .. NLINE_TABLE[1]
		end
	end
)
full_v:addDeps{en}
full_v:setDepChangeHook(function ()
    if (en.val == 0) then
        full_v:setShow(false);
    else
        full_v:setShow(true);
    end
end);

-- 子项显示
local full_v_view = cfg:hBox {
        cfg:stLabel(full_v.name .. "："),
        cfg:enumView(full_v),
        cfg:stSpacer(),
};



--[[=============================== 配置子项1-4: full_mA ================================--]]
local full_mA = cfg:enum("充电满电流", CHARGE_FULL_MA, 3)
full_mA:setOSize(1)
full_mA:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_CHARGE_FULL_MA" .. TAB_TABLE[5] .. CHARGE_FULL_MA_OUPUT_TABLE[full_mA.val] .. NLINE_TABLE[1]
		end
	end
)
full_mA:addDeps{en}
full_mA:setDepChangeHook(function ()
    if (en.val == 0) then
        full_mA:setShow(false);
    else
        full_mA:setShow(true);
    end
end);

-- 子项显示
local full_mA_view = cfg:hBox {
        cfg:stLabel(full_mA.name .. "："),
        cfg:enumView(full_mA),
        cfg:stSpacer(),
};


--[[=============================== 配置子项1-5: mA ================================--]]
local mA = cfg:enum("充电电流", CHARGE_MA, 2)
mA:setOSize(1)
mA:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_CHARGE_MA" .. TAB_TABLE[6] .. mA.val .. NLINE_TABLE[2]
		end
	end
)
mA:addDeps{en}
mA:setDepChangeHook(function ()
    if (en.val == 0) then
        mA:setShow(false);
    else
        mA:setShow(true);
    end
end);

-- 子项显示
local mA_view = cfg:hBox {
        cfg:stLabel(mA.name .. "：  "),
        cfg:enumView(mA),
        cfg:stSpacer(),
};

--[[=============================== 配置子项1-7: 低电提示音电压设置 ================================--]]
local lowpoer_table = {
    [310] = " 3.1",
    [320] = " 3.2",
    [330] = " 3.3",
    [340] = " 3.4",
    [350] = " 3.5",
};

local LOWER_POWER_TABLE = cfg:enumMap("低电压列表", lowpoer_table);

local lowpower_tone_voltage = cfg:enum("低电提醒电压设置", LOWER_POWER_TABLE, 340);
lowpower_tone_voltage:setOSize(2);
depend_item_en_show(en, lowpower_tone_voltage);

-- 子项显示
local lowpower_tone_voltage_hbox_view = cfg:hBox {
        cfg:stLabel(lowpower_tone_voltage.name .. "："),
        cfg:enumView(lowpower_tone_voltage),
        cfg:stLabel("V"),
        cfg:stSpacer(),
};

local lowpower_power_off_voltage = cfg:enum("低电关机电压设置", LOWER_POWER_TABLE, 330);
lowpower_power_off_voltage:setOSize(2);
depend_item_en_show(en, lowpower_power_off_voltage);

-- 子项显示
local lowpower_power_off_voltage_hbox_view = cfg:hBox {
        cfg:stLabel(lowpower_power_off_voltage .name .. "："),
        cfg:enumView(lowpower_power_off_voltage),
        cfg:stLabel("V"),
        cfg:stSpacer(),
};

--[[===================================================================================
==================================== 模块返回汇总 =====================================
====================================================================================--]]
-- A. 输出htext
local charge_output_htext = {};
insert_item_to_list(charge_output_htext, charge_comment_begin_htext);
--insert_item_to_list(charge_output_htext, en);
insert_item_to_list(charge_output_htext, poweron_en);
--insert_item_to_list(charge_output_htext, off_poweron_en);
insert_item_to_list(charge_output_htext, full_v);
insert_item_to_list(charge_output_htext, full_mA);
insert_item_to_list(charge_output_htext, mA);

--insert_list_to_list(board_output_text_tabs, charge_output_htext);

if enable_moudles["charge"] == false then
    return;
end
-- B. 输出ctext：无
-- C. 输出bin：
local charge_output_bin_list = {
        en,
        poweron_en,
        full_v,
        full_mA,
        mA, 
};

local lowpower_voltage_output_bin_list = {
    lowpower_tone_voltage,
    lowpower_power_off_voltage,
};

local charge_output_bin = cfg:group("CHARGE_CFG",
    BIN_ONLY_CFG["HW_CFG"].charge.id,
    1,
    charge_output_bin_list
);

local lowpower_voltage_output_bin = cfg:group("LOWERPOWER_VOLTAGE",
    BIN_ONLY_CFG["HW_CFG"].lowpower.id,
    1,
    lowpower_voltage_output_bin_list
);
insert_item_to_list(board_output_bin_tabs, charge_output_bin);
insert_item_to_list(board_output_bin_tabs, lowpower_voltage_output_bin);

-- E. 默认值
local charge_cfg_default_table = {};

if open_by_program == "create" then
	insert_item_to_list(charge_cfg_default_table, en);
end

insert_item_to_list(charge_cfg_default_table, poweron_en);
insert_item_to_list(charge_cfg_default_table, full_v);
insert_item_to_list(charge_cfg_default_table, full_mA);
insert_item_to_list(charge_cfg_default_table, mA);
insert_item_to_list(charge_cfg_default_table, lowpower_tone_voltage);
insert_item_to_list(charge_cfg_default_table, lowpower_power_off_voltage);

-- D. 显示
local charge_group_view_list = {}
local lowpower_voltage_group_view_list = {}

if open_by_program == "create" then
	insert_item_to_list(charge_group_view_list, charge_en_view);
end
insert_item_to_list(charge_group_view_list, poweron_en_view);
--insert_item_to_list(charge_group_view_list, off_poweron_en_view);
insert_item_to_list(charge_group_view_list, full_v_view);
insert_item_to_list(charge_group_view_list, full_mA_view);
insert_item_to_list(charge_group_view_list, mA_view);

insert_item_to_list(lowpower_voltage_group_view_list, lowpower_tone_voltage_hbox_view);
insert_item_to_list(lowpower_voltage_group_view_list, lowpower_power_off_voltage_hbox_view);
local charge_group_view = cfg:stGroup("",
        cfg:vBox(charge_group_view_list)
);

local lowpower_voltage_group_view = cfg:stGroup("",
        cfg:vBox(lowpower_voltage_group_view_list)
);


local charge_default_button_view = cfg:stButton(" 配置恢复默认值 ", reset_to_default(charge_cfg_default_table));

local charge_view_list = cfg:vBox {
    cfg:stHScroll(cfg:vBox {
        charge_group_view,
        lowpower_voltage_group_view,
    })
};

local charge_view = {"充电配置",
    cfg:vBox {
        charge_view_list,
        charge_default_button_view,
    }
};

insert_item_to_list(board_view_tabs, charge_view);

-- F. bindGroup：无




