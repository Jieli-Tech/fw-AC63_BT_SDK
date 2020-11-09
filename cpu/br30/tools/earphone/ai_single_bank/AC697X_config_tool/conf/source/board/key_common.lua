local KEY_MSG_EVENT_TABLE = {
        [0xFF] = "NO_MSG",
        [0] = "KEY_POWER_ON",
		[1] = "KEY_POWEROFF",
		[2] = "KEY_POWEROFF_HOLD",
        [3] = "KEY_MUSIC_PP",
        [4] = "KEY_MUSIC_PREV",
        [5] = "KEY_MUSIC_NEXT",
        [6] = "KEY_VOL_UP",
        [7] = "KEY_VOL_DOWN",
        [8] = "KEY_CALL_LAST_NO",
        [9] = "KEY_CALL_HANG_UP",
        [10] = "KEY_CALL_ANSWER",
        [11] = "KEY_OPEN_SIRI",
        [12] = "KEY_HID_CONTROL",
};

local KEY_MSG_EVENT = cfg:enumMap("按键消息类型", KEY_MSG_EVENT_TABLE);



--[[=============================== 配置子项1-3: 按键消息配置 ================================--]]
function key_depend_key_en_key_num_show(cnt, key_en_item, key_num_item, new_item)
    new_item:addDeps{key_en_item, key_num_item};
    new_item:setDepChangeHook(
        function()
            new_item:setShow((key_en_item.val ~= 0) and (cnt < key_num_item.val));
        end);
end

function key_depend_key_num_show(cnt, key_num_item, new_item)
    new_item:addDeps{key_num_item};
    new_item:setDepChangeHook(
        function()
            new_item:setShow(cnt < key_num_item.val);
        end);
end

--短按
function key_short_click_msg_cfg_add(prefix, cnt, key_en_item, key_num_item)
    local  key_short_click_msg_cfg = cfg:enum(prefix .. "KEY" .. cnt  .. " 按键短按消息:", KEY_MSG_EVENT,  0xFF);
    key_short_click_msg_cfg:setOSize(1);
    key_short_click_msg_cfg:setTextOut(
        function (ty)
            if (ty == 3) then
                return "#define TCFG_KEY" .. cnt  .. "_KEY_SHORT_CLICK_MSG" .. TAB_TABLE[2] .. KEY_MSG_EVENT_TABLE[key_short_click_msg_cfg.val] .. NLINE_TABLE[1]
            end
        end
    )

    key_depend_key_en_key_num_show(cnt, key_en_item, key_num_item, key_short_click_msg_cfg);

    return key_short_click_msg_cfg;
end

--长按
function key_long_click_msg_cfg_add(prefix, cnt, key_en_item, key_num_item)
    local  key_long_click_msg_cfg = cfg:enum(prefix .. "KEY" .. cnt  .. " 按键长按消息:", KEY_MSG_EVENT,  0xFF);
    key_long_click_msg_cfg:setOSize(1);
    key_long_click_msg_cfg:setTextOut(
        function (ty)
            if (ty == 3) then
                return "#define TCFG_KEY" .. cnt  .. "_KEY_LONG_CLICK_MSG" .. TAB_TABLE[2] .. KEY_MSG_EVENT_TABLE[key_long_click_msg_cfg.val] .. NLINE_TABLE[1]
            end
        end
    )

    key_depend_key_en_key_num_show(cnt, key_en_item, key_num_item, key_long_click_msg_cfg);

    return key_long_click_msg_cfg;
end

--HOLD
function key_hold_click_msg_cfg_add(prefix, cnt, key_en_item, key_num_item)
    local  key_hold_click_msg_cfg = cfg:enum(prefix .. "KEY" .. cnt  .. " 按键HOLD消息:", KEY_MSG_EVENT,  0xFF);
    key_hold_click_msg_cfg:setOSize(1);
    key_hold_click_msg_cfg:setTextOut(
        function (ty)
            if (ty == 3) then
                return "#define TCFG_KEY" .. cnt  .. "_KEY_HOLD_CLICK_MSG" .. TAB_TABLE[2] .. KEY_MSG_EVENT_TABLE[key_hold_click_msg_cfg.val] .. NLINE_TABLE[1]
            end
        end
    )

    key_depend_key_en_key_num_show(cnt, key_en_item, key_num_item, key_hold_click_msg_cfg);

    return key_hold_click_msg_cfg;
end


--按键抬按消息
function key_up_click_msg_cfg_add(prefix, cnt, key_en_item, key_num_item)
    local  key_up_click_msg_cfg = cfg:enum(prefix .. "KEY" .. cnt  .. " 按键抬按消息:", KEY_MSG_EVENT,  0xFF);
    key_up_click_msg_cfg:setOSize(1);
    key_up_click_msg_cfg:setTextOut(
        function (ty)
            if (ty == 3) then
                return "#define TCFG_KEY" .. cnt  .. "_KEY_UP_CLICK_MSG" .. TAB_TABLE[3] .. KEY_MSG_EVENT_TABLE[key_up_click_msg_cfg.val] .. NLINE_TABLE[1]
            end
        end
    )

    key_depend_key_en_key_num_show(cnt, key_en_item, key_num_item, key_up_click_msg_cfg);

    return key_up_click_msg_cfg;
end

--按键双击消息
function key_double_click_msg_cfg_add(prefix, cnt, key_en_item, key_num_item)
    local  key_double_click_msg_cfg = cfg:enum(prefix .. "KEY" .. cnt  .. " 按键双击消息:", KEY_MSG_EVENT,  0xFF);
    key_double_click_msg_cfg:setOSize(1);
    key_double_click_msg_cfg:setTextOut(
        function (ty)
            if (ty == 3) then
                return "#define TCFG_KEY" .. cnt  .. "_KEY_DOUBLE_CLICK_MSG" .. TAB_TABLE[2] .. KEY_MSG_EVENT_TABLE[key_double_click_msg_cfg.val] .. NLINE_TABLE[1]
            end
        end
    )

    key_depend_key_en_key_num_show(cnt, key_en_item, key_num_item, key_double_click_msg_cfg);

    return key_double_click_msg_cfg;
end

--按键三击消息
function key_triple_click_msg_cfg_add(prefix, cnt, key_en_item, key_num_item)
    local  key_triple_click_msg_cfg = cfg:enum(prefix .. "KEY" .. cnt  .. " 按键三击消息:", KEY_MSG_EVENT,  0xFF);
    key_triple_click_msg_cfg:setOSize(1);
    key_triple_click_msg_cfg:setTextOut(
        function (ty)
            if (ty == 3) then
                return "#define TCFG_KEY" .. cnt  .. "_KEY_TRIPLE_CLICK_MSG" .. TAB_TABLE[2] .. KEY_MSG_EVENT_TABLE[key_triple_click_msg_cfg.val] .. NLINE_TABLE[2]
            end
        end
    )

    key_depend_key_en_key_num_show(cnt, key_en_item, key_num_item, key_triple_click_msg_cfg);

    return key_triple_click_msg_cfg;
end


function enum_items_group_vbox_view(...)
    local items_group_view;
    local hBox_item_view_list = {};
    local hBox_item_view;
    for k, v in pairs{...} do
        hBox_item_view = enum_item_hbox_view(v);
        insert_item_to_list(hBox_item_view_list, hBox_item_view);
    end

    items_group_view = cfg:stGroup("",
        cfg:vBox(hBox_item_view_list)
    );

    return items_group_view;
end

function groups_hbox_view(group_name, ...)
    local ret_groups_hbox_view;
    local space_view = cfg:stSpacer();
    local groups_hbox_view_list = {};
    for k, v in pairs{...} do
        insert_item_to_list(groups_hbox_view_list, v);
    end
    insert_item_to_list(groups_hbox_view_list, space_view);
    ret_groups_hbox_view = cfg:stGroup(group_name,
        cfg:hBox(groups_hbox_view_list)
    );

    return ret_groups_hbox_view;
end


function groups_vbox_view(group_name, ...)
    local ret_groups_vbox_view;
    local groups_vbox_view_list = {};
    local space_view = cfg:stSpacer();
    for k, v in pairs{...} do
        insert_item_to_list(groups_vbox_view_list, v);
    end
    insert_item_to_list(groups_vbox_view_list, space_view);
    ret_groups_vbox_view = cfg:stGroup(group_name,
        cfg:vBox(groups_vbox_view_list)
    );

    return ret_groups_vbox_view;
end

