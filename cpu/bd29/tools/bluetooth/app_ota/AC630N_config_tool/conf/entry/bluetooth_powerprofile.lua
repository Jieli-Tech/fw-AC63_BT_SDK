if false then
    return;
else
	local bt_voltage = cfg:dbf("bt.ppf", 3.3); -- 1.8 ~ 3.6v
	local bt_voltage_view = cfg:dspinView(bt_voltage, 1.8, 5.5, 0.1, 1);
	
	local btppf_onff_ety = cfg:enumStr("btppf_onff_ety", {"OFF", "ON"});
	
	local btppf_regulator_ety = cfg:enumStr("btppf_regulator_ety", {"LDO", "DCDC"});
	local bt_regulator = cfg:enum("bt.ppf_dcdc", btppf_regulator_ety, 1);
	local bt_regulator_view = cfg:enumView(bt_regulator);
	
	local btppf_lf_clock_ety = cfg:enumStr("btppf_lf_clock_ety", {"Internal RC"});
	local bt_lf_clock = cfg:enum("bt.ppf_lf_clock", btppf_lf_clock_ety, 0);
	local bt_lf_clock_view = cfg:enumView(bt_lf_clock);
	
	local supported_tx_power = {
		{p = -18.3, ldo = 22.3},
		{p = -14.6, ldo = 23},
		{p = -12.1, ldo = 23.6},
		{p = -8.5 , ldo = 25},
		{p = -6 , ldo = 26.4},
		{p = -4.1 , ldo = 27.6},
		{p = -1.1 , ldo = 30.2},
		{p =  1.1 , ldo = 32.5},
		{p =  4 , ldo = 36.7},
		{p =  6.1 , ldo = 40}
	};
	local supported_tx_power_map = {};
	for k, v in ipairs(supported_tx_power) do
		supported_tx_power_map[#supported_tx_power_map+1] = string.format("%4.1f dBm", v.p)
	end
	local get_tx_power = function (x) return supported_tx_power[x+1].p end;
	local get_tx_ldo_currency = function (x) return supported_tx_power[x+1].ldo end;
	local get_tx_dcdc_currency = function (x) return ((supported_tx_power[x+1].ldo*1.25)/0.8/bt_voltage.val) end;

	local btppf_tx_power_ety = cfg:enumStr("btppf_tx_power_ety", supported_tx_power_map);
	local bt_tx_power = cfg:enum("bt.ppf_tx_power", btppf_tx_power_ety, 9);
	local bt_tx_power_view = cfg:enumView(bt_tx_power);
	
	local bt_chip_settings_grp_view = cfg:stGroup(
		"芯片设置",
		cfg:vBox{
			cfg:hBox{cfg:stLabel("Voltage  :"), bt_voltage_view, cfg:stSpacer()},
			cfg:hBox{cfg:stLabel("Regulator:"), bt_regulator_view, cfg:stSpacer()},
			cfg:hBox{cfg:stLabel("LF clock :"), bt_lf_clock_view, cfg:stSpacer()},
			cfg:hBox{cfg:stLabel("Radio TX :"), bt_tx_power_view, cfg:stSpacer()},
		}
	);
	
	
	-- ble settings
	local btppf_ble_role_ety = cfg:enumStr("btppf_ble_role_ety",
									{"Advertising (connectable)",
									 "Advertising (non-con/-scon)",
									 "Connection (peripheral)",
									 "Connection (central)"});
	local bt_ble_role = cfg:enum("bt.ppf_ble_role", btppf_ble_role_ety, 0);
	local bt_ble_role_view = cfg:enumView(bt_ble_role);

	-- 20ms ~ 10240ms
	local bt_ble_advertising_interval = cfg:dbf("bt.ppf_ble_adv_interval", 20.0);
	local bt_ble_advertising_interval_view = cfg:dspinView(bt_ble_advertising_interval,
		20.0, 10240.0, 0.01, 2);

	-- 7.50ms ~ 4000.00ms
	local bt_ble_connection_interval = cfg:dbf("bt.ppf_ble_connection_interval", 7.5);
	local bt_ble_connection_interval_view = cfg:dspinView(bt_ble_connection_interval,
		7.50, 4000.00, 0.01, 2);

	-- 0 ~ 31 byte
	local bt_ble_tx_payload = cfg:i32("bt.ppf_tx_payload", 0);
	local bt_ble_tx_payload_view = cfg:ispinView(bt_ble_tx_payload, 0, 31, 1);

	-- tx payload per event 0 ~ 351
	local bt_ble_tx_payload_per_ev = cfg:i32("bt.ppf_tx_payload_per_ev", 0);
	local bt_ble_tx_payload_per_ev_view = cfg:ispinView(bt_ble_tx_payload_per_ev, 0, 351, 1);
	
	-- rx payload per event 0 ~ 351
	local bt_ble_rx_payload_per_ev = cfg:i32("bt.ppf_rx_payload_per_ev", 0);
	local bt_ble_rx_payload_per_ev_view = cfg:ispinView(bt_ble_rx_payload_per_ev, 0, 351, 1);
	
	-- data packet length extension
	local bt_ble_data_packet_length_ext = cfg:enum("bt.ppf_len_ext", btppf_onff_ety, 0);
	local bt_ble_data_packet_length_ext_view = cfg:enumView(bt_ble_data_packet_length_ext);
	
	-- connection event length extension
	local bt_ble_conn_evt_len_ext = cfg:enum("bt.ppf_conn_evt_len_ext", btppf_onff_ety, 0);
	local bt_ble_conn_evt_len_ext_view = cfg:enumView(bt_ble_conn_evt_len_ext);

	local bt_ble_role_adv_grp_view = cfg:stGroup(
		"",
		cfg:vBox{
			cfg:hBox{cfg:stLabel("Advertising interval (ms):"), bt_ble_advertising_interval_view, cfg:stSpacer()},
			cfg:hBox{cfg:stLabel("TX payload (Byte):"), bt_ble_tx_payload_view, cfg:stSpacer()},
		}
	);
	bt_ble_role_adv_grp_view:setHide(false);
	
	local bt_ble_role_conn_grp_view = cfg:stGroup(
		"",
		cfg:vBox{
			cfg:hBox{cfg:stLabel("Connection Interval(ms):"), bt_ble_connection_interval_view, cfg:stSpacer()},
			cfg:hBox{cfg:stLabel("TX payload per event(Byte):"), bt_ble_tx_payload_per_ev_view, cfg:stSpacer()},
			cfg:hBox{cfg:stLabel("RX payload per event(Byte):"), bt_ble_rx_payload_per_ev_view, cfg:stSpacer()},
			cfg:hBox{cfg:stLabel("Data Packet Length Extension"), bt_ble_data_packet_length_ext_view, cfg:stSpacer()},
			cfg:hBox{cfg:stLabel("Connection Event Length Ext"), bt_ble_conn_evt_len_ext_view, cfg:stSpacer()},
		}
	);
	bt_ble_role_conn_grp_view:setHide(true);
	
	local bt_ble_settings_grp_view = cfg:stGroup(
		"BLE设置",
		cfg:vBox{
			cfg:hBox{cfg:stLabel("Role:"), bt_ble_role_view, cfg:stSpacer()},
			bt_ble_role_adv_grp_view,
			bt_ble_role_conn_grp_view,
		}
	);

	
	-- advanced
	local btppf_ble_adv_phy_ety = cfg:enumStr("btppf_ble_adv_phy_ety",
										{"1 Mbps", "2 Mbps", "Coded PHY S=8"});
	local bt_ble_adv_phy = cfg:enum("bt.ppf_ble_adv_phy", btppf_ble_adv_phy_ety, 0);
	local bt_ble_adv_phy_view = cfg:enumView(bt_ble_adv_phy);
	
	-- 0 ~ 3
	local bt_ble_slave_latency = cfg:int("bt.ppf_ble_slave_latency", 0);
	local bt_ble_slave_latency_view = cfg:ispinView(bt_ble_slave_latency, 0, 65535, 1);

	-- master -- 20 ~ 500
	local bt_ble_slave_sleep_clock_accuray = cfg:int("bt.ppf_ble_slave_sleep_clock_accuray", 20);
	local bt_ble_slave_sleep_clock_accuray_view = cfg:ispinView(bt_ble_slave_sleep_clock_accuray, 20, 500, 1);

	-- slave -- 20 ~ 500
	local bt_ble_master_sleep_clock_accuray = cfg:int("bt.ppf_ble_master_sleep_clock_accuray", 20);
	local bt_ble_master_sleep_clock_accuray_view = cfg:ispinView(bt_ble_master_sleep_clock_accuray, 20, 500, 1);

	local bt_ble_slave_ppm_layout_view = cfg:hBox{cfg:stLabel("Slave sleep clock accuracy (ppm):"), bt_ble_slave_sleep_clock_accuray_view, cfg:stSpacer()};
	local bt_ble_master_ppm_layout_view= cfg:hBox{cfg:stLabel("Master sleep clock accuracy (ppm):"), bt_ble_master_sleep_clock_accuray_view, cfg:stSpacer()};

	local bt_advanced_grp_view = cfg:stGroup(
		"高级设置",
		cfg:vBox{
			cfg:hBox{cfg:stLabel("PHY:"), bt_ble_adv_phy_view, cfg:stSpacer()},
			cfg:hBox{cfg:stLabel("Slave Latency:"), bt_ble_slave_latency_view, cfg:stSpacer()},
			bt_ble_slave_ppm_layout_view,
			bt_ble_master_ppm_layout_view,
		}
	);
	bt_advanced_grp_view:setHide(true);

	bt_ble_role:setValChangeHook(function ()
		bt_ble_role_adv_grp_view:setHide(not (bt_ble_role.val == 0 or bt_ble_role.val == 1));
		bt_ble_role_conn_grp_view:setHide(bt_ble_role.val == 0 or bt_ble_role.val == 1);
		bt_advanced_grp_view:setHide(bt_ble_role.val == 0 or bt_ble_role.val == 1);
		bt_ble_slave_ppm_layout_view:setHide(bt_ble_role.val ~= 2);
		bt_ble_master_ppm_layout_view:setHide(bt_ble_role.val ~= 3);
		print('hook -- btble role ' .. bt_ble_role.val);
	end);

	-- 计算
	local bt_avr_power = cfg:str("bt.ppf_avr_power", "0.00 uA");
	bt_avr_power:addDeps{
		bt_voltage, bt_regulator, bt_lf_clock, bt_tx_power,
		bt_ble_role,
			bt_ble_connection_interval,
			bt_ble_tx_payload_per_ev, bt_ble_rx_payload_per_ev,
			bt_ble_data_packet_length_ext, bt_ble_conn_evt_len_ext,
			bt_ble_adv_phy,
				bt_ble_slave_latency,
				bt_ble_slave_sleep_clock_accuray, bt_ble_master_sleep_clock_accuray,
			bt_ble_advertising_interval, bt_ble_tx_payload,
	};
	
	local bt_avr_power_eval_fun = function ()
		local items = {};
		local total_x = 0;
		
		local txpeak_cur = 0;
		local rxpeak_cur = 0;
		local switch_cur = 12.42 * 1000;
		local start_cur = 23.45 * 1000;

		if bt_regulator.val == 0 then
			-- ldo
			txpeak_cur = get_tx_ldo_currency(bt_tx_power.val) * 1000;
			rxpeak_cur = 25.1 * 1000;
		else
			txpeak_cur = get_tx_dcdc_currency(bt_tx_power.val) * 1000;
			rxpeak_cur = 20.7 * 1000;
		end
		if bt_ble_role.val == 0 or bt_ble_role.val == 1 then
			table.insert(items, {x = 60, y = start_cur}); -- radio start
			table.insert(items, {x = (80 + bt_ble_tx_payload.val * 8) * 3 , y = txpeak_cur});
			table.insert(items, {x = 150*3                                , y = switch_cur});
			table.insert(items, {x = 40                                   , y = rxpeak_cur});
			table.insert(items, {x = 3500 + 4*625 + 3*846                  , y = 4000});
			-- table.insert(items, {x = 3500 + 4*625 + 3*585                  , y = 6000});
		
			for k, v in ipairs(items) do total_x = total_x + v.x end

			table.insert(items, {x = bt_ble_advertising_interval.val * 1000 - total_x, y =30});
		else
			table.insert(items, {x = 3500 + 4500, y = 4000}); -- system busy
			table.insert(items, {x = 60, y = start_cur}); -- radio start
			table.insert(items, {x = 80 + bt_ble_tx_payload_per_ev.val * 8, y = txpeak_cur}); -- radio tx
			table.insert(items, {x = 80 + bt_ble_rx_payload_per_ev.val * 8, y = rxpeak_cur}); -- radio rx
			table.insert(items, {x = 150 * 2, y = switch_cur}); -- radio switch
			
			for k, v in ipairs(items) do total_x = total_x + v.x end
			
			table.insert(items, {x = (bt_ble_connection_interval.val * (bt_ble_slave_latency.val +1 )) * 1000 - total_x, y = 30});
		end
		
		total_x = 0;
		local total_avr = 0.0;
		for k, v in ipairs(items) do
			total_x = total_x + v.x;
			total_avr = total_avr + v.x * v.y;
		end
		total_avr = total_avr / total_x;
		return string.format("%.2f uA", total_avr);
	end;
	
	bt_avr_power:setEval(bt_avr_power_eval_fun);
	local bt_avr_power_view = cfg:labelView(bt_avr_power);

	bt_power_profile_view = cfg:vBox{cfg:stHScroll(
		cfg:vBox{
			bt_chip_settings_grp_view,
			bt_ble_settings_grp_view,
			bt_advanced_grp_view,
			cfg:hBox{cfg:stLabel("Total average current: "), bt_avr_power_view, cfg:stSpacer()},
		})};
end

cfg:setLayout(bt_power_profile_view);

