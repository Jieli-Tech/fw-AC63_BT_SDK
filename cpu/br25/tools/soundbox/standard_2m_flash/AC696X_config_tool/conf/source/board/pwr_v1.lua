
local MOUDLE_COMMENT_NAME = "//                                  唤醒-复位配置                                  //"

local comment_begin = cfg:i32("pwr 注释开始", 0);
local pwr_comment_begin_htext = module_output_comment(comment_begin, MOUDLE_COMMENT_NAME);

--print(MOUDLE_COMMENT_NAME);
--[[
//==========================================================================================//
//                                  配置项: PWR_CFG                                         //
//==========================================================================================//
--]]

local PWR_MODE_TIME = cfg:enumMap("长按复位时间",
    {
        [0] = " 长按 4 秒复位",
        [1] = " 长按 8 秒复位",
    }
);

local PWR_LEVEL = cfg:enumMap("复位电平",
    {
        [0] = " 低电平复位",
        [1] = " 高电平复位",
    }
);


--[[=============================== 配置子项 0: pwr_en ================================--]]
local pwr_en = cfg:enum("长按复位配置使能: ", ENABLE_SWITCH, 0);
pwr_en:setOSize(1);
pwr_en:setTextOut(
    function (ty)
        if (ty == 3) then
			return "#define TCFG_PWR_ENABLE" .. TAB_TABLE[6] .. ENABLE_SWITCH_TABLE[pwr_en.val] .. NLINE_TABLE[1]
        end
    end
);

-- 子项显示
local pwr_en_group_view = cfg:stGroup("",
        cfg:hBox {
            cfg:stLabel(pwr_en.name),
            cfg:enumView(pwr_en),
            cfg:stSpacer(),
        }
);

local function depend_pwr_en_show(dep_cfg, new_cfg)
    new_cfg:addDeps{dep_cfg};
    new_cfg:setDepChangeHook(function() new_cfg:setShow(dep_cfg.val ~= 0); end);
end;


--[[=============================== 配置子项1: pwr_mode_time_sel ================================--]]
local pwr_mode_time_sel = cfg:enum("长按复位时间选择: ", PWR_MODE_TIME, 0);
pwr_mode_time_sel:setOSize(1);
pwr_mode_time_sel:setTextOut(
	function (ty)
		if (ty == 3) then
			return "#define TCFG_PWR_MODE_TIME" .. TAB_TABLE[5] .. pwr_mode_time_sel.val .. NLINE_TABLE[1]
		end
	end
);
depend_pwr_en_show(pwr_en, pwr_mode_time_sel);


-- 子项显示
local pwr_mode_time_sel_hbox_view = cfg:hBox {
            cfg:stLabel(pwr_mode_time_sel.name),
            cfg:enumView(pwr_mode_time_sel),
            cfg:stSpacer(),
};

--[[=============================== 配置子项3: 复位电平选择 ================================--]]
local pwr_level_sel = cfg:enum("复位电平选择: " .. TAB_TABLE[1], PWR_LEVEL, 0);
pwr_level_sel:setOSize(1);
pwr_level_sel:setTextOut(
	function (ty)
		if (ty == 3) then
			return "#define TCFG_PWR_LEVEL" .. TAB_TABLE[6] .. pwr_level_sel.val .. NLINE_TABLE[1]
		end
	end
);
depend_pwr_en_show(pwr_en, pwr_level_sel);

local pwr_level_child_item_hbox_view = cfg:hBox {
    cfg:stLabel(pwr_level_sel.name);
    cfg:enumView(pwr_level_sel);
    cfg:stSpacer();
};

--[[=============================== 配置子项4: 复位引脚选择 ================================--]]
local pwr_port_sel = cfg:enum("复位引脚选择: " .. TAB_TABLE[1], PORTS, 17);
pwr_port_sel:setOSize(1);
pwr_port_sel:setTextOut(
	function (ty)
		if (ty == 3) then
			return "#define TCFG_PWR_PORT" .. TAB_TABLE[6] .. PORTS_TABLE[pwr_port_sel.val] .. NLINE_TABLE[2]
		end
	end
);
depend_pwr_en_show(pwr_en, pwr_port_sel);

local pwr_port_child_item_hbox_view = cfg:hBox {
    cfg:stLabel(pwr_port_sel.name);
    cfg:enumView(pwr_port_sel);
    cfg:stSpacer();
};

-- 子项显示
local pwr_group_view = cfg:stGroup("",
        cfg:vBox {
            pwr_mode_time_sel_hbox_view,
            pwr_level_child_item_hbox_view,
            pwr_port_child_item_hbox_view,
        }
);

--[[===================================================================================
==================================== 模块返回汇总 =====================================
====================================================================================--]]
-- A. 输出htext
local pwr_output_htext = {};
insert_item_to_list(pwr_output_htext, pwr_comment_begin_htext);
--insert_item_to_list(pwr_output_htext, pwr_en);
insert_item_to_list(pwr_output_htext, pwr_mode_time_sel);
insert_item_to_list(pwr_output_htext, pwr_level_sel);
insert_item_to_list(pwr_output_htext, pwr_port_sel);

insert_list_to_list(board_output_text_tabs, pwr_output_htext);

if enable_moudles["pwr"] == false then
    return;
end

-- B. 输出ctext：无

-- C. 输出bin：
local pwr_output_bin_list = {
    pwr_en,
    pwr_mode_time_sel,
    pwr_level_sel,
    pwr_port_sel,
};

local pwr_output_bin = cfg:group("PWR_CFG",
    BIN_ONLY_CFG["HW_CFG"].pwr.id,
    1,
    pwr_output_bin_list
);

insert_item_to_list(board_output_bin_tabs, pwr_output_bin);

-- E. 默认值
local pwr_cfg_default_table = {};

if open_by_program == "create" then
	insert_item_to_list(pwr_cfg_default_table, pwr_en);
end

insert_item_to_list(pwr_cfg_default_table, pwr_mode_time_sel);
insert_item_to_list(pwr_cfg_default_table, pwr_level_sel);
insert_item_to_list(pwr_cfg_default_table, pwr_port_sel);

local pwr_default_button_view = cfg:stButton(" 长按复位配置恢复默认值 ", reset_to_default(pwr_cfg_default_table));

-- D. 显示
local pwr_group_view_list = {}

if open_by_program == "create" then
	insert_item_to_list(pwr_group_view_list, pwr_en_group_view);
end
insert_item_to_list(pwr_group_view_list, pwr_group_view);

local pwr_view_list = cfg:vBox {
    cfg:stHScroll(cfg:vBox(pwr_group_view_list)),
	pwr_default_button_view,
};

local pwr_view = {"长按复位设置",
    pwr_view_list,
};

insert_item_to_list(board_view_tabs, pwr_view);

-- F. bindGroup：无


