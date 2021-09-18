
--[[=============================== 配置项列表 ================================--]]
local iokey_item_table = {};
local iokey_output_msg_bin_list = {};

--[[===========================================================================--]]

local iokey_num = cfg:i32("按键数量", 3);
local iokey_num_hBox_view = cfg:hBox {
            cfg:stLabel(iokey_num.name .. "："),
            cfg:ispinView(iokey_num, 0, enable_moudles["key_msg"].num, 1),
            cfg:stLabel("（1 ~ " .. enable_moudles["key_msg"].num .. "个）"),
            cfg:stSpacer(),
        }


function enum_item_group_hbox_view(item)
    local group_view = cfg:stGroup(item.name,
        cfg:hBox {
            cfg:enumView(item),
            cfg:stSpacer(),
        }
    );

    return group_view;
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
for i = 1, enable_moudles["key_msg"].num  do
    local iokey_cfg = {
            short_click_msg_item,
            long_click_msg_item,
            hold_click_msg_item,
            up_click_msg_item,
            double_click_msg_item,
            triple_click_msg_item,
            msg_group_view,
        };

    local index = i - 1;
    iokey_item_table[index] = iokey_cfg; 

    iokey_cfg.short_click_msg_item = key_short_click_msg_cfg_add("", index, iokey_en, iokey_num);
    iokey_cfg.long_click_msg_item = key_long_click_msg_cfg_add("", index, iokey_en, iokey_num);
    iokey_cfg.hold_click_msg_item = key_hold_click_msg_cfg_add("", index, iokey_en, iokey_num);
    iokey_cfg.up_click_msg_item = key_up_click_msg_cfg_add("", index, iokey_en, iokey_num);
    iokey_cfg.double_click_msg_item = key_double_click_msg_cfg_add("", index, iokey_en, iokey_num);
    iokey_cfg.triple_click_msg_item = key_triple_click_msg_cfg_add("", index, iokey_en, iokey_num);

    iokey_cfg.msg_group_view = enum_items_group_vbox_view(iokey_cfg.short_click_msg_item,
                                                          iokey_cfg.long_click_msg_item,
                                                          iokey_cfg.hold_click_msg_item,
                                                          iokey_cfg.up_click_msg_item,
                                                          iokey_cfg.double_click_msg_item,
                                                          iokey_cfg.triple_click_msg_item);

	insert_item_to_list(iokey_hw_msg_default_list, iokey_cfg.short_click_msg_item);
	insert_item_to_list(iokey_hw_msg_default_list, iokey_cfg.long_click_msg_item);
	insert_item_to_list(iokey_hw_msg_default_list, iokey_cfg.hold_click_msg_item);
	insert_item_to_list(iokey_hw_msg_default_list, iokey_cfg.up_click_msg_item);
	insert_item_to_list(iokey_hw_msg_default_list, iokey_cfg.double_click_msg_item);
	insert_item_to_list(iokey_hw_msg_default_list, iokey_cfg.triple_click_msg_item);
	
--按键显示
    insert_item_to_list(iokey_hw_msg_group_view_list, iokey_cfg.msg_group_view);

--按键bin输出
    insert_item_to_list(iokey_output_msg_bin_list, iokey_cfg.short_click_msg_item);
    insert_item_to_list(iokey_output_msg_bin_list, iokey_cfg.long_click_msg_item);
    insert_item_to_list(iokey_output_msg_bin_list, iokey_cfg.hold_click_msg_item);
    insert_item_to_list(iokey_output_msg_bin_list, iokey_cfg.up_click_msg_item);
    insert_item_to_list(iokey_output_msg_bin_list, iokey_cfg.double_click_msg_item);
    insert_item_to_list(iokey_output_msg_bin_list, iokey_cfg.triple_click_msg_item);
end

--[[===================================================================================
==================================== 模块返回汇总 =====================================
====================================================================================--]]
-- A. 输出htext

if enable_moudles["key_msg"].enable == false then
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

-- E. 默认值
local iokey_default_items = {};

if open_by_program == "create" then
    insert_item_to_list(iokey_default_items, iokey_num);
end
insert_list_to_list(iokey_default_items, iokey_hw_msg_default_list);

-- D. 显示
local iokey_en_hw_msg_view = {};

if open_by_program == "fw_edit" then
    iokey_num:setShow(false);
end

insert_item_to_list(iokey_en_hw_msg_view, iokey_num_hBox_view);

local iokey_hw_msg_group_view = cfg:vBox(iokey_hw_msg_group_view_list);
insert_item_to_list(iokey_en_hw_msg_view, iokey_hw_msg_group_view);

local iokey_view_list = cfg:vBox {
    cfg:stHScroll(cfg:vBox(iokey_en_hw_msg_view)),
	cfg:stButton("恢复按键消息配置默认值", reset_to_default(iokey_default_items)),
};

local iokey_output_view = {"按键消息配置", iokey_view_list};

insert_item_to_list(board_view_tabs, iokey_output_view);


-- F. bindGroup：无

