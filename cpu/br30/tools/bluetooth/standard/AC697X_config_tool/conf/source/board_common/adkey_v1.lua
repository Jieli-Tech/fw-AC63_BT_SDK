
local MOUDLE_COMMENT_NAME = "//                                  ADKEY配置                                      //"
local comment_begin = cfg:i32("adkey注释开始", 0)
local adkey_comment_begin_htext = module_output_comment(comment_begin, MOUDLE_COMMENT_NAME);

local adkey_mcro_begin = cfg:i32("adkey宏开始", 0)
adkey_mcro_begin:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#if (TCFG_ADKEY_ENABLE == ENABLE_THIS_MODULE)" .. NLINE_TABLE[2];
		end
	end
)

local adkey_mcro_end = cfg:i32("adkey宏结束", 0)
adkey_mcro_end:setTextOut(
	function (ty) 
		if (ty == 3) then
			return NLINE_TABLE[1] .. "#endif" .. NLINE_TABLE[2];
		end
	end
)

--print(MOUDLE_COMMENT_NAME);
--[[===================================================================================
================================= 模块全局变量列表 ====================================
====================================================================================--]]

local adkey_msg_htext_table = {};
local adkey_vol_htext_table = {};

local ad_channel_port_table = {
	[0xFF] = "NO_CONFIG_PORT", 
	[1] = "IO_PORTA_01",
	[3] = "IO_PORTA_03",
	[4] = "IO_PORTA_04",
	[5] = "IO_PORTA_05",
	[9] = "IO_PORTA_09",
	[10] = "IO_PORTA_10",
	[17] = "IO_PORTB_01",
	[20] = "IO_PORTB_04",
	[22] = "IO_PORTB_06",
	[23] = "IO_PORTB_07",
	[61] = "IO_PORT_DP",
	[62] = "IO_PORT_DM",
	[18] = "IO_PORTB_02",
};


local ad_channel_port = cfg:enumMap("ad_channel_port", ad_channel_port_table);

local ad_channel_name_table = {
	[0xFF] = "NO_CONFIG_PORT",
	[0x00] = "AD_CH_PA1",
	[0x01] = "AD_CH_PA3",
	[0x02] = "AD_CH_PA4",
	[0x03] = "AD_CH_PA5",
	[0x04] = "AD_CH_PA9",
	[0x05] = "AD_CH_PA10",
	[0x06] = "AD_CH_PB1",
	[0x07] = "AD_CH_PB4",
	[0x08] = "AD_CH_PB6",
	[0x09] = "AD_CH_PB7",
	[0x0A] = "AD_CH_DP",
	[0x0B] = "AD_CH_DM",
	[0x0C] = "AD_CH_PB2",
};

local ad_port_to_channel_table = {
    [0xFF] = 0xFF;
    [1] = 0x00,
	[3] = 0x01,
	[4] = 0x02,
	[5] = 0x03,
	[9] = 0x04,
	[10] = 0x05,
	[17] = 0x06,
	[20] = 0x07,
	[22] = 0x08,
	[23] = 0x09,
	[61] = 0x0A,
	[62] = 0x0B,
	[18] = 0x0C,
};


--[[=============================== 配置项列表 ================================--]]
local adkey_item_table = {};
local adkey_cfg_group_view_list = {};
local adkey_output_msg_bin_list = {};
local adkey_output_text_table = {};
--local adkey_output_vol = {};

local adkey_default_items = {};

-- ADKEY使能
adkey_en = cfg:enum("ADKEY 使能开关:", ENABLE_SWITCH, 0)
adkey_en:setOSize(1);
adkey_en:setTextOut(
	function (ty) 
		if (ty == 3) then
            if enable_moudles["adkey"].enable == false then
			    return "#define TCFG_ADKEY_ENABLE" .. TAB_TABLE[5] .. ENABLE_SWITCH_TABLE[0] .. NLINE_TABLE[2]
            end
			return "#define TCFG_ADKEY_ENABLE" .. TAB_TABLE[5] .. ENABLE_SWITCH_TABLE[adkey_en.val] .. NLINE_TABLE[2]
		end
	end
)

local adkey_en_hBox_view = enum_item_hbox_view(adkey_en);


--[[=============================== 配置子项: adkey_num ================================--]]
local adkey_num_max = cfg:i32("ADKEY按键预留最大数量", enable_moudles["adkey"].num);
local adkey_num_max_comment_str = "最大可配置的按键数量";
local adkey_num_max_output_htext = item_output_htext(adkey_num_max, "TCFG_KEY_NUM_MAX", 5, nil, adkey_num_max_comment_str, 2);

local adkey_num = cfg:i32("ADKEY按键数量(1 ~ " .. enable_moudles["adkey"].num .. ")", 0);
adkey_num:setOSize(1);
if open_by_program == "fw_create" then
    depend_item_en_show(adkey_en, adkey_num);
end
local adkey_num_comment_str = "配置有效的按键数量";
local adkey_num_output_htext = item_output_htext(adkey_num, "TCFG_KEY_NUM", 6, nil, adkey_num_comment_str, 2);

local adkey_num_hBox_view = cfg:hBox {
            cfg:stLabel(adkey_num.name);
            cfg:ispinView(adkey_num, 0, enable_moudles["adkey"].num, 1),
            cfg:stSpacer();
}

-- ADKEY 输入引脚
local adkey_port_sel = cfg:enum("ADKEY PORT:", ad_channel_port, 0xFF);
adkey_port_sel:setOSize(1);
depend_item_en_show(adkey_en, adkey_port_sel);
adkey_port_sel:setTextOut(
	function (ty) 
		if (ty == 3) then	
			return "#define TCFG_ADKEY_PORT" .. TAB_TABLE[6] .. ad_channel_port_table[adkey_port_sel.val] .. NLINE_TABLE[1]
		end
	end
)

local adkey_port_sel_hBox_view = enum_item_hbox_view(adkey_port_sel);

-- ADKEY 通道选择, port映射到channel
local adkey_chanel_sel = cfg:i32("ADKEY CHANNEL SEL", 0xFF);
adkey_chanel_sel:setOSize(1);
depend_item_en_show(adkey_en, adkey_chanel_sel);
adkey_chanel_sel:addDeps{adkey_port_sel};
adkey_chanel_sel:setEval(function() return ad_port_to_channel_table[adkey_port_sel.val]; end);
adkey_chanel_sel:setTextOut(
	function (ty) 
		if (ty == 3) then
		    return "#define TCFG_ADKEY_AD_CHANNEL" .. TAB_TABLE[4] .. ad_channel_name_table[adkey_chanel_sel.val] .. NLINE_TABLE[1]
		end
	end
);

-- ADKEY 外部上拉使能
local adkey_extern_up_en = cfg:enum("ADKEY 使用外部上拉:", ENABLE_SWITCH, 0);
adkey_extern_up_en:setOSize(1);
depend_item_en_show(adkey_en, adkey_extern_up_en);
adkey_extern_up_en:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_ADKEY_EXTERN_UP_ENABLE" .. TAB_TABLE[3] .. FUNCTION_SWITCH_TABLE[adkey_extern_up_en.val] .. NLINE_TABLE[1]
		end
	end
)


local adkey_extern_up_en_hBox_view = enum_item_hbox_view(adkey_extern_up_en);
-----------------------------------------------------------     key res     ------------------------------------------------------
-- ADKEY 上拉阻值
local adkey_pull_up_res_val = cfg:dbf("内部或外部上拉的阻值:", -1);
adkey_pull_up_res_val:setOSize(4);
depend_item_en_show(adkey_en, adkey_pull_up_res_val);
insert_item_to_list(lua_cfg_output_bin_tabs, adkey_pull_up_res_val);

local adkey_pull_up_res_val_hBox_view = cfg:hBox {
            cfg:stLabel(adkey_pull_up_res_val.name);
            cfg:dspinView(adkey_pull_up_res_val, -1, 10000, 0.01, 2),
            cfg:stLabel(" kΩ"),
            cfg:stSpacer();
        }

-- ADKEY 参考电压
local adkey_extern_vdd_vol = cfg:dbf("ADKEY 参考电压:", 3.0);
adkey_extern_vdd_vol:setOSize(4);
depend_item_en_show(adkey_en, adkey_extern_vdd_vol);
insert_item_to_list(lua_cfg_output_bin_tabs, adkey_extern_vdd_vol);
-- 显示
local adkey_extern_vdd_vol_hBox_view = cfg:hBox {
            cfg:stLabel(adkey_extern_vdd_vol.name);
            cfg:dspinView(adkey_extern_vdd_vol, 0, 10, 0.01, 2),
            cfg:stLabel(" V"),
            cfg:stSpacer();
        }

-- ADKEY 没有按键被按下时测量的电压
local adkey_nokey_measure_vol = cfg:dbf("NO KEY测量电压:", 3.0);
adkey_nokey_measure_vol:setOSize(4);
depend_item_en_show(adkey_en, adkey_nokey_measure_vol);
insert_item_to_list(lua_cfg_output_bin_tabs, adkey_nokey_measure_vol);
-- 显示
local adkey_nokey_measure_vol_hBox_view = cfg:hBox {
            cfg:stLabel(adkey_nokey_measure_vol.name),
            cfg:dspinView(adkey_nokey_measure_vol, 0, 10, 0.01, 2),
            cfg:stLabel(" V"),
            cfg:stSpacer(),
        }


-- ADC内部参考电压
local adkey_inner_ref_vol = 3.0;
-- AD电压值系数
local adkey_ad_val_ratio = cfg:dbf("AD电压值系数", 1);
adkey_ad_val_ratio:setOSize(4);
adkey_ad_val_ratio:addDeps{adkey_extern_vdd_vol};
adkey_ad_val_ratio:setEval(function() return (adkey_extern_vdd_vol.val / adkey_inner_ref_vol); end);
insert_item_to_list(lua_cfg_output_bin_tabs, adkey_ad_val_ratio);


-- 最后按键电压值, 用于比较
local last_key_ad_vol_compare_val = cfg:i32("最后按键电压值", 1);
last_key_ad_vol_compare_val:setOSize(4);
last_key_ad_vol_compare_val:addDeps{adkey_num, adkey_pull_up_res_val, adkey_nokey_measure_vol, adkey_ad_val_ratio};
last_key_ad_vol_compare_val:setEval(function() 
                                        local result = math.floor(adkey_nokey_measure_vol.val * 1024 * adkey_ad_val_ratio.val / adkey_extern_vdd_vol.val);
                                        if result > 1024 then
                                            result = 1024;
                                        end
                                        return result;
                                    end);
insert_item_to_list(lua_cfg_output_bin_tabs, last_key_ad_vol_compare_val);


local function adkey_res_input_item_add(index)
    local res_input_item = cfg:dbf("ADKEY" .. index .. " 电阻阻值" .. " = ", -1);
    res_input_item:setOSize(4);
    key_depend_key_en_key_num_show(index, adkey_en, adkey_num, res_input_item);

    return res_input_item;
end

local function adkey_res_input_item_hbox_view(item)
    local res_input_hbox_view = cfg:hBox {
            cfg:stLabel(item.name);
            cfg:dspinView(item, -1, 10000, 0.01, 2),
            cfg:stLabel(" kΩ"),
            cfg:stSpacer();
    };

    return res_input_hbox_view;
end

local function adkey_vol_cal_item_hbox_view(item)
    local adkey_vol_cal_hbox_view = cfg:hBox {
            cfg:stLabel("测量AD值：");
            cfg:labelView(item),
            cfg:stSpacer();
    };

    return adkey_vol_cal_hbox_view;
end


local function adkey_vol_item_add(cnt)
    local vol_item = cfg:i32("vol" .. cnt, 0);
    vol_item:setOSize(2);
    vol_item:addDeps{adkey_num};
    vol_item:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_ADKEY_VOLTAGE" .. cnt .. TAB_TABLE[5] .. vol_item.val .. NLINE_TABLE[1]
		end
	end)

    return vol_item;

end

local function adkey_vol_set_depends(cnt, depend_cfg_table, new_cfg)
    --if (cnt < (enable_moudles["adkey"].num - 1)) then
    if (cnt < enable_moudles["adkey"].num - 1) then
        new_cfg:addDeps{adkey_pull_up_res_val,
                        adkey_ad_val_ratio,
                        last_key_ad_vol_compare_val,
                        depend_cfg_table[cnt].res_input_item,
                        depend_cfg_table[cnt + 1].res_input_item};
    else  --最后一个按键
        new_cfg:addDeps{adkey_pull_up_res_val,
                        last_key_ad_vol_compare_val,
                        depend_cfg_table[cnt].res_input_item};
    end
end

local function calculate_vol_mid_value(res_self, res_next)
    local result;
    local vol_self = (res_self.val * 1024 * adkey_ad_val_ratio.val) / (adkey_pull_up_res_val.val + res_self.val);
    local vol_next = (res_next.val * 1024 * adkey_ad_val_ratio.val) / (adkey_pull_up_res_val.val + res_next.val);

    if vol_self > 1024 then
    vol_self = 1024;
    end
    if vol_next > 1024 then
    vol_next = 1024;
    end

    result = math.floor((vol_self + vol_next) / 2);
    return result;
end

local function calculate_vol_mid_value_last(res_self)
    local vol_self = (res_self.val * 1024 * adkey_ad_val_ratio.val) / (adkey_pull_up_res_val.val + res_self.val);
    local vol_next = last_key_ad_vol_compare_val.val;

    if vol_self > 1024 then
    vol_self = 1024;
    end
    if vol_next > 1024 then
    vol_next = 1024;
    end

    local result = math.floor((vol_self + vol_next) / 2);

    return result;
end

local function adkey_res_to_vol(cnt, depend_cfg_table)
    return function()
        local res_self = depend_cfg_table[cnt].res_input_item;
        if ((adkey_pull_up_res_val.val <= 0) or (res_self.val < 0) or (cnt >= adkey_num.val)) then 
            return 0;
        end

        --if (cnt < enable_moudles["adkey"].num - 1) then
        if (cnt < adkey_num.val - 1) then
            local res_next = depend_cfg_table[cnt + 1].res_input_item;
            if (res_next.val <= 0) then
                return 0;
            end
            return calculate_vol_mid_value(res_self, res_next);
        else --最后一个按键
            return calculate_vol_mid_value_last(res_self);

        end
    end
end

local function adkey_vol_set_value(cnt, depend_cfg_table, new_cfg)
    new_cfg:setEval(adkey_res_to_vol(cnt, depend_cfg_table));
end

for i = 1, enable_moudles["adkey"].num  do
    local adkey_cfg = {
        res_input_item,
        adkey_vol_item;
        res_input_item_hbox_view,
        vol_cal_item_hbox_view,
        res_vol_group_view,
        short_click_msg_item,
        long_click_msg_item,
        hold_click_msg_item,
        up_click_msg_item,
        double_click_msg_item,
        triple_click_msg_item,
        msg_group_view,
        res_vol_msg_group_view,
    };

    local index = i - 1;
    adkey_item_table[index] = adkey_cfg;

    adkey_cfg.res_input_item = adkey_res_input_item_add(index);
    adkey_cfg.res_input_item_hbox_view = adkey_res_input_item_hbox_view(adkey_cfg.res_input_item);

    adkey_cfg.short_click_msg_item = key_short_click_msg_cfg_add("AD", index, adkey_en, adkey_num);
    adkey_cfg.long_click_msg_item = key_long_click_msg_cfg_add("AD", index, adkey_en, adkey_num);
    adkey_cfg.hold_click_msg_item = key_hold_click_msg_cfg_add("AD", index, adkey_en, adkey_num);
    adkey_cfg.up_click_msg_item = key_up_click_msg_cfg_add("AD", index, adkey_en, adkey_num);
    adkey_cfg.double_click_msg_item = key_double_click_msg_cfg_add("AD", index, adkey_en, adkey_num);
    adkey_cfg.triple_click_msg_item = key_triple_click_msg_cfg_add("AD", index, adkey_en, adkey_num);

    adkey_cfg.msg_group_view = enum_items_group_vbox_view(adkey_cfg.short_click_msg_item,
                                                          adkey_cfg.long_click_msg_item,
                                                          adkey_cfg.hold_click_msg_item,
                                                          adkey_cfg.up_click_msg_item,
                                                          adkey_cfg.double_click_msg_item,
                                                          adkey_cfg.triple_click_msg_item);

    --adkey_cfg.res_msg_group_view = groups_hbox_view(index, adkey_cfg.res_input_item_hbox_group_view,
                                                    --adkey_cfg.msg_group_view);
	
	insert_item_to_list(adkey_default_items, adkey_cfg.res_input_item);
	insert_item_to_list(adkey_default_items, adkey_cfg.short_click_msg_item);
	insert_item_to_list(adkey_default_items, adkey_cfg.long_click_msg_item);
	insert_item_to_list(adkey_default_items, adkey_cfg.hold_click_msg_item);
	insert_item_to_list(adkey_default_items, adkey_cfg.up_click_msg_item);
	insert_item_to_list(adkey_default_items, adkey_cfg.double_click_msg_item);
	insert_item_to_list(adkey_default_items, adkey_cfg.triple_click_msg_item);
	

--按键显示输出
    --insert_item_to_list(adkey_cfg_group_view_list, adkey_cfg.res_msg_group_view);
--按键bin输出
    insert_item_to_list(adkey_output_msg_bin_list, adkey_cfg.short_click_msg_item);
    insert_item_to_list(adkey_output_msg_bin_list, adkey_cfg.long_click_msg_item);
    insert_item_to_list(adkey_output_msg_bin_list, adkey_cfg.hold_click_msg_item);
    insert_item_to_list(adkey_output_msg_bin_list, adkey_cfg.up_click_msg_item);
    insert_item_to_list(adkey_output_msg_bin_list, adkey_cfg.double_click_msg_item);
    insert_item_to_list(adkey_output_msg_bin_list, adkey_cfg.triple_click_msg_item);
-- 中间值保留输出
    insert_item_to_list(lua_cfg_output_bin_tabs, adkey_cfg.res_input_item);
--按键text输出
    insert_item_to_list(adkey_msg_htext_table, adkey_cfg.short_click_msg_item);
    insert_item_to_list(adkey_msg_htext_table, adkey_cfg.long_click_msg_item);
    insert_item_to_list(adkey_msg_htext_table, adkey_cfg.hold_click_msg_item);
    insert_item_to_list(adkey_msg_htext_table, adkey_cfg.up_click_msg_item);
    insert_item_to_list(adkey_msg_htext_table, adkey_cfg.double_click_msg_item);
    insert_item_to_list(adkey_msg_htext_table, adkey_cfg.triple_click_msg_item);
end

local adkey_output_vol_bin_table = {};
for i = 1, enable_moudles["adkey"].num  do
    local index = i - 1;
    local adkey_vol = adkey_vol_item_add(index);
    adkey_item_table[index].adkey_vol_item = adkey_vol;
    adkey_vol_set_depends(index, adkey_item_table, adkey_vol);
    --adkey_vol:addDeps({adkey_pull_up_res_val, adkey_extern_vdd_vol, adkey_nokey_measure_vol});
    adkey_vol_set_value(index, adkey_item_table, adkey_vol);

    adkey_item_table[index].vol_cal_item_hbox_view = adkey_vol_cal_item_hbox_view(adkey_vol);
    adkey_item_table[index].res_vol_group_view = groups_vbox_view("", adkey_item_table[index].res_input_item_hbox_view,
                                                                      adkey_item_table[index].vol_cal_item_hbox_view);

    adkey_item_table[index].res_vol_msg_group_view = groups_hbox_view("KEY" .. index .. "配置", adkey_item_table[index].res_vol_group_view,
                                                               adkey_item_table[index].msg_group_view);

	insert_item_to_list(adkey_default_items, adkey_vol);
--按键显示输出
    insert_item_to_list(adkey_cfg_group_view_list, adkey_item_table[index].res_vol_msg_group_view);
--电压值bin输出
    insert_item_to_list(adkey_output_vol_bin_table, adkey_vol);
--电压值text输出
    insert_item_to_list(adkey_vol_htext_table, adkey_vol);
end


local adkey_res_msg_view = cfg:stGroup("",
        cfg:vBox {
            adkey_num_hBox_view,
            adkey_pull_up_res_val_hBox_view,
            adkey_extern_vdd_vol_hBox_view,
            adkey_nokey_measure_vol_hBox_view,
            cfg:hBox {
                cfg:stLabel("AD值满量程：");
                cfg:labelView(last_key_ad_vol_compare_val),
                cfg:stSpacer();
            },
            cfg:stLabel("\n注意：按键电阻阻值由小到大配置: \n");
            cfg:vBox(adkey_cfg_group_view_list);
        });


--[[===================================================================================
==================================== 模块返回汇总 =====================================
====================================================================================--]]
-- A. 输出htext
local adkey_output_htext_table = {};
insert_item_to_list(adkey_output_htext_table, adkey_comment_begin_htext);
insert_item_to_list(adkey_output_htext_table, adkey_en);

if enable_moudles["adkey"].enable == true then
insert_item_to_list(adkey_output_htext_table, adkey_mcro_begin);
insert_item_to_list(adkey_output_htext_table, adkey_num_max);
insert_item_to_list(adkey_output_htext_table, adkey_num);
insert_item_to_list(adkey_output_htext_table, adkey_port_sel);
insert_item_to_list(adkey_output_htext_table, adkey_chanel_sel);
insert_item_to_list(adkey_output_htext_table, adkey_extern_up_en);
insert_list_to_list(adkey_output_htext_table, adkey_msg_htext_table);
insert_list_to_list(adkey_output_htext_table, adkey_vol_htext_table);
insert_item_to_list(adkey_output_htext_table, adkey_mcro_end);
end

insert_list_to_list(board_output_text_tabs, adkey_output_htext_table);

if enable_moudles["adkey"].enable == false then 
    return; -- 只输出htext, 不显示返回
end

-- B. 输出ctext：无

-- C. 输出bin：
local adkey_output_hw_bin_list = {};
insert_item_to_list(adkey_output_hw_bin_list, adkey_en);
insert_item_to_list(adkey_output_hw_bin_list, adkey_num);
insert_item_to_list(adkey_output_hw_bin_list, adkey_port_sel);
insert_item_to_list(adkey_output_hw_bin_list, adkey_chanel_sel);
insert_item_to_list(adkey_output_hw_bin_list, adkey_extern_up_en);
insert_list_to_list(adkey_output_hw_bin_list, adkey_output_vol_bin_table);

local adkey_output_hw_bin = cfg:group("ADKEY_HW_CFG_BIN",
    BIN_ONLY_CFG["HW_CFG"].adkey.id,
    1,
    adkey_output_hw_bin_list
);

local adkey_output_msg_bin = cfg:group("ADKEY_MSG_CFG_BIN",
    BIN_ONLY_CFG["BT_CFG"].key_msg.id,
    1,
    adkey_output_msg_bin_list
);

insert_item_to_list(board_output_bin_tabs, adkey_output_hw_bin);
insert_item_to_list(board_output_bin_tabs, adkey_output_msg_bin);

-- E. 默认值
insert_item_to_list(adkey_default_items, adkey_num);

if open_by_program == "create" then
	insert_item_to_list(adkey_default_items, adkey_en);
	insert_item_to_list(adkey_default_items, adkey_num);
end

insert_item_to_list(adkey_default_items, adkey_port_sel);
insert_item_to_list(adkey_default_items, adkey_extern_up_en);
insert_item_to_list(adkey_default_items, adkey_pull_up_res_val);
insert_item_to_list(adkey_default_items, adkey_extern_vdd_vol);
insert_item_to_list(adkey_default_items, adkey_nokey_measure_vol);
insert_item_to_list(adkey_default_items, adkey_ad_val_ratio);
insert_item_to_list(adkey_default_items, last_key_ad_vol_compare_val);

-- D. 显示
local adkey_hardware_view_table = {};
local adkey_group_view_table = {};

if open_by_program == "create" then
    insert_item_to_list(adkey_hardware_view_table, adkey_en_hBox_view);
end

insert_item_to_list(adkey_hardware_view_table, adkey_port_sel_hBox_view);
insert_item_to_list(adkey_hardware_view_table, adkey_extern_up_en_hBox_view);

local adkey_hardware_group_view = cfg:stGroup("", cfg:vBox(adkey_hardware_view_table));

insert_item_to_list(adkey_group_view_table, adkey_hardware_group_view);
insert_item_to_list(adkey_group_view_table, adkey_res_msg_view);

if open_by_program == "fw_edit" then
    adkey_num:setShow(false);
end

local adkey_group_view_list = cfg:stGroup("ADKEY按键配置",
        cfg:stHScroll(
        cfg:vBox (adkey_group_view_table))
);

local adkey_view_list = cfg:vBox {
        adkey_group_view_list,
		cfg:stButton("恢复 ADKEY按键配置默认值", reset_to_default(adkey_default_items));
};

local adkey_view = {"ADKEY 按键配置",
        adkey_view_list,
};

insert_item_to_list(board_view_tabs, adkey_view);

-- F. bindGroup：无


