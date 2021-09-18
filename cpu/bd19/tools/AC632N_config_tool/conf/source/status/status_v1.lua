if enable_moudles["status"] == false then
    return;
else

local MOUDLE_COMMENT_NAME = "//                                  状态同步配置                                       //"

local comment_begin = cfg:i32("状态同步注释开始", 0);

--local status_comment_begin_htext = module_output_comment(comment_begin, MOUDLE_COMMENT_NAME);

local status_sync_output_bin_list = {};
local status_sync_ui_tone_view_table = {};
local status_sync_group_view_table = {};
local status_sync_cfg_default_table = {};

-- A. 输出htext, 无
-- B. 输出ctext：无

local pwmled_display_table = {
    [0xFF] = "无LED灯显示效果",
    [1] = "蓝灯和红灯全灭",
    [2] = "蓝灯和红灯全亮", 

    [3] = "蓝灯亮",
    [4] = "蓝灯灭",
    [5] = "蓝灯慢闪",
    [6] = "蓝灯快闪",
    [7] = "蓝灯五秒内连续闪烁两下",
    [8] = "蓝灯五秒内闪烁一下",

    [9] = "红灯亮",
    [10] = "红灯灭",
    [11] = "红灯慢闪",
    [12] = "红灯快闪",
    [13] = "红灯五秒内连续闪烁两下",
    [14] = "红灯五秒内闪烁一下",

    [15] = "红灯和蓝灯交替闪烁(快闪)",
    [16] = "红灯和蓝灯交替闪烁(慢闪)",

    [17] = "蓝灯呼吸状态",
    [18] = "红灯呼吸模式",
    [19] = "红蓝交替呼吸模式",

    [21] = "红灯闪烁三下",
    [22] = "蓝灯闪烁三下",
};

local PWMLED_DISPLAY_LIST = cfg:enumMap("LED灯显示效果列表", pwmled_display_table);

local tone_display_table = {
    [0xFF] = "无提示音",
    [0] = "数字0",
    [1] = "数字1",
    [2] = "数字2",
    [3] = "数字3",
    [4] = "数字4",
    [5] = "数字5",
    [6] = "数字6",
    [7] = "数字7",
    [8] = "数字8",
    [9] = "数字9",

    [10] = "蓝牙模式",
    [11] = "连接成功",
    [12] = "断开连接",
    [13] = "对耳连接成功",
    [14] = "对耳断开连接",
    [15] = "低电量",
    [16] = "关机",
    [17] = "开机",
    [18] = "来电",
    [19] = "最大音量",
};

local TONE_DISPLAY_LIST = cfg:enumMap("提示音播放列表", tone_display_table);

--[[
//==========================================================================================//
//                                  配置项: PWM_LED_CFG                                     //
//==========================================================================================//
--]]

--[[



--]]
local status_cfg_table = {
    [1] = {name = "开始充电",       ui_item = {cfg_item, default = 9},      tone_item = {cfg_item, default = 0xFF}, ui_tone_sel_view},
    [2] = {name = "充电完成",       ui_item = {cfg_item, default = 3},      tone_item = {cfg_item, default = 0xFF}, ui_tone_sel_view},
    [3] = {name = "开机",           ui_item = {cfg_item, default = 3},      tone_item = {cfg_item, default = 17},   ui_tone_sel_view},
    [4] = {name = "关机",           ui_item = {cfg_item, default = 21},     tone_item = {cfg_item, default = 16},   ui_tone_sel_view},
    [5] = {name = "低电",           ui_item = {cfg_item, default = 14},     tone_item = {cfg_item, default = 15},   ui_tone_sel_view},
    [6] = {name = "最大音量",       ui_item = {cfg_item, default = 0xFF},   tone_item = {cfg_item, default = 19},   ui_tone_sel_view},
    [7] = {name = "来电",           ui_item = {cfg_item, default = 0xFF},   tone_item = {cfg_item, default = 18},   ui_tone_sel_view},
    [8] = {name = "去电",           ui_item = {cfg_item, default = 0xFF},   tone_item = {cfg_item, default = 0xFF}, ui_tone_sel_view},
    [9] = {name = "通话中",         ui_item = {cfg_item, default = 0xFF},   tone_item = {cfg_item, default = 0xFF}, ui_tone_sel_view},
    [10] = {name = "蓝牙初始化完成", ui_item = {cfg_item, default = 16},     tone_item = {cfg_item, default = 10},   ui_tone_sel_view},
    [11] = {name = "蓝牙连接成功",   ui_item = {cfg_item, default = 8},      tone_item = {cfg_item, default = 11},   ui_tone_sel_view},
    [12] = {name = "蓝牙断开连接",   ui_item = {cfg_item, default = 15},     tone_item = {cfg_item, default = 12},   ui_tone_sel_view},
    [13] = {name = "对耳连接成功",   ui_item = {cfg_item, default = 15},     tone_item = {cfg_item, default = 13},   ui_tone_sel_view},
    [14] = {name = "对耳断开连接",   ui_item = {cfg_item, default = 16},     tone_item = {cfg_item, default = 14},   ui_tone_sel_view},
};
--[[=============================== 配置子项0: status_sync_en ================================--]]
local status_sync_en = cfg:enum("状态同步配置使能开关", ENABLE_SWITCH, 1)
status_sync_en:setOSize(1);
local status_sync_en_group_view = cfg:stGroup("",
        cfg:hBox {
            cfg:stLabel(status_sync_en.name .. "："),
            cfg:enumView(status_sync_en),
            cfg:stSpacer(),
        }
);

insert_item_to_list(status_sync_output_bin_list, status_sync_en);

if open_by_program == "create" then
	insert_item_to_list(status_sync_cfg_default_table, status_sync_en);
	insert_item_to_list(status_sync_group_view_table, status_sync_en_group_view);
end

--[[=============================== 配置子项2: charge_start_ui_sel ================================--]]
local function status_ui_item_add(status)
    local ui_sel_item = cfg:enum(status.name .. "UI选择", PWMLED_DISPLAY_LIST, status.ui_item.default);
    ui_sel_item:setOSize(1);
    depend_item_en_show(status_sync_en, ui_sel_item);
	insert_item_to_list(status_sync_output_bin_list, ui_sel_item);

-- E. 默认值
	insert_item_to_list(status_sync_cfg_default_table, ui_sel_item);

    return ui_sel_item;
end

local function status_tone_item_add(status)
    local tone_sel_item = cfg:enum(status.name .. "TONE选择", TONE_DISPLAY_LIST, status.tone_item.default);
    tone_sel_item:setOSize(1);
    depend_item_en_show(status_sync_en, tone_sel_item);
	insert_item_to_list(status_sync_output_bin_list, tone_sel_item);
-- E. 默认值
	insert_item_to_list(status_sync_cfg_default_table, tone_sel_item);

    return tone_sel_item;
end

local function status_ui_tone_sel_view_add(status)
    local name_str = status.name;
    local tab_num = 0;
    local name_len = #name_str / 3;

    if (name_len == 2) then
        tab_num = 2;
    else if (name_len == 3) then
        tab_num = 1;
    else if (name_len == 4) then
        tab_num = 1;
    else if (name_len == 5) then
        tab_num = 1;
    else if (name_len == 6) then
        tab_num = 1;
    end
    end
    end
    end
    end

    local ui_tone_sel_hbox_view = cfg:hBox {
        cfg:stLabel(status.name .. "：" .. TAB_TABLE[tab_num]),
        cfg:enumView(status.ui_item.cfg_item),
        cfg:stLabel(TAB_TABLE[1]),
        cfg:enumView(status.tone_item.cfg_item),
        cfg:stSpacer(),
    };
	insert_item_to_list(status_sync_ui_tone_view_table, ui_tone_sel_hbox_view);

    return ui_tone_sel_hbox_view;
end

for index, status in ipairs(status_cfg_table) do
    status.ui_item.cfg_item = status_ui_item_add(status); 
end

for index, status in ipairs(status_cfg_table) do
    status.tone_item.cfg_item = status_tone_item_add(status);
    status.ui_tone_sel_view = status_ui_tone_sel_view_add(status);
end

local status_sync_default_button_view = cfg:stButton(" 状态同步配置恢复默认值 ", reset_to_default(status_sync_cfg_default_table));

-- C. 输出bin
local status_sync_output_bin = cfg:group("STATUS_SYNC",
    BIN_ONLY_CFG["BT_CFG"].status.id,
    1,
    status_sync_output_bin_list
);

insert_item_to_list(status_output_bin_tabs, status_sync_output_bin);

-- D. 显示
local status_sync_ui_tone_sel_group_view = cfg:vBox {
    cfg:stGroup("不同状态下 LED 灯显示效果和提示音设置",
        cfg:stHScroll(
            cfg:vBox(status_sync_ui_tone_view_table)))
};

insert_item_to_list(status_sync_group_view_table, status_sync_ui_tone_sel_group_view);

local status_sync_view = cfg:vBox {
    cfg:stHScroll(
        cfg:vBox(status_sync_group_view_table)
    ),
	status_sync_default_button_view,
};

insert_item_to_list(status_view_tabs, status_sync_view);

-- F. bindGroup：无
--cfg:bindStGroup(status_sync_view, status_sync_output_bin);

end

