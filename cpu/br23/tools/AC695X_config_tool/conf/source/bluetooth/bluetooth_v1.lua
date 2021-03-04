if enable_moudles["bluetooth"] == false then
    return;
else

--[[===================================================================================
================================= 模块全局变量列表 ====================================
====================================================================================--]]

--[[===================================================================================
================================= 配置子项1: 注释 =====================================
====================================================================================--]]
local MOUDLE_COMMENT_NAME = "//                                  蓝牙配置                                       //"
local comment_begin = cfg:i32("bluetooth注释开始", 0);
local bt_comment_begin_htext = module_output_comment(comment_begin, MOUDLE_COMMENT_NAME);


--[[===================================================================================
=============================== 配置子项2: 蓝牙配置使能 ===============================
====================================================================================--]]
local bluetooth_en_init_val = 1;
if open_by_program == "fw_edit" then bluetooth_en_init_val = 1 end

local bluetooth_en = cfg:enum("蓝牙配置使能开关:", ENABLE_SWITCH, bluetooth_en_init_val);
local bt_en_comment_str = item_comment_context("蓝牙模块使能", ENABLE_SWITCH_TABLE);
-- A. 输出htext
local bt_en_htext = item_output_htext(bluetooth_en, "TCFG_BT_ENABLE", 6, ENABLE_SWITCH_TABLE, bt_en_comment_str, 1);
-- B. 输出ctext：无
-- C. 输出bin：无
-- D. 显示
local bt_module_en_hbox_view = cfg:hBox {
		cfg:stLabel(bluetooth_en.name), 
		cfg:enumView(bluetooth_en),
        cfg:stSpacer();
};

-- E. 默认值, 见汇总

-- F. bindGroup：无

--[[===================================================================================
=============================== 配置子项3: 蓝牙名称配置 ===============================
====================================================================================--]]
-- 若干个蓝牙名字
local bluenames = {}
local bluename_switchs = {}
local bluename_layouts = {}
local bluenames_bin_cfgs = {}
for i = 1, 20 do
	local c = cfg:str("蓝牙名字" .. i, "AC695X_" .. i);
	local on = 0;
	if i == 1 then on = 1 end;
	local onc = cfg:enum("蓝牙名字开关" .. i, ENABLE_SWITCH, on);
	onc:setOSize(1);
	c:setOSize(32); -- 最长 32
	c:addDeps{bluetooth_en, onc};
	c:setShow(on);
	c:setDepChangeHook(function () c:setShow(bluetooth_en.val ~= 0 and onc.val ~= 0) end);
	
	local l = cfg:hBox{ cfg:stLabel("开关："), cfg:enumView(onc),  cfg:stLabel(c.name), cfg:inputMaxLenView(c, 32)};
	
	-- bluenames[#bluenames+1] = c;
	-- bluename_switchs[#bluename_switchs+1] = onc;
	bluename_layouts[#bluename_layouts+1] = l;
	
	bluenames_bin_cfgs[#bluenames_bin_cfgs+1] = onc;
	bluenames_bin_cfgs[#bluenames_bin_cfgs+1] = c;
end

-- A. 输出htext
bluenames_bin_cfgs[2]:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_BT_NAME" .. TAB_TABLE[6] .. "\"" .. bluenames_bin_cfgs[2].val .. "\"" .. TAB_TABLE[1]  .. "//蓝牙名字" .. NLINE_TABLE[1]
		end
	end
);

local bt_name_htext = bluenames_bin_cfgs[2];

-- B. 输出ctext：无

-- C. 输出bin
local bluenames_output_bin = cfg:group("bluetooth names bin",
	MULT_AREA_CFG.bt_name.id,
	1,
	bluenames_bin_cfgs
);

-- D. 显示
local bluenames_group_view = cfg:stGroup("蓝牙名字", cfg:vBox(bluename_layouts));

-- E. 默认值, 无

-- F. bindGroup
cfg:bindStGroup(bluenames_group_view, bluenames_output_bin); -- 绑定 UI 显示和 group


--[[===================================================================================
=============================== 配置子项4: MAC地址配置 ================================
====================================================================================--]]
local bluetooth_mac = cfg:mac("蓝牙MAC地址:", {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF});
depend_item_en_show(bluetooth_en, bluetooth_mac);
-- A. 输出htext
bluetooth_mac:setTextOut(
	function (ty) 
		if (ty == 3) then
            return "#define TCFG_BT_MAC" .. TAB_TABLE[7] .. 
            "{" .. item_val_dec2hex(bluetooth_mac.val[1]) .. ", " .. item_val_dec2hex(bluetooth_mac.val[2]) .. ", "
                .. item_val_dec2hex(bluetooth_mac.val[3]) .. ", " .. item_val_dec2hex(bluetooth_mac.val[4]) .. ", "
                .. item_val_dec2hex(bluetooth_mac.val[5]) .. ", " .. item_val_dec2hex(bluetooth_mac.val[6]) .. "}"
                .. TAB_TABLE[1] .. "//蓝牙MAC地址" .. NLINE_TABLE[1]
		end
	end
);

local bt_mac_htext = bluetooth_mac;

-- B. 输出ctext：无

-- C. 输出bin
local bt_mac_output_bin = cfg:group("bluetooth mac bin",
	MULT_AREA_CFG.bt_mac.id,
	1,
	{
		bluetooth_mac,
	}
);

-- D. 显示
local bt_mac_group_view = cfg:stGroup("",
	cfg:hBox {
        cfg:stLabel("蓝牙MAC地址："),
		cfg:macAddrView(bluetooth_mac),
        cfg:stSpacer(),
	}
);

-- E. 默认值, 见汇总

-- F. bindGroup：无
cfg:bindStGroup(bt_mac_group_view, bt_mac_output_bin); -- 绑定 UI 显示和 group

--[[===================================================================================
============================= 配置子项5: 蓝牙发射功率配置 =============================
====================================================================================--]]
local bluetooth_rf_power = cfg:i32("蓝牙发射功率：", 10);
depend_item_en_show(bluetooth_en, bluetooth_rf_power);
bluetooth_rf_power:setOSize(1);
-- A. 输出htext
local bt_rf_power_htext = item_output_htext(bluetooth_rf_power, "TCFG_BT_RF_POWER", 5, nil, "蓝牙发射功率", 2);

-- B. 输出ctext：无

-- C. 输出bin：
local bt_rf_power_output_bin = cfg:group("rf power bin",
	BIN_ONLY_CFG["BT_CFG"].rf_power.id,
	1,
	{
		bluetooth_rf_power,
	}
);

-- D. 显示
local bt_rf_power_group_view = cfg:stGroup("",
	cfg:hBox {
			cfg:stLabel(bluetooth_rf_power.name),
			cfg:ispinView(bluetooth_rf_power, 0, 10, 1),
            cfg:stLabel("(设置范围: 0 ~ 10)");
            cfg:stSpacer(),
	});

-- E. 默认值, 见汇总

-- F. bindGroup：无
cfg:bindStGroup(bt_rf_power_group_view, bt_rf_power_output_bin);

--[[===================================================================================
================================= 配置子项6: AEC配置 ==================================
====================================================================================--]]
-- aec gloal
local aec_global = {};
-- item_htext
aec_global.aec_start_comment_str = module_comment_context("AEC参数");

aec_global.reference_book_view = cfg:stButton("JL通话调试手册.pdf",
    function()
        local ret = cfg:utilsShellOpenFile(aec_reference_book_path);
        if (ret == false) then
            if cfg.lang == "zh" then
		        cfg:msgBox("info", aec_reference_book_path .. "文件不存在");
            else
		        cfg:msgBox("info", aec_reference_book_path .. " file not exist.");
            end
        end
    end);

-- 1) mic_again
local mic_analog_gain = {};
mic_analog_gain.cfg = cfg:i32("MIC_AGAIN：", 3);

depend_item_en_show(bluetooth_en, mic_analog_gain.cfg);
mic_analog_gain.cfg:setOSize(1);
-- item_htext
mic_analog_gain.comment_str = "设置范围：0 ~ 14, 默认值：3";
mic_analog_gain.htext = item_output_htext(mic_analog_gain.cfg, "TCFG_AEC_MIC_ANALOG_GAIN", 3, nil, mic_analog_gain.comment_str, 1);
-- item_view
mic_analog_gain.hbox_view = cfg:hBox {
    cfg:stLabel(mic_analog_gain.cfg.name .. TAB_TABLE[1]),
    cfg:ispinView(mic_analog_gain.cfg, 0, 14, 1),
    cfg:stLabel("(MIC增益，设置范围: 0 ~ 14，默认值：3)"),
    cfg:stSpacer(),
};

-- 2) dac_again
local dac_analog_gain = {};
dac_analog_gain.cfg = cfg:i32("DAC_AGAIN：", 22);
global_cfgs["DAC_AGAIN"] = dac_analog_gain.cfg;
depend_item_en_show(bluetooth_en, dac_analog_gain.cfg);
dac_analog_gain.cfg:setOSize(1);
-- item_htext
dac_analog_gain.comment_str = "设置范围：0 ~ 31, 默认值：22";
dac_analog_gain.htext = item_output_htext(dac_analog_gain.cfg, "TCFG_AEC_DAC_ANALOG_GAIN", 3, nil, dac_analog_gain.comment_str, 1);
-- item_view
dac_analog_gain.hbox_view = cfg:hBox {
    cfg:stLabel(dac_analog_gain.cfg.name .. TAB_TABLE[1]),
    cfg:ispinView(dac_analog_gain.cfg, 0, 31, 1),
    cfg:stLabel("(DAC 增益，设置范围: 0 ~ 31，默认值：22)");
    cfg:stSpacer(),
};

-- 3) aec_mode
local aec_mode_sel_table = {
        [0] = " disable",
        [1] = " reduce",
        [2] = " advance",
};

local AEC_MODE_SEL_TABLE = cfg:enumMap("aec_mode_sel_table", aec_mode_sel_table);
local aec_mode = {};
aec_mode.cfg = cfg:enum("AEC_MODE: ", AEC_MODE_SEL_TABLE, 2);
depend_item_en_show(bluetooth_en, aec_mode.cfg);
aec_mode.cfg:setOSize(1);
-- item_htext
aec_mode.comment_str = item_comment_context("AEC_MODE", aec_mode_sel_table) .. "默认值：2";
aec_mode.htext = item_output_htext(aec_mode.cfg, "TCFG_AEC_MODE", 6, nil, aec_mode.comment_str, 1);
-- item_view
aec_mode.hbox_view = cfg:hBox {
    cfg:stLabel(aec_mode.cfg.name .. TAB_TABLE[1]),
    cfg:enumView(aec_mode.cfg),
    cfg:stLabel("(AEC 模式，默认值：advance)"),
    cfg:stSpacer(),
};

-- 4) ul_eq_en
local ul_eq_en_sel_table = {
        [0] = " disable",
        [1] = " enable",
};
local UL_EQ_EN_SEL_TABLE = cfg:enumMap("ul_eq_en_sel_table", ul_eq_en_sel_table);

local ul_eq_en = {};
ul_eq_en.cfg = cfg:enum("UL_EQ_EN: ", UL_EQ_EN_SEL_TABLE, 1);
depend_item_en_show(bluetooth_en, ul_eq_en.cfg);
ul_eq_en.cfg:setOSize(1);
-- item_htext
ul_eq_en.comment_str = item_comment_context("UL_EQ_EN", ul_eq_en_sel_table) .. "默认值：1";
ul_eq_en.htext = item_output_htext(ul_eq_en.cfg, "TCFG_AEC_UL_EQ_EN", 5, nil, ul_eq_en.comment_str, 2);
-- item_view
ul_eq_en.hbox_view = cfg:hBox {
    cfg:stLabel(ul_eq_en.cfg.name .. TAB_TABLE[1]),
    cfg:enumView(ul_eq_en.cfg),
    cfg:stLabel("(上行 EQ 使能，默认值：enable)"),
    cfg:stSpacer(),
};


-- 5) fade_gain
local fade_gain = {ndt_fade_in = {}, ndt_fade_out = {}, dt_fade_in = {}, dt_fade_out = {}};
-- 5-1) ndt_fade_in
fade_gain.ndt_fade_in.cfg = cfg:dbf("NDT_FADE_IN：", 1.3);
depend_item_en_show(bluetooth_en, fade_gain.ndt_fade_in.cfg);
fade_gain.ndt_fade_in.cfg:setOSize(4);
-- item_htext
fade_gain.ndt_fade_in.comment_str = "设置范围：0.1 ~ 5 dB, float类型(4Bytes), 默认值：1.3";
fade_gain.ndt_fade_in.htext = item_output_htext(fade_gain.ndt_fade_in.cfg, "TCFG_NDT_FADE_IN", 4, nil, fade_gain.ndt_fade_in.comment_str, 1);
-- item_view
fade_gain.ndt_fade_in.hbox_view = cfg:hBox {
    cfg:stLabel(fade_gain.ndt_fade_in.cfg.name .. TAB_TABLE[1]),
    cfg:dspinView(fade_gain.ndt_fade_in.cfg, 0.1, 5.0, 0.1, 1),
    cfg:stLabel("dB (单端讲话淡入步进，设置范围: 0.1 ~ 5 dB，默认值：1.3 dB)"),
    cfg:stSpacer(),
};

-- 5-2) ndt_fade_out
fade_gain.ndt_fade_out.cfg = cfg:dbf("NDT_FADE_OUT：", 0.7);
depend_item_en_show(bluetooth_en, fade_gain.ndt_fade_out.cfg);
fade_gain.ndt_fade_out.cfg:setOSize(4);
-- item_htext
fade_gain.ndt_fade_out.comment_str = "设置范围：0.1 ~ 5 dB, float类型(4Bytes), 默认值：0.7";
fade_gain.ndt_fade_out.htext = item_output_htext(fade_gain.ndt_fade_out.cfg, "TCFG_NDT_FADE_OUT", 4, nil, fade_gain.ndt_fade_out.comment_str, 1);
-- item_view
fade_gain.ndt_fade_out.hbox_view = cfg:hBox {
    cfg:stLabel(fade_gain.ndt_fade_out.cfg.name .. TAB_TABLE[1]),
    cfg:dspinView(fade_gain.ndt_fade_out.cfg, 0.1, 5.0, 0.1, 1),
    cfg:stLabel("dB (单端讲话淡出步进，设置范围: 0.1 ~ 5 dB，默认值：0.7 dB)"),
    cfg:stSpacer(),
};

-- 5-3) dt_fade_in
fade_gain.dt_fade_in.cfg = cfg:dbf("DT_FADE_IN：", 1.3);
depend_item_en_show(bluetooth_en, fade_gain.dt_fade_in.cfg);
fade_gain.dt_fade_in.cfg:setOSize(4);
-- item_htext
fade_gain.dt_fade_in.comment_str = "设置范围：0.1 ~ 5 dB, float类型(4Bytes), 默认值：1.3";
fade_gain.dt_fade_in.htext = item_output_htext(fade_gain.dt_fade_in.cfg, "TCFG_DT_FADE_IN", 4, nil, fade_gain.dt_fade_in.comment_str, 1);
-- item_view
fade_gain.dt_fade_in.hbox_view = cfg:hBox {
    cfg:stLabel(fade_gain.dt_fade_in.cfg.name .. TAB_TABLE[1]),
    cfg:dspinView(fade_gain.dt_fade_in.cfg, 0.1, 5.0, 0.1, 1),
    cfg:stLabel("dB (双端讲话淡入步进，设置范围: 0.1 ~ 5 dB，默认值：1.3 dB)"),
    cfg:stSpacer(),
};

-- 5-4) dt_fade_out
fade_gain.dt_fade_out.cfg = cfg:dbf("DT_FADE_OUT：", 0.7);
depend_item_en_show(bluetooth_en, fade_gain.dt_fade_out.cfg);
fade_gain.dt_fade_out.cfg:setOSize(4);
-- item_htext
fade_gain.dt_fade_out.comment_str = "设置范围：0.1 ~ 5 dB, float类型(4Bytes), 默认值：0.7";
fade_gain.dt_fade_out.htext = item_output_htext(fade_gain.dt_fade_out.cfg, "TCFG_DT_FADE_OUT", 4, nil, fade_gain.dt_fade_out.comment_str, 1);
-- item_view
fade_gain.dt_fade_out.hbox_view = cfg:hBox {
    cfg:stLabel(fade_gain.dt_fade_out.cfg.name .. TAB_TABLE[1]),
    cfg:dspinView(fade_gain.dt_fade_out.cfg, 0.1, 5.0, 0.1, 1),
    cfg:stLabel("dB (双端讲话淡出步进，设置范围: 0.1 ~ 5 dB，默认值：0.7 dB)"),
    cfg:stSpacer(),
};

-- 6) ndt_max_gain
local ndt_max_gain = {};
ndt_max_gain.cfg = cfg:dbf("NDT_MAX_GAIN：", 12.0);
depend_item_en_show(bluetooth_en, ndt_max_gain.cfg);
ndt_max_gain.cfg:setOSize(4);
-- item_htext
ndt_max_gain.comment_str = "设置范围：-60.0 ~ 40.0 dB, 默认值：12.0 dB";
ndt_max_gain.htext = item_output_htext(ndt_max_gain.cfg, "TCFG_AEC_NDT_MAX_GAIN", 4, nil, ndt_max_gain.comment_str, 1);
-- item_view
ndt_max_gain.hbox_view = cfg:hBox {
    cfg:stLabel(ndt_max_gain.cfg.name .. TAB_TABLE[1]),
    cfg:dspinView(ndt_max_gain.cfg, -60.0, 40.0, 0.1, 1),
    cfg:stLabel("(单端讲话放大上限， 设置范围: -60.0 ~ 40.0 dB，默认值：12.0 dB)"),
    cfg:stSpacer(),
};

-- 7) ndt_min_gain
local ndt_min_gain = {};
ndt_min_gain.cfg = cfg:dbf("NDT_MIN_GAIN：", 0.0);
depend_item_en_show(bluetooth_en, ndt_min_gain.cfg);
ndt_min_gain.cfg:setOSize(4);
-- item_htext
ndt_min_gain.comment_str = "设置范围：-60 ~ 40 dB, 默认值：0 dB";
ndt_min_gain.htext = item_output_htext(ndt_min_gain.cfg, "TCFG_AEC_NDT_MIN_GAIN", 4, nil, ndt_min_gain.comment_str, 1);
-- item_view
ndt_min_gain.hbox_view = cfg:hBox {
    cfg:stLabel(ndt_min_gain.cfg.name .. TAB_TABLE[1]),
    cfg:dspinView(ndt_min_gain.cfg, -60.0, 40.0, 0.1, 1),
    cfg:stLabel("(单端讲话放大下限， 设置范围: -60.0 ~ 40.0 dB，默认值：0 dB)"),
    cfg:stSpacer(),
};

-- 8) ndt_speech_thr
local ndt_speech_thr = {};
ndt_speech_thr.cfg = cfg:dbf("NDT_SPEECH_THR：", -50.0);
depend_item_en_show(bluetooth_en, ndt_speech_thr.cfg);
ndt_speech_thr.cfg:setOSize(4);
-- item_htext
ndt_speech_thr.comment_str = "设置范围：-70.0 ~ -40.0 dB, 默认值：-50.0 dB";
ndt_speech_thr.htext = item_output_htext(ndt_speech_thr.cfg, "TCFG_AEC_NDT_SPEECH_THR", 4, nil, ndt_speech_thr.comment_str, 1);
-- item_view
ndt_speech_thr.hbox_view = cfg:hBox {
    cfg:stLabel(ndt_speech_thr.cfg.name),
    cfg:dspinView(ndt_speech_thr.cfg, -70.0, -40.0, 0.1, 1),
    cfg:stLabel("(单端讲话放大阈值， 设置范围: -70.0 ~ -40.0 dB，默认值：-50.0 dB)"),
    cfg:stSpacer(),
};


-- 9) dt_max_gain 
local dt_max_gain = {};
dt_max_gain.cfg = cfg:dbf("DT_MAX_GAIN：", 12.0);
depend_item_en_show(bluetooth_en, dt_max_gain.cfg);
dt_max_gain.cfg:setOSize(4);
-- item_htext
dt_max_gain.comment_str = "设置范围：-60 ~ 40.0 dB, 默认值：12.0 dB";
dt_max_gain.htext = item_output_htext(dt_max_gain.cfg, "TCFG_AEC_DT_MAX_GAIN", 4, nil, dt_max_gain.comment_str, 1);
-- item_view
dt_max_gain.hbox_view = cfg:hBox {
    cfg:stLabel(dt_max_gain.cfg.name .. TAB_TABLE[1]),
    cfg:dspinView(dt_max_gain.cfg, -60.0, 40.0, 0.1, 1),
    cfg:stLabel("(双端讲话放大上限， 设置范围: -60 ~ 40.0 dB，默认值：12.0 dB)"),
    cfg:stSpacer(),
};

-- 10) dt_min_gain
local dt_min_gain = {};
dt_min_gain.cfg = cfg:dbf("DT_MIN_GAIN：", 0.0);
depend_item_en_show(bluetooth_en, dt_min_gain.cfg);
dt_min_gain.cfg:setOSize(4);
-- item_htext
dt_min_gain.comment_str = "设置范围：-60.0 ~ 40.0 dB, 默认值：0 dB";
dt_min_gain.htext = item_output_htext(dt_min_gain.cfg, "TCFG_AEC_DT_MIN_GAIN", 4, nil, dt_min_gain.comment_str, 1);
-- item_view
dt_min_gain.hbox_view = cfg:hBox {
    cfg:stLabel(dt_min_gain.cfg.name .. TAB_TABLE[1]),
    cfg:dspinView(dt_min_gain.cfg, -60.0, 40.0, 0.1, 1),
    cfg:stLabel("(双端讲话放大下限， 设置范围: -60.0 ~ 40.0 dB，默认值：0 dB)"),
    cfg:stSpacer(),
};


-- 11) dt_speech_thr
local dt_speech_thr = {};
dt_speech_thr.cfg = cfg:dbf("DT_SPEECH_THR：", -40.0);
depend_item_en_show(bluetooth_en, dt_speech_thr.cfg);
dt_speech_thr.cfg:setOSize(4);
-- item_htext
dt_speech_thr.comment_str = "设置范围：-70.0 ~ -40.0 dB, 默认值：-40.0 dB";
dt_speech_thr.htext = item_output_htext(dt_speech_thr.cfg, "TCFG_AEC_DT_SPEECH_THR", 4, nil, dt_speech_thr.comment_str, 1);
-- item_view
dt_speech_thr.hbox_view = cfg:hBox {
    cfg:stLabel(dt_speech_thr.cfg.name .. TAB_TABLE[1]),
    cfg:dspinView(dt_speech_thr.cfg, -70.0, -40.0, 0.1, 1),
    cfg:stLabel("(双端讲话放大阈值， 设置范围: -70.0 ~ -40.0 dB，默认值：-40.0 dB)"),
    cfg:stSpacer(),
};

-- 12) echo_present_thr
local echo_present_thr = {};
echo_present_thr.cfg = cfg:dbf("ECHO_PRESENT_THR：", -70.0);
depend_item_en_show(bluetooth_en, echo_present_thr.cfg);
echo_present_thr.cfg:setOSize(4);
-- item_htext
echo_present_thr.comment_str = "设置范围：-70.0 ~ -40.0 dB, 默认值：-70.0 dB";
echo_present_thr.htext = item_output_htext(echo_present_thr.cfg, "TCFG_AEC_ECHO_PRESENT_THR", 4, nil, echo_present_thr.comment_str, 1);
-- item_view
echo_present_thr.hbox_view = cfg:hBox {
    cfg:stLabel(echo_present_thr.cfg.name),
    cfg:dspinView(echo_present_thr.cfg, -70.0, -40.0, 0.1, 1),
    cfg:stLabel("(单端双端讲话阈值， 设置范围: -70.0 ~ -40.0 dB，默认值：-70.0 dB)"),
    cfg:stSpacer(),
};

-- 13) aec_dt_aggress
local aec_dt_aggress = {};
aec_dt_aggress.cfg = cfg:dbf("AEC_DT_AGGRES：", 1.0);
depend_item_en_show(bluetooth_en, aec_dt_aggress.cfg);
aec_dt_aggress.cfg:setOSize(4);
-- item_htext
aec_dt_aggress.comment_str = "设置范围：1.0 ~ 5.0, float类型(4Bytes), 默认值：1.0";
aec_dt_aggress.htext = item_output_htext(aec_dt_aggress.cfg, "TCFG_AEC_AEC_DT_AGGRESS", 4, nil, aec_dt_aggress.comment_str, 1);
-- item_view
aec_dt_aggress.hbox_view = cfg:hBox {
    cfg:stLabel(aec_dt_aggress.cfg.name),
    cfg:dspinView(aec_dt_aggress.cfg, 1.0, 5.0, 0.1, 1),
    cfg:stLabel("(原音回音追踪等级， 设置范围: 1.0 ~ 5.0，默认值：1.0)"),
    cfg:stSpacer(),
};

-- 14) aec_refengthr
local aec_refengthr = {};
aec_refengthr.cfg = cfg:dbf("AEC_REFENGTHR：", -70.0);
depend_item_en_show(bluetooth_en, aec_refengthr.cfg);
aec_refengthr.cfg:setOSize(4);
-- item_htext
aec_refengthr.comment_str = "设置范围：-90.0 ~ -60.0 dB, float类型(4Bytes), 默认值：-70.0";
aec_refengthr.htext = item_output_htext(aec_refengthr.cfg, "TCFG_AEC_REFENGTHR", 4, nil, aec_refengthr.comment_str, 1);
-- item_view
aec_refengthr.hbox_view = cfg:hBox {
    cfg:stLabel(aec_refengthr.cfg.name .. TAB_TABLE[1]),
    cfg:dspinView(aec_refengthr.cfg, -90.0, -60.0, 0.1, 1),
    cfg:stLabel("(进入回音消除参考值， 设置范围: -90.0 ~ -60.0 dB，默认值：-70.0 dB)"),
    cfg:stSpacer(),
};

-- 15) es_aggress_factor
local es_aggress_factor = {};
es_aggress_factor.cfg = cfg:dbf("ES_AGGRESS_FACTOR：", -3.0);
depend_item_en_show(bluetooth_en, es_aggress_factor.cfg);
es_aggress_factor.cfg:setOSize(4);
-- item_htext
es_aggress_factor.comment_str = "设置范围：-1.0 ~ -5.0, float类型(4Bytes), 默认值：-3.0";
es_aggress_factor.htext = item_output_htext(es_aggress_factor.cfg, "TCFG_ES_AGGRESS_FACTOR", 4, nil, es_aggress_factor.comment_str, 1);
-- item_view
es_aggress_factor.hbox_view = cfg:hBox {
    cfg:stLabel(es_aggress_factor.cfg.name),
    cfg:dspinView(es_aggress_factor.cfg, -5.0, -1.0, 0.1, 1),
    cfg:stLabel("(回音前级动态压制,越小越强， 设置范围: -5.0 ~ -1.0，默认值：-3.0)"),
    cfg:stSpacer(),
};

-- 16) es_min_suppress
--      //回音后级动态压制,越大越强,default: 4.f(0 ~ 10)
local es_min_suppress = {};
es_min_suppress.cfg = cfg:dbf("ES_MIN_SUPPRESS：", 4.0);
depend_item_en_show(bluetooth_en, es_min_suppress.cfg);
es_min_suppress.cfg:setOSize(4);
-- item_htext
es_min_suppress.comment_str = "设置范围：0 ~ 10.0, float类型(4Bytes), 默认值：4.0";
es_min_suppress.htext = item_output_htext(es_min_suppress.cfg, "TCFG_ES_MIN_SUPPRESS", 4, nil, es_min_suppress.comment_str, 1);
-- item_view
es_min_suppress.hbox_view = cfg:hBox {
    cfg:stLabel(es_min_suppress.cfg.name),
    cfg:dspinView(es_min_suppress.cfg, 0.0, 10.0, 0.1, 1),
    cfg:stLabel("(回音后级静态压制,越大越强， 设置范围: 0 ~ 10.0，默认值：4.0)"),
    cfg:stSpacer(),
};

-- 17) ans_aggress
--  //噪声前级动态压制,越大越强default: 1.25f(1 ~ 2)
local ans_aggress = {};
ans_aggress.cfg = cfg:dbf("ANS_AGGRESS：", 1.25);
depend_item_en_show(bluetooth_en, ans_aggress.cfg);
ans_aggress.cfg:setOSize(4);
-- item_htext
ans_aggress.comment_str = "设置范围：1 ~ 2, float类型(4Bytes), 默认值：1.25";
ans_aggress.htext = item_output_htext(ans_aggress.cfg, "TCFG_AEC_ANS_AGGRESS", 4, nil, ans_aggress.comment_str, 1);
-- item_view
ans_aggress.hbox_view = cfg:hBox {
    cfg:stLabel(ans_aggress.cfg.name .. TAB_TABLE[1]),
    cfg:dspinView(ans_aggress.cfg, 1.0, 2.0, 0.01, 2),
    cfg:stLabel("(噪声前级动态压制,越大越强， 设置范围: 1 ~ 2.0，默认值：1.25)"),
    cfg:stSpacer(),
};

-- 18) ans_suppress
-- float ans_suppress; //噪声后级动态压制,越小越强default: 0.04f(0 ~ 1)
local ans_suppress = {};
ans_suppress.cfg = cfg:dbf("ANS_SUPPRESS：", 0.09);
depend_item_en_show(bluetooth_en, ans_suppress.cfg);
ans_suppress.cfg:setOSize(4);
-- item_htext
ans_suppress.comment_str = "设置范围：0 ~ 1, float类型(4Bytes), 默认值：0.09";
ans_suppress.htext = item_output_htext(ans_suppress.cfg, "TCFG_AEC_ANS_SUPPRESS", 4, nil, ans_suppress.comment_str, 1);
-- item_view
ans_suppress.hbox_view = cfg:hBox {
    cfg:stLabel(ans_suppress.cfg.name .. TAB_TABLE[1]),
    cfg:dspinView(ans_suppress.cfg, 0.0, 1.0, 0.01, 2),
    cfg:stLabel("(噪声后级静态压制,越小越强， 设置范围: 0 ~ 1.0，默认值：0.09)"),
    cfg:stSpacer(),
};



--========================= AEC输出汇总  ============================
local aec_item_table = {
    -- aec cfg
        mic_analog_gain.cfg, -- 1 Bytes
        dac_analog_gain.cfg, -- 1 Bytes
        aec_mode.cfg,        -- 1 Bytes
        ul_eq_en.cfg,        -- 1 Bytes
    -- AGC 
        fade_gain.ndt_fade_in.cfg,        -- 4 Bytes
        fade_gain.ndt_fade_out.cfg,        -- 4 Bytes
        fade_gain.dt_fade_in.cfg,        -- 4 Bytes
        fade_gain.dt_fade_out.cfg,        -- 4 Bytes
        ndt_max_gain.cfg,     -- 4 Bytes
        ndt_min_gain.cfg,     -- 4 Bytes
        ndt_speech_thr.cfg,   -- 4 Bytes
        dt_max_gain.cfg,      -- 4 Bytes
        dt_min_gain.cfg,      -- 4 Bytes
        dt_speech_thr.cfg,      -- 4 Bytes
        echo_present_thr.cfg,   -- 4 Bytes
    -- AEC
        aec_dt_aggress.cfg,   -- 4 Bytes
        aec_refengthr.cfg,    -- 4 Bytes
    -- ES
        es_aggress_factor.cfg,  -- 4 Bytes
        es_min_suppress.cfg,    -- 4 Bytes
    -- ANS
        ans_aggress.cfg,    -- 4 Bytes
        ans_suppress.cfg,   -- 4 Bytes
};

local aec_output_view_table = {
        aec_global.reference_book_view,
    -- aec cfg
        mic_analog_gain.hbox_view,
        dac_analog_gain.hbox_view,
        aec_mode.hbox_view,
        ul_eq_en.hbox_view,
    -- AGC 
        fade_gain.ndt_fade_in.hbox_view,
        fade_gain.ndt_fade_out.hbox_view,
        fade_gain.dt_fade_in.hbox_view,
        fade_gain.dt_fade_out.hbox_view,
        ndt_max_gain.hbox_view,
        ndt_min_gain.hbox_view,
        ndt_speech_thr.hbox_view,
        dt_max_gain.hbox_view,
        dt_min_gain.hbox_view,
        dt_speech_thr.hbox_view,
        echo_present_thr.hbox_view,
    -- AEC
        aec_dt_aggress.hbox_view,
        aec_refengthr.hbox_view,
    -- ES
        es_aggress_factor.hbox_view,
        es_min_suppress.hbox_view,
    -- ANS
        ans_aggress.hbox_view,
        ans_suppress.hbox_view,
};

-- A. 输出htext
local aec_output_htext_table = {
        aec_global.aec_start_comment_str,
-- aec cfg
        mic_analog_gain.htext,
        dac_analog_gain.htext,
        aec_mode.htext,
        ul_eq_en.htext,
    -- AGC 
        fade_gain.ndt_fade_in.htext,
        fade_gain.ndt_fade_out.htext,
        fade_gain.dt_fade_in.htext,
        fade_gain.dt_fade_out.htext,
        ndt_max_gain.htext,
        ndt_min_gain.htext,
        ndt_speech_thr.htext,
        dt_max_gain.htext,
        dt_min_gain.htext,
        dt_speech_thr.htext,
        echo_present_thr.htext,
    -- AEC
        aec_dt_aggress.htext,
        aec_refengthr.htext,
    -- ES
        es_aggress_factor.htext,
        es_min_suppress.htext,
    -- ANS
        ans_aggress.htext,
        ans_suppress.htext,
};

-- B. 输出ctext：无

-- C. 输出bin：无
local aec_output_bin = cfg:group("AEC_OUTPUT_BIN",
	BIN_ONLY_CFG["BT_CFG"].aec.id,
    1,
    aec_item_table
);

-- D. 显示
local aec_group_view = cfg:stGroup("AEC 参数配置",
    cfg:vBox(aec_output_view_table)
);

-- E. 默认值, 见汇总

-- F. bindGroup
cfg:bindStGroup(aec_group_view, aec_output_bin);

--[[===================================================================================
=============================== 配置子项7: MIC类型配置 ================================
====================================================================================--]]
local mic_capless_sel_table = {
    [0] = " 不省电容模式 ",
    [1] = " 省电容模式 ",
};

local MIC_CAPLESS_SEL_TABLE = cfg:enumMap("电容选择", mic_capless_sel_table);

local mic_bias_vol_sel_table = {
    [1] = " 8.5 K",
    [2] = " 8.0 K",
    [3] = " 7.6 K",
    [4] = " 7.0 K",
    [5] = " 6.5 K",
    [6] = " 6.0 K",
    [7] = " 5.6 K",
    [8] = " 5.0 K",
    [9] = " 4.41 K",
    [10] = " 3.91 K",
    [11] = " 3.73 K",
    [12] = " 3.5 K",
    [13] = " 3.05 K",
    [14] = " 2.91 K",
    [15] = " 2.6 K",
    [16] = " 2.4 K",
    [17] = " 2.2 K",
    [18] = " 1.99 K",
    [19] = " 1.55 K",
    [20] = " 1.42 K",
    [21] = " 1.18 K",
};

local MIC_BIAS_VOL_SEL_TABLE = cfg:enumMap("电容选择", mic_bias_vol_sel_table);

local mic_ldo_vol_sel_table = {
    [0] = " 2.3 V",
    [1] = " 2.5 V",
    [2] = " 2.7 V",
    [3] = " 3.0 V",
};

local MIC_LDO_VOL_SEL_TABLE = cfg:enumMap("电容选择", mic_ldo_vol_sel_table);

-- 0) MIC开头注释
-- item_htext
local mic_type_sel_comment_str = module_comment_context("MIC类型配置");

-- 1) 省电容选择
local mic_capless_sel = cfg:enum("MIC 电容方案选择：", MIC_CAPLESS_SEL_TABLE, 1);
depend_item_en_show(bluetooth_en, mic_capless_sel);
mic_capless_sel:setOSize(1);
-- item_htext
local mic_capless_sel_comment_str = item_comment_context("MIC电容方案选择", mic_capless_sel_table);
local mic_capless_sel_htext = item_output_htext(mic_capless_sel, "TCFG_MIC_CAPLESS", 5, nil, mic_capless_sel_comment_str, 1);
-- item_view
local mic_capless_sel_hbox_view = cfg:hBox {
	cfg:stLabel(mic_capless_sel.name),
    cfg:enumView(mic_capless_sel),
	cfg:stLabel(" (硅 MIC 要选不省电容模式)"),
    cfg:stSpacer();
};

-- 2) 省电容偏置电压
local mic_bias_vol_sel = cfg:enum("MIC 省电容方案偏置电压选择：", MIC_BIAS_VOL_SEL_TABLE, 16);
mic_bias_vol_sel:setOSize(1);
-- item_htext
local mic_bias_vol_sel_comment_str = item_comment_context("MIC省电容方案偏置电压选择", mic_bias_vol_sel_table);
local mic_bias_vol_sel_htext = item_output_htext(mic_bias_vol_sel, "TCFG_MIC_BIAS_RES", 5, nil, mic_bias_vol_sel_comment_str, 1);
mic_bias_vol_sel:addDeps{bluetooth_en, mic_capless_sel};
mic_bias_vol_sel:setDepChangeHook(function ()
    mic_bias_vol_sel:setShow(((bluetooth_en.val == 1) and (mic_capless_sel.val == 1)) ~= false);
end);
-- item_view
local mic_bias_vol_sel_hbox_view = cfg:hBox {
	cfg:stLabel(mic_bias_vol_sel.name),
    cfg:enumView(mic_bias_vol_sel),
    cfg:stSpacer();
};

-- 3) MIC ldo电压选择
local mic_ldo_vol_sel = cfg:enum("MIC LDO 电压选择：", MIC_LDO_VOL_SEL_TABLE, 2);
depend_item_en_show(bluetooth_en, mic_ldo_vol_sel);
mic_ldo_vol_sel:setOSize(1);
-- item_htext
local mic_ldo_vol_sel_comment_str = item_comment_context("MIC LDO电压选择", mic_ldo_vol_sel_table);
local mic_ldo_vol_sel_htext = item_output_htext(mic_ldo_vol_sel, "TCFG_MIC_LDO_VSEL", 5, nil, mic_ldo_vol_sel_comment_str, 2);
-- item_view
local mic_ldo_vol_sel_hbox_view = cfg:hBox {
	cfg:stLabel(mic_ldo_vol_sel.name),
    cfg:enumView(mic_ldo_vol_sel),
    cfg:stSpacer();
};

--========================= MIC 类型输出汇总  ============================
local mic_type_item_table = {
    mic_capless_sel,
    mic_bias_vol_sel,
    mic_ldo_vol_sel,
};

local mic_type_output_view_table = {
};

-- A. 输出htext
local mic_type_htext_output_table = {
    mic_type_sel_comment_str,
    mic_capless_sel_htext,
    mic_bias_vol_sel_htext,
    mic_ldo_vol_sel_htext,
};

-- B. 输出ctext：无

-- C. 输出bin
local mic_type_output_bin = cfg:group("MIC_TYPE_BIN",
	BIN_ONLY_CFG["HW_CFG"].mic_type.id,
	1,
    mic_type_item_table
);

-- D. 显示
local mic_type_group_view = cfg:stGroup("MIC 类型配置",
    cfg:vBox {
        mic_capless_sel_hbox_view,
        mic_bias_vol_sel_hbox_view,
        mic_ldo_vol_sel_hbox_view,
    }
);

-- E. 默认值, 见汇总

-- F. bindGroup
cfg:bindStGroup(mic_type_group_view, mic_type_output_bin);

--[[===================================================================================
=============================== 配置子项8: 对耳配对码 =================================
====================================================================================--]]
local tws_pair_code = cfg:i32("对耳配对码(2字节)", 0xFFFF);
depend_item_en_show(bluetooth_en, tws_pair_code);
tws_pair_code:setOSize(2);
-- 约束
tws_pair_code:addConstraint(
    function ()
        local warn_str;
        if cfg.lang == "zh" then
            warn_str = "请输入 2 Bytes 对耳配对码";
        else
            warn_str = "Please input 2 Bytes Pair Code";
        end

        return (tws_pair_code.val >= 0 and tws_pair_code.val <= 0xFFFF)  or warn_str;
    end
);

-- A. 输出htext
local tws_pair_code_comment_str = "对耳配对码(2 Bytes)";
local tws_pair_code_htext = item_output_htext_hex(tws_pair_code, "TCFG_TWS_PAIR_CODE", 5, nil, tws_pair_code_comment_str, 2);

-- B. 输出ctext：无

-- C. 输出bin：无
local tws_pair_code_output_bin = cfg:group("TWS_PAIR_CODE_CFG",
	BIN_ONLY_CFG["BT_CFG"].tws_pair_code.id,
	1,
	{
	    tws_pair_code,
	}
);

-- D. 显示
local tws_pair_code_group_view = cfg:stGroup("",
    cfg:hBox {
        cfg:stLabel("对耳配对码配置：0x"),
        cfg:hexInputView(tws_pair_code),
        cfg:stLabel("(2字节)"),
        cfg:stSpacer(),
    }
);

-- E. 默认值, 见汇总

-- F. bindGroup：无
cfg:bindStGroup(tws_pair_code_group_view, tws_pair_code_output_bin);


--[[===================================================================================
=============================== 配置子项8: 按键开机配置 ===============================
====================================================================================--]]
local need_key_on_table = {
    [0] = " 不需要按按键开机",
    [1] = " 需要按按键开机",
};

local NEED_KEY_ON_TABLE = cfg:enumMap("是否需要按键开机列表", need_key_on_table);

local power_on_need_key_on = cfg:enum("是否需要按键开机配置（只输出宏）：", NEED_KEY_ON_TABLE, 1);
depend_item_en_show(bluetooth_en, power_on_need_key_on);
-- A. 输出htext
local power_on_need_key_on_comment_str = item_comment_context("是否需要按键开机配置", need_key_on_table);
local power_on_need_key_on_htext = item_output_htext(power_on_need_key_on, "TCFG_POWER_ON_KEY_ENABLE", 3, nil, power_on_need_key_on_comment_str, 2);

-- B. 输出ctext：无
-- C. 输出bin：无

-- D. 显示
local power_on_need_key_on_group_view = cfg:stGroup("",
    cfg:hBox {
        cfg:stLabel(power_on_need_key_on.name),
        cfg:enumView(power_on_need_key_on),
        cfg:stSpacer(),
    }
);

-- E. 默认值, 见汇总

-- F. bindGroup：无

--[[===================================================================================
=============================== 配置子项9: 蓝牙特性配置 ===============================
====================================================================================--]]
--蓝牙类配置, 1t2和tws是互斥的
local blue_1t2_en = cfg:enum("一拖二使能开关: ", ENABLE_SWITCH, 1);
depend_item_en_show(bluetooth_en, blue_1t2_en);
-- item_text
local blue_1t2_en_comment_str = item_comment_context("一拖二使能开关", FUNCTION_SWITCH_TABLE) .. "与TWS功能 TCFG_TWS_ENABLE 互斥";
local blue_1t2_en_htext = item_output_htext(blue_1t2_en, "TCFG_BD_NUM_ENABLE", 5, FUNCTION_SWITCH_TABLE, blue_1t2_en_comment_str, 1);
-- item_view
local blue_1t2_en_hbox_view = cfg:hBox {
    cfg:stLabel(blue_1t2_en.name),
    cfg:enumView(blue_1t2_en),
    cfg:stSpacer(),
};

local blue_1t2_stop_page_scan_time = cfg:i32("自动取消可连接时间: ", 20);
-- item_text
local blue_1t2_stop_page_scan_time_comment_str = "自动取消可连接时间, 单位：秒";
local blue_1t2_stop_page_scan_time_htext = item_output_htext(blue_1t2_stop_page_scan_time, "TCFG_AUTO_STOP_PAGE_SCAN_TIME", 2, nil, blue_1t2_stop_page_scan_time_comment_str, 1);
blue_1t2_stop_page_scan_time:addDeps{bluetooth_en, blue_1t2_en};
blue_1t2_stop_page_scan_time:setDepChangeHook(function ()
    blue_1t2_stop_page_scan_time:setShow(((bluetooth_en.val == 1) and (blue_1t2_en.val == 1)) ~= false);
end);

local blue_1t2_stop_page_scan_time_hbox_view = cfg:hBox {
    cfg:stLabel(blue_1t2_stop_page_scan_time.name),
    cfg:ispinView(blue_1t2_stop_page_scan_time, 0, 99999, 1),
    cfg:stLabel("单位: 秒"),
    cfg:stSpacer(),
};

-- item_view
local blue_1t2_group_view = cfg:stGroup("一拖二参数配置",
    cfg:vBox {
        blue_1t2_en_hbox_view,
        blue_1t2_stop_page_scan_time_hbox_view,
    }
);

--蓝牙类配置, 1t2和tws是互斥的
local blue_tws_en = cfg:enum("对耳使能开关: ", ENABLE_SWITCH, 0);
depend_item_en_show(bluetooth_en, blue_tws_en);
-- item_text
local blue_tws_en_comment_str = item_comment_context("对耳使能开关", FUNCTION_SWITCH_TABLE) .. "与一拖二功能 TCFG_BD_NUM_ENABLE 互斥";
local blue_tws_en_htext = item_output_htext(blue_tws_en, "TCFG_TWS_ENABLE", 6, FUNCTION_SWITCH_TABLE, blue_tws_en_comment_str, 1);
-- item_view
local blue_tws_en_hbox_view = cfg:hBox {
    cfg:stLabel(blue_tws_en.name),
    cfg:enumView(blue_tws_en),
    cfg:stSpacer(),
};


local blue_tws_pair_timeout = cfg:i32("对耳配对超时: ", 30);
local blue_tws_pair_timeout_conmment_str = "对耳配对超时, 单位: 秒";
-- item_text
local blue_tws_pair_timeout_htext = item_output_htext(blue_tws_pair_timeout, "TCFG_TWS_PAIR_TIMEOUT", 4, nil, blue_tws_pair_timeout_conmment_str, 1);
blue_tws_pair_timeout:addDeps{bluetooth_en, blue_tws_en};
blue_tws_pair_timeout:setDepChangeHook(function ()
    blue_tws_pair_timeout:setShow(((bluetooth_en.val == 1) and (blue_tws_en.val == 1)) ~= false);
end);
local blue_tws_pair_timeout_hbox_view = cfg:hBox{
    cfg:stLabel(blue_tws_pair_timeout.name),
    cfg:ispinView(blue_tws_pair_timeout, 0, 99999, 1),
    cfg:stLabel("单位: 秒"),
    cfg:stSpacer(),
};

-- item_view
local blue_tws_group_view = cfg:stGroup("对耳参数配置",
    cfg:vBox {
        blue_tws_en_hbox_view,
        blue_tws_pair_timeout_hbox_view,
    }
);

cfg:addGlobalConstraint(function()
    return not(blue_tws_en.val == 1 and blue_1t2_en.val == 1) or "对耳和一拖二不能同时使能";
end);

--蓝牙类配置: BLE使能
local blue_ble_en = cfg:enum("BLE功能使能: ", ENABLE_SWITCH, 0);
depend_item_en_show(bluetooth_en, blue_ble_en);
-- item_text
local blue_ble_en_comment_str = item_comment_context("BLE功能使能", FUNCTION_SWITCH_TABLE);
local blue_ble_en_htext = item_output_htext(blue_ble_en, "TCFG_BLE_ENABLE", 6, FUNCTION_SWITCH_TABLE, blue_ble_en_comment_str, 1);
-- item_view
local blue_ble_en_group_view = cfg:stGroup("",
    cfg:hBox {
        cfg:stLabel(blue_ble_en.name),
        cfg:enumView(blue_ble_en),
        cfg:stSpacer(),
    }
);

--蓝牙类配置: 来电报号使能, 来电报号和播手机自己提示音互斥
local phone_in_play_num_en = cfg:enum("来电报号使能: ", ENABLE_SWITCH, 1);
depend_item_en_show(bluetooth_en, phone_in_play_num_en);
-- item_htext
local phone_in_play_num_en_comment_str = item_comment_context("来电报号使能", FUNCTION_SWITCH_TABLE);
local phone_in_play_num_en_htext = item_output_htext(phone_in_play_num_en, "TCFG_BT_PHONE_NUMBER_ENABLE", 3, FUNCTION_SWITCH_TABLE, phone_in_play_num_en_comment_str, 1);
-- item_view
local phone_in_play_num_en_hbox_view = cfg:hBox {
    cfg:stLabel(phone_in_play_num_en.name),
    cfg:enumView(phone_in_play_num_en),
    cfg:stSpacer(),
};

--蓝牙类配置: 播放手机自带来电提示音使能, 来电报号和播手机自己提示音互斥
local phone_in_play_ring_tone_en = cfg:enum("播放手机自带来电提示音使能: ", ENABLE_SWITCH, 0);
depend_item_en_show(bluetooth_en, phone_in_play_ring_tone_en);
-- item_htext
local phone_in_play_ring_tone_en_comment_str = item_comment_context("播放手机自带来电提示音使能", FUNCTION_SWITCH_TABLE);
local phone_in_play_ring_tone_en_htext = item_output_htext(phone_in_play_ring_tone_en, "TCFG_BT_INBAND_RINGTONE_ENABLE", 2, FUNCTION_SWITCH_TABLE, phone_in_play_ring_tone_en_comment_str, 1);
local phone_in_play_ring_tone_en_hbox_view = cfg:hBox {
    cfg:stLabel(phone_in_play_ring_tone_en.name),
    cfg:enumView(phone_in_play_ring_tone_en),
    cfg:stSpacer(),
};

-- item_view
local phone_in_play_group_view = cfg:stGroup("",
    cfg:vBox {
        phone_in_play_num_en_hbox_view,
        phone_in_play_ring_tone_en_hbox_view,
    }
);

-- 约束
cfg:addGlobalConstraint(function()
    return not(phone_in_play_num_en.val == 1 and phone_in_play_ring_tone_en.val == 1) or "来电报号和播手机自带提示音不能同时使能";
end);

local sys_auto_shut_down_time = cfg:i32("没有连接自动关机时间配置: ", 3);
sys_auto_shut_down_time:setOSize(1);
depend_item_en_show(bluetooth_en, sys_auto_shut_down_time);
-- item_htext
local sys_auto_shut_down_time_comment_str = "没有连接自动关机时间配置, 单位：分钟";
local sys_auto_shut_down_time_htext = item_output_htext(sys_auto_shut_down_time, "TCFG_SYS_AUTO_SHUT_DOWN_TIME", 2, nil, sys_auto_shut_down_time_comment_str, 1);
-- 输出bin
local sys_auto_shut_down_time_output_bin = cfg:group("auto_shut_down",
	BIN_ONLY_CFG["BT_CFG"].auto_shut_down_time.id,
	1,
	{
		sys_auto_shut_down_time,
	}
);

-- item_view
local sys_auto_shut_time_group_view = cfg:stGroup("",
    cfg:hBox {
        cfg:stLabel(sys_auto_shut_down_time.name),
        cfg:ispinView(sys_auto_shut_down_time, 0, 99999, 1),
        cfg:stLabel("单位: 分钟(为0不打开)"),
        cfg:stSpacer(),
    }
);

-- F. bindGroup：
cfg:bindStGroup(sys_auto_shut_time_group_view, sys_auto_shut_down_time_output_bin);
--========================= 蓝牙 Feature 输出汇总  ============================
local bt_feature_item_table = {
    blue_1t2_en,
    blue_1t2_stop_page_scan_time,
    blue_tws_en,
    blue_tws_pair_timeout,
    blue_ble_en,
    phone_in_play_num_en,
    phone_in_play_ring_tone_en,
    sys_auto_shut_down_time,
};

-- A. 输出htext
local bt_feature_output_htext_table = {
    blue_1t2_en_htext,
    blue_1t2_stop_page_scan_time_htext,
    blue_tws_en_htext,
    blue_tws_pair_timeout_htext,
    blue_ble_en_htext,
    phone_in_play_num_en_htext,
    phone_in_play_ring_tone_en_htext,
    sys_auto_shut_down_time_htext,
};
-- B. 输出ctext：无
-- C. 输出bin：无

-- D. 显示
local bt_feature_group_view = cfg:stGroup("",
    cfg:vBox {
        blue_1t2_group_view,
        blue_tws_group_view,
        blue_ble_en_group_view,
        phone_in_play_group_view,
        --sys_auto_shut_time_group_view,
    }
);

-- E. 默认值, 见汇总

-- F. bindGroup：无

--[[===================================================================================
=============================== 配置子项10: LRC参数配置 ================================
====================================================================================--]]
local lrc_change_mode_table = {
    [0] = " disable ",
    [1] = " enable ",
};

local LRC_CHANGE_MODE_TABLE = cfg:enumMap("LRC切换使能", lrc_change_mode_table);

-- item_htext
local lrc_start_comment_str = module_comment_context("LRC参数配置");

-- 1) lrc_ws_inc
local lrc_ws_inc = cfg:i32("lrc_ws_inc", 480);
depend_item_en_show(bluetooth_en, lrc_ws_inc);
lrc_ws_inc:setOSize(2);
-- item_htext
local lrc_ws_inc_comment_str = "设置范围：480 ~ 600, 默认值：480";
local lrc_ws_inc_htext = item_output_htext(lrc_ws_inc, "TCFG_LRC_WS_INC", 4, nil, lrc_ws_inc_comment_str, 1);
-- item_view
local lrc_ws_inc_hbox_view = cfg:hBox {
    cfg:stLabel(lrc_ws_inc.name .. "：" .. TAB_TABLE[1]),
    cfg:ispinView(lrc_ws_inc, 480, 600, 1),
    cfg:stLabel("（lrc窗口步进值，单位：us，设置范围: 480 ~ 600, 默认值480）");
    cfg:stSpacer(),
};

-- 2) lrc_ws_init
local lrc_ws_init = cfg:i32("lrc_ws_init", 400);
depend_item_en_show(bluetooth_en, lrc_ws_init);
lrc_ws_init:setOSize(2);
-- item_htext
local lrc_ws_init_comment_str = "设置范围：400 ~ 600, 默认值：480";
local lrc_ws_init_htext = item_output_htext(lrc_ws_init, "TCFG_LRC_WS_INIT", 4, nil, lrc_ws_init_comment_str, 1);
-- item_view
local lrc_ws_init_hbox_view = cfg:hBox {
    cfg:stLabel(lrc_ws_init.name .. "：" .. TAB_TABLE[1]),
    cfg:ispinView(lrc_ws_init, 400, 600, 1),
    cfg:stLabel("（lrc窗口初始值，单位：us，设置范围: 400 ~ 600, 默认值400）");
    cfg:stSpacer(),
};

-- 3) btosc_ws_inc
local btosc_ws_inc = cfg:i32("btosc_ws_inc", 480);
depend_item_en_show(bluetooth_en, btosc_ws_inc);
btosc_ws_inc:setOSize(2);
-- item_htext
local btosc_ws_inc_comment_str = "设置范围：480 ~ 600, 默认值：480";
local btosc_ws_inc_htext = item_output_htext(btosc_ws_inc, "TCFG_BTOSC_WS_INC", 4, nil, btosc_ws_inc_comment_str, 1);
-- item_view
local btosc_ws_inc_hbox_view = cfg:hBox {
    cfg:stLabel(btosc_ws_inc.name .. "：" .. TAB_TABLE[1]),
    cfg:ispinView(btosc_ws_inc, 480, 600, 1),
    cfg:stLabel("（btosc窗口步进值，单位：us，设置范围: 480 ~ 600, 默认值480）");
    cfg:stSpacer(),
};

-- 4) btosc_ws_init
local btosc_ws_init = cfg:i32("btosc_ws_init", 140);
depend_item_en_show(bluetooth_en, btosc_ws_init);
btosc_ws_init:setOSize(2);
-- item_htext
local btosc_ws_init_comment_str = "设置范围：140 ~ 480, 默认值：140";
local btosc_ws_init_htext = item_output_htext(btosc_ws_init, "TCFG_BTOSC_WS_INIT", 4, nil, btosc_ws_init_comment_str, 1);
-- item_view
local btosc_ws_init_hbox_view = cfg:hBox {
    cfg:stLabel(btosc_ws_init.name .. "：" .. TAB_TABLE[1]),
    cfg:ispinView(btosc_ws_init, 140, 480, 1),
    cfg:stLabel("（btosc窗口初始值，单位：us，设置范围: 140 ~ 480, 默认值140）");
    cfg:stSpacer(),
};

-- 5) lrc切换使能
local lrc_change_mode = cfg:enum("lrc_change_mode", LRC_CHANGE_MODE_TABLE, 1);
depend_item_en_show(bluetooth_en, lrc_change_mode);
lrc_change_mode:setOSize(1);
-- item_htext
local lrc_change_mode_comment_str = item_comment_context("LRC切换使能", lrc_change_mode_table);
local lrc_change_mode_htext = item_output_htext(lrc_change_mode, "TCFG_LRC_CHANGE_MODE", 4, nil, lrc_change_mode_comment_str, 1);
-- item_view
local lrc_change_mode_hbox_view = cfg:hBox {
	cfg:stLabel(lrc_change_mode.name .. "："),
    cfg:enumView(lrc_change_mode),
	cfg:stLabel(" （lrc切换使能，建议开启）"),
    cfg:stSpacer();
};

--========================= lrc cfg 参数输出汇总  ============================
local lrc_cfg_item_table = {
    lrc_ws_inc,
    lrc_ws_init,
    btosc_ws_inc,
    btosc_ws_init,
    lrc_change_mode,
};

-- A. 输出htext
local lrc_htext_output_table = {
    lrc_start_comment_str,
    lrc_ws_inc_htext,
    lrc_ws_init_htext,
    btosc_ws_inc_htext,
    btosc_ws_init_htext,
    lrc_change_mode_htext,
};

-- B. 输出ctext：无

-- C. 输出bin
local lrc_cfg_output_bin = cfg:group("LRC_CFG_BIN",
	BIN_ONLY_CFG["BT_CFG"].lrc_cfg.id,
	1,
    lrc_cfg_item_table
);

-- D. 显示
local lrc_cfg_group_view = cfg:stGroup("lrc 参数配置",
    cfg:vBox {
        lrc_ws_inc_hbox_view,
        lrc_ws_init_hbox_view,
        btosc_ws_inc_hbox_view,
        btosc_ws_init_hbox_view,
        lrc_change_mode_hbox_view,
    }
);

-- E. 默认值, 见汇总

-- F. bindGroup
cfg:bindStGroup(lrc_cfg_group_view, lrc_cfg_output_bin);

--[[===================================================================================
=============================== 配置子项11: BLE参数配置 ================================
====================================================================================--]]
local function ble_cfg_depend_show(ble_cfg, dep_cfg)
    ble_cfg:addDeps{bluetooth_en, dep_cfg};
    ble_cfg:setDepChangeHook(function()
        ble_cfg:setShow((bluetooth_en.val ~= 0) and (dep_cfg.val ~= 0));
        end
    );
end
-- 1) ble_en
local ble_en = cfg:enum("BLE 配置使能", ENABLE_SWITCH, 1);
ble_en:setOSize(1);
depend_item_en_show(bluetooth_en, ble_en);
-- A. 输出htext: 无
-- B. 输出ctext：无
-- C. 输出bin:见汇总
-- D. 显示
local ble_en_hbox_view = cfg:hBox {
		cfg:stLabel(ble_en.name .. "："),
		cfg:enumView(ble_en),
        cfg:stLabel("设置为不使能BLE配置将跟随EDR配置");
        cfg:stSpacer();
};
-- E. 默认值: 见汇总
-- F. bindGroup：见汇总

-- 2) ble_name
local ble_name = cfg:str("BLE 蓝牙名字", "AI800X-BLE");
ble_name:setOSize(32);
ble_cfg_depend_show(ble_name, ble_en);
-- A. 输出htext: 无
-- B. 输出ctext：无
-- C. 输出bin:见汇总
-- D. 显示
local ble_name_hbox_view = cfg:hBox {
		cfg:stLabel(ble_name.name .. "："),
		cfg:inputMaxLenView(ble_name, 32),
        cfg:stSpacer();
};
-- E. 默认值: 见汇总
-- F. bindGroup：见汇总

-- 3) ble_rf_power
local ble_rf_power = cfg:i32("BLE 蓝牙发射功率", 5);
ble_rf_power:setOSize(1);
ble_cfg_depend_show(ble_rf_power, ble_en);
-- A. 输出htext: 无
-- B. 输出ctext：无
-- C. 输出bin:见汇总
-- D. 显示
local ble_rf_power_hbox_view = cfg:hBox {
		cfg:stLabel(ble_rf_power.name .. "："),
        cfg:ispinView(ble_rf_power, 0, 10, 1),
        cfg:stLabel("(设置范围：0 ~ 10)");
        cfg:stSpacer();
};
-- E. 默认值: 见汇总
-- F. bindGroup：见汇总

-- 4) ble_addr_en
local ble_addr_en = cfg:enum("BLE MAC 地址配置使能", ENABLE_SWITCH, 1);
ble_addr_en:setOSize(1);
ble_cfg_depend_show(ble_addr_en, ble_en);
-- A. 输出htext: 无
-- B. 输出ctext：无
-- C. 输出bin:见汇总
-- D. 显示
local ble_addr_en_hbox_view = cfg:hBox {
		cfg:stLabel(ble_addr_en.name .. "："),
		cfg:enumView(ble_addr_en),
        cfg:stLabel("设置为不使能BLE配置将随机生成或者烧写时指定");
        cfg:stSpacer();
};
-- E. 默认值: 见汇总
-- F. bindGroup：见汇总

-- 5) ble_mac_addr
local ble_mac_addr = cfg:mac("BLE 蓝牙MAC地址", {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF});
ble_mac_addr:addDeps{bluetooth_en, ble_en, ble_addr_en};
ble_mac_addr:setDepChangeHook(function()
    ble_mac_addr:setShow((ble_en.val ~= 0) and (ble_addr_en.val ~= 0) and (bluetooth_en.val ~= 0));
    end
);
-- A. 输出htext: 无
-- B. 输出ctext：无
-- C. 输出bin:见汇总
-- D. 显示
local ble_mac_addr_hbox_view = cfg:hBox {
		cfg:stLabel(ble_mac_addr.name .. "："),
		cfg:macAddrView(ble_mac_addr),
        cfg:stSpacer();
};

-- addr 汇总显示
local ble_mac_addr_group_view = cfg:stGroup("",
    cfg:vBox {
        ble_addr_en_hbox_view,
        ble_mac_addr_hbox_view,
    }
);
-- E. 默认值: 见汇总
-- F. bindGroup：见汇总

--============================== ble cfg 参数输出汇总  ================================
local ble_cfg_item_table = {
    ble_en,
    ble_name,
    ble_rf_power,
    ble_addr_en,
    ble_mac_addr,
};

-- A. 输出htext
local ble_htext_output_table = {};

-- B. 输出ctext：无

-- C. 输出bin
local ble_cfg_output_bin = cfg:group("BLE_CFG_BIN",
	BIN_ONLY_CFG["BT_CFG"].ble_cfg.id,
	1,
    ble_cfg_item_table
);

-- D. 显示
local ble_cfg_group_view = cfg:stGroup("BLE 参数配置",
    cfg:vBox {
        ble_en_hbox_view,
        ble_name_hbox_view,
        ble_rf_power_hbox_view,
        ble_mac_addr_group_view,
    }
);

-- E. 默认值, 见汇总

-- F. bindGroup
cfg:bindStGroup(ble_cfg_group_view, ble_cfg_output_bin);

--[[===================================================================================
==================================== 模块返回汇总 =====================================
====================================================================================--]]
-- A. 输出htext
--[[
insert_item_to_list(bt_output_htext_tabs, bt_comment_begin_htext);
insert_item_to_list(bt_output_htext_tabs, bt_en_htext);
insert_item_to_list(bt_output_htext_tabs, bt_name_htext);
insert_item_to_list(bt_output_htext_tabs, bt_mac_htext);
insert_item_to_list(bt_output_htext_tabs, bt_rf_power_htext);
-- AEC
insert_list_to_list(bt_output_htext_tabs, aec_output_htext_table);
-- MIC
insert_list_to_list(bt_output_htext_tabs, mic_type_htext_output_table);
insert_item_to_list(bt_output_htext_tabs, tws_pair_code_htext);
-- key_on
insert_item_to_list(bt_output_htext_tabs, power_on_need_key_on_htext);
-- Feature
insert_list_to_list(bt_output_htext_tabs, bt_feature_output_htext_table);
--]]
-- B. 输出ctext：无
-- C. 输出bin：无
insert_item_to_list(bt_output_bin_tabs, bluenames_output_bin);
insert_item_to_list(bt_output_bin_tabs, bt_mac_output_bin);
insert_item_to_list(bt_output_bin_tabs, bt_rf_power_output_bin);
insert_item_to_list(bt_output_bin_tabs, aec_output_bin);
insert_item_to_list(bt_output_bin_tabs, mic_type_output_bin);
insert_item_to_list(bt_output_bin_tabs, tws_pair_code_output_bin);
insert_item_to_list(bt_output_bin_tabs, sys_auto_shut_down_time_output_bin);
insert_item_to_list(bt_output_bin_tabs, lrc_cfg_output_bin);
if enable_moudles["ble_config"] == true then
insert_item_to_list(bt_output_bin_tabs, ble_cfg_output_bin);
end

-- E. 默认值
local bt_cfg_default_table = {};
for k, v in pairs(bluenames_bin_cfgs) do
	insert_item_to_list(bt_cfg_default_table, v);
end

if open_by_program == "create" then
	--insert_item_to_list(bt_cfg_default_table, bluetooth_en);
    insert_item_to_list(bt_cfg_default_table, power_on_need_key_on);
    insert_list_to_list(bt_cfg_default_table, bt_feature_item_table);
end

insert_item_to_list(bt_cfg_default_table, bluetooth_mac);
insert_item_to_list(bt_cfg_default_table, bluetooth_rf_power);
-- aec
insert_list_to_list(bt_cfg_default_table, aec_item_table);
-- mic
insert_list_to_list(bt_cfg_default_table, mic_type_item_table);

insert_item_to_list(bt_cfg_default_table, tws_pair_code);
-- lrc_cfg
insert_list_to_list(bt_cfg_default_table, lrc_cfg_item_table);

-- ble_cfg
insert_list_to_list(bt_cfg_default_table, ble_cfg_item_table);

local bt_default_button_view = cfg:stButton(" 蓝牙配置恢复默认值 ", reset_to_default(bt_cfg_default_table));

-- D. 显示
local bt_base_group_view = cfg:stGroup("基本配置",
    cfg:vBox {
        bluenames_group_view,
        bt_mac_group_view,
        bt_rf_power_group_view,
        sys_auto_shut_time_group_view,
    }
);

local bt_output_group_view_table = {};

if open_by_program == "create" then
    insert_item_to_list(bt_output_group_view_table, bt_module_en_hbox_view);
end

insert_item_to_list(bt_output_group_view_table, bt_base_group_view);
if enable_moudles["ble_config"] == true then
    insert_item_to_list(bt_output_group_view_table, ble_cfg_group_view);
end
insert_item_to_list(bt_output_group_view_table, tws_pair_code_group_view);
insert_item_to_list(bt_output_group_view_table, aec_group_view);
insert_item_to_list(bt_output_group_view_table, mic_type_group_view);
insert_item_to_list(bt_output_group_view_table, lrc_cfg_group_view);

if open_by_program == "create" then
    --insert_item_to_list(bt_output_group_view_table, power_on_need_key_on_group_view);
    --insert_item_to_list(bt_output_group_view_table, bt_feature_group_view);
end

bt_output_vbox_view = cfg:vBox {
    cfg:stHScroll(cfg:vBox(bt_output_group_view_table)),
    bt_default_button_view,
};

-- F. bindGroup：item单独处理

end

