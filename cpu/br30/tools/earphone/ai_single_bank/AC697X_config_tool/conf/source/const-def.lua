
--[[
--BR22 IO口可供选择:
--PORTA: PA1 ~ PA10
--PORTB: PB0 ~ PB8
--usb_dp usb_dm
--]]
PORTS = cfg:enumMap("BR22引脚", 
	{
		[0xFF] = "NULL", 
--[[
		[0]  = "PORT_A_0",
--]]
		[1]  = "PORT_A_1",
		[2]  = "PORT_A_2", 
		[3]  = "PORT_A_3",
		[4]  = "PORT_A_4",
		[5]  = "PORT_A_5",
		[6]  = "PORT_A_6",
		[7]  = "PORT_A_7",
		[8]  = "PORT_A_8",
		[9]  = "PORT_A_9",
		[10] = "PORT_A_10",
--[[
		[11] = "PORT_A_11",
		[12] = "PORT_A_12",
		[13] = "PORT_A_13",
		[14] = "PORT_A_14",
		[15] = "PORT_A_15",
--]]

		[16] = "PORT_B_0", 
		[17] = "PORT_B_1",
		[18] = "PORT_B_2", 
		[19] = "PORT_B_3",
		[20] = "PORT_B_4",
		[21] = "PORT_B_5",
		[22] = "PORT_B_6",
		[23] = "PORT_B_7",
		[24] = "PORT_B_8",
--[[
		[25] = "PORT_B_9",
		[26] = "PORT_B_10",
		[27] = "PORT_B_11",
		[28] = "PORT_B_12",
		[29] = "PORT_B_13",
		[30] = "PORT_B_14",
		[31] = "PORT_B_15",

		[32] = "PORT_C_0", 
		[33] = "PORT_C_1",
		[34] = "PORT_C_2", 
		[35] = "PORT_C_3",
		[36] = "PORT_C_4",
		[37] = "PORT_C_5",
		[38] = "PORT_C_6",
		[39] = "PORT_C_7",
		[40] = "PORT_C_8",
		[41] = "PORT_C_9",
		[42] = "PORT_C_10",
		[43] = "PORT_C_11",
		[44] = "PORT_C_12",
		[45] = "PORT_C_13",
		[46] = "PORT_C_14",
		[47] = "PORT_C_15",

		[48] = "PORT_D_0", 
		[49] = "PORT_D_1",
		[50] = "PORT_D_2", 
		[51] = "PORT_D_3",
		[52] = "PORT_D_4",
		[53] = "PORT_D_5",
		[54] = "PORT_D_6",
		[55] = "PORT_D_7",
		[56] = "PORT_D_8",
		[57] = "PORT_D_9",
		[58] = "PORT_D_10",
		[59] = "PORT_D_11",
		[60] = "PORT_D_12",
		[61] = "PORT_D_13",
		[62] = "PORT_D_14",
		[63] = "PORT_D_15",
--]]
		[61] = "PORT_USB_DP",
		[62] = "PORT_USB_DM",


	}
)

PORTS_TABLE = 
{
		[0xFF] = "NO_CONFIG_PORT", 
		[0]  = "IO_PORTA_00", 
		[1]  = "IO_PORTA_01",
		[2]  = "IO_PORTA_02", 
		[3]  = "IO_PORTA_03",
		[4]  = "IO_PORTA_04",
		[5]  = "IO_PORTA_05",
		[6]  = "IO_PORTA_06",
		[7]  = "IO_PORTA_07",
		[8]  = "IO_PORTA_08",
		[9]  = "IO_PORTA_09",
		[10] = "IO_PORTA_10",
		[11] = "IO_PORTA_11",
		[12] = "IO_PORTA_12",
		[13] = "IO_PORTA_13",
		[14] = "IO_PORTA_14",
		[15] = "IO_PORTA_15",

		[16] = "IO_PORTB_00", 
		[17] = "IO_PORTB_01",
		[18] = "IO_PORTB_02", 
		[19] = "IO_PORTB_03",
		[20] = "IO_PORTB_04",
		[21] = "IO_PORTB_05",
		[22] = "IO_PORTB_06",
		[23] = "IO_PORTB_07",
		[24] = "IO_PORTB_08",
		[25] = "IO_PORTB_09",
		[26] = "IO_PORTB_10",
		[27] = "IO_PORTB_11",
		[28] = "IO_PORTB_12",
		[29] = "IO_PORTB_13",
		[30] = "IO_PORTB_14",
		[31] = "IO_PORTB_15",

		[32] = "IO_PORTC_00", 
		[33] = "IO_PORTC_01",
		[34] = "IO_PORTC_02", 
		[35] = "IO_PORTC_03",
		[36] = "IO_PORTC_04",
		[37] = "IO_PORTC_05",
		[38] = "IO_PORTC_06",
		[39] = "IO_PORTC_07",
		[40] = "IO_PORTC_08",
		[41] = "IO_PORTC_09",
		[42] = "IO_PORTC_10",
		[43] = "IO_PORTC_11",
		[44] = "IO_PORTC_12",
		[45] = "IO_PORTC_13",
		[46] = "IO_PORTC_14",
		[47] = "IO_PORTC_15",

		[48] = "IO_PORTD_00", 
		[49] = "IO_PORTD_01",
		[50] = "IO_PORTD_02", 
		[51] = "IO_PORTD_03",
		[52] = "IO_PORTD_04",
		[53] = "IO_PORTD_05",
		[54] = "IO_PORTD_06",
		[55] = "IO_PORTD_07",
--[[
		[56] = "IO_PORTD_08",
		[57] = "IO_PORTD_09",
		[58] = "IO_PORTD_10",
		[59] = "IO_PORTD_11",
		[60] = "IO_PORTD_12",
		[61] = "IO_PORTD_13",
		[62] = "IO_PORTD_14",
		[63] = "IO_PORTD_15",
--]]
		[61] = "IO_PORT_DP",
		[62] = "IO_PORT_DM",
}

PORTS_TABLE_ISD_TOOL = 
{
		[0xFF] = "NO_CONFIG_PORT", 
		[0]  = "PA00", 
		[1]  = "PA01",
		[2]  = "PA02", 
		[3]  = "PA03",
		[4]  = "PA04",
		[5]  = "PA05",
		[6]  = "PA06",
		[7]  = "PA07",
		[8]  = "PA08",
		[9]  = "PA09",
		[10] = "PA10",
		[11] = "PA11",
		[12] = "PA12",
		[13] = "PA13",
		[14] = "PA14",
		[15] = "PA15",

		[16] = "PB00", 
		[17] = "PB01",
		[18] = "PB02", 
		[19] = "PB03",
		[20] = "PB04",
		[21] = "PB05",
		[22] = "PB06",
		[23] = "PB07",
		[24] = "PB08",
		[25] = "PB09",
		[26] = "PB10",
		[27] = "PB11",
		[28] = "PB12",
		[29] = "PB13",
		[30] = "PB14",
		[31] = "PB15",

		[32] = "PC00", 
		[33] = "PC01",
		[34] = "PC02", 
		[35] = "PC03",
		[36] = "PC04",
		[37] = "PC05",
		[38] = "PC06",
		[39] = "PC07",
		[40] = "PC08",
		[41] = "PC09",
		[42] = "PC10",
		[43] = "PC11",
		[44] = "PC12",
		[45] = "PC13",
		[46] = "PC14",
		[47] = "PC15",

		[48] = "PD00", 
		[49] = "PD01",
		[50] = "PD02", 
		[51] = "PD03",
		[52] = "PD04",
		[53] = "PD05",
		[54] = "PD06",
		[55] = "PD07",
--[[
		[56] = "PD08",
		[57] = "PD09",
		[58] = "PD10",
		[59] = "PD11",
		[60] = "PD12",
		[61] = "PD13",
		[62] = "PD14",
		[63] = "PD15",
--]]
		[61] = "USBDP",
		[62] = "USBDM",
}


--用于输出字符串对齐
TAB_TABLE = {
    [0] = "",
    [1] = "\t",
    [2] = "\t\t",
    [3] = "\t\t\t",
    [4] = "\t\t\t\t",
    [5] = "\t\t\t\t\t",
    [6] = "\t\t\t\t\t\t",
    [7] = "\t\t\t\t\t\t\t",
    [8] = "\t\t\t\t\t\t\t\t",
    [9] = "\t\t\t\t\t\t\t\t\t",
}

NLINE_TABLE = {
    [1] = "\n",
    [2] = "\n\n",
    [3] = "\n\n\n",
    [4] = "\n\n\n\n",
}

--用于注释类
COMMMENT_LINE = "//*********************************************************************************//"
--"//*********************************************************************************//"

enable_switch_table = {
    [0] = "不使能",
    [1] = "使能",
};

ENABLE_SWITCH = cfg:enumStr("开关", {"OFF", "ON"})	-- 0: 关闭, 1:打开

ENABLE_SWITCH_TABLE = {
    [0] = "DISABLE_THIS_MODULE",
    [1] = "ENABLE_THIS_MODULE",
}

FUNCTION_SWITCH_TABLE = {
    [0] = "DISABLE",
    [1] = "ENABLE",
}



