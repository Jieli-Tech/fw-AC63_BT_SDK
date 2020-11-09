
require('com_vol.config'); -- 联合音量的配置

local find_nearest_vol_from_table = function (expect_db)
	-- 找到第一个比当前值大的数
	local fi = #com_vol_config.table + 1
	for i = 1, #com_vol_config.table do
		if com_vol_config.table[i].rdb < expect_db then
			fi = i;
			break
		end
	end
	if fi ~= 1 then
		fi = fi - 1
	end
	
	local diff = expect_db - com_vol_config.table[fi].rdb;
	
	local x = (10.0 ^ (diff / 20.0)) * com_vol_config.max_dvol;
	return {x = math.floor(x + 0.5), vol = expect_db, avol = com_vol_config.table[fi].avol};
end

local find_rdb_of_avol_from_table = function (avol)
	-- 找到第一个比当前值大的数
	local fi = #com_vol_config.table
	for i = 1, #com_vol_config.table do
		if com_vol_config.table[i].avol < avol then
			fi = i;
			break;
		end
	end
	if fi ~= 1 then
		fi = fi - 1;
	end

	return com_vol_config.table[fi].rdb;
end

local max_vol_config_count = 100; -- 最多的音量级别

local com_vol_invals = {}
local com_vol_enable_switch = {}
local com_vol_layouts = {}

for i = 1, max_vol_config_count do
	local c = cfg:dbf(string.format('期望音量 %2d', i), com_vol_config.max_config_rdb);
	local on = 0;

	local onc = cfg:enum("使能" .. i, ENABLE_SWITCH, 0);
	c:addDeps{onc};
	c:setShow(on);
	c:setDepChangeHook(function () c:setShow(onc.val ~= 0) end);
	com_vol_invals[#com_vol_invals+1] = c;
	com_vol_enable_switch[#com_vol_enable_switch+1] = onc;

	local l = cfg:hBox{ cfg:stLabel("开关："), cfg:enumView(onc),  cfg:stLabel(c.name),
		cfg:dspinView(c, com_vol_config.min_config_rdb,
						 com_vol_config.max_config_rdb,
						 1,
						 3),
		cfg:stSpacer() };
	com_vol_layouts[#com_vol_layouts+1] = l;
end

local com_vol_lev_group_view = cfg:stGroup("音量档位", cfg:vBox(com_vol_layouts));

local first_type = {}
first_type.levs = cfg:i32("第一种，档位个数", max_vol_config_count);
first_type.levs_ui = cfg:ispinView(first_type.levs, 2, max_vol_config_count, 1);
first_type.max_vol = cfg:dbf("第一种，最大音量", com_vol_config.max_config_rdb);
first_type.max_vol_ui = cfg:dspinView(first_type.max_vol,
						 com_vol_config.min_config_rdb,
						 com_vol_config.max_config_rdb, 1, 3);
first_type.min_vol = cfg:dbf("第一种，最小音量", com_vol_config.min_config_rdb);
first_type.min_vol_ui = cfg:dspinView(first_type.min_vol,
						 com_vol_config.min_config_rdb,
						 com_vol_config.max_config_rdb, 1, 3);

-- 第一种，用户设置最大音量，最小音量，级数
local first_set_button = cfg:stButton("设置档位", function ()
	local step_val = (first_type.min_vol.val - first_type.max_vol.val) / (first_type.levs.val - 1);
	print('step_val ' .. step_val)
	for i = 1, max_vol_config_count do
		cfg:set(com_vol_enable_switch[i], 0);
	end
	for i = 1, first_type.levs.val do
		cfg:set(com_vol_enable_switch[i], 1); -- 使能
		cfg:set(com_vol_invals[i], first_type.max_vol.val + (i - 1) * step_val);
	end
end);

local com_vol_lev_setting_first_type_group_view = cfg:stGroup("设置档位",
	cfg:vBox{
		cfg:hBox{ cfg:stLabel("档位个数"), first_type.levs_ui, cfg:stSpacer() },
		cfg:hBox{ cfg:stLabel("最大音量"), first_type.max_vol_ui, cfg:stSpacer() },
		cfg:hBox{ cfg:stLabel("最小音量"), first_type.min_vol_ui, cfg:stSpacer() },
		first_set_button
	});


-- 第二种，用户设置最大音量，最小音量，等差
local second_type = {}
second_type.step = cfg:dbf("第二种，递减音量", 2);
second_type.step_ui = cfg:dspinView(second_type.step, 0, 100, 1, 2);
second_type.max_vol = cfg:dbf("第二种，最大音量", com_vol_config.max_config_rdb);
second_type.max_vol_ui = cfg:dspinView(second_type.max_vol,
						 com_vol_config.min_config_rdb,
						 com_vol_config.max_config_rdb, 1, 3);
second_type.min_vol = cfg:dbf("第二种，最小音量", com_vol_config.min_config_rdb);
second_type.min_vol_ui = cfg:dspinView(second_type.min_vol,
						 com_vol_config.min_config_rdb,
						 com_vol_config.max_config_rdb, 1, 3);

-- 第二种，用户设置最大音量，最小音量，等差
local second_set_button = cfg:stButton("设置档位", function ()
	for i = 1, max_vol_config_count do
		cfg:set(com_vol_enable_switch[i], 0);
	end
	local step_val = second_type.step.val;
	local init_val = second_type.max_vol.val;
	local stop_val = second_type.min_vol.val;
	for i = 1, max_vol_config_count do
		local val = init_val - (i - 1) * step_val;
		if val < com_vol_config.min_config_rdb or val > com_vol_config.max_config_rdb or val < stop_val then
			break
		end
		cfg:set(com_vol_enable_switch[i], 1);
		cfg:set(com_vol_invals[i], val);
	end
end);

local com_vol_lev_setting_second_type_group_view = cfg:stGroup("设置档位",
	cfg:vBox{
		cfg:hBox{ cfg:stLabel("递减音量"), second_type.step_ui, cfg:stSpacer() },
		cfg:hBox{ cfg:stLabel("最大音量"), second_type.max_vol_ui, cfg:stSpacer() },
		cfg:hBox{ cfg:stLabel("最小音量"), second_type.min_vol_ui, cfg:stSpacer() },
		second_set_button
	});

-- 通话音量最小DAC增益
local call_vol_min_dac = {}
call_vol_min_dac.cfg = cfg:dbf("通话音量最小DAC增益", com_vol_config.min_config_rdb);
call_vol_min_dac.ui = cfg:dspinView(call_vol_min_dac.cfg,
							com_vol_config.min_config_rdb,
							com_vol_config.max_config_rdb, 1, 3);
call_vol_min_dac.layout = cfg:hBox{ cfg:stLabel(call_vol_min_dac.cfg.name), call_vol_min_dac.ui};



-- 第三种，用户手动选择音量级别

local output_com_vol_array_header = function (arrayname, output)
	local outstr = string.format("static unsigned short %s[%d][2] = {\n",
		arrayname,
		#output + 1);
	outstr = outstr .. string.format("\t{%2d, %5d}, //0: None\n", 0, 0);
	for i = 1, #output do
		local x = output[#output - i + 1];
		outstr = outstr .. string.format("\t{%2d, %5d}, // %d:%.2f db\n", x.avol, x.x, i, x.vol);
	end
	outstr = outstr .. "};\n";
	return outstr;
end

local round = function (number, precision)
   local fmtStr = string.format('%%.%df',precision)
   number = string.format(fmtStr,number)
   return tonumber(number)
end

local save_button = cfg:stButton("保存配置", function ()
	local output = {}
	for i = 1, max_vol_config_count do
		if com_vol_enable_switch[i].val ~= 0 then
			local x = find_nearest_vol_from_table(com_vol_invals[i].val);
			output[#output+1] = x;
		end
	end
	
	local filepath = cfg.workDir .. '/com_vol_cfg.h'

	local outstr = output_com_vol_array_header('combined_vol_list', output);
	
	if global_cfgs ~= nil and global_cfgs["DAC_AGAIN"] ~= nil then
		local call_output = {}
		local max_rdb = find_rdb_of_avol_from_table(global_cfgs["DAC_AGAIN"].val);
		local step_val = (call_vol_min_dac.cfg.val - max_rdb) / (com_vol_config.call_vol_levels - 1);

		for i = 1, com_vol_config.call_vol_levels do
			local db = round(max_rdb + (i - 1) * step_val, 3);
			local x = find_nearest_vol_from_table(db);
			call_output[#call_output+1] = x;
		end

		outstr = outstr .. output_com_vol_array_header('call_combined_vol_list', call_output);
	end
	
	cfg:utilsWriteString(filepath, outstr);
	
	cfg:runProgNoWait{"notepad.exe", filepath};
end);


--- 设置界面 ---

comvol_view = {'联合音量', 
	cfg:stHScroll(cfg:vBox{
		cfg:hBox{
			com_vol_lev_setting_first_type_group_view,
			com_vol_lev_setting_second_type_group_view,
		},
		call_vol_min_dac.layout,
		save_button,
		com_vol_lev_group_view,
	})
}