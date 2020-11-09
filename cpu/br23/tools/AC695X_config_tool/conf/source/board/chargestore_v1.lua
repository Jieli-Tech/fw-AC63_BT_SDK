
local MOUDLE_COMMENT_NAME = "//                                  充电仓配置                                     //"

local comment_begin = cfg:i32("chargestore注释开始", 0)
local chargestore_comment_begin_htext = module_output_comment(comment_begin, MOUDLE_COMMENT_NAME);
--print(MOUDLE_COMMENT_NAME);

CHARGESTORE_PORTS = cfg:enumMap("BR22引脚", 
	{
		[-1] = "NULL", 
		[2]  = "PORT_A_02", 
		[16] = "PORT_B_00", 
		[21] = "PORT_B_05",
	}
)


local CHARGESTORE_UARTIRQ = cfg:enumMap("充电仓串口", 
                                {
                                    [18] = "UART0",
                                    [19] = "UART1",
                                    [20] = "UART2",
                                })

local CHARGESTORE_UARTIRQ_OUTPUT_TABLE = {
        [18] = "IRQ_UART0_IDX",
        [19] = "IRQ_UART1_IDX",
        [20] = "IRQ_UART2_IDX",
}

local MOS33_CONNECT_DISPLAY = cfg:enumMap("MOS3.3管连接",
                                {
                                    [0] = "MOS 管需要IO输出高电平",
                                    [1] = "MOS 管接 VDDIO",
                                })

local MOS33_CONNECT_OUTPUT_TABLE = {
        [0] = "0",
        [1] = "1",
}



--[[
//==========================================================================================//
//                                  配置项: CHARGESTORE_CFG                                 //
//==========================================================================================//
--]]
--[[=============================== 配置子项1-0: chargestore_en ================================--]]
local en = cfg:enum("充电仓配置使能开关:", ENABLE_SWITCH, 0)
en:setOSize(1)
en:setTextOut(
	function (ty)
		if (ty == 3) then
			return "#define TCFG_CHARGESTORE_ENABLE" .. TAB_TABLE[4] .. ENABLE_SWITCH_TABLE[en.val] .. NLINE_TABLE[1]
		end
	end
)

-- 子项显示
local chargestore_en_view = cfg:hBox {
            cfg:stLabel(en.name),
            cfg:enumView(en),
            cfg:stSpacer(),
};


--[[=============================== 配置子项1-1: mos33_use_vddio ================================--]]
local mos33_use_vddio = cfg:enum("MOS3.3管设置:" .. TAB_TABLE[1], MOS33_CONNECT_DISPLAY, 1)
mos33_use_vddio:setOSize(1)
mos33_use_vddio:setTextOut(
	function (ty)
		if (ty == 3) then
			--return "#define TCFG_MOS33_USE_VDDIO" .. TAB_TABLE[3] .. MOS33_CONNECT_OUTPUT_TABLE[mos33_use_vddio.val] .. NLINE_TABLE[1]
			return "#define TCFG_MOS33_USE_VDDIO" .. TAB_TABLE[4] .. MOS33_CONNECT_OUTPUT_TABLE[1] .. NLINE_TABLE[1] --BR22不改, 直接设置1
		end
	end
)

mos33_use_vddio:addDeps{en}
mos33_use_vddio:setDepChangeHook(function ()
    if (en.val == 0) then
        mos33_use_vddio:setShow(false);
    else
        mos33_use_vddio:setShow(false);
    end
end);

-- 子项显示
local mos33_use_vddio_view = cfg:hBox {
            cfg:stLabel(mos33_use_vddio.name),
            cfg:enumView(mos33_use_vddio),
            cfg:stSpacer(),
};


--[[=============================== 配置子项1-2: port_sel ================================--]]
local port = cfg:enum("充电仓引脚选择:" .. TAB_TABLE[1], CHARGESTORE_PORTS, 2)
port:setOSize(1)
port:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_CHARGESTORE_PORT" .. TAB_TABLE[4] .. PORTS_TABLE[port.val] .. NLINE_TABLE[1]
		end
	end
)
port:addDeps{en}
port:setDepChangeHook(function ()
    if (en.val == 0) then
        port:setShow(false);
    else
        port:setShow(true);
    end
end);

-- 子项显示
local port_sel_view = cfg:hBox {
            cfg:stLabel(port.name),
            cfg:enumView(port),
            cfg:stSpacer(),
};


--[[=============================== 配置子项1-3: uart_id_sel ================================--]]
local uart_id = cfg:enum("充电仓串口选择:" .. TAB_TABLE[1], CHARGESTORE_UARTIRQ, 19)
uart_id:setOSize(1)
uart_id:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_CHARGESTORE_UART_ID" .. TAB_TABLE[3] .. CHARGESTORE_UARTIRQ_OUTPUT_TABLE[uart_id.val] .. NLINE_TABLE[2]
		end
	end
)
uart_id:addDeps{en}
uart_id:setDepChangeHook(function ()
    if (en.val == 0) then
        uart_id:setShow(false);
    else
        uart_id:setShow(true);
    end
end);

-- 子项显示
local uart_id_sel_view = cfg:hBox {
            cfg:stLabel(uart_id.name),
            cfg:enumView(uart_id),
            cfg:stSpacer(),
};


--[[===================================================================================
==================================== 模块返回汇总 =====================================
====================================================================================--]]
-- A. 输出htext
local chargestore_output_htext = {};
insert_item_to_list(chargestore_output_htext, chargestore_comment_begin_htext);
--insert_item_to_list(chargestore_output_htext, en);
insert_item_to_list(chargestore_output_htext, mos33_use_vddio);
insert_item_to_list(chargestore_output_htext, port);
insert_item_to_list(chargestore_output_htext, uart_id);

insert_list_to_list(board_output_text_tabs, chargestore_output_htext);
if enable_moudles["chargestore"] == false then
    return;
end
-- B. 输出ctext：无

-- C. 输出bin：无

-- E. 默认值
local chargestore_cfg_default_table = {};

if open_by_program == "create" then
	insert_item_to_list(chargestore_cfg_default_table, en);
end

insert_item_to_list(chargestore_cfg_default_table, mos33_use_vddio);
insert_item_to_list(chargestore_cfg_default_table, port);
insert_item_to_list(chargestore_cfg_default_table, uart_id);

-- D. 显示
local chargestore_group_view_list = {}
if open_by_program == "create" then
	insert_item_to_list(chargestore_group_view_list, chargestore_en_view);
end
insert_item_to_list(chargestore_group_view_list, mos33_use_vddio_view);
insert_item_to_list(chargestore_group_view_list, port_sel_view);
insert_item_to_list(chargestore_group_view_list, uart_id_sel_view);

local chargestore_group_view = cfg:stGroup("",
        cfg:vBox (chargestore_group_view_list)
);

local chargestore_default_button_view = cfg:stButton(" 充电仓配置恢复默认值 ", reset_to_default(chargestore_cfg_default_table));
local chargestore_view_list = cfg:vBox {
        chargestore_group_view,
        chargestore_default_button_view,
};

local chargestore_view = {"充电仓配置",
        chargestore_view_list,
};

if open_by_program == "create" then
    insert_item_to_list(board_view_tabs, chargestore_view);
end

-- F. bindGroup：无



