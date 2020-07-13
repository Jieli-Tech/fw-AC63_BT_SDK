
local MOUDLE_COMMENT_NAME = "//                                  SD 配置                                        //"

local comment_begin = cfg:i32("sd注释开始", 0);
local sd_comment_begin_htext = module_output_comment(comment_begin, MOUDLE_COMMENT_NAME);

--print(MOUDLE_COMMENT_NAME);
local DAT_MODE = cfg:enumMap("SD_DAT_MODE", {[1] = "1线", [4] = "4线"})
local DET_MODE = cfg:enumStr("SD_DET_MODE", {"CMD", "CLK", "IO"})


---------------------------------[[ sd0 ]]---------------------------------
local SD0_PORTS = cfg:enumStr("SD0_PORTS", {"A组", "B组"})
---------------------------------[[ sd1 ]]---------------------------------
local SD1_PORTS = cfg:enumStr("SD1_PORTS", {"A组"})

--[[
//==========================================================================================//
//                                  配置项: SD_CFG                                          //
//==========================================================================================//
--]]

--[[=============================== 配置子项1-0: sd0_en ================================--]]
local sd0_en = cfg:enum("SD_0 使能开关:" .. TAB_TABLE[2], ENABLE_SWITCH, 0)
sd0_en:setOSize(1)
sd0_en:setTextOut(
	function (ty) 
		if (ty == 3) then
            if enable_moudles["sd"] == false then
			    return "#define TCFG_SD_ENABLE" .. TAB_TABLE[6] .. ENABLE_SWITCH_TABLE[0] .. NLINE_TABLE[2]
            end
			return "#define TCFG_SD0_ENABLE" .. TAB_TABLE[6] .. ENABLE_SWITCH_TABLE[sd0_en.val] .. NLINE_TABLE[2]
		end
	end
)

-- 子项显示
local sd0_en_view = cfg:hBox {
            cfg:stLabel(sd0_en.name),
            cfg:enumView(sd0_en),
            cfg:stSpacer(),
};


--[[=============================== 配置子项1-1: sd0_ports_sel ================================--]]
local sd0_ports = cfg:enum("SD_0 引脚选择:" .. TAB_TABLE[2], SD0_PORTS, 1)
sd0_ports:setOSize(1)
sd0_ports:setTextOut(
	function (ty) 
		local i = { [0] = "A"; [1] = "B"}
		if (ty == 3) then
			return "#define TCFG_SD0_PORTS" .. TAB_TABLE[6] .. "\'" .. i[sd0_ports.val] .. "\'" .. NLINE_TABLE[1]
		end
	end
)
sd0_ports:addDeps{sd0_en}
sd0_ports:setDepChangeHook(function ()
    if (sd0_en.val == 0) then
        sd0_ports:setShow(false);
    else
        sd0_ports:setShow(true);
    end
end);

-- 子项显示
local sd0_ports_view = cfg:hBox {
            cfg:stLabel(sd0_ports.name),
            cfg:enumView(sd0_ports),
            cfg:stSpacer(),
};


--[[=============================== 配置子项1-2: sd0_dat_mode_sel ================================--]]
local sd0_dat_mode = cfg:enum("SD_0 数据传输模式选择:".. TAB_TABLE[1], DAT_MODE, 1)
sd0_dat_mode:setOSize(1)
sd0_dat_mode:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_SD0_DAT_MODE" .. TAB_TABLE[5] .. sd0_dat_mode.val .. NLINE_TABLE[1]
		end
	end
)
sd0_dat_mode:addDeps{sd0_en}
sd0_dat_mode:setDepChangeHook(function ()
    if (sd0_en.val == 0) then
        sd0_dat_mode:setShow(false);
    else
        sd0_dat_mode:setShow(true);
    end
end);

-- 子项显示
local sd0_dat_mode_view = cfg:hBox {
            cfg:stLabel(sd0_dat_mode.name),
            cfg:enumView(sd0_dat_mode),
            cfg:stSpacer(),
};


--[[=============================== 配置子项1-3: sd0_det_mode_sel ================================--]]
local sd0_det_mode = cfg:enum("SD_0 在线检测模式选择:".. TAB_TABLE[1], DET_MODE, 0)
sd0_det_mode:setOSize(1)
sd0_det_mode:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_SD0_DET_MODE" .. TAB_TABLE[5] .. sd0_det_mode.val .. NLINE_TABLE[1] 
		end
	end
)
sd0_det_mode:addDeps{sd0_en}
sd0_det_mode:setDepChangeHook(function ()
    if (sd0_en.val == 0) then
        sd0_det_mode:setShow(false);
    else
        sd0_det_mode:setShow(true);
    end
end);

-- 子项显示
local sd0_det_mode_view = cfg:hBox {
            cfg:stLabel(sd0_det_mode.name),
            cfg:enumView(sd0_det_mode),
            cfg:stSpacer(),
};



--[[=============================== 配置子项1-4: sd0_clk_sel ================================--]]
local sd0_clk = cfg:i32("SD_0 频率配置:".. TAB_TABLE[2], 3000000)
sd0_clk:setOSize(4)
sd0_clk:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_SD0_CLK" .. TAB_TABLE[6] .. sd0_clk.val .. NLINE_TABLE[2]
		end
	end
)
sd0_clk:addConstraint(
	function () 
		return sd0_clk.val < 50000000 or "sd卡控制器频率不能大于50M";
	end
)
sd0_clk:addDeps{sd0_en}
sd0_clk:setDepChangeHook(function ()
    if (sd0_en.val == 0) then
        sd0_clk:setShow(false);
    else
        sd0_clk:setShow(true);
    end
end);

-- 子项显示
local sd0_clk_view = cfg:hBox {
            cfg:stLabel(sd0_clk.name),
            cfg:inputView(sd0_clk),
            cfg:stSpacer(),
};



--[[=============================== 配置子项2-0: sd1_en ================================--]]
local sd1_en = cfg:enum("SD_1 使能开关:".. TAB_TABLE[2], ENABLE_SWITCH, 0)
sd1_en:setOSize(1)
sd1_en:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_SD1_ENABLE" .. TAB_TABLE[6] .. ENABLE_SWITCH_TABLE[sd1_en.val] .. NLINE_TABLE[1]
		end
	end
)

-- 子项显示
local sd1_en_view = cfg:hBox {
            cfg:stLabel(sd1_en.name),
            cfg:enumView(sd1_en),
            cfg:stSpacer(),
};


--[[=============================== 配置子项2-1: sd1_ports ================================--]]
local sd1_ports = cfg:enum("SD_1 引脚选择:" .. TAB_TABLE[2], SD1_PORTS, 0)
sd1_ports:setOSize(1)
sd1_ports:setTextOut(
	function (ty) 
		local i = { [0] = "A"}
		if (ty == 3) then
			return "#define TCFG_SD1_PORTS" .. TAB_TABLE[6] .. "\'".. i[sd1_ports.val] .. "\'" .. NLINE_TABLE[1]
		end
	end
)
sd1_ports:addDeps{sd1_en}
sd1_ports:setDepChangeHook(function ()
    if (sd1_en.val == 0) then
        sd1_ports:setShow(false);
    else
        sd1_ports:setShow(true);
    end
end);

-- 子项显示
local sd1_ports_view = cfg:hBox {
            cfg:stLabel(sd1_ports.name),
            cfg:enumView(sd1_ports),
            cfg:stSpacer(),
};

--[[=============================== 配置子项2-2: sd1_dat_mode ================================--]]
local sd1_dat_mode = cfg:enum("SD_1 数据传输模式:" .. TAB_TABLE[1], DAT_MODE, 1)
sd1_dat_mode:setOSize(1)
sd1_dat_mode:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_SD1_DAT_MODE" .. TAB_TABLE[5] .. sd1_dat_mode.val .. NLINE_TABLE[1] 
		end
	end
)
sd1_dat_mode:addDeps{sd1_en}
sd1_dat_mode:setDepChangeHook(function ()
    if (sd1_en.val == 0) then
        sd1_dat_mode:setShow(false);
    else
        sd1_dat_mode:setShow(true);
    end
end);

-- 子项显示
local sd1_dat_mode_view = cfg:hBox {
            cfg:stLabel(sd1_dat_mode.name),
            cfg:enumView(sd1_dat_mode),
            cfg:stSpacer(),
};

--[[=============================== 配置子项2-3: sd1_dat_mode ================================--]]
local sd1_det_mode = cfg:enum("SD_1 在线检测模式选择:" .. TAB_TABLE[1], DET_MODE, 0)
sd1_det_mode:setOSize(1)
sd1_det_mode:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_SD1_DET_MODE" .. TAB_TABLE[5] .. sd1_det_mode.val .. NLINE_TABLE[1]
		end
	end
)
sd1_det_mode:addDeps{sd1_en}
sd1_det_mode:setDepChangeHook(function ()
    if (sd1_en.val == 0) then
        sd1_det_mode:setShow(false);
    else
        sd1_det_mode:setShow(true);
    end
end);

-- 子项显示
local sd1_det_mode_view = cfg:hBox {
            cfg:stLabel(sd1_det_mode.name),
            cfg:enumView(sd1_det_mode),
            cfg:stSpacer(),
};


--[[=============================== 配置子项2-4: sd1_clk_sel ================================--]]
local sd1_clk = cfg:i32("SD_1 频率配置:" .. TAB_TABLE[2], 3000000)
sd1_clk:setOSize(4)
sd1_clk:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_SD1_CLK" .. TAB_TABLE[6] .. sd1_clk.val .. NLINE_TABLE[2]
		end
	end
)
sd1_clk:addConstraint(
	function () 
		return sd1_clk.val < 50000000 or "sd卡控制器频率不能大于50M";
	end
)
sd1_clk:addDeps{sd1_en}
sd1_clk:setDepChangeHook(function ()
    if (sd1_en.val == 0) then
        sd1_clk:setShow(false);
    else
        sd1_clk:setShow(true);
    end
end);

-- 子项显示
local sd1_clk_view = cfg:hBox {
            cfg:stLabel(sd1_clk.name),
            cfg:inputView(sd1_clk),
            cfg:stSpacer(),
};


--[[===================================================================================
==================================== 模块返回汇总 =====================================
====================================================================================--]]
-- A. 输出htext
local sd_output_htext = {};
insert_item_to_list(sd_output_htext, sd_comment_begin_htext);
insert_item_to_list(sd_output_htext, sd0_en);

if enable_moudles["sd"] == true then
    insert_item_to_list(sd_output_htext, sd0_ports);
    insert_item_to_list(sd_output_htext, sd0_dat_mode);
    insert_item_to_list(sd_output_htext, sd0_det_mode);
    insert_item_to_list(sd_output_htext, sd0_clk);

    insert_item_to_list(sd_output_htext, sd1_en);
    insert_item_to_list(sd_output_htext, sd1_ports);
    insert_item_to_list(sd_output_htext, sd1_dat_mode);
    insert_item_to_list(sd_output_htext, sd1_det_mode);
    insert_item_to_list(sd_output_htext, sd1_clk);

end

insert_list_to_list(board_output_text_tabs, sd_output_htext);

if enable_moudles["sd"] == false then
    return;
end

-- B. 输出ctext：无
-- C. 输出bin：无

-- E. 默认值
local sd_cfg_default_table = {};

if open_by_program == "create" then
	insert_item_to_list(sd_cfg_default_table, sd0_en);
end

insert_item_to_list(sd_cfg_default_table, sd0_en);
insert_item_to_list(sd_cfg_default_table, sd0_ports);
insert_item_to_list(sd_cfg_default_table, sd0_dat_mode);
insert_item_to_list(sd_cfg_default_table, sd0_det_mode);
insert_item_to_list(sd_cfg_default_table, sd0_clk);

insert_item_to_list(sd_cfg_default_table, sd1_en);
insert_item_to_list(sd_cfg_default_table, sd1_ports);
insert_item_to_list(sd_cfg_default_table, sd1_dat_mode);
insert_item_to_list(sd_cfg_default_table, sd1_det_mode);
insert_item_to_list(sd_cfg_default_table, sd1_clk);

local sd_default_button_view = cfg:stButton(" SD卡配置恢复默认值 ", reset_to_default(sd_cfg_default_table));

-- D. 显示
local sd0_group_view_content_list = {}

if open_by_program == "create" then
	insert_item_to_list(sd0_group_view_content_list, sd0_en_view);
end

insert_item_to_list(sd0_group_view_content_list, sd0_ports_view);
insert_item_to_list(sd0_group_view_content_list, sd0_dat_mode_view);
insert_item_to_list(sd0_group_view_content_list, sd0_det_mode_view);
insert_item_to_list(sd0_group_view_content_list, sd0_clk_view);

local sd0_group_view = cfg:stGroup("SD_0 配置",
        cfg:vBox (sd0_group_view_content_list)
);

local sd1_group_view_content_list = {}

if open_by_program == "create" then
	insert_item_to_list(sd1_group_view_content_list, sd1_en_view);
end

insert_item_to_list(sd1_group_view_content_list, sd1_ports_view);
insert_item_to_list(sd1_group_view_content_list, sd1_dat_mode_view);
insert_item_to_list(sd1_group_view_content_list, sd1_det_mode_view);
insert_item_to_list(sd1_group_view_content_list, sd1_clk_view);

local sd1_group_view = cfg:stGroup("SD_1 配置",
        cfg:vBox (sd1_group_view_content_list)
);

local sd_view_list = cfg:stGroup("",
    cfg:vBox {
        cfg:hBox {
            sd0_group_view,
            sd1_group_view,
        },
        sd_default_button_view,
    }
);

local sd_view = {"SD 配置",
        sd_view_list,
};

if open_by_program == "create" then
    insert_item_to_list(board_view_tabs, sd_view);
end

-- F. bindGroup：无

