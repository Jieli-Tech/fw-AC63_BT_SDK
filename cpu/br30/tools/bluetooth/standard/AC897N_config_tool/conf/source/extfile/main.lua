-- EQ 参数文件
local eqfile = {};

eqfile.cfg = cfg:path("eq-ext-path", "");
eqfile.view = cfg:pathView(eqfile.cfg, {"bin"});
eqfile.layout = cfg:vBox{ eqfile.view };

if is_open_by_ufw ~= true then
	cfg:addFirmwareFile("eq_cfg_hw.bin",
			"EQ 参数文件",
			51, -- 外部文件
			eqfile.cfg,
			eqfile.layout);
end
