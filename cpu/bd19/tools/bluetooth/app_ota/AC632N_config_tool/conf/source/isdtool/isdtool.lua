if enable_moudles["isdtool"] == false then
	return
else


--[[=============================== 通用变量和函数 ================================--]]
local ISD_TYPE_NUM = 6;

local  common_comment_str1 = "#####################################################" .. NLINE_TABLE[1];

local function isd_output_comment(comment_cfg_name, comment_str)
    local comment_cfg = cfg:i32(comment_cfg_name, 0);
    comment_cfg:setTextOut(
        function (ty)
            if (ty == ISD_TYPE_NUM) then
			    return common_comment_str1 .. comment_str  .. common_comment_str1;
            end
        end)

    return comment_cfg;
end

local function isd_cfg_output_tag_str(tag_name)
    local tag_cfg = cfg:i32(tag_name, 0);
    tag_cfg:setTextOut(
        function (ty)
            if (ty == ISD_TYPE_NUM) then
			    return tag_name .. NLINE_TABLE[1];
            end
        end)

    return tag_cfg;
end

local function item_output_text(item, item_key_name, unmap_table, item_comment_str, tab_num, nline_num)
    item:setTextOut(
        function (ty)
            if (ty == ISD_TYPE_NUM) then
                if (unmap_table == nil) then
                    return item_key_name .. "=" .. item.val .. ";" .. TAB_TABLE[tab_num] .. "//" .. item_comment_str .. NLINE_TABLE[nline_num];
                else
                    return item_key_name .. "=" .. unmap_table[item.val] .. ";" .. TAB_TABLE[tab_num] .. "//" .. item_comment_str .. NLINE_TABLE[nline_num];
                end
            end
        end)
    return item;
end

local function item_output_hex_text(item, item_key_name, item_comment_str, tab_num, nline_num)
    item:setTextOut(
        function (ty)
            if (ty == ISD_TYPE_NUM) then
                    return item_key_name .. "=" .. item_val_dec2hex(item.val) .. ";" .. TAB_TABLE[tab_num] .. "//" .. item_comment_str .. NLINE_TABLE[nline_num];
                end
        end)
    return item;
end

local function isd_output_table_comment(comment_name, comment_table)
    local comment_cfg = cfg:i32(comment_name, 0);
    comment_cfg:setTextOut(
        function (ty)
            local output_str = "";
            if (ty == ISD_TYPE_NUM) then
                for k, value in pairs(comment_table) do
                    output_str = output_str .. value .. NLINE_TABLE[1];
                end
			    return output_str;
            end
        end)

    return comment_cfg;
end


--[[===================================================================================
=============================== 配置区域1：[EXTRA_CFG_PARAM] =========================
====================================================================================--]]
local extra_cfg_tag_str = "[EXTRA_CFG_PARAM]";
local extra_cfg_name = "EXTERA_CFG_PARAM 注释";
local extra_cfg_param_comment_str = "#   配置数据按照 长度+配置名字+数据的方式存储" .. NLINE_TABLE[1];
local extra_cfg_param_comment_output_text = isd_output_comment(extra_cfg_name, extra_cfg_param_comment_str);

local extra_cfg_tag_output_text = isd_cfg_output_tag_str(extra_cfg_tag_str);

--[[=============================== 配置项1-1：FLASH_FS ================================--]]
local flash_fs_talbe = {
    [0] = "NO",
    [1] = "YES",
};
local FLASH_FS_TABLE = cfg:enumMap("文件系统选择列表", flash_fs_talbe);
local extra_cfg_flash_fs = cfg:enum("FLASH FS 配置", FLASH_FS_TABLE, 1);
local extra_cfg_flash_fs_comment_str = "文件系统类型选择";
-- 输出text
extra_cfg_flash_fs_output_text = item_output_text(extra_cfg_flash_fs, "NEW_FLASH_FS", flash_fs_talbe, extra_cfg_flash_fs_comment_str, 1, 1);
-- 显示
local extra_cfg_flash_fs_hbox_view = cfg:hBox {
    cfg:stLabel(extra_cfg_flash_fs.name .. "："),
    cfg:enumView(extra_cfg_flash_fs),
    cfg:stLabel(extra_cfg_flash_fs_comment_str),
    cfg:stSpacer(),
};
extra_cfg_flash_fs:setShow(false);

--[[=============================== 配置项1-2：CHIP_NAME ================================--]]
local chip_name_talbe = {
    [0] = "AC693X",
};
local CHIP_NAME_TABLE = cfg:enumMap("芯片名称选择列表", chip_name_talbe);
local extra_cfg_chip_name = cfg:enum("芯片名称", CHIP_NAME_TABLE, 0);
local extra_cfg_chip_name_comment_str = "";
-- 输出text
extra_cfg_chip_name_output_text = item_output_text(extra_cfg_chip_name, "CHIP_NAME", chip_name_talbe, extra_cfg_chip_name_comment_str, 1, 1);
-- 显示
local extra_cfg_chip_name_hbox_view = cfg:hBox {
    cfg:stLabel(extra_cfg_chip_name.name .. "："),
    cfg:enumView(extra_cfg_chip_name),
    cfg:stLabel(extra_cfg_chip_name_comment_str),
    cfg:stSpacer(),
};
extra_cfg_chip_name:setShow(false);

--[[=============================== 配置项1-3：ENTRY ================================--]]
local extra_cfg_entry_addr = cfg:i32("ENTRY", 0x1D000C0);
local extra_cfg_entry_addr_comment_str = "程序入口地址";
-- 输出text
extra_cfg_entry_addr_output_text = item_output_hex_text(extra_cfg_entry_addr, "ENTRY", extra_cfg_entry_addr_comment_str, 1, 1);
-- 显示
local extra_cfg_entry_addr_hbox_view = cfg:hBox {
    cfg:stLabel(extra_cfg_entry_addr.name .. " = 0x"),
        cfg:hexInputView(extra_cfg_entry_addr),
    cfg:stLabel(extra_cfg_entry_addr_comment_str),
    cfg:stSpacer(),
};
extra_cfg_entry_addr:setShow(false);

--[[=============================== 配置项1-4：PID ================================--]]
local extra_cfg_pid = cfg:str("PID配置", "AC693X_TWS");
local extra_cfg_pid_comment_str = "长度16byte, 示例：芯片封装_应用方向_方案名称";
extra_cfg_pid_output_text = item_output_text(extra_cfg_pid, "PID", nil, extra_cfg_pid_comment_str, 2, 1);

-- 显示
local extra_cfg_pid_hbox_view = cfg:hBox {
    cfg:stLabel(extra_cfg_pid.name .. "："),
    cfg:inputMaxLenView(extra_cfg_pid, 16),
    cfg:stLabel(extra_cfg_pid_comment_str),
    cfg:stSpacer(),
};

--[[=============================== 配置项1-5：VID ================================--]]
local extra_cfg_vid = cfg:str("VID配置", "0.01");
local extra_cfg_vid_comment_str = "版本号4byte, 格式示例: 0.01";
extra_cfg_vid_output_text = item_output_text(extra_cfg_vid, "VID", nil, extra_cfg_vid_comment_str, 3, 2);

-- 显示
local extra_cfg_vid_hbox_view = cfg:hBox {
    cfg:stLabel(extra_cfg_vid.name .. "："),
    cfg:inputMaxLenView(extra_cfg_vid, 4),
    cfg:stLabel(extra_cfg_vid_comment_str),
    cfg:stSpacer(),
};

--[[=============================== 配置区域1汇总 ================================--]]
-- 显示
local extra_cfg_param_group_view = cfg:stGroup(extra_cfg_tag_str,
    cfg:vBox {
        --extra_cfg_flash_fs_hbox_view,
        extra_cfg_chip_name_hbox_view,
        extra_cfg_entry_addr_hbox_view,
        extra_cfg_pid_hbox_view,
        extra_cfg_vid_hbox_view,
    }
);

-- 输出text
local extra_cfg_output_text_list = {
    --extra_cfg_param_comment_output_text,
    extra_cfg_tag_output_text,
    extra_cfg_flash_fs_output_text,
    extra_cfg_chip_name_output_text,
    extra_cfg_entry_addr_output_text,
    extra_cfg_pid_output_text,
    extra_cfg_vid_output_text,
};


--[[===================================================================================
=============================== 配置区域2：[SYS_CFG_PARAM] =========================
====================================================================================--]]
local sys_cfg_tag_str = "[SYS_CFG_PARAM]";
local sys_cfg_name = "SYS_CFG_PARAM 注释";
local sys_cfg_param_comment_str = "#   UBOOT配置项, 请勿随意调整顺序" .. NLINE_TABLE[1];
local sys_cfg_param_comment_output_text = isd_output_comment(sys_cfg_name, sys_cfg_param_comment_str);
local sys_cfg_tag_output_text = isd_cfg_output_tag_str(sys_cfg_tag_str);


--[[============================ 配置区域2-1: SPI 配置 ==============================--]]
-- 0)注释
local spi_comment_table = {
    [0] = "#clk [3-255]",
    [1] = "#data_width[2 3 4], 3的时候uboot自动识别2或者4线",
    [2] = "#mode:",
    [3] = "#      0 RD_OUTPUT,      1 cmd        1 addr",
    [4] = "#      1 RD_I/O,         1 cmd        x addr",
    [6] = "#SPI = data_width, clk, mode",
};

local sys_cfg_spi_comment_output_text = isd_output_table_comment("SPI 配置注释", spi_comment_table);

-- 1)data_width
local spi_data_width_table = {
    [2] = " 2线 ";
    [3] = " 自动识别 ";
    [4] = " 4线 ";
};
local SPI_DATA_WIDTH_TABLE = cfg:enumMap("SPI数据宽度列表", spi_data_width_table);

local sys_cfg_spi_data_width = cfg:enum("spi data width", SPI_DATA_WIDTH_TABLE, 3);
-- 显示
local sys_cfg_spi_data_width_hbox_view = cfg:hBox {
    cfg:stLabel(sys_cfg_spi_data_width.name .. "："),
    cfg:enumView(sys_cfg_spi_data_width),
    cfg:stSpacer(),
};

-- 2)clk
local sys_cfg_spi_clock = cfg:i32("spi clk", 3);
-- 显示
local sys_cfg_spi_clock_hbox_view = cfg:hBox {
		cfg:stLabel(sys_cfg_spi_clock.name .. "："),
		cfg:ispinView(sys_cfg_spi_clock, 3, 255, 1),
        cfg:stLabel("(设置范围: 3 ~ 255)");
        cfg:stSpacer(),
};

-- 3)spi_mode
local spi_mode_table = {
    [0] = " RD_OUTPUT     1 cmd        1 addr ";
    [1] = " RD_I/O        1 cmd        x addr ";
};
local SPI_MODE_TABLE = cfg:enumMap("SPI模式列表", spi_mode_table);

local sys_cfg_spi_mode = cfg:enum("spi mode", SPI_MODE_TABLE, 1);
-- 显示
local sys_cfg_spi_mode_hbox_view = cfg:hBox {
    cfg:stLabel(sys_cfg_spi_mode.name .. "："),
    cfg:enumView(sys_cfg_spi_mode),
    cfg:stSpacer(),
};

--[[============================ 配置区域2-1: SPI 配置汇总 ==============================--]]
-- 显示
local sys_cfg_spi_group_view = cfg:stGroup("",
    cfg:vBox {
        sys_cfg_spi_data_width_hbox_view,
        sys_cfg_spi_clock_hbox_view,
        sys_cfg_spi_mode_hbox_view,
    }
);

-- 输出text, spi字段拼接
local sys_cfg_spi_output_text = cfg:i32("SPI", 1);
sys_cfg_spi_output_text:addDeps{sys_cfg_spi_data_width, sys_cfg_spi_clock, sys_cfg_spi_mode};
sys_cfg_spi_output_text:setTextOut(
    function (ty)
        if (ty == ISD_TYPE_NUM) then
            return "SPI=" .. sys_cfg_spi_data_width.val .. "_"  .. sys_cfg_spi_clock.val .. "_" .. sys_cfg_spi_mode.val .. ";" ..
            TAB_TABLE[1]  .. "#data_width,clk,mode" .. NLINE_TABLE[1];
        end
    end)

--[[=============================== 配置区域2-2: UART注释 ================================--]]
-- 1)注释
local uart_comment_table = {
    [0] = "#UTTX=PA05;     //uboot串口tx",
    [1] = "#UTBD=1000000;  //uboot串口波特率",
};

local sys_cfg_uart_comment_output_text = isd_output_table_comment("UART 串口配置", uart_comment_table);

-- 2)update_io
local uart_update_io_talbe = {
    [0] = "PB00",
    [1] = "PB05",
    [2] = "PA02",
};
local UART_UPDATE_IO_TABLE = cfg:enumMap("串口升级IO列表", uart_update_io_talbe);
local sys_cfg_uart_update_io_sel = cfg:enum("uart升级IO选择", UART_UPDATE_IO_TABLE, 1);
local sys_cfg_uart_update_io_sel_comment_str = "串口升级[PB00 PB05 PA02], 默认PB05";
-- 输出 text
sys_cfg_uart_update_io_sel_output_text = item_output_text(sys_cfg_uart_update_io_sel, "UTRX", uart_update_io_talbe, sys_cfg_uart_update_io_sel_comment_str, 1, 1);

-- 显示
local sys_cfg_uart_update_io_sel_group_view = cfg:stGroup("",
    cfg:hBox {
        cfg:stLabel(sys_cfg_uart_update_io_sel.name .. "："),
        cfg:enumView(sys_cfg_uart_update_io_sel),
        cfg:stSpacer(),
    }
);


--[[============================ 配置区域2-3: RESET 配置 ==============================--]]
-- 1)port
local reset_port_table = {
		[1]  = "PA01",
		[2]  = "PA02",
		[3]  = "PA03",
		[4]  = "PA04",
		[5]  = "PA05",
		[6]  = "PA06",
		[7]  = "PA07",
		[8]  = "PA08",
		[9]  = "PA09",
		[10]  = "PA10",

		[11] = "PB00", 
		[12] = "PB01",
		[13] = "PB02", 
		[14] = "PB03",
		[15] = "PB04",
		[16] = "PB05",
		[17] = "PB06",
		[18] = "PB07",
		[19] = "PB08",

		[20] = "USB_DP",
		[21] = "USB_DM",
};

local RESET_PORT_TABLE = cfg:enumMap("长按复位IO列表", reset_port_table);

local sys_cfg_reset_port_sel = cfg:enum("长按复位port口选择", RESET_PORT_TABLE, 12);
-- 显示
local sys_cfg_reset_port_sel_hbox_view = cfg:hBox {
    cfg:stLabel(sys_cfg_reset_port_sel.name .. "："),
    cfg:enumView(sys_cfg_reset_port_sel),
    cfg:stSpacer(),
};

-- 2)长按复位时间
local reset_time_show_table = {
    [0] = "关闭长按复位功能",
    [1] = "长按4秒复位",
    [2] = "长按8秒复位",
};

local reset_time_output_table = {
    [0] = "00",
    [1] = "04",
    [2] = "08",
};

local RESET_TIME_TABLE = cfg:enumMap("长按复位时间列表", reset_time_show_table);

local sys_cfg_reset_time_sel = cfg:enum("长按复位时间", RESET_TIME_TABLE, 2);
-- 显示
local sys_cfg_reset_time_sel_hbox_view = cfg:hBox {
    cfg:stLabel(sys_cfg_reset_time_sel.name .. "："),
    cfg:enumView(sys_cfg_reset_time_sel),
    cfg:stSpacer(),
};

-- 3)长按复位电平
local reset_logic_table = {
    [0] = "低电平复位",
    [1] = "高电平复位",
};

local RESET_LOGIC_TABLE = cfg:enumMap("长按复位时间列表", reset_logic_table);

local sys_cfg_reset_logic_sel = cfg:enum("长按复位电平", RESET_LOGIC_TABLE, 0);
-- 显示
local sys_cfg_reset_logic_sel_hbox_view = cfg:hBox {
    cfg:stLabel(sys_cfg_reset_logic_sel.name .. "："),
    cfg:enumView(sys_cfg_reset_logic_sel),
    cfg:stSpacer(),
};

--[[============================ 配置区域2-3: RESET 配置汇总 ==============================--]]
-- 显示
local sys_cfg_reset_group_view = cfg:stGroup("",
    cfg:vBox {
        sys_cfg_reset_port_sel_hbox_view,
        sys_cfg_reset_time_sel_hbox_view,
        sys_cfg_reset_logic_sel_hbox_view,
    }
);

-- 输出text, reset字段拼接
local sys_cfg_reset_output_text = cfg:i32("RESET", 1);
sys_cfg_reset_output_text:addDeps{sys_cfg_reset_port_sel, sys_cfg_reset_time_sel, sys_cfg_reset_logic_sel};
sys_cfg_reset_output_text:setTextOut(
    function (ty)
        if (ty == ISD_TYPE_NUM) then
            return "RESET=" .. reset_port_table[sys_cfg_reset_port_sel.val] .. "_"  .. reset_time_output_table[sys_cfg_reset_time_sel.val] .. "_" .. sys_cfg_reset_logic_sel.val .. ";" ..
            TAB_TABLE[1]  .. "//port口_长按时间_有效电平(长按时间有00、04、08三个值可选，单位为秒，当长按时间为00时，则关闭长按复位功能)" .. NLINE_TABLE[2];
        end
    end)

--[[=============================== 配置区域2汇总 ================================--]]
-- 显示
local sys_cfg_param_group_view = cfg:stGroup(sys_cfg_tag_str,
    cfg:vBox {
        sys_cfg_spi_group_view,
        sys_cfg_uart_update_io_sel_group_view,
        sys_cfg_reset_group_view,
    }
);

-- 输出text
local sys_cfg_output_text_list = {
    sys_cfg_tag_output_text,
    sys_cfg_param_comment_output_text,
    sys_cfg_spi_comment_output_text,
    sys_cfg_spi_output_text,
    sys_cfg_uart_comment_output_text,
    sys_cfg_uart_update_io_sel_output_text,
    sys_cfg_reset_output_text,
};

--[[============================ 配置区域3: RESERVED 配置 ==============================--]]
local reserved_cfg_tag_str = "[RESERVED_CONFIG]";
-- 0)注释
local reserved_comment_table = {
    [0] = "###########################flash空间使用配置区域############################",
    [1] = "#PDCTNAME:    产品名，对应此代码，用于标识产品，升级时可以选择匹配产品名",
    [2] = "#BOOT_FIRST:  1=代码更新后，提示APP是第一次启动；0=代码更新后，不提示",
    [3] = "#UPVR_CTL：   0：不允许高版本升级低版本   1：允许高版本升级低版本",
    [4] = "#XXXX_ADR:    区域起始地址AUTO：由工具自动分配起始地址",
    [5] = "#XXXX_LEN:    区域长度CODE_LEN：代码长度",
    [6] = "#XXXX_OPT:    区域操作属性",
    [7] = "#",
    [8] = "#",
    [9] = "#",
    [10] = "#操作符说明  OPT:",
    [11] = "#   0:  下载代码时擦除指定区域",
    [12] = "#   1:  下载代码时擦除指定区域1:  下载代码时不操作指定区域",
    [13] = "#   2:  下载代码时不操作指定区域2:  下载代码时给指定区域加上保护",
    [14] = "############################################################################",
};

local reserved_cfg_param_comment_output_text = isd_output_table_comment("预留区域配置注释", reserved_comment_table);
local reserved_cfg_tag_output_text = isd_cfg_output_tag_str(reserved_cfg_tag_str);

-- 1)common reserved_config
local reserved_cfg_const_table = {
    [0] = "BTIF_ADR=AUTO;",
    [1] = "BTIF_LEN=0x1000;",
    [2] = "BTIF_OPT=1;",
    --[4] = "WTIF_ADR=BEGIN_END;",
    --[5] = "WTIF_LEN=0x1000;",
    --[6] = "WTIF_OPT=1;",
    [3] = "",
    [4] = "PRCT_ADR=0;",
    [5] = "PRCT_LEN=CODE_LEN;",
    [6] = "PRCT_OPT=2;",
    [7] = "",
};

local reserved_cfg_const_output_text = isd_output_table_comment("预留通用信息", reserved_cfg_const_table);

-- 1)vm cfg
-- VM_ADR
local reserved_cfg_vm_adr = cfg:i32("VM_ADR", 0);
local reserved_cfg_vm_adr_output_text = item_output_text(reserved_cfg_vm_adr, "VM_ADR", nil, "", 1, 1);
-- 显示
local reserved_cfg_vm_adr_hbox_view = cfg:hBox {
    cfg:stLabel(reserved_cfg_vm_adr.name .. "="),
	cfg:ispinView(reserved_cfg_vm_adr, 0, 255, 1),
    cfg:stSpacer(),
};
reserved_cfg_vm_adr:setShow(false);

-- VM_LEN
local reserved_cfg_vm_len = cfg:str("VM_LEN", "4K");
local reserved_cfg_vm_len_comment_str = "支持格式: 512 或 4K";
local reserved_cfg_vm_len_output_text = item_output_text(reserved_cfg_vm_len, "VM_LEN", nil, reserved_cfg_vm_len_comment_str, 1, 1);
-- 显示
local reserved_cfg_vm_len_hbox_view = cfg:hBox {
    cfg:stLabel(reserved_cfg_vm_len.name .. "="),
    cfg:inputMaxLenView(reserved_cfg_vm_len, 10),
    cfg:stLabel(reserved_cfg_vm_len_comment_str),
    cfg:stSpacer(),
};

-- VM_OPT
local vm_opt_table = {
    [0] = " 升级代码时擦除VM区域",
    [1] = " 升级代码时不操作VM区域",
    [2] = " 升级代码时保护VM区域",
};
local VM_OPT_TABLE = cfg:enumMap("VM区域操作列表", vm_opt_table);
local reserved_cfg_vm_opt = cfg:enum("VM_OPT", VM_OPT_TABLE, 1);
local reserved_cfg_vm_opt_output_text = item_output_text(reserved_cfg_vm_opt, "VM_OPT", nil, "", 1, 2);
-- 显示
local reserved_cfg_vm_opt_hbox_view = cfg:hBox {
    cfg:stLabel(reserved_cfg_vm_opt.name .. "="),
    cfg:enumView(reserved_cfg_vm_opt),
    cfg:stSpacer(),
};

--[[=============================== 配置区域3汇总 ================================--]]
-- 显示
local reserved_cfg_param_group_view = cfg:stGroup(reserved_cfg_tag_str,
    cfg:vBox {
        reserved_cfg_vm_adr_hbox_view,
        reserved_cfg_vm_len_hbox_view,
        reserved_cfg_vm_opt_hbox_view,
    }
);

-- 输出text
local reserved_cfg_output_text_list = {
    reserved_cfg_tag_output_text,
    reserved_cfg_param_comment_output_text,
    reserved_cfg_const_output_text,
    reserved_cfg_vm_adr_output_text,
    reserved_cfg_vm_len_output_text,
    reserved_cfg_vm_opt_output_text,
};

--[[============================ 配置区域4: RESERVED 配置 ==============================--]]
local burner_cfg_tag_str = "[BURNER_CONFIG]";
local burner_cfg_tag_output_text = isd_cfg_output_tag_str(burner_cfg_tag_str);
local burner_cfg_const_table = {
    [0] = "SIZE=32;",
};
local burner_cfg_const_output_text = isd_output_table_comment("烧写器通用信息", burner_cfg_const_table);

--[[=============================== 配置区域4汇总 ================================--]]
-- 显示: 无
-- 输出text
local burner_cfg_output_text_list = {
    burner_cfg_tag_output_text,
    burner_cfg_const_output_text,
};


--[[===================================================================================
==================================== 模块返回汇总 =====================================
====================================================================================--]]
-- A. 输出htext
local isdtool_cfg_output_text_list = {};
insert_list_to_list(isdtool_cfg_output_text_list, extra_cfg_output_text_list);
insert_list_to_list(isdtool_cfg_output_text_list, sys_cfg_output_text_list);
insert_list_to_list(isdtool_cfg_output_text_list, reserved_cfg_output_text_list);
insert_list_to_list(isdtool_cfg_output_text_list, burner_cfg_output_text_list);

-- B. 输出ctext：无
-- C. 输出bin：无
-- E. 默认值
local isd_cfg_default_table = {
    extra_cfg_entry_addr,
    extra_cfg_pid,
    extra_cfg_vid,
    sys_cfg_spi_data_width,
    sys_cfg_spi_clock,
    sys_cfg_spi_mode,
    sys_cfg_uart_update_io_sel,
    sys_cfg_reset_port_sel,
    sys_cfg_reset_time_sel,
    sys_cfg_reset_logic_sel,
    reserved_cfg_vm_len,
    reserved_cfg_vm_opt,
};
local isd_default_button_view = cfg:stButton(" 恢复ISD配置默认值 ", reset_to_default(isd_cfg_default_table));

-- D. 显示
local function save_ini_file()
    local ini_file_path = ini_out_path .. 'isd_config.ini';
    cfg:savePartTextFile(ini_file_path, ISD_TYPE_NUM, isdtool_cfg_output_text_list);
    cfg:msgBox("info", "ini 文件保存在路径: " .. ini_file_path);
end

local isd_save_file_button_view = cfg:stButton("保存ini配置", save_ini_file);


local isd_view_table = {
    extra_cfg_param_group_view,
    sys_cfg_param_group_view,
    reserved_cfg_param_group_view,
};

isdtool_view = {"ISD 配置",
    cfg:vBox {
        cfg:stHScroll(cfg:vBox {
                cfg:vBox(isd_view_table),
                cfg:hBox {
                    isd_default_button_view,
                    isd_save_file_button_view,
                };
            }
        );
    }
};

-- F. bindGroup：无

end


