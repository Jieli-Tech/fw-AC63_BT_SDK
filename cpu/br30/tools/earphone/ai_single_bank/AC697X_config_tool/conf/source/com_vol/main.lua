
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

local round = function (number, precision)
   local fmtStr = string.format('%%.%df',precision)
   number = string.format(fmtStr,number)
   return tonumber(number)
end


local max_vol_config_count = 100; -- 最多的音量级别

local all_com_vol_cfgs = {}

local all_com_vol_groups = {}

local default_val = {
	{ lev = 16, minV = -50, maxV = -12 },
	{ lev = 15, minV = -45, maxV = -14 },
}

local all_com_vol_groups_id = {
	BIN_ONLY_CFG["HW_CFG"].sys_com_vol.id,
	BIN_ONLY_CFG["HW_CFG"].call_com_vol.id,
};

for k = 1, 2 do
	all_com_vol_cfgs[k] = {}
	all_com_vol_cfgs[k].com_vol_invals = {}
	all_com_vol_cfgs[k].com_vol_enable_switch = {}
	all_com_vol_cfgs[k].com_vol_layouts = {}
	all_com_vol_cfgs[k].com_vol_outval_x = {}
	all_com_vol_cfgs[k].com_vol_outval_avol = {}
	
	for i = 1, max_vol_config_count do
		local c = cfg:dbf(string.format('期望音量-%d %2d', k,  i), com_vol_config.max_config_rdb);
		local on = 0;

		local onc = cfg:enum("使能-" .. k .. '-' .. i, ENABLE_SWITCH, 0);
		c:addDeps{onc};
		c:setShow(on);
		c:setDepChangeHook(function () c:setShow(onc.val ~= 0) end);
		all_com_vol_cfgs[k].com_vol_invals[#all_com_vol_cfgs[k].com_vol_invals+1] = c;
		all_com_vol_cfgs[k].com_vol_enable_switch[#all_com_vol_cfgs[k].com_vol_enable_switch+1] = onc;

		local l = cfg:hBox{ cfg:stLabel("开关："), cfg:enumView(onc),  cfg:stLabel(string.format("期望音量 %2d", i)),
			cfg:dspinView(c, com_vol_config.min_config_rdb,
							 com_vol_config.max_config_rdb,
							 1,
							 3),
			cfg:stSpacer() };
		all_com_vol_cfgs[k].com_vol_layouts[#all_com_vol_cfgs[k].com_vol_layouts+1] = l;
	end

	local group_list = {}
	local always_on = cfg:enum("总是使能-" .. k .. "-", ENABLE_SWITCH, 1);
	local always_zero_x = cfg:i32("期望音量-out-x-zero" .. k, 0);
	local always_zero_avol = cfg:i32("期望音量-out-avol-zero" .. k, 0);

	always_zero_avol:setOSize(2);
	always_zero_x:setOSize(2);

	group_list[#group_list+1] = {always_on, {always_zero_x, always_zero_avol}};

	for i = 1, max_vol_config_count do
		local x = cfg:i32("期望音量-out-x-" .. k .. '-' .. i, 0);
		local avol = cfg:i32("期望音量-out-avol" .. k .. "-" .. i, 0);
		
		x:setOSize(2);
		avol:setOSize(2);
		
		x:addDeps(all_com_vol_cfgs[k].com_vol_invals);
		x:addDeps(all_com_vol_cfgs[k].com_vol_enable_switch);
		
		avol:addDeps(all_com_vol_cfgs[k].com_vol_invals);
		avol:addDeps(all_com_vol_cfgs[k].com_vol_enable_switch);
		
		local valid_counts = function()
			local count = 0
			for ii = 1, max_vol_config_count do
				if all_com_vol_cfgs[k].com_vol_enable_switch[ii].val ~= 0 then
					count = count + 1
				end
			end
			return count;
		end;
		
		x:setEval(function ()
			local count = valid_counts();
			if i > count then return 0; end
			if all_com_vol_cfgs[k].com_vol_enable_switch[i].val ~= 0 then
				local x = find_nearest_vol_from_table(all_com_vol_cfgs[k].com_vol_invals[count-i+1].val);
				return x.x;
			else
				return 0;
			end
		end);

		avol:setEval(function ()
			local count = valid_counts();
			if i > count then return 0; end
			if all_com_vol_cfgs[k].com_vol_enable_switch[i].val ~= 0 then
				local x = find_nearest_vol_from_table(all_com_vol_cfgs[k].com_vol_invals[count-i+1].val);
				return x.avol;
			else
				return 0;
			end
		end);
		
		group_list[#group_list+1] = {all_com_vol_cfgs[k].com_vol_enable_switch[i],
			{ avol, x }};
	end

	all_com_vol_groups[k] = cfg:listGroup("SYS_COM_VOL_HW_CFG_BIN-" .. k,
		all_com_vol_groups_id[k],
		1,
		group_list
	);
	
	cfg:addOutputGroups{all_com_vol_groups[k]};
	
	all_com_vol_groups[k]:setRecoverHook(function ()
		print('recovered')
	end);
	

	local com_vol_lev_group_view = cfg:stGroup("音量档位", cfg:vBox(all_com_vol_cfgs[k].com_vol_layouts));

	local first_type = {}
	first_type.levs = cfg:i32("第一种，档位个数"..k, default_val[k].lev);
	first_type.levs_ui = cfg:ispinView(first_type.levs, 2, max_vol_config_count, 1);
	first_type.max_vol = cfg:dbf("第一种，最大音量"..k, default_val[k].maxV);
	first_type.max_vol_ui = cfg:dspinView(first_type.max_vol,
							 com_vol_config.min_config_rdb,
							 com_vol_config.max_config_rdb, 1, 3);
	first_type.min_vol = cfg:dbf("第一种，最小音量"..k, default_val[k].minV);
	first_type.min_vol_ui = cfg:dspinView(first_type.min_vol,
							 com_vol_config.min_config_rdb,
							 com_vol_config.max_config_rdb, 1, 3);

	-- 第一种，用户设置最大音量，最小音量，级数
	local first_set_button = cfg:stButton("设置档位", function ()
		local step_val = (first_type.min_vol.val - first_type.max_vol.val) / (first_type.levs.val - 1);
		print('step_val ' .. step_val)
		for i = 1, max_vol_config_count do
			cfg:set(all_com_vol_cfgs[k].com_vol_enable_switch[i], 0);
		end
		for i = 1, first_type.levs.val do
			cfg:set(all_com_vol_cfgs[k].com_vol_enable_switch[i], 1); -- 使能
			cfg:set(all_com_vol_cfgs[k].com_vol_invals[i], first_type.max_vol.val + (i - 1) * step_val);
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
	second_type.step = cfg:dbf("第二种，递音量"..k, 2);
	second_type.step_ui = cfg:dspinView(second_type.step, 0, 100, 1, 2);
	second_type.max_vol = cfg:dbf("第二种，最大音量"..k, default_val[k].maxV);
	second_type.max_vol_ui = cfg:dspinView(second_type.max_vol,
							 com_vol_config.min_config_rdb,
							 com_vol_config.max_config_rdb, 1, 3);
	second_type.min_vol = cfg:dbf("第二种，最小音量"..k, default_val[k].minV);
	second_type.min_vol_ui = cfg:dspinView(second_type.min_vol,
							 com_vol_config.min_config_rdb,
							 com_vol_config.max_config_rdb, 1, 3);

	-- 第二种，用户设置最大音量，最小音量，等差
	local second_set_button = cfg:stButton("设置档位", function ()
		for i = 1, max_vol_config_count do
			cfg:set(all_com_vol_cfgs[k].com_vol_enable_switch[i], 0);
		end
		local step_val = second_type.step.val;
		local init_val = second_type.max_vol.val;
		local stop_val = second_type.min_vol.val;
		for i = 1, max_vol_config_count do
			local val = init_val - (i - 1) * step_val;
			if val < com_vol_config.min_config_rdb or val > com_vol_config.max_config_rdb or val < stop_val then
				break
			end
			cfg:set(all_com_vol_cfgs[k].com_vol_enable_switch[i], 1);
			cfg:set(all_com_vol_cfgs[k].com_vol_invals[i], val);
		end
	end);

	local com_vol_lev_setting_second_type_group_view = cfg:stGroup("设置档位",
		cfg:vBox{
			cfg:hBox{ cfg:stLabel("音量间隔"), second_type.step_ui, cfg:stSpacer() },
			cfg:hBox{ cfg:stLabel("最大音量"), second_type.max_vol_ui, cfg:stSpacer() },
			cfg:hBox{ cfg:stLabel("最小音量"), second_type.min_vol_ui, cfg:stSpacer() },
			second_set_button
		});

	all_com_vol_cfgs[k].layout = cfg:stHScroll(cfg:vBox{
		cfg:hBox{
			com_vol_lev_setting_first_type_group_view,
			com_vol_lev_setting_second_type_group_view,
		},
		com_vol_lev_group_view,
	})

end


local output_com_vol_array_header = function (arrayname, output)
	local outstr = string.format("static unsigned short %s[%d][2] = {\n",
		arrayname,
		#output + 1);
	outstr = outstr .. string.format("\t{%2d, %5d}, //0: None\n", 0, 0);
	for i = 1, #output do
		local x = output[#output-i+1];
		outstr = outstr .. string.format("\t{%2d, %5d}, // %d:%.2f db\n", x.avol, x.x, i, x.vol);
	end
	outstr = outstr .. "};\n";
	return outstr;
end

local save_button = cfg:stButton("保存头文件", function ()
	local filepath = cfg.workDir .. '/com_vol_cfg.h'
	local outstr = '';
	
	local arr_names = {'combined_vol_list', 'call_combined_vol_list'};
	
	for k = 1, 2 do
		local output = {}

		for i = 1, max_vol_config_count do
			if all_com_vol_cfgs[k].com_vol_enable_switch[i].val ~= 0 then
				local x = find_nearest_vol_from_table(all_com_vol_cfgs[k].com_vol_invals[i].val);
				output[#output+1] = x;
			end
		end
		
		outstr = outstr .. output_com_vol_array_header(arr_names[k], output);
	end

	cfg:utilsWriteString(filepath, outstr);
	
	cfg:runProgNoWait{"notepad.exe", filepath};
end);


--- 设置界面 ---

comvol_view = {'联合音量', 
	cfg:vBox{
		save_button,
		cfg:stTab{
			{"系统音量", all_com_vol_cfgs[1].layout},
			{"通话音量", all_com_vol_cfgs[2].layout},
		}
	}
}

comvol_bin_groups = all_com_vol_groups