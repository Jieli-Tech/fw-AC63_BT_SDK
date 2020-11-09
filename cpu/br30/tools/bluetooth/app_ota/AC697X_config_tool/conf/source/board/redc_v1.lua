

local MOUDLE_COMMENT_NAME = "//                                  RDEC配置                                       //"

local comment_begin = cfg:i32("rdec注释开始", 0)
local rdec_comment_begin_htext = module_output_comment(comment_begin, MOUDLE_COMMENT_NAME);

--[[
//==========================================================================================//
//                                  配置项: RDEC_CFG                                       //
//==========================================================================================//
--]]
--print(MOUDLE_COMMENT_NAME);


--[[=============================== 配置子项1-0: rdec_en ================================--]]
local redc_en = cfg:enum("旋转编码器使能开关:", ENABLE_SWITCH, 0)
redc_en:setOSize(1)
redc_en:setTextOut(
	function (ty) 
		if (ty == 3) then
            if enable_moudles["rdec"] == false then
			    return "#define TCFG_ROTATE_DEC_ENABLE" .. TAB_TABLE[4]  .. ENABLE_SWITCH_TABLE[0] .. NLINE_TABLE[2]
            end
			return "#define TCFG_ROTATE_DEC_ENABLE" .. TAB_TABLE[4]  .. ENABLE_SWITCH_TABLE[redc_en.val] .. NLINE_TABLE[1]
		end
	end
)

-- 子项显示
local rdec_en_view = cfg:hBox {
        cfg:stLabel(redc_en.name),
        cfg:enumView(redc_en),
        cfg:stSpacer(),
};



--[[=============================== 配置子项1-1: rdec_in_port ================================--]]
local redc_in_port = cfg:enum("旋转编码器端口 1 引脚选择:", PORTS, 0xFF)
redc_in_port:setOSize(1)
redc_in_port:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_ROTATE_DEC_IN_PORT" .. TAB_TABLE[4] .. PORTS_TABLE[redc_in_port.val] .. NLINE_TABLE[1]
		end
	end
)
redc_in_port:addDeps{redc_en}
redc_in_port:setDepChangeHook(function ()
    if (redc_en.val == 0) then
        redc_in_port:setShow(false);
    else
        redc_in_port:setShow(true);
    end
end);


-- 子项显示
local rdec_in_port_view = cfg:hBox {
        cfg:stLabel(redc_in_port.name),
        cfg:enumView(redc_in_port),
        cfg:stSpacer(),
};



--[[=============================== 配置子项1-2: rdec_out_port ================================--]]
local redc_out_port = cfg:enum("旋转编码器端口 2 引脚选择:", PORTS, 0xFF)
redc_out_port:setOSize(1)
redc_out_port:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_ROTATE_DEC_OUT_PORT" .. TAB_TABLE[3]  .. PORTS_TABLE[redc_out_port.val] .. NLINE_TABLE[2]
		end
	end
)
redc_out_port:addDeps{redc_en}
redc_out_port:setDepChangeHook(function ()
    if (redc_en.val == 0) then
        redc_out_port:setShow(false);
    else
        redc_out_port:setShow(true);
    end
end);

-- 子项显示
local rdec_out_port_view = cfg:hBox {
        cfg:stLabel(redc_out_port.name),
        cfg:enumView(redc_out_port),
        cfg:stSpacer(),
};

--[[===================================================================================
==================================== 模块返回汇总 =====================================
====================================================================================--]]
-- A. 输出htext
local rdec_output_htext = {};
insert_item_to_list(rdec_output_htext, rdec_comment_begin_htext);
insert_item_to_list(rdec_output_htext, redc_en);

if enable_moudles["rdec"] == true then
    insert_item_to_list(rdec_output_htext, redc_in_port);
    insert_item_to_list(rdec_output_htext, redc_out_port);
end

insert_list_to_list(board_output_text_tabs, rdec_output_htext);

if enable_moudles["rdec"] == false then
    return;
end

-- B. 输出ctext：无
-- C. 输出bin：无

-- E. 默认值
local rdec_cfg_default_table = {};

if open_by_program == "create" then
	insert_item_to_list(rdec_cfg_default_table, rdec_en);
end

insert_item_to_list(rdec_cfg_default_table, rdec_in_port);
insert_item_to_list(rdec_cfg_default_table, rdec_out_port);

local rdec_default_button_view = cfg:stButton(" 旋转编码器恢复默认值 ", reset_to_default(rdec_cfg_default_table));

-- D. 显示
local rdec_group_view_list = {}

if open_by_program == "create" then
	insert_item_to_list(rdec_group_view_list, rdec_en_view);
end

insert_item_to_list(rdec_group_view_list, rdec_in_port_view);
insert_item_to_list(rdec_group_view_list, rdec_out_port_view);

local rdec_group_view = cfg:stGroup("",
        cfg:vBox(rdec_group_view_list)
);

local rdec_view_list = cfg:vBox {
        rdec_group_view,
        rdec_default_button_view,
};

local rdec_view = {"旋转编码器配置",
        rdec_view_list,
};

if open_by_program == "create" then
    insert_item_to_list(board_view_tabs, rdec_view);
end
-- F. bindGroup：无


