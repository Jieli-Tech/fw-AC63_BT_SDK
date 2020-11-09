
local MOUDLE_COMMENT_NAME = "//                                  内置触摸按键参数配置                                   //"

local comment_begin = cfg:i32("lp touch key注释开始", 0);
local lp_touch_comment_begin_htext = module_output_comment(comment_begin, MOUDLE_COMMENT_NAME);

local TOUCH_KEY_SENSITY_TABLE = cfg:enumMap("触摸按键电容检测灵敏度(0~9级):",
	{
		[0] = "电容检测灵敏度级数0",
		[1] = "电容检测灵敏度级数1", 
		[2] = "电容检测灵敏度级数2",
		[3] = "电容检测灵敏度级数3",
		[4] = "电容检测灵敏度级数4",
		[5] = "电容检测灵敏度级数5",
		[6] = "电容检测灵敏度级数6",
		[7] = "电容检测灵敏度级数7",
		[8] = "电容检测灵敏度级数8",
		[9] = "电容检测灵敏度级数9",
	}
)


--[[
//==========================================================================================//
//                                  配置项: TOUCH_KEY_CFG                                   //
//==========================================================================================//
--]]

--[[=============================== 配置子项1-0: lp_touch_cfg_en ================================--]]
local en = cfg:enum("触摸按键灵敏度配置使能开关", ENABLE_SWITCH, 1)
en:setOSize(1)
en:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_LP_TOUCH_CFG_ENABLE" .. TAB_TABLE[5] .. ENABLE_SWITCH_TABLE[en.val] .. NLINE_TABLE[1]
		end
	end
)

-- 子项显示
local lp_touch_key_en_view = cfg:hBox {
        cfg:stLabel(en.name .. "："),
        cfg:enumView(en),
	    cfg:stLabel("(ON：使用配置工具配置值, OFF：使用固件默认值)"),
        cfg:stSpacer(),
};


--[[=============================== 配置子项1-1: touch_key_sensity ================================--]]
local touch_key_sensity = cfg:enum("触摸按键灵敏度配置", TOUCH_KEY_SENSITY_TABLE, 5)
touch_key_sensity:setOSize(1)
touch_key_sensity:setTextOut(
	function (ty)
		if (ty == 3) then
			return "#define TCFG_TOUCH_KEY_CH0_SENSITY" .. TAB_TABLE[5] .. touch_key_sensity.val .. NLINE_TABLE[1]
		end
	end
)
touch_key_sensity:addDeps{en}
touch_key_sensity:setDepChangeHook(function ()
    if (en.val == 0) then
        touch_key_sensity:setShow(false);
    else
        touch_key_sensity:setShow(true);
    end
end);

-- 子项显示
local touch_key_sensity_hbox_view = cfg:hBox {
        cfg:stLabel(touch_key_sensity.name .. "："),
        cfg:enumView(touch_key_sensity),
	    cfg:stLabel("该参数配置与触摸时电容变化量有关, 触摸时电容变化量跟模具厚度, 触摸片材质, 面积等有关, \n触摸时电容变化量越小, 推荐选择灵敏度级数越大, \n触摸时电容变化量越大, 推荐选择灵敏度级数越小, \n用户可以从灵敏度级数为0开始调试, 级数逐渐增大, 直到选择一个合适的灵敏度配置值。"),
        cfg:stSpacer(),
};

--[[=============================== 配置子项1-1: earin_key_sensity ================================--]]
local earin_key_sensity = cfg:enum("入耳检测按键灵敏度配置", TOUCH_KEY_SENSITY_TABLE, 5)
earin_key_sensity:setOSize(1)
earin_key_sensity:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_EARIN_KEY_CH1_SENSITY" .. TAB_TABLE[5] .. earin_key_sensity.val .. NLINE_TABLE[1]
		end
	end
)
earin_key_sensity:addDeps{en}
earin_key_sensity:setDepChangeHook(function ()
    if (en.val == 0) then
        earin_key_sensity:setShow(false);
    else
        earin_key_sensity:setShow(true);
    end
end);

-- 子项显示
local earin_key_sensity_hbox_view = cfg:hBox {
        cfg:stLabel(earin_key_sensity.name .. "："),
        cfg:enumView(earin_key_sensity),
        cfg:stSpacer(),
};

--[[===================================================================================
==================================== 模块返回汇总 =====================================
====================================================================================--]]
-- A. 输出htext
local lp_touch_key_output_htext = {};
insert_item_to_list(lp_touch_key_output_htext, lp_touch_comment_begin_htext);
insert_item_to_list(lp_touch_key_output_htext, touch_key_sensity);

--insert_list_to_list(board_output_text_tabs, lp_touch_key_output_htext);

if enable_moudles["lp_touch_key"] == false then
    return;
end
-- B. 输出ctext：无
-- C. 输出bin：
local lp_touch_key_output_bin_list = {
        en,
        touch_key_sensity,
        earin_key_sensity,
};


local lp_touch_key_output_bin = cfg:group("LP_TOUCH_KEY_CFG",
    BIN_ONLY_CFG["HW_CFG"].lp_touch_key.id,
    1,
    lp_touch_key_output_bin_list
);

insert_item_to_list(board_output_bin_tabs, lp_touch_key_output_bin);

-- E. 默认值
local lp_touch_key_cfg_default_table = {};

insert_item_to_list(lp_touch_key_cfg_default_table, en);
insert_item_to_list(lp_touch_key_cfg_default_table, touch_key_sensity);
insert_item_to_list(lp_touch_key_cfg_default_table, earin_key_sensity);

-- D. 显示
local lp_touch_key_group_view_list = {}

insert_item_to_list(lp_touch_key_group_view_list, lp_touch_key_en_view);
insert_item_to_list(lp_touch_key_group_view_list, touch_key_sensity_hbox_view);
--insert_item_to_list(lp_touch_key_group_view_list, earin_key_sensity_hbox_view);


local lp_touch_key_group_view = cfg:stGroup("",
        cfg:vBox(lp_touch_key_group_view_list)
);


local lp_touch_key_default_button_view = cfg:stButton(" 触摸配置恢复默认值 ", reset_to_default(lp_touch_key_cfg_default_table));

local lp_touch_key_view_list = cfg:vBox {
    cfg:stHScroll(cfg:vBox {
        lp_touch_key_group_view,
    })
};

local lp_touch_key_view = {"内置触摸按键参数配置",
    cfg:vBox {
        lp_touch_key_view_list,
        lp_touch_key_default_button_view,
    }
};

insert_item_to_list(board_view_tabs, lp_touch_key_view);

-- F. bindGroup：无




