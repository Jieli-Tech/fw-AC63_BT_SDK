
com_vol_config = {}

-- 数据表
com_vol_config.table = {
	{avol = 30, rdb =   0.000},
	{avol = 29, rdb =  -1.381},
	{avol = 28, rdb =  -2.766},
	{avol = 27, rdb =  -4.152},
	{avol = 26, rdb =  -5.522},
	{avol = 25, rdb =  -6.898},
	{avol = 24, rdb =  -8.288},
	{avol = 23, rdb =  -9.675},
	{avol = 22, rdb = -11.085},
	{avol = 21, rdb = -12.465},
	{avol = 20, rdb = -13.825},
	{avol = 19, rdb = -15.195},
	{avol = 18, rdb = -16.555},
	{avol = 17, rdb = -17.925},
	{avol = 16, rdb = -19.295},
	{avol = 15, rdb = -20.815},
	{avol = 14, rdb = -22.195},
	{avol = 13, rdb = -23.525},
	{avol = 12, rdb = -24.845},
	{avol = 11, rdb = -26.205},
	{avol = 10, rdb = -27.525},
	{avol =  9, rdb = -28.845},
	{avol =  8, rdb = -30.165},
	{avol =  7, rdb = -31.505},
	{avol =  6, rdb = -32.845},
	{avol =  5, rdb = -34.175},
	{avol =  4, rdb = -35.525},
	{avol =  3, rdb = -36.885},
	{avol =  2, rdb = -38.225},
	{avol =  1, rdb = -39.525},
	{avol =  0, rdb = -40.925},
};

com_vol_config.max_config_rdb = com_vol_config.table[1].rdb;
com_vol_config.min_config_rdb = -100; -- com_vol_config.table[#com_vol_config.table].rdb;
com_vol_config.max_dvol = 16384; -- 最大数字音量
com_vol_config.call_vol_levels = 15;