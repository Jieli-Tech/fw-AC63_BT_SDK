package.path = package.path .. ';' .. cfg.dir .. '/?.lua'

require("fw_common");

open_by_program = "fw_edit"; -- 被 fw 编辑工具打开
is_open_by_ufw = true;


output_view_tabs = {}
output_bin_tabs = {}

enable_moudles = {
    ["isdtool"] = false,
    ["audio"]   = true,
    ["charge"]  = true,
    ["status"]  = true,
    ["tone"]    = true,
    ["bluetooth"]   = true,
    ["key_msg"] = {enable = true, num = 10},
};

require("common")

-- 把下面这些insert放到模块里面去
insert_item_to_list(output_view_tabs, bluetooth_view);
insert_item_to_list(output_view_tabs, tone_view);

insert_list_to_list(output_bin_tabs, bt_output_bin_tabs);

local output_bin_tabs_view = cfg:vBox {
    cfg:stTab(output_view_tabs);
};



cfg:addFirmwareFile("cfg_tool.bin",
		"蓝牙配置",
		2, -- 文件类型，是个bin文件
		output_bin_tabs, -- 组的列表
		output_bin_tabs_view
);

