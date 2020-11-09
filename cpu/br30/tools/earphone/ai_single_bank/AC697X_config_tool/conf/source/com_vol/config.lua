
com_vol_config = {}

-- 数据表
com_vol_config.table = {
	{avol = 15, rdb =   0.000},
	{avol = 14, rdb =  -1.999},
	{avol = 13, rdb =  -3.991},
	{avol = 12, rdb =  -5.933},
	{avol = 11, rdb =  -7.936},
	{avol = 10, rdb =  -9.872},
	{avol =  9, rdb = -11.789},
	{avol =  8, rdb = -13.754},
	{avol =  7, rdb = -15.793},
	{avol =  6, rdb = -17.694},
	{avol =  5, rdb = -19.686},
	{avol =  4, rdb = -21.678},
	{avol =  3, rdb = -23.572},
	{avol =  2, rdb = -25.480},
	{avol =  1, rdb = -27.423},
	{avol =  0, rdb = -29.199},
};

com_vol_config.max_config_rdb = com_vol_config.table[1].rdb;
com_vol_config.min_config_rdb = -100; -- com_vol_config.table[#com_vol_config.table].rdb;
com_vol_config.max_dvol = 16384; -- 最大数字音量
com_vol_config.call_vol_levels = 15;