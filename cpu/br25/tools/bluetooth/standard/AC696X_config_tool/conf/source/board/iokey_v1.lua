
local MOUDLE_COMMENT_NAME = "//                                 IOKEY 配置                                      //"

local comment_begin = cfg:i32("iokey注释开始", 0)
local iokey_comment_begin_htext = module_output_comment(comment_begin, MOUDLE_COMMENT_NAME);

--print(MOUDLE_COMMENT_NAME);
local iokey_mcro_begin = cfg:i32("iokey宏开始", 0)
iokey_mcro_begin:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#if (TCFG_IOKEY_ENABLE == ENABLE_THIS_MODULE)" .. NLINE_TABLE[2];
		end
	end
)

local iokey_mcro_end = cfg:i32("iokey宏结束", 0)
iokey_mcro_end:setTextOut(
	function (ty) 
		if (ty == 3) then
			return NLINE_TABLE[1] .. "#endif" .. NLINE_TABLE[2];
		end
	end
)

local connect_way_table = {
    [0] = "按键一端接低电平一端接IO",
	[1] = "按键一端接高电平一端接IO",
	[2] = "按键两端接IO",
};

local CONNECT_WAY = cfg:enumMap("IO 链接方式", connect_way_table);


---------------------------------[[ io_key输出列表 ]]---------------------------------
local CONNECT_WAY_OUTPUT_TABLE = {
    [0] = "ONE_PORT_TO_LOW  //按键一端接低电平一端接IO",
    [1] = "ONE_PORT_TO_HIGH //按键一端接高电平一端接IO",
    [2] = "DOUBLE_PORT_TO_IO //按键两端接IO",
}


--[[=============================== 配置项列表 ================================--]]
local iokey_item_table = {};
local iokey_output_msg_bin_list = {};

--[[===========================================================================--]]

--[[
//==========================================================================================//
//                                  配置项: IOKEY_CFG                                       //
//==========================================================================================//
--]]

--[[=============================== 配置子项0: iokey_en ================================--]]
iokey_en = cfg:enum("IOKEY 使能总开关:", ENABLE_SWITCH, 0)
iokey_en:setOSize(1);
iokey_en:setTextOut(
	function (ty) 
		if (ty == 3) then
            if enable_moudles["iokey"].enable == false then
			    return "#define TCFG_IOKEY_ENABLE" .. TAB_TABLE[5] .. ENABLE_SWITCH_TABLE[0] .. NLINE_TABLE[2]
            end
			return "#define TCFG_IOKEY_ENABLE" .. TAB_TABLE[5] .. ENABLE_SWITCH_TABLE[iokey_en.val] .. NLINE_TABLE[2]
		end
	end
);
local iokey_en_group_view = enum_item_hbox_view(iokey_en);  -- 子项显示


--[[=============================== 配置子项1: iokey_num ================================--]]
local iokey_num_max = cfg:i32("IOKEY按键预留最大数量", enable_moudles["iokey"].num);
iokey_num_max:setTextOut(
    function (ty) 
		if (ty == 3) then
			return "#define TCFG_KEY_NUM_MAX" .. TAB_TABLE[5] .. iokey_num_max.val .. TAB_TABLE[1]  .. "//最大可配置的按键数量" .. NLINE_TABLE[2]
		end
	end
);


local iokey_num = cfg:i32("IOKEY按键数量(1 ~ " .. enable_moudles["iokey"].num .. ")", 0);
iokey_num:setOSize(1);
if open_by_program == "create" then
depend_item_en_show(iokey_en, iokey_num);
end
iokey_num:setTextOut(
    function (ty) 
		if (ty == 3) then
			return "#define TCFG_KEY_NUM" .. TAB_TABLE[6] .. iokey_num.val .. "//配置有效的按键数量" .. NLINE_TABLE[2]
		end
	end
);
local iokey_num_hBox_view = cfg:hBox {
            cfg:stLabel(iokey_num.name);
            cfg:ispinView(iokey_num, 0, enable_moudles["iokey"].num, 1),
            cfg:stSpacer();
        }


--[[=============================== 函数定义 ================================--]]
function key_connect_way_add(cnt)
    local item = cfg:enum("KEY" .. cnt .. " 按键端口接线方式选择", CONNECT_WAY, 0);
    key_depend_key_en_key_num_show(cnt, iokey_en, iokey_num, item);
    item:setOSize(1);
    item:setTextOut(
        function (ty) 
            if (ty == 3) then
                return "#define TCFG_IOKEY" .. cnt .. "_CONNECT_WAY" .. TAB_TABLE[4] .. CONNECT_WAY_OUTPUT_TABLE[item.val] .. NLINE_TABLE[1]
            end
        end
    )
    return item;
end

function key_one_port_add(cnt)
    local one_port = cfg:enum("KEY" .. cnt .. " 按键一端接 IO 方式引脚选择:", PORTS,  0xFF)
    one_port:setOSize(1);
    one_port:setTextOut(
        function (ty) 
            if (ty == 3) then
                if (iokey_item_table[cnt].connect_way.val == 2) then
                    return "#define TCFG_IOKEY" .. cnt .. "_ONE_PORT" .. TAB_TABLE[4] .. PORTS_TABLE[0xFF] .. NLINE_TABLE[1]
                else
                    return "#define TCFG_IOKEY" .. cnt .. "_ONE_PORT" .. TAB_TABLE[4] .. PORTS_TABLE[one_port.val] .. NLINE_TABLE[1]
                end
            end
        end
    )

    one_port:addDeps{iokey_en, iokey_num}
    one_port:addDeps{iokey_item_table[cnt].connect_way}
    one_port:setDepChangeHook(function ()
        if (iokey_en.val == 0 or (cnt >= iokey_num.val)) then
            one_port:setShow(false);
        else
            if (iokey_item_table[cnt].connect_way.val == 2) then
                one_port:setShow(false);
            else
                one_port:setShow(true);
            end
        end
    end);

     return one_port;
end

function key_in_port_add(cnt)
    local in_port = cfg:enum("KEY" .. cnt .. " 按键端口 1 引脚选择:", PORTS,  0xFF)
    in_port:setOSize(1);
    in_port:setTextOut(
        function (ty) 
            if (ty == 3) then
                if (iokey_item_table[cnt].connect_way.val == 2) then
                    return "#define TCFG_IOKEY" .. cnt .. "_IN_PORT" .. TAB_TABLE[5] .. PORTS_TABLE[in_port.val] .. NLINE_TABLE[1]
                else
                    return "#define TCFG_IOKEY" .. cnt .. "_IN_PORT" .. TAB_TABLE[5] .. PORTS_TABLE[0xFF] .. NLINE_TABLE[1]
                end
            end
        end
    )

    in_port:addDeps{iokey_en, iokey_num}
    in_port:addDeps{iokey_item_table[cnt].connect_way}
    in_port:setDepChangeHook(function ()
        if (iokey_en.val == 0 or (cnt >= iokey_num.val)) then
            in_port:setShow(false);
        else
            if (iokey_item_table[cnt].connect_way.val == 2) then
                in_port:setShow(true);
            else
                in_port:setShow(false);
            end
        end
    end);

     return in_port;
end


function key_out_port_add(cnt)
    local out_port = cfg:enum("KEY" .. cnt .. " 按键端口 2 引脚选择:", PORTS,  0xFF)
    out_port:setOSize(1);
    out_port:setTextOut(
        function (ty) 
            if (ty == 3) then
                if (iokey_item_table[cnt].connect_way.val == 2) then
                    return "#define TCFG_IOKEY" .. cnt .. "_OUT_PORT" .. TAB_TABLE[4] .. PORTS_TABLE[out_port.val] .. NLINE_TABLE[1]
                else
                    return "#define TCFG_IOKEY" .. cnt .. "_OUT_PORT" .. TAB_TABLE[4] .. PORTS_TABLE[0xFF] .. NLINE_TABLE[1]
                end
            end
        end
    )

    out_port:addDeps{iokey_en, iokey_num}
    out_port:addDeps{iokey_item_table[cnt].connect_way}
    out_port:setDepChangeHook(function ()
        if (iokey_en.val == 0 or (cnt >= iokey_num.val)) then
            out_port:setShow(false);
        else
            if (iokey_item_table[cnt].connect_way.val == 2) then
                out_port:setShow(true);
            else
                out_port:setShow(false);
            end
        end
    end);

     return out_port;
end


function enum_item_group_hbox_view(item)
    local group_view = cfg:stGroup(item.name,
        cfg:hBox {
            cfg:enumView(item),
            cfg:stSpacer(),
        }
    );

    return group_view;
end


function key_port_sel_vbox_group_view(...)
    local port_sel_group_view;
    local vBox_item_view_list = {};
    local vBox_item_view;
    for k, v in pairs{...} do
        insert_item_to_list(vBox_item_view_list, v);
    end

    port_sel_group_view = cfg:stGroup("",
        cfg:vBox(vBox_item_view_list)
    );

    return port_sel_group_view;
end


function key_group_hbox_layout_view(cnt, ...)
    local key_items_group_layout;
    local vBox_item_view_list = {};
    for k, v in pairs{...} do
        insert_item_to_list(vBox_item_view_list, v);
    end

    key_items_group_layout = cfg:stGroup("KEY" .. cnt .. " 配置",
        cfg:hBox(vBox_item_view_list)
    );

    return key_items_group_layout;
end

local iokey_hw_msg_output_htext = {};
local iokey_hw_output_bin_table = {};
local iokey_hw_msg_group_view_list = {};
local iokey_hw_msg_default_list = {};
for i = 1, enable_moudles["iokey"].num  do
    local iokey_cfg = {
            connect_way,
            connect_way_group_view,
            one_port,
            one_port_group_view,
            in_port,
            out_port,
            two_port_group_view,
            short_click_msg_item,
            long_click_msg_item,
            hold_click_msg_item,
            up_click_msg_item,
            double_click_msg_item,
            triple_click_msg_item,
            msg_group_view,
            key_port_sel_group_view,
            key_top_group_view,
        };

    local index = i - 1;
    iokey_item_table[index] = iokey_cfg;

    iokey_cfg.connect_way = key_connect_way_add(index);
    iokey_cfg.connect_way_group_view = enum_item_group_hbox_view(iokey_cfg.connect_way);

    iokey_cfg.one_port = key_one_port_add(index);
    iokey_cfg.one_port_group_view = enum_item_group_hbox_view(iokey_cfg.one_port);

    iokey_cfg.in_port = key_in_port_add(index);
    iokey_cfg.out_port = key_out_port_add(index);
    iokey_cfg.two_port_group_view = enum_items_group_vbox_view(iokey_cfg.in_port, iokey_cfg.out_port);

    iokey_cfg.short_click_msg_item = key_short_click_msg_cfg_add("IO", index, iokey_en, iokey_num);
    iokey_cfg.long_click_msg_item = key_long_click_msg_cfg_add("IO", index, iokey_en, iokey_num);
    iokey_cfg.hold_click_msg_item = key_hold_click_msg_cfg_add("IO", index, iokey_en, iokey_num);
    iokey_cfg.up_click_msg_item = key_up_click_msg_cfg_add("IO", index, iokey_en, iokey_num);
    iokey_cfg.double_click_msg_item = key_double_click_msg_cfg_add("IO", index, iokey_en, iokey_num);
    iokey_cfg.triple_click_msg_item = key_triple_click_msg_cfg_add("IO", index, iokey_en, iokey_num);

    iokey_cfg.msg_group_view = enum_items_group_vbox_view(iokey_cfg.short_click_msg_item,
                                                          iokey_cfg.long_click_msg_item,
                                                          iokey_cfg.hold_click_msg_item,
                                                          iokey_cfg.up_click_msg_item,
                                                          iokey_cfg.double_click_msg_item,
                                                          iokey_cfg.triple_click_msg_item);

    iokey_cfg.key_port_sel_group_view = key_port_sel_vbox_group_view(iokey_cfg.connect_way_group_view,
                                                              iokey_cfg.one_port_group_view,
                                                              iokey_cfg.two_port_group_view);

    iokey_cfg.key_top_group_view = key_group_hbox_layout_view(index, iokey_cfg.key_port_sel_group_view,
                                                              iokey_cfg.msg_group_view);
	insert_item_to_list(iokey_hw_msg_default_list, iokey_cfg.connect_way);
	insert_item_to_list(iokey_hw_msg_default_list, iokey_cfg.one_port);
	insert_item_to_list(iokey_hw_msg_default_list, iokey_cfg.in_port);
	insert_item_to_list(iokey_hw_msg_default_list, iokey_cfg.out_port);
	
	insert_item_to_list(iokey_hw_msg_default_list, iokey_cfg.short_click_msg_item);
	insert_item_to_list(iokey_hw_msg_default_list, iokey_cfg.long_click_msg_item);
	insert_item_to_list(iokey_hw_msg_default_list, iokey_cfg.hold_click_msg_item);
	insert_item_to_list(iokey_hw_msg_default_list, iokey_cfg.up_click_msg_item);
	insert_item_to_list(iokey_hw_msg_default_list, iokey_cfg.double_click_msg_item);
	insert_item_to_list(iokey_hw_msg_default_list, iokey_cfg.triple_click_msg_item);
	
--按键显示
    insert_item_to_list(iokey_hw_msg_group_view_list, iokey_cfg.key_top_group_view);

--按键bin输出
    insert_item_to_list(iokey_hw_output_bin_table, iokey_cfg.connect_way);
    insert_item_to_list(iokey_hw_output_bin_table, iokey_cfg.one_port);
    insert_item_to_list(iokey_hw_output_bin_table, iokey_cfg.out_port);
    insert_item_to_list(iokey_hw_output_bin_table, iokey_cfg.in_port);

    insert_item_to_list(iokey_output_msg_bin_list, iokey_cfg.short_click_msg_item);
    insert_item_to_list(iokey_output_msg_bin_list, iokey_cfg.long_click_msg_item);
    insert_item_to_list(iokey_output_msg_bin_list, iokey_cfg.hold_click_msg_item);
    insert_item_to_list(iokey_output_msg_bin_list, iokey_cfg.up_click_msg_item);
    insert_item_to_list(iokey_output_msg_bin_list, iokey_cfg.double_click_msg_item);
    insert_item_to_list(iokey_output_msg_bin_list, iokey_cfg.triple_click_msg_item);
--按键text输出
    insert_item_to_list(iokey_hw_msg_output_htext, iokey_cfg.connect_way);
    insert_item_to_list(iokey_hw_msg_output_htext, iokey_cfg.one_port);
    insert_item_to_list(iokey_hw_msg_output_htext, iokey_cfg.in_port);
    insert_item_to_list(iokey_hw_msg_output_htext, iokey_cfg.out_port);

    insert_item_to_list(iokey_hw_msg_output_htext, iokey_cfg.short_click_msg_item);
    insert_item_to_list(iokey_hw_msg_output_htext, iokey_cfg.long_click_msg_item);
    insert_item_to_list(iokey_hw_msg_output_htext, iokey_cfg.hold_click_msg_item);
    insert_item_to_list(iokey_hw_msg_output_htext, iokey_cfg.up_click_msg_item);
    insert_item_to_list(iokey_hw_msg_output_htext, iokey_cfg.double_click_msg_item);
    insert_item_to_list(iokey_hw_msg_output_htext, iokey_cfg.triple_click_msg_item);
end

--[[===================================================================================
==================================== 模块返回汇总 =====================================
====================================================================================--]]
-- A. 输出htext
local iokey_output_htext_table = {};
insert_item_to_list(iokey_output_htext_table, iokey_comment_begin_htext);
insert_item_to_list(iokey_output_htext_table, iokey_en);

if enable_moudles["iokey"].enable == true then
insert_item_to_list(iokey_output_htext_table, iokey_mcro_begin);
insert_item_to_list(iokey_output_htext_table, iokey_num_max);
insert_item_to_list(iokey_output_htext_table, iokey_num);
insert_list_to_list(iokey_output_htext_table, iokey_hw_msg_output_htext);
insert_item_to_list(iokey_output_htext_table, iokey_mcro_end);
end

insert_list_to_list(board_output_text_tabs, iokey_output_htext_table);

if enable_moudles["iokey"].enable == false then
    return; -- 只输出htext, 不显示返回
end
-- B. 输出ctext：无

-- C. 输出bin：
local iokey_output_msg_bin = cfg:group("IOKEY_MSG_CFG_BIN",
    BIN_ONLY_CFG["BT_CFG"].key_msg.id,
    1,
    iokey_output_msg_bin_list
);
insert_item_to_list(board_output_bin_tabs, iokey_output_msg_bin);

local iokey_output_hw_bin_list = {};
insert_item_to_list(iokey_output_hw_bin_list, iokey_en);
insert_item_to_list(iokey_output_hw_bin_list, iokey_num);
insert_list_to_list(iokey_output_hw_bin_list, iokey_hw_output_bin_table);

local iokey_output_hw_bin = cfg:group("IOKEY_HW_CFG_BIN",
    BIN_ONLY_CFG["HW_RES"].iokey.id,
    1,
    iokey_output_hw_bin_list
);
insert_item_to_list(board_output_bin_tabs, iokey_output_hw_bin);

-- E. 默认值
local iokey_default_items = {};

if open_by_program == "create" then
	insert_item_to_list(iokey_default_items, iokey_en);
    insert_item_to_list(iokey_default_items, iokey_num);
end
insert_list_to_list(iokey_default_items, iokey_hw_msg_default_list);

-- D. 显示
local iokey_en_hw_msg_view = {};

if open_by_program == "fw_edit" then
    iokey_num:setShow(false);
end

if open_by_program == "create" then
	insert_item_to_list(iokey_en_hw_msg_view, iokey_en_group_view);
end

insert_item_to_list(iokey_en_hw_msg_view, iokey_num_hBox_view);

local iokey_hw_msg_group_view = cfg:vBox(iokey_hw_msg_group_view_list);
insert_item_to_list(iokey_en_hw_msg_view, iokey_hw_msg_group_view);

local iokey_view_list = cfg:vBox {
    cfg:stHScroll(cfg:vBox(iokey_en_hw_msg_view)),
	cfg:stButton("恢复IOKEY按键配置默认值", reset_to_default(iokey_default_items)),
};

local iokey_output_view = {"IOKEY 按键配置", iokey_view_list};

insert_item_to_list(board_view_tabs, iokey_output_view);


-- F. bindGroup：无

