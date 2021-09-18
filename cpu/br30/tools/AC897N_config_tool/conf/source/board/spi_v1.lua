
local MOUDLE_COMMENT_NAME = "//                                 SPI 配置                                        //"

local comment_begin = cfg:i32("spi注释开始", 0);
local spi_comment_begin_htext = module_output_comment(comment_begin, MOUDLE_COMMENT_NAME);

--print(MOUDLE_COMMENT_NAME);
local HW_SPI_MODE_SEL = cfg:enumStr("主从模式选择", {"master", "slave"})-- 0:master, 1:slave
local HW_SPI_PORTS = cfg:enumStr("SPI 引脚选择", {"A组", "B组", "C组", "D组",})
local HW_SPI_DAT_MODE_SEL = cfg:enumStr("传输模式选择", 
	{
		"双向1位", 
		"单向1位",
		"单向2位",
		"单向4位",
	}
)



--[[
//==========================================================================================//
//                                  配置项: SPI_CFG                                         //
//==========================================================================================//
--]]

--[[=============================== 配置子项1-0: hw_spi_en ================================--]]
local hw_spi0_en = cfg:enum("硬件 SPI_0 使能开关:" .. TAB_TABLE[1], ENABLE_SWITCH, 0)
hw_spi0_en:setOSize(1)
hw_spi0_en:setTextOut(
	function (ty) 
		if (ty == 3) then
            if enable_moudles["spi"] == false then
			    return "#define TCFG_SPI_ENABLE" .. TAB_TABLE[6] .. ENABLE_SWITCH_TABLE[0].. NLINE_TABLE[2]
            end
			return "#define TCFG_HW_SPI0_ENABLE" .. TAB_TABLE[5] .. ENABLE_SWITCH_TABLE[hw_spi0_en.val].. NLINE_TABLE[1]
		end
	end
)

-- 子项显示
local hw_spi0_en_view = cfg:hBox {
            cfg:stLabel(hw_spi0_en.name),
            cfg:enumView(hw_spi0_en),
            cfg:stSpacer(),
};


--[[=============================== 配置子项1-1: hw_spi_mode_sel ================================--]]
local hw_spi0_mode = cfg:enum("硬件 SPI_0 主从模式选择:", HW_SPI_MODE_SEL, 0)
hw_spi0_mode:setOSize(1)
hw_spi0_mode:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_HW_SPI0_MODE" .. TAB_TABLE[5] .. hw_spi0_mode.val .. NLINE_TABLE[1]
		end
	end
)
hw_spi0_mode:addDeps{hw_spi0_en}
hw_spi0_mode:setDepChangeHook(function ()
    if (hw_spi0_en.val == 0) then
        hw_spi0_mode:setShow(false);
    else
        hw_spi0_mode:setShow(true);
    end
end);

-- 子项显示
local hw_spi0_mode_view = cfg:hBox {
            cfg:stLabel(hw_spi0_mode.name),
            cfg:enumView(hw_spi0_mode),
            cfg:stSpacer(),
};


--[[=============================== 配置子项1-2: hw_spi_dat_mode_sel ================================--]]
local hw_spi0_dat_mode = cfg:enum("硬件 SPI_0 传输模式选择:", HW_SPI_DAT_MODE_SEL, 1)
hw_spi0_dat_mode:setOSize(1)
hw_spi0_dat_mode:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_HW_SPI0_DAT_MODE" .. TAB_TABLE[4] .. hw_spi0_dat_mode.val .. NLINE_TABLE[1]
		end
	end
)
hw_spi0_dat_mode:addDeps{hw_spi0_en}
hw_spi0_dat_mode:setDepChangeHook(function ()
    if (hw_spi0_en.val == 0) then
        hw_spi0_dat_mode:setShow(false);
    else
        hw_spi0_dat_mode:setShow(true);
    end
end);

-- 子项显示
local hw_spi0_dat_mode_view = cfg:hBox {
            cfg:stLabel(hw_spi0_dat_mode.name),
            cfg:enumView(hw_spi0_dat_mode),
            cfg:stSpacer(),
};


--[[=============================== 配置子项1-3: hw_spi_ports_sel ================================--]]
local hw_spi0_ports = cfg:enum("硬件 SPI_0 引脚选择:" .. TAB_TABLE[1], HW_SPI_PORTS, 0)
hw_spi0_ports:setOSize(1)
hw_spi0_ports:setTextOut(
	function (ty) 
		local i = { [0] = "A"; [1] = "B"; [2] = "C"; [3]= "D"}
		if (ty == 3) then
			return "#define TCFG_HW_SPI0_PORT" .. TAB_TABLE[5] .. "\'" .. i[hw_spi0_ports.val] .. "\'" .. NLINE_TABLE[1]
		end
	end
)
hw_spi0_ports:addDeps{hw_spi0_en}
hw_spi0_ports:setDepChangeHook(function ()
    if (hw_spi0_en.val == 0) then
        hw_spi0_ports:setShow(false);
    else
        hw_spi0_ports:setShow(true);
    end
end);

-- 子项显示
local hw_spi0_ports_view = cfg:hBox {
            cfg:stLabel(hw_spi0_ports.name),
            cfg:enumView(hw_spi0_ports),
            cfg:stSpacer(),
};


--[[=============================== 配置子项1-4: hw_spi_clk ================================--]]
local hw_spi0_clk = cfg:i32("硬件 SPI_0 频率配置:" .. TAB_TABLE[1], 60000000)
hw_spi0_clk:setOSize(4)
hw_spi0_clk:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_HW_SPI0_CLK" .. TAB_TABLE[5] .. hw_spi0_clk.val .. NLINE_TABLE[2]
		end
	end
)
hw_spi0_clk:addConstraint(
	function () 
		return hw_spi0_clk.val < 400000000 or "spi控制频率不能大于400M";
	end
)
hw_spi0_clk:addDeps{hw_spi0_en}
hw_spi0_clk:setDepChangeHook(function ()
    if (hw_spi0_en.val == 0) then
        hw_spi0_clk:setShow(false);
    else
        hw_spi0_clk:setShow(true);
    end
end);

-- 子项显示
local hw_spi0_clk_view = cfg:hBox {
            cfg:stLabel(hw_spi0_clk.name),
            cfg:inputView(hw_spi0_clk),
            cfg:stSpacer(),
};


--[[=============================== 配置子项2-0: sw_spi_en ================================--]]
local sw_spi0_en = cfg:enum("软件 SPI_0 使能开关:" .. TAB_TABLE[1], ENABLE_SWITCH, 0)
sw_spi0_en:setOSize(1)
sw_spi0_en:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_SW_SPI0_ENABLE" .. TAB_TABLE[5] .. ENABLE_SWITCH_TABLE[sw_spi0_en.val] .. NLINE_TABLE[1]
		end
	end
)

-- 子项显示
local sw_spi0_en_view = cfg:hBox {
            cfg:stLabel(sw_spi0_en.name),
            cfg:enumView(sw_spi0_en),
            cfg:stSpacer(),
};



--[[=============================== 配置子项2-1: sw_spi_cs_port ================================--]]
local sw_spi0_cs_port = cfg:enum("软件 SPI_0 CS 引脚选择:" .. TAB_TABLE[1], PORTS, 0xFF)
sw_spi0_cs_port:setOSize(1)
sw_spi0_cs_port:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_SW_SPI0_CS_PORT" .. TAB_TABLE[4] .. PORTS_TABLE[sw_spi0_cs_port.val] .. NLINE_TABLE[1]
		end
	end
)
sw_spi0_cs_port:addDeps{sw_spi0_en}
sw_spi0_cs_port:setDepChangeHook(function ()
    if (sw_spi0_en.val == 0) then
        sw_spi0_cs_port:setShow(false);
    else
        sw_spi0_cs_port:setShow(true);
    end
end);

-- 子项显示
local sw_spi0_cs_port_view = cfg:hBox {
            cfg:stLabel(sw_spi0_cs_port.name),
            cfg:enumView(sw_spi0_cs_port),
            cfg:stSpacer(),
};


--[[=============================== 配置子项2-2: sw_spi_clk_port ================================--]]
local sw_spi0_clk_port = cfg:enum("软件 SPI_0 CLK 引脚选择:", PORTS, 0xFF)
sw_spi0_clk_port:setOSize(1)
sw_spi0_clk_port:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_SW_SPI0_CLK_PORT" .. TAB_TABLE[4] .. PORTS_TABLE[sw_spi0_clk_port.val] .. NLINE_TABLE[1]
		end
	end
)
sw_spi0_clk_port:addDeps{sw_spi0_en}
sw_spi0_clk_port:setDepChangeHook(function ()
    if (sw_spi0_en.val == 0) then
        sw_spi0_clk_port:setShow(false);
    else
        sw_spi0_clk_port:setShow(true);
    end
end);

-- 子项显示
local sw_spi0_clk_port_view = cfg:hBox {
            cfg:stLabel(sw_spi0_clk_port.name),
            cfg:enumView(sw_spi0_clk_port),
            cfg:stSpacer(),
};


--[[=============================== 配置子项2-3: sw_spi_in_port ================================--]]
local sw_spi0_in_port = cfg:enum("软件 SPI_0 IN 引脚选择:" .. TAB_TABLE[1], PORTS, 0xFF)
sw_spi0_in_port:setOSize(1)
sw_spi0_in_port:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_SW_SPI0_IN_PORT" .. TAB_TABLE[4] .. PORTS_TABLE[sw_spi0_in_port.val] .. NLINE_TABLE[1]
		end
	end
)
sw_spi0_in_port:addDeps{sw_spi0_en}
sw_spi0_in_port:setDepChangeHook(function ()
    if (sw_spi0_en.val == 0) then
        sw_spi0_in_port:setShow(false);
    else
        sw_spi0_in_port:setShow(true);
    end
end);

-- 子项显示
local sw_spi0_in_port_view = cfg:hBox {
            cfg:stLabel(sw_spi0_in_port.name),
            cfg:enumView(sw_spi0_in_port),
            cfg:stSpacer(),
};


--[[=============================== 配置子项2-4: sw_spi_out_port ================================--]]
local sw_spi0_out_port = cfg:enum("软件 SPI_0 OUT 引脚选择:", PORTS, 0xFF)
sw_spi0_out_port:setOSize(1)
sw_spi0_out_port:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_SW_SPI0_OUT_PORT" .. TAB_TABLE[4] .. PORTS_TABLE[sw_spi0_out_port.val] .. NLINE_TABLE[2]
		end
	end
)
sw_spi0_out_port:addDeps{sw_spi0_en}
sw_spi0_out_port:setDepChangeHook(function ()
    if (sw_spi0_en.val == 0) then
        sw_spi0_out_port:setShow(false);
    else
        sw_spi0_out_port:setShow(true);
    end
end);


-- 子项显示
local sw_spi0_out_port_view = cfg:hBox {
            cfg:stLabel(sw_spi0_out_port.name),
            cfg:enumView(sw_spi0_out_port),
            cfg:stSpacer(),
};


--[[===================================================================================
==================================== 模块返回汇总 =====================================
====================================================================================--]]
-- A. 输出htext
local spi_output_htext = {};
insert_item_to_list(spi_output_htext, spi_comment_begin_htext);
insert_item_to_list(spi_output_htext, hw_spi0_en);

if enable_moudles["spi"] == true then 
    insert_item_to_list(spi_output_htext, hw_spi0_mode);
	insert_item_to_list(spi_output_htext, hw_spi0_dat_mode);
	insert_item_to_list(spi_output_htext, hw_spi0_ports);
	insert_item_to_list(spi_output_htext, hw_spi0_clk);

	insert_item_to_list(spi_output_htext, sw_spi0_en);
	insert_item_to_list(spi_output_htext, sw_spi0_cs_port);
	insert_item_to_list(spi_output_htext, sw_spi0_clk_port);
	insert_item_to_list(spi_output_htext, sw_spi0_in_port);
	insert_item_to_list(spi_output_htext, sw_spi0_out_port);
end

insert_list_to_list(board_output_text_tabs, spi_output_htext);

if enable_moudles["spi"] == false then
    return;
end

-- B. 输出ctext：无
-- C. 输出bin：无

-- E. 默认值
local spi_cfg_default_table = {};
if open_by_program == "create" then
	insert_item_to_list(spi_cfg_default_table , hw_spi0_en);
end
insert_item_to_list(spi_cfg_default_table , hw_spi0_mode);
insert_item_to_list(spi_cfg_default_table , hw_spi0_dat_mode);
insert_item_to_list(spi_cfg_default_table , hw_spi0_ports);
insert_item_to_list(spi_cfg_default_table , hw_spi0_clk);

insert_item_to_list(spi_cfg_default_table , sw_spi0_en);
insert_item_to_list(spi_cfg_default_table , sw_spi0_cs_port);
insert_item_to_list(spi_cfg_default_table , sw_spi0_clk_port);
insert_item_to_list(spi_cfg_default_table , sw_spi0_in_port);
insert_item_to_list(spi_cfg_default_table , sw_spi0_out_port);

local spi_default_button_view = cfg:stButton(" SPI配置恢复默认值 ", reset_to_default(spi_cfg_default_table));

-- D. 显示
local hw_spi_group_view = cfg:stGroup("硬件 SPI 配置",
        cfg:vBox {
            hw_spi0_en_view,
            hw_spi0_mode_view,
            hw_spi0_ports_view,
            hw_spi0_clk_view,
        }
);

local sw_spi_group_view = cfg:stGroup("软件 SPI 配置",
        cfg:vBox {
            sw_spi0_en_view,
            sw_spi0_cs_port_view,
            sw_spi0_clk_port_view,
            sw_spi0_in_port_view,
            sw_spi0_out_port_view,
        }
);

local spi_view_list = cfg:stGroup("",
    cfg:vBox {
        cfg:hBox {
            hw_spi_group_view,
            sw_spi_group_view,
        },
        spi_default_button_view,
    }
);

local spi_view = {"SPI 配置",
        spi_view_list,
};

if open_by_program == "create" then
    insert_item_to_list(board_view_tabs, spi_view);
end

-- F. bindGroup：无
