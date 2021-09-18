
LUA_CFG_RESERVED_ID = 0;  --配置工具保留ID

-- 用户自定义类配置项 ID (1 ~ 49), 保留
USER_DEF_CFG = {};

-- 只存VM配置项 ID (50 ~ 99), 保留
VM_CFG = {};

-- 可存于VM, sys_cfg.bin, 和 BTIF 区域配置项ID (100 ~ 127)
MULT_AREA_CFG = {
    bt_name = {id = 101},
    bt_mac = {id = 102},
};

-- 只存于sys_cfg.bin区域配置项 ID (512 ~ 700)
-- 硬件资源类键值(513 ~ 600)
HW_CFG = {
	uart =			{id = 513},
	hwi2c =			{id = 514},
	swi2c =			{id = 515},
	hwspi =			{id = 516},
	swspi =			{id = 517},
	sd =			{id = 518},
	usb =			{id = 519},
	lcd =			{id = 520},
	touch =			{id = 521},
	iokey =			{id = 522},
	adkey =			{id = 523},
	audio =			{id = 524},
	video =			{id = 525},
	wifi =			{id = 526},
	nic =			{id = 527},
	led = 			{id = 528},
	power_mang = 	{id = 529},
	irflt =			{id = 530},
	plcnt =			{id = 531},
	pwmled =		{id = 532},
	rdec =			{id = 533},
	charge_store =	{id = 534},   --充电仓
	charge =    	{id = 535},   --充电
	lowpower =      {id = 536},   --低电电压
    mic_type =      {id = 537},  -- mic类型配置
};


-- 蓝牙配置类键值(601 ~ 650)
BT_CFG = {
    rf_power = {id = 601},
    tws_pair_code = {id = 602},
    auto_shut_down_time = {id = 603},
    aec = {id = 604},
    status = {id = 605},
    key_msg = {id = 606},
    lrc_cfg = {id = 607},
    ble_cfg = {id = 608},
};

BIN_ONLY_CFG = {
    ["HW_CFG"] = HW_CFG,
    ["BT_CFG"] = BT_CFG,
};

-- 其它配置键值(651 ~ 700)
OTHERS_CFG = {
};





