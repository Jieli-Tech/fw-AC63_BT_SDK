
local MOUDLE_COMMENT_NAME = "//                                 Audio配置                                       //"

local comment_begin = cfg:i32("audio注释开始", 0)
local audio_comment_begin_htext = module_output_comment(comment_begin, MOUDLE_COMMENT_NAME);

--print(MOUDLE_COMMENT_NAME);
---------------------------------[[ audio ]]---------------------------------
local AUDIO_ADC_LDO_SEL = cfg:enumMap("AUDIO_ADC_LDO_SEL", 
                                {
                                    [0] = "LDO_SEL值 = 0" ,
                                    [1] = "LDO_SEL值 = 1", 
                                    [2] = "LDO_SEL值 = 2", 
                                    [3] = "LDO_SEL值 = 3"
                                })

local AUDIO_ADC_LDO_SEL_OUTPUT_TABLE = {
    [0] = "0", 
    [1] = "1", 
	[2] = "2",
	[3] = "3",
}



local AUDIO_ADC_MIC_CNA_SEL = cfg:enumMap("AUDIO_ADC_MIC_CNA_SEL", 
									{
										[1] = "左声道", 
										[2] = "右声道", 
										[3] = "双声道"
									}
								)

local AUDIO_ADC_MIC_CHA_SEL_OUTPUT_TABLE = {
    [1] = "LADC_CH_MIC_L", 
	[2] = "LADC_CH_MIC_R",
	[3] = "(LADC_CH_MIC_L | LADC_CH_MIC_R)",
}

local AUDIO_ADC_LINE_CNA_SEL = cfg:enumMap("AUDIO_ADC_LINE_CNA_SEL", 
									{
										[4]  = "LINE0 左声道", 
										[8]  = "LINE0 右声道",
										[12] = "LINE0 双声道",
										[16] = "LINE1 左声道", 
										[32] = "LINE1 右声道",
										[48] = "LINE1 双声道",
									}
								)

local AUDIO_ADC_LINE_CNA_SEL_OUTPUT_TABLE = {
	[4]  = "LADC_CH_LINE0_L",
    [8]  = "LADC_CH_LINE0_R",
    [12] = "LADC_LINE0_MASK",
    [16] = "LADC_CH_LINE1_L", 
    [32] = "LADC_CH_LINE1_R",
    [48] = "LADC_LINE1_MASK",
}



local AUDIO_DAC_LDO_SEL = cfg:enumMap("AUDIO_DAC_LDO_SEL", 
                                {
                                    [0] = "LDO_ID值 = 0",
                                    [1] = "LDO_ID值 = 1",
                                    [2] = "LDO_ID值 = 2", 
                                    [3] = "LDO_ID值 = 3"
                                })
local AUDIO_DAC_LDO_SEL_OUTPUT_TABLE = {
    [0] = "0", 
    [1] = "1", 
	[2] = "2",
	[3] = "3",
}



local AUDIO_DAC_LDO_VOLT = cfg:enumMap("AUDIO_DAC_LDO_VOLT", 
                                {
                                    [0] = "DACVDD_LDO: 1.20v",
                                    [1] = "DACVDD_LDO: 1.30v",
                                    [2] = "DACVDD_LDO: 2.35v",
                                    [3] = "DACVDD_LDO: 2.50v",
                                    [4] = "DACVDD_LDO: 2.65v",
                                    [5] = "DACVDD_LDO: 2.80v",
                                    [6] = "DACVDD_LDO: 2.95v",
                                    [7] = "DACVDD_LDO: 3.10v",
                                } )

local AUDIO_DAC_LDO_VOLT_OUTPUT_TABLE = {
    [0] = "DACVDD_LDO_1_20V",
    [1] = "DACVDD_LDO_1_30V",
    [2] = "DACVDD_LDO_2_35V",
    [3] = "DACVDD_LDO_2_50V",
    [4] = "DACVDD_LDO_2_65V",
    [5] = "DACVDD_LDO_2_80V",
    [6] = "DACVDD_LDO_2_95V",
    [7] = "DACVDD_LDO_3_10V",
}


--[[
//==========================================================================================//
//                                  配置项: AUDIO_CFG                                       //
//==========================================================================================//
--]]

--[[=============================== 配置子项1-0: audio_adc_en ================================--]]
local audio_adc_en = cfg:enum("AUDIO ADC 配置使能开关:" .. TAB_TABLE[1], ENABLE_SWITCH, 0)
audio_adc_en:setOSize(1)
audio_adc_en:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_AUDIO_ADC_ENABLE" .. TAB_TABLE[4] .. ENABLE_SWITCH_TABLE[audio_adc_en.val] .. NLINE_TABLE[1]
		end
	end
)

-- 子项显示
local audio_adc_en_view = cfg:hBox {
            cfg:stLabel(audio_adc_en.name),
            cfg:enumView(audio_adc_en),
            cfg:stSpacer(),
};


--[[=============================== 配置子项1-1: audio_adc_mic_cha_sel ================================--]]
local audio_adc_mic_cha_sel = cfg:enum("AUDIO ADC MIC通道选择:" .. TAB_TABLE[1], AUDIO_ADC_MIC_CNA_SEL, 2)
audio_adc_mic_cha_sel:setOSize(1)
audio_adc_mic_cha_sel:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_AUDIO_ADC_MIC_CHA" .. TAB_TABLE[4] .. AUDIO_ADC_MIC_CHA_SEL_OUTPUT_TABLE[audio_adc_mic_cha_sel.val] .. NLINE_TABLE[1]
		end
	end
)
audio_adc_mic_cha_sel:addDeps{audio_adc_en}
audio_adc_mic_cha_sel:setDepChangeHook(function ()
    if (audio_adc_en.val == 0) then
        audio_adc_mic_cha_sel:setShow(false);
    else
        audio_adc_mic_cha_sel:setShow(true);
    end
end);

-- 子项显示
local audio_adc_mic_cha_sel_view = cfg:hBox {
            cfg:stLabel(audio_adc_mic_cha_sel.name),
            cfg:enumView(audio_adc_mic_cha_sel),
            cfg:stSpacer(),
};


--[[=============================== 配置子项1-2: audio_adc_line_cha_sel ================================--]]
local audio_adc_line_cha_sel = cfg:enum("AUDIO ADC LINE通道选择:" .. TAB_TABLE[1], AUDIO_ADC_LINE_CNA_SEL, 12)
audio_adc_line_cha_sel:setOSize(1)
audio_adc_line_cha_sel:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_AUDIO_ADC_LINE_CHA" .. TAB_TABLE[4] .. AUDIO_ADC_LINE_CNA_SEL_OUTPUT_TABLE[audio_adc_line_cha_sel.val] .. NLINE_TABLE[1]
		end
	end
)
audio_adc_line_cha_sel:addDeps{audio_adc_en}
audio_adc_line_cha_sel:setDepChangeHook(function ()
    if (audio_adc_en.val == 0) then
        audio_adc_line_cha_sel:setShow(false);
    else
        audio_adc_line_cha_sel:setShow(true);
    end
end);

-- 子项显示
local audio_adc_line_cha_sel_view = cfg:hBox {
            cfg:stLabel(audio_adc_line_cha_sel.name),
            cfg:enumView(audio_adc_line_cha_sel),
            cfg:stSpacer(),
};


--[[=============================== 配置子项1-3: audio_adc_ldo_sel ================================--]]
local audio_adc_ldo_sel = cfg:enum("AUDIO ADC LDO 选择:" .. TAB_TABLE[1], AUDIO_ADC_LDO_SEL, 3)
audio_adc_ldo_sel:setOSize(1)
audio_adc_ldo_sel:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_AUDIO_ADC_LD0_SEL" .. TAB_TABLE[4] .. AUDIO_ADC_LDO_SEL_OUTPUT_TABLE[audio_adc_ldo_sel.val] .. NLINE_TABLE[2]
		end
	end
)
audio_adc_ldo_sel:addDeps{audio_adc_en}
audio_adc_ldo_sel:setDepChangeHook(function ()
    if (audio_adc_en.val == 0) then
        audio_adc_ldo_sel:setShow(false);
    else
        audio_adc_ldo_sel:setShow(true);
    end
end);

-- 子项显示
local audio_adc_ldo_sel_view = cfg:hBox {
            cfg:stLabel(audio_adc_ldo_sel.name),
            cfg:enumView(audio_adc_ldo_sel),
            cfg:stSpacer(),
};


--[[=============================== 配置子项2-0: audio_dac_en ================================--]]
local audio_dac_en = cfg:enum("AUDIO DAC 配置使能开关:" .. TAB_TABLE[1], ENABLE_SWITCH, 0)
audio_dac_en:setOSize(1)
audio_dac_en:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_AUDIO_DAC_ENABLE" .. TAB_TABLE[4] .. ENABLE_SWITCH_TABLE[audio_dac_en.val] .. NLINE_TABLE[1]
		end
	end
)

-- 子项显示
local audio_dac_en_view = cfg:hBox {
            cfg:stLabel(audio_dac_en.name),
            cfg:enumView(audio_dac_en),
            cfg:stSpacer(),
};


--[[=============================== 配置子项2-1: audio_dac_ldo_sel ================================--]]
local audio_dac_ldo_sel = cfg:enum("AUDIO DAC LDO 选择:" .. TAB_TABLE[1], AUDIO_DAC_LDO_SEL, 1)
audio_dac_ldo_sel:setOSize(1)
audio_dac_ldo_sel:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_AUDIO_DAC_LDO_SEL" .. TAB_TABLE[4] .. AUDIO_DAC_LDO_SEL_OUTPUT_TABLE[audio_dac_ldo_sel.val] .. NLINE_TABLE[1]
		end
	end
)
audio_dac_ldo_sel:addDeps{audio_dac_en}
audio_dac_ldo_sel:setDepChangeHook(function ()
    if (audio_dac_en.val == 0) then
        audio_dac_ldo_sel:setShow(false);
    else
        audio_dac_ldo_sel:setShow(true);
    end
end);

-- 子项显示
local audio_dac_ldo_sel_view = cfg:hBox {
            cfg:stLabel(audio_dac_ldo_sel.name),
            cfg:enumView(audio_dac_ldo_sel),
            cfg:stSpacer(),
};


--[[=============================== 配置子项2-2: audio_dac_ldo_volt ================================--]]
local audio_dac_ldo_volt = cfg:enum("AUDIO DAC LDO VOLT选择:" .. TAB_TABLE[1], AUDIO_DAC_LDO_VOLT, 4)
audio_dac_ldo_volt:setOSize(1)
audio_dac_ldo_volt:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_AUDIO_DAC_LDO_VOLT" .. TAB_TABLE[4] .. AUDIO_DAC_LDO_VOLT_OUTPUT_TABLE[audio_dac_ldo_volt.val] .. NLINE_TABLE[1]
		end
	end
)
audio_dac_ldo_volt:addDeps{audio_dac_en}
audio_dac_ldo_volt:setDepChangeHook(function ()
    if (audio_dac_en.val == 0) then
        audio_dac_ldo_volt:setShow(false);
    else
        audio_dac_ldo_volt:setShow(true);
    end
end);

-- 子项显示
local audio_dac_ldo_volt_view = cfg:hBox {
            cfg:stLabel(audio_dac_ldo_volt.name),
            cfg:enumView(audio_dac_ldo_volt),
            cfg:stSpacer(),
};


--[[=============================== 配置子项2-2: audio_dac_pa_port ================================--]]
local audio_dac_pa_port = cfg:enum("ADUIO DAC PA PORT:" .. TAB_TABLE[1], PORTS, 0xFF)
audio_dac_pa_port:setOSize(1)
audio_dac_pa_port:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_AUDIO_DAC_PA_PORT" .. TAB_TABLE[4] .. PORTS_TABLE[audio_dac_pa_port.val] .. NLINE_TABLE[2]
		end
	end
)
audio_dac_pa_port:addDeps{audio_dac_en}
audio_dac_pa_port:setDepChangeHook(function ()
    if (audio_dac_en.val == 0) then
        audio_dac_pa_port:setShow(false);
    else
        audio_dac_pa_port:setShow(true);
    end
end);


-- 子项显示
local audio_dac_pa_port_view = cfg:hBox {
            cfg:stLabel(audio_dac_pa_port.name),
            cfg:enumView(audio_dac_pa_port),
            cfg:stSpacer(),
};



--[[=============================== 配置子项3-1: audio_volume_en ================================--]]
local audio_volume_en = cfg:enum("音量配置使能开关：", ENABLE_SWITCH, 1)
audio_volume_en:setOSize(1)
audio_volume_en:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_AUDIO_VOLUME_ENABLE" .. TAB_TABLE[4] .. ENABLE_SWITCH_TABLE[audio_volume_en.val] .. NLINE_TABLE[1]
		end
	end
)

-- 子项显示
local audio_volume_en_view = cfg:hBox {
            cfg:stLabel(audio_volume_en.name),
            cfg:enumView(audio_volume_en),
            cfg:stLabel(""),
            cfg:stSpacer(),
};



--[[=============================== 配置子项3-2: audio_max_volume ================================--]]
local max_vol = cfg:i32("系统最大音量：", 31);
max_vol:setOSize(1)
max_vol:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_AUDIO_MAX_VOLUME" .. TAB_TABLE[4]  .. max_vol.val .. NLINE_TABLE[1]
		end
	end
)
max_vol:addDeps{audio_volume_en}
max_vol:setDepChangeHook(function ()
	max_vol:setShow(audio_volume_en.val ~= 0);
end);

-- 子项显示
local audio_max_volume_view = cfg:hBox {
            cfg:stLabel(max_vol.name),
            cfg:ispinView(max_vol, 0, 31, 1),
            cfg:stLabel("(设置范围: 0~31)"),
            cfg:stSpacer(),
};

--cfg:bindStGroup(default_vol_bin_grp, default_vol_bin);

--[[=============================== 配置子项3-3: audio_default_volume ================================--]]
local default_vol = cfg:i32("系统默认音量：", 25);
default_vol:setOSize(1)
default_vol:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_AUDIO_DEFAULT_VOL" .. TAB_TABLE[4] .. default_vol.val .. NLINE_TABLE[2]
		end
	end
)

default_vol:addDeps{audio_volume_en}
default_vol:setDepChangeHook(function ()
	default_vol:setShow(audio_volume_en.val ~= 0);
end);

cfg:addGlobalConstraint(
	function ()
        local warn_str;
        if cfg.lang == "zh" then
            warn_str = "系统默认音量不能大于系统最大音量";
        else
            warn_str = "system default volume class should less than max volume class";
        end
		return default_vol.val <= max_vol.val  or warn_str;
	end
);

-- 子项显示
local audio_default_volume_view = cfg:hBox {
            cfg:stLabel(default_vol.name),
            cfg:ispinView(default_vol, 0, 31, 1),
            cfg:stLabel("(设置范围: 0~31)"),
            cfg:stSpacer(),
};

--[[=============================== 配置子项3-4: tone_volume ================================--]]
local tone_vol = cfg:i32("提示音音量：", 25);
tone_vol:setOSize(1);
tone_vol:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_AUDIO_TONE_VOL" .. TAB_TABLE[5] .. tone_vol.val .. NLINE_TABLE[2]
		end
	end
)

tone_vol:addDeps{audio_volume_en}
tone_vol:setDepChangeHook(function ()
	tone_vol:setShow(audio_volume_en.val ~= 0);
end);

cfg:addGlobalConstraint(
	function () 
		return tone_vol.val <= max_vol.val  or "提示音音量不能大于系统最大音量";
	end
);

-- 子项显示
local audio_tone_volume_view = cfg:hBox {
            cfg:stLabel(tone_vol.name),
            cfg:ispinView(tone_vol, 0, 31, 1),
            cfg:stLabel("(设置范围: 0~31, 配置为 0 将跟随系统音量)"),
            cfg:stSpacer(),
};


--[[===================================================================================
==================================== 模块返回汇总 =====================================
====================================================================================--]]
-- A. 输出htext
local audio_output_htext_table = {};
insert_item_to_list(audio_output_htext_table, audio_comment_begin_htext);
--insert_item_to_list(audio_output_htext_table, audio_adc_en);
--[[
insert_item_to_list(audio_output_htext_table, audio_adc_mic_cha_sel);
insert_item_to_list(audio_output_htext_table, audio_adc_line_cha_sel);
insert_item_to_list(audio_output_htext_table, audio_adc_ldo_sel);
insert_item_to_list(audio_output_htext_table, audio_dac_en);
insert_item_to_list(audio_output_htext_table, audio_dac_ldo_sel);
insert_item_to_list(audio_output_htext_table, audio_dac_ldo_volt);
insert_item_to_list(audio_output_htext_table, audio_dac_pa_port);
--]]
insert_item_to_list(audio_output_htext_table, max_vol);
insert_item_to_list(audio_output_htext_table, default_vol);
insert_item_to_list(audio_output_htext_table, tone_vol);

--insert_list_to_list(board_output_text_tabs, audio_output_htext_table);
if enable_moudles["audio"] == false then
    return;
end

-- B. 输出ctext：无

-- C. 输出bin：无
local audio_output_bin_list = {
    audio_volume_en,
    max_vol,
    default_vol,
    tone_vol,
};

local audio_output_bin = cfg:group("AUDIO_CFG_BIN",
    BIN_ONLY_CFG["HW_CFG"].audio.id,
    1,
    audio_output_bin_list
);
insert_item_to_list(board_output_bin_tabs, audio_output_bin);

-- E. 默认值
local audio_cfg_default_table = {};

if open_by_program == "create" then
	insert_item_to_list(audio_cfg_default_table, audio_volume_en);
end

insert_item_to_list(audio_cfg_default_table, default_vol);
insert_item_to_list(audio_cfg_default_table, tone_vol);
insert_item_to_list(audio_cfg_default_table, max_vol);

-- D. 显示
local audio_vol_output_view_table = {}
if open_by_program == "create" then
	insert_item_to_list(audio_vol_output_view_table, audio_volume_en_view);
end

insert_item_to_list(audio_vol_output_view_table, audio_max_volume_view);
insert_item_to_list(audio_vol_output_view_table, audio_default_volume_view);
insert_item_to_list(audio_vol_output_view_table, audio_tone_volume_view);

local audio_volume_group_view = cfg:stGroup("",
            cfg:vBox(audio_vol_output_view_table));

local audio_view_list = cfg:vBox {
        cfg:stHScroll(cfg:vBox {audio_volume_group_view}),
        cfg:stButton(" AUDIO 配置恢复默认值 ", reset_to_default(audio_cfg_default_table));
};

local audio_view = {"音量配置", audio_view_list};

insert_item_to_list(board_view_tabs, audio_view);


-- F. bindGroup：无



