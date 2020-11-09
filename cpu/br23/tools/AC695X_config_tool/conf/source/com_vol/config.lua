
com_vol_config = {}

-- 数据表
com_vol_config.table = {
	{avol = 30, rdb =   0.000},
	{avol = 29, rdb =  -1.377},
	{avol = 28, rdb =  -2.768},
	{avol = 27, rdb =  -4.139},
	{avol = 26, rdb =  -5.516},
	{avol = 25, rdb =  -6.898},
	{avol = 24, rdb =  -8.292},
	{avol = 23, rdb =  -9.674},
	{avol = 22, rdb = -11.083},
	{avol = 21, rdb = -12.467},
	{avol = 20, rdb = -13.821},
	{avol = 19, rdb = -15.187},
	{avol = 18, rdb = -16.544},
	{avol = 17, rdb = -17.910},
	{avol = 16, rdb = -19.295},
	{avol = 15, rdb = -21.048},
	{avol = 14, rdb = -22.434},
	{avol = 13, rdb = -23.759},
	{avol = 12, rdb = -25.067},
	{avol = 11, rdb = -26.422},
	{avol = 10, rdb = -27.757},
	{avol =  9, rdb = -29.079},
	{avol =  8, rdb = -30.400},
	{avol =  7, rdb = -31.755},
	{avol =  6, rdb = -33.112},
	{avol =  5, rdb = -34.446},
	{avol =  4, rdb = -35.828},
	{avol =  3, rdb = -37.167},
	{avol =  2, rdb = -38.485},
	{avol =  1, rdb = -39.787},
	{avol =  0, rdb = -41.207},
};

com_vol_config.max_config_rdb = com_vol_config.table[1].rdb;
com_vol_config.min_config_rdb = -100; -- com_vol_config.table[#com_vol_config.table].rdb;
com_vol_config.max_dvol = 16384; -- 最大数字音量
com_vol_config.call_vol_levels = 15;