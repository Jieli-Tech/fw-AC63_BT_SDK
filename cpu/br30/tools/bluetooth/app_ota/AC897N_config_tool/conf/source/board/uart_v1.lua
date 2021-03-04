

local UART_BAUDRATE = cfg:enumInt("串口波特率", {9600, 115200, 460800, 1000000, 1500000})


local MOUDLE_COMMENT_NAME = "//                                 UART配置                                        //"

local comment_begin = cfg:i32("uart注释开始", 0);
local uart_comment_begin_htext = module_output_comment(comment_begin, MOUDLE_COMMENT_NAME);

--print(MOUDLE_COMMENT_NAME);
--[[=============================== 配置项列表 ================================--]]
local uart0_en;
local uart0_en_view;
local uart0_rx_port;
local uart0_rx_port_view;
local uart0_tx_port;
local uart0_tx_port_view;
local uart0_baudrate;
local uart0_baudrate_view;
local uart0_view_list;
local uart0_group_view;

local uart1_en;
local uart1_en_view;
local uart1_rx_port;
local uart1_rx_port_view;
local uart1_tx_port;
local uart1_tx_port_view;
local uart1_baudrate;
local uart1_baudrate_view;
local uart1_view_list;
local uart1_group_view;

--[[
//==========================================================================================//
//                                  配置项: UART_CFG                                        //
//==========================================================================================//
--]]

--[[=============================== 配置子项1-0: uart0_en ================================--]]
uart0_en = cfg:enum("UART0 使能开关:" .. TAB_TABLE[2], ENABLE_SWITCH, 1)
uart0_en:setTextOut(
    function (ty)  
        if (ty == 3) then
            if enable_moudles["uart0"] == false then
                return "#define TCFG_UART0_ENABLE" .. TAB_TABLE[5] .. ENABLE_SWITCH_TABLE[0] .. NLINE_TABLE[2]
            end
            return "#define TCFG_UART0_ENABLE" .. TAB_TABLE[5] .. ENABLE_SWITCH_TABLE[uart0_en.val] .. NLINE_TABLE[1]
        end
    end
)

--item_output_htext(uart0_en, "#define TCFG_UART0_ENABLE", 5, ENABLE_SWITCH_TABLE[uart0_en.val], 1);

uart0_en_view = enum_item_hbox_view(uart0_en);


--[[=============================== 配置子项1-1: uart0_rx_port ================================--]]
uart0_rx_port = cfg:enum("UART_0 RX 引脚选择:" .. TAB_TABLE[1], PORTS, 0xFF);
uart0_rx_port:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_UART0_RX_PORT  " .. TAB_TABLE[4] .. PORTS_TABLE[uart0_rx_port.val] .. NLINE_TABLE[1]
		end
	end
)
depend_item_en_show(uart0_en, uart0_rx_port); --显示依赖
uart0_rx_port_view = enum_item_hbox_view(uart0_rx_port);  -- 子项显示




--[[=============================== 配置子项1-2: uart0_tx_port ================================--]]
uart0_tx_port = cfg:enum("UART_0 TX 引脚选择:" .. TAB_TABLE[1], PORTS, 5)
uart0_tx_port:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_UART0_TX_PORT  " .. TAB_TABLE[4] ..PORTS_TABLE[uart0_tx_port.val] .. NLINE_TABLE[1]
		end
	end
)
depend_item_en_show(uart0_en, uart0_tx_port);
uart0_tx_port_view = enum_item_hbox_view(uart0_tx_port);  -- 子项显示




--[[=============================== 配置子项1-3: uart0_baudrate ================================--]]
uart0_baudrate = cfg:enum("UART_0 波特率选择:" .. TAB_TABLE[1], UART_BAUDRATE, 1000000)
uart0_baudrate:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_UART0_BAUDRATE" .. TAB_TABLE[5] .. uart0_baudrate.val .. NLINE_TABLE[2]
		end
	end
)
depend_item_en_show(uart0_en, uart0_baudrate);
uart0_baudrate_view = enum_item_hbox_view(uart0_baudrate);  -- 子项显示


uart0_view_list = cfg:vBox {
        uart0_en_view,
        uart0_rx_port_view,
        uart0_tx_port_view,
        uart0_baudrate_view,
};


uart0_group_view = cfg:stGroup("UART0 配置",
        uart0_view_list
);


--[[=============================== 配置子项2-0: uart1_en ================================--]]
uart1_en = cfg:enum("UART1 使能开关:" .. TAB_TABLE[2], ENABLE_SWITCH, 0)
uart1_en:setTextOut(
	function (ty) 
		if (ty == 3) then
            if enable_moudles["uart1"] == false then
                return "#define TCFG_UART1_ENABLE" .. TAB_TABLE[5] .. ENABLE_SWITCH_TABLE[0] .. NLINE_TABLE[2]
            end

			return "#define TCFG_UART1_ENABLE" .. TAB_TABLE[5] .. ENABLE_SWITCH_TABLE[uart1_en.val] .. NLINE_TABLE[1]
		end
	end
)
-- 子项显示
uart1_en_view = enum_item_hbox_view(uart1_en);



--[[=============================== 配置子项2-1: uart1_rx_port ================================--]]
local uart1_rx_port = cfg:enum("UART_1 RX 引脚选择:" .. TAB_TABLE[1], PORTS, 5)
uart1_rx_port:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_UART1_RX_PORT" .. TAB_TABLE[5] .. PORTS_TABLE[uart1_rx_port.val] .. NLINE_TABLE[1]
		end
	end
)
depend_item_en_show(uart1_en, uart1_rx_port); --显示依赖
uart1_rx_port_view = enum_item_hbox_view(uart1_rx_port);  -- 子项显示


--[[=============================== 配置子项2-2: uart1_tx_port ================================--]]
local uart1_tx_port = cfg:enum("UART_1 TX 引脚选择:" .. TAB_TABLE[1], PORTS, 5)
uart1_tx_port:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_UART1_TX_PORT  " .. TAB_TABLE[4] ..PORTS_TABLE[uart1_tx_port.val] .. NLINE_TABLE[1]
		end
	end
)
depend_item_en_show(uart1_en, uart1_tx_port); --显示依赖
uart1_tx_port_view = enum_item_hbox_view(uart1_tx_port);  -- 子项显示


--[[=============================== 配置子项2-3: uart1_baudrate ================================--]]
local uart1_baudrate = cfg:enum("UART_1 波特率选择:" .. TAB_TABLE[1], UART_BAUDRATE, 1000000)
uart1_baudrate:setOSize(4)
uart1_baudrate:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_UART1_BAUDRATE" .. TAB_TABLE[5] .. uart1_baudrate.val .. NLINE_TABLE[2]
		end
	end
)

depend_item_en_show(uart1_en, uart1_baudrate); --显示依赖
uart1_baudrate_view = enum_item_hbox_view(uart1_baudrate);  -- 子项显示


uart1_view_list = cfg:vBox {
        uart1_en_view,
        uart1_rx_port_view,
        uart1_tx_port_view,
        uart1_baudrate_view,
};

uart1_group_view = cfg:stGroup("UART1 配置",
        uart1_view_list
);

--[[===================================================================================
==================================== 模块返回汇总 =====================================
====================================================================================--]]
-- A. 输出htext
local uart_output_htext = {};
insert_item_to_list(uart_output_htext, uart_comment_begin_htext);
 
insert_item_to_list(uart_output_htext, uart0_en);

if enable_moudles["uart0"] == true then
    insert_item_to_list(uart_output_htext, uart0_rx_port);
    insert_item_to_list(uart_output_htext, uart0_tx_port);
    insert_item_to_list(uart_output_htext, uart0_baudrate);
end

insert_item_to_list(uart_output_htext, uart1_en);
if enable_moudles["uart1"] == true then
    insert_item_to_list(uart_output_htext, uart1_rx_port);
    insert_item_to_list(uart_output_htext, uart1_tx_port);
    insert_item_to_list(uart_output_htext, uart1_baudrate);
end

insert_list_to_list(board_output_text_tabs, uart_output_htext);

if (enable_moudles["uart0"] == false) and (enable_moudles["uart1"] == false) then
    return;
end

-- B. 输出ctext：无
-- C. 输出bin：无

-- E. 默认值
local uart_cfg_default_table = {};

if enable_moudles["uart0"] == true then
    insert_item_to_list(uart_cfg_default_table, uart0_en);
    insert_item_to_list(uart_cfg_default_table, uart0_rx_port);
    insert_item_to_list(uart_cfg_default_table, uart0_tx_port);
    insert_item_to_list(uart_cfg_default_table, uart0_baudrate);
end

if enable_moudles["uart1"] == true then
    insert_item_to_list(uart_cfg_default_table, uart1_en);
    insert_item_to_list(uart_cfg_default_table, uart1_rx_port);
    insert_item_to_list(uart_cfg_default_table, uart1_tx_port);
    insert_item_to_list(uart_cfg_default_table, uart1_baudrate);
end

local uart_default_button_view = cfg:stButton(" UART配置恢复默认值 ", reset_to_default(uart_cfg_default_table));

-- D. 显示
local uart_module_view_table = {};
if enable_moudles["uart0"] == true then
    insert_item_to_list(uart_module_view_table, uart0_group_view);
end

if enable_moudles["uart1"] == true then
    insert_item_to_list(uart_module_view_table, uart1_group_view);
end

local uart_view_list = cfg:vBox {
    cfg:stHScroll(cfg:vBox(uart_module_view_table)),
	uart_default_button_view,
};


local uart_view = {"UART 配置",
	uart_view_list,
};

if open_by_program == "create" then
    insert_item_to_list(board_view_tabs, uart_view);
end

-- F. bindGroup：无


