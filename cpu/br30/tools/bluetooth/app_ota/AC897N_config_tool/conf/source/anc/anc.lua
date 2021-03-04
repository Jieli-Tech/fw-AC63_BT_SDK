if enable_moudles["anc"] == false then
    return;
else


local anc_config_version       = "ANCGAIN01"; -- 固定10字节长
local anc_coeff_config_version = "ANCCOEF01"; -- 固定10字节长

local anc_coeff_size = 588 * 4;

--[[===================================================================================
=============================== 配置子项8: ANC参数配置 ================================
====================================================================================--]]
local anc_config = {
	coeff_header = {},
	coeff = {}, -- a fix size binary, store coeffes


	header = {}, -- a fix size string
    dac_gain = {},
    ref_mic_gain = {},
    err_mic_gain = {},
    reserverd_cfg = {},
    anc_gain = {},
    transparency_gain = {},
	
	ui = {},

    output = {
        anc_config_item_table = {},
        anc_config_output_view_table = {},
        anc_config_htext_output_table = {},
    },
};


anc_config.reference_book_view = cfg:stButton("JL ANC调试手册.pdf",
    function()
        local ret = cfg:utilsShellOpenFile(anc_reference_book_path);
        if (ret == false) then
            if cfg.lang == "zh" then
		        cfg:msgBox("info", anc_reference_book_path .. "文件不存在");
            else
		        cfg:msgBox("info", anc_reference_book_path .. " file not exist.");
            end
        end
    end);


anc_config.header.cfg = cfg:fixbin("anc-header", 10, anc_config_version);
anc_config.header.view = cfg:hBox {
	cfg:stLabel("版本: " .. anc_config_version)
};

anc_config.coeff_header.cfg = cfg:fixbin("anc-coeff-header", 10, anc_coeff_config_version);
anc_config.coeff_header.view = cfg:hBox {
	cfg:stLabel("系数版本：" .. anc_coeff_config_version)
};

anc_config.coeff.cfg = cfg:fixbin("anc-coeff", anc_coeff_size, ""); -- default empty
anc_config.coeff.view = cfg:hBox {
	cfg:stLabel("系数"), cfg:labelView(anc_config.coeff.cfg)
};

-- 8-1 dac增益
anc_config.dac_gain.cfg = cfg:i32("dac_gain:  ", 8);
anc_config.dac_gain.cfg:setOSize(1);
anc_config.dac_gain.hbox_view = cfg:hBox {
    cfg:stLabel(anc_config.dac_gain.cfg.name .. TAB_TABLE[1]),
    cfg:ispinView(anc_config.dac_gain.cfg, 0, 15, 1),
    cfg:stLabel("(DAC增益，设置范围: 0 ~ 15，步进：1，默认值：8)"),
    cfg:stSpacer(),
};

-- 8-2 ref_mic_gain
anc_config.ref_mic_gain.cfg = cfg:i32("ref_mic_gain:  ", 6);
anc_config.ref_mic_gain.cfg:setOSize(1);
anc_config.ref_mic_gain.hbox_view = cfg:hBox {
    cfg:stLabel(anc_config.ref_mic_gain.cfg.name .. TAB_TABLE[1]),
    cfg:ispinView(anc_config.ref_mic_gain.cfg, 0, 19, 1),
    cfg:stLabel("(参考mic增益，设置范围: 0 ~ 19，步进：1，默认值：6)"),
    cfg:stSpacer(),
};

-- 8-3 err_mic_gain
anc_config.err_mic_gain.cfg = cfg:i32("err_mic_gain:  ", 6);
anc_config.err_mic_gain.cfg:setOSize(1);
anc_config.err_mic_gain.hbox_view = cfg:hBox {
    cfg:stLabel(anc_config.err_mic_gain.cfg.name .. TAB_TABLE[1]),
    cfg:ispinView(anc_config.err_mic_gain.cfg, 0, 19, 1),
    cfg:stLabel("(误差mic增益，设置范围: 0 ~ 19，步进：1，默认值：6)"),
    cfg:stSpacer(),
};

-- 8-4 reserverd_cfg 
anc_config.reserverd_cfg.cfg = cfg:i32("reserverd_cfg:  ", 8);
anc_config.reserverd_cfg.cfg:setOSize(1);
anc_config.reserverd_cfg.hbox_view = cfg:hBox {
    cfg:stLabel(anc_config.reserverd_cfg.cfg.name .. TAB_TABLE[1]),
    cfg:ispinView(anc_config.reserverd_cfg.cfg, 0, 15, 1),
    cfg:stLabel("(保留参数，设置范围: 0 ~ 15，步进：2dB 默认值：8)"),
    cfg:stSpacer(),
};

-- 8-5 anc_gain
anc_config.anc_gain.cfg = cfg:i32("anc_gain:  ", -8096);
anc_config.anc_gain.cfg:setOSize(4);
anc_config.anc_gain.hbox_view = cfg:hBox {
    cfg:stLabel(anc_config.anc_gain.cfg.name .. TAB_TABLE[1]),
    cfg:ispinView(anc_config.anc_gain.cfg, -32768, 32767, 1),
    cfg:stLabel("(降噪模式增益，设置范围: -32768 ~ 32767，步进：1， 默认值：-8096)"),
    cfg:stSpacer(),
};

-- 8-6 transparency_gain
anc_config.transparency_gain.cfg = cfg:i32("transparency_gain:  ", 7096);
anc_config.transparency_gain.cfg:setOSize(4);
anc_config.transparency_gain.hbox_view = cfg:hBox {
    cfg:stLabel(anc_config.transparency_gain.cfg.name),
    cfg:ispinView(anc_config.transparency_gain.cfg, -32768, 32767, 1),
    cfg:stLabel("(通透模式增益，设置范围: -32768 ~ 32767，步进：2dB 默认值：7096)"),
    cfg:stSpacer(),
};

--========================= ANC 参数输出汇总  ============================

anc_config.output.anc_coeff_config_items = {
	anc_config.coeff_header.cfg,
	anc_config.coeff.cfg,
};

anc_config.output.anc_config_items = {
	anc_config.header.cfg,
    anc_config.dac_gain.cfg,
    anc_config.ref_mic_gain.cfg,
    anc_config.err_mic_gain.cfg,
    anc_config.reserverd_cfg.cfg,
    anc_config.anc_gain.cfg,
    anc_config.transparency_gain.cfg,
};

anc_config.output.anc_coeff_config_output_views = {
	anc_config.coeff_header.view,
	anc_config.coeff.view,
};

anc_config.output.anc_config_output_views = {
	anc_config.header.view,
    anc_config.reference_book_view,
    anc_config.dac_gain.hbox_view,
    anc_config.ref_mic_gain.hbox_view,
    anc_config.err_mic_gain.hbox_view,
    anc_config.anc_gain.hbox_view,
    anc_config.transparency_gain.hbox_view,
	cfg:stSpacer()
};

-- A. 输出htext
anc_config.output.anc_config_htext_output_table = {
};

-- B. 输出ctext：无

-- C. 输出bin
anc_config.output.anc_config_output_bin = cfg:group("anc_config",
	BIN_ONLY_CFG["HW_CFG"].anc_config.id,
	1,
    anc_config.output.anc_config_items
);

anc_config.output.anc_coeff_config_output_bin = cfg:group("anc_coeff_config",
	BIN_ONLY_CFG["HW_CFG"].anc_coeff_config.id,
	1,
	anc_config.output.anc_coeff_config_items
);

anc_config.output.layout = cfg:vBox(anc_config.output.anc_config_output_views);
anc_config.output.coeff_layout = cfg:vBox(anc_config.output.anc_coeff_config_output_views);

-- D. 显示
anc_config.output.anc_config_group_view = cfg:stGroup("ANC 参数配置",
	anc_config.output.layout
);

anc_config.output.anc_coeff_config_group_view = cfg:stGroup("ANC 系数配置",
	anc_config.output.coeff_layout
);

-- E. 默认值, 见汇总

-- F. bindGroup
cfg:bindStGroup(anc_config.output.anc_config_group_view, anc_config.output.anc_config_output_bin);
cfg:bindStGroup(anc_config.output.anc_coeff_config_group_view, anc_config.output.anc_coeff_config_output_bin);

--[[===================================================================================
==================================== 模块返回汇总 =====================================
====================================================================================--]]
-- A. 输出htext
--[[
-- AEC
insert_list_to_list(anc_output_htext_tabs, aec_output_htext_table);
--]]
-- B. 输出ctext：无
-- C. 输出bin：无

-- E. 默认值

local anc_default_button_view = cfg:stButton(" ANC配置恢复默认值 ", reset_to_default(anc_config.output.anc_config_items));


local anc_file_info = cfg:addFirmwareFileNoLayout("ANCIF",
		"ANC配置",
		2, -- 文件类型，是个bin文件
		{ anc_config.output.anc_config_output_bin } -- 组的列表
);

local anc_coeff_file_info = cfg:addFirmwareFileNoLayout("ANCIF1",
		"ANC系数",
		2, -- 文件类型，是个bin文件
		{ anc_config.output.anc_coeff_config_output_bin } -- 组的列表
);


anc_config.ui.load_button = cfg:stButton("加载ANC参数", function ()
	local filepath = cfg:utilsGetOpenFilePath("选择ANC参数", "", "ANC 参数 (*.bin)");
	if string.len(filepath) ~= 0 then
		anc_file_info:load(filepath);
	end
end);

anc_config.ui.load_coeff_button = cfg:stButton("加载ANC系数", function ()
	local filepath = cfg:utilsGetOpenFilePath("选择ANC系数", "", "ANC 系数 (*.bin)");
	if string.len(filepath) ~= 0 then
		anc_coeff_file_info:load(filepath);
	end
end);


anc_config.ui.save_button = cfg:stButton("保存ANC参数", function ()
	if open_by_program == "create" then
		anc_file_info:save(bin_out_path .. 'anc_gains.bin');
	else
		local filepath = cfg:utilsGetSaveFilePath("保存ANC参数文件", "anc_gains.bin", "ANC 参数 (*.bin)");
		if string.len(filepath) ~= 0 then
			anc_file_info:save(filepath);
		end
	end
end);

anc_config.ui.save_coeff_button = cfg:stButton("保存ANC系数", function ()
	if open_by_program == "create" then
		anc_coeff_file_info:save(bin_out_path .. 'anc_coeff.bin');
	else
		local filepath = cfg:utilsGetOpenFilePath("保存ANC系数文件", "anc_coeff.bin", "ANC 系数 (*.bin)");
		if string.len(filepath) ~= 0 then
			anc_coeff_file_info:save(filepath);
		end
	end
end);

if open_by_program == "create" then
end

anc_output_vbox_view = cfg:vBox {
    cfg:stHScroll(cfg:vBox{ anc_config.output.anc_config_group_view }),
	cfg:hBox{ anc_config.ui.load_button, anc_config.ui.save_button },
    anc_default_button_view,
};

anc_coeff_output_box_view = cfg:vBox {
	cfg:vBox{ anc_config.output.anc_coeff_config_group_view },
	cfg:hBox{ anc_config.ui.load_coeff_button },
};

anc_file_info:setAttr("layout", anc_output_vbox_view);
anc_file_info:setAttr("binaryFormat", "old");

anc_coeff_file_info:setAttr("layout", anc_coeff_output_box_view);
anc_coeff_file_info:setAttr("binaryFormat", "old");

anc_output_combine_vbox_view = cfg:stTab {
	{ "ANC参数", anc_output_vbox_view },
	{ "ANC系数", anc_coeff_output_box_view },
};

end

