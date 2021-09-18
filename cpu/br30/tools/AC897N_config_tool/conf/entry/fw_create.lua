package.path = package.path .. ';' .. cfg.dir .. '/?.lua'
package.path = package.path .. ';' .. cfg.projDir .. '/?.lua'

require("fw_common");

open_by_program = "create"; -- 生成编辑

require("common");

-- 把下面这些insert放到模块里面去

fw_create_output_view_tabs = {}
insert_item_to_list(fw_create_output_view_tabs, board_view);
insert_item_to_list(fw_create_output_view_tabs, bluetooth_view);
insert_item_to_list(fw_create_output_view_tabs, tone_view);
insert_item_to_list(fw_create_output_view_tabs, status_view);
insert_item_to_list(fw_create_output_view_tabs, isdtool_view);
insert_item_to_list(fw_create_output_view_tabs, comvol_view);
insert_item_to_list(fw_create_output_view_tabs, anc_view);

-- isdtool 的，已经放置过去了

---------------------bin文件输出-----------------------
cfg:addOutputGroups(board_output_bin_tabs);
cfg:addOutputGroups(status_output_bin_tabs);
cfg:addOutputGroups(bt_output_bin_tabs);
cfg:addOutputGroups({lua_cfg_output_bin});

-----------------------C文件或者H文件输出-----------------------
cfg:addTextOutputs(generic_def_output_text_begin_tabs);
cfg:addTextOutputs(board_output_text_tabs);
cfg:addTextOutputs(bt_output_htext_tabs);
cfg:addTextOutputs(status_output_text_tabs);
cfg:addTextOutputs(generic_def_output_text_end_tabs);
cfg:addTextOutputs(anc_output_htext_tabs);


-----------------------设置界面-----------------------
local fw_create_view = cfg:vBox {
	cfg:stTab(fw_create_output_view_tabs)
}
cfg:setLayout(fw_create_view);


dump_save_file_path = state_out_path .. 'cfg_tool_state_complete.lua'; -- 生成目录也是加载目录

-------------------- 设置文件输出目录 --------------------
-- ty 是输出的文件类型，1 是ｃ文件，2是bin文件  3是文件
cfg:setOutputPath(function (ty)
	if (ty == 1) then
		-- 对于c文件，输出到当前配置文件所在目录的 myconfig.c 下
		return c_out_path .. 'cfg_tool.c';
	elseif (ty == 2) then 
		-- 对于bin文件，输出到当前配置文件所在目录的 myconfig.bin 下
		return bin_out_path .. 'cfg_tool.bin';
	elseif (ty == 3) then
		return h_out_path .. 'cfg_tool.h';
	elseif (ty == 4) then
		return dump_save_file_path; -- 生成目录也是加载目录
	elseif (ty == 5) then
		return ver_out_path .. 'script.ver';
    elseif (ty == 6) then
        return default_out_path .. 'default_cfg.lua';
	end
end);



-------------------- 设置文件输出后的回调函数 --------------------
-- ty 是输出的文件类型，1 是ｃ文件，2是bin文件  3是文件
cfg:setSaveAllHook(function (ty)
	if (ty == 1) then 
		print('you are saving c source file');
	elseif (ty == 2) then
        if cfg.lang == "zh" then
		    cfg:msgBox("info", "cfg_tool.bin 文件保存在" .. bin_out_path);
        else
		    cfg:msgBox("info", "cfg_tool.bin file have been saved in" .. bin_out_path);
        end
		print('you are saving binary file');
	elseif (ty == 3) then
		print('you are saving header file');
	elseif (ty == 0) then
		-- 如果全部保存完毕，调用这个分支
	end
end);


