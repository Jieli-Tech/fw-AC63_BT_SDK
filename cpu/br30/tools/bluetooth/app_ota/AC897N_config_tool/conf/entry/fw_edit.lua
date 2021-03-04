package.path = package.path .. ';' .. cfg.dir .. '/?.lua'

require("fw_common");

projdir = cfg.projDir;

open_by_program = "fw_edit"; -- 被 fw 编辑工具打开

output_view_tabs = {}
output_bin_tabs = {}

require("common");

-- 把下面这些insert放到模块里面去
insert_item_to_list(output_view_tabs, board_view);
insert_item_to_list(output_view_tabs, bluetooth_view);
-- insert_item_to_list(output_view_tabs, tone_view); -- 已经放到了 tone 单独配置下
insert_item_to_list(output_view_tabs, status_view);
insert_item_to_list(output_view_tabs, comvol_view);
-- insert_item_to_list(output_view_tabs, anc_view); -- 已经放到单独的 anc 配置下

insert_list_to_list(output_bin_tabs, comvol_bin_groups);
insert_list_to_list(output_bin_tabs, board_output_bin_tabs);
insert_list_to_list(output_bin_tabs, status_output_bin_tabs);
insert_list_to_list(output_bin_tabs, bt_output_bin_tabs);
insert_item_to_list(output_bin_tabs, lua_cfg_output_bin);
-- insert_list_to_list(output_bin_tabs, anc_output_bin_tabs);


local output_bin_tabs_view = cfg:vBox {
    cfg:stTab(output_view_tabs);
};

cfg:addFirmwareFile("cfg_tool.bin",
		"BIN文件配置",
		2, -- 文件类型，是个bin文件
		output_bin_tabs, -- 组的列表
		output_bin_tabs_view
);

