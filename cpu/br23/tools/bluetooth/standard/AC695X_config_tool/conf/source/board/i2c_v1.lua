
require("const-def")
require("typeids")


i2c_module = {}


local MOUDLE_COMMENT_NAME = "//                                 IIC 配置                                        //"

local comment_begin = cfg:i32("iic注释开始", 0)
local iic_comment_begin_htext = module_output_comment(comment_begin, MOUDLE_COMMENT_NAME);

--print(MOUDLE_COMMENT_NAME);
---------------------------------[[ hw_i2c0 ]]---------------------------------
local HW_I2C_MODE_SEL = cfg:enumStr("主从模式选择", {"master", "slave"})-- 0:master, 1:slave
local HW_I2C_PORTS = cfg:enumStr("I2C 引脚选择", {"A组", "B组", "C组", "D组",})

---------------------------------[[ hw_i2c输出列表]]---------------------------------
local HW_I2C_PORT_OUTPUT_TABLE = {
    [0] = "HW_IIC_PORTA",
    [1] = "HW_IIC_PORTB",
    [2] = "HW_IIC_PORTC",
    [3] = "HW_IIC_PORTD",
}

local HW_I2C_MODE_OUTPUT_TABLE = {
    [0] = "IIC_MASTER_MODE",
    [1] = "IIC_SLAVE_MODE",
}

--[[
//==========================================================================================//
//                                  配置项: IIC_CFG                                         //
//==========================================================================================//
--]]

--[[=============================== 配置子项1-0: hw_iic_en ================================--]]
local hw_i2c0_en = cfg:enum("硬件 IIC_0 使能开关:" .. TAB_TABLE[1], ENABLE_SWITCH, 0)
hw_i2c0_en:setOSize(1)
hw_i2c0_en:setTextOut(
	function (ty) 
		if (ty == 3) then
            if enable_moudles["iic"] == false then
			    return "#define TCFG_HW_I2C0_ENABLE" .. TAB_TABLE[5] .. ENABLE_SWITCH_TABLE[0] .. NLINE_TABLE[2]
            end
			return "#define TCFG_HW_I2C0_ENABLE" .. TAB_TABLE[5] .. ENABLE_SWITCH_TABLE[hw_i2c0_en.val] .. NLINE_TABLE[1]
		end
	end
)

-- 子项显示
local hw_i2c0_en_view = cfg:hBox {
            cfg:stLabel(hw_i2c0_en.name),
            cfg:enumView(hw_i2c0_en),
            cfg:stSpacer(),
};

--[[=============================== 配置子项1-1: hw_iic_mode ================================--]]
local hw_i2c0_mode = cfg:enum("硬件 IIC_0 主从模式选择:", HW_I2C_MODE_SEL, 0)
hw_i2c0_mode:setOSize(1)
hw_i2c0_mode:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_HW_I2C0_MODE" .. TAB_TABLE[5] .. HW_I2C_MODE_OUTPUT_TABLE[hw_i2c0_mode.val] .. NLINE_TABLE[1]
		end
	end
)
hw_i2c0_mode:addDeps{hw_i2c0_en}
hw_i2c0_mode:setDepChangeHook(function ()
    if (hw_i2c0_en.val == 0) then
        hw_i2c0_mode:setShow(false);
    else
        hw_i2c0_mode:setShow(true);
    end
end);

-- 子项显示
local hw_i2c0_mode_view = cfg:hBox {
            cfg:stLabel(hw_i2c0_mode.name),
            cfg:enumView(hw_i2c0_mode),
            cfg:stSpacer(),
};

--[[=============================== 配置子项1-2: hw_iic_port ================================--]]
local hw_i2c0_ports = cfg:enum("硬件 IIC_0 引脚选择:" .. TAB_TABLE[1], HW_I2C_PORTS, 0)
hw_i2c0_ports:setOSize(1)
hw_i2c0_ports:setTextOut(
	function (ty) 
		local i = { [0] = "A"; [1] = "B"; [2] = "C"; [3]= "D"}
		if (ty == 3) then
			return "#define TCFG_HW_I2C0_PORTS" .. TAB_TABLE[5] .. HW_I2C_PORT_OUTPUT_TABLE[hw_i2c0_ports.val] .. NLINE_TABLE[1]
		end
	end
)
hw_i2c0_ports:addDeps{hw_i2c0_en}
hw_i2c0_ports:setDepChangeHook(function ()
    if (hw_i2c0_en.val == 0) then
        hw_i2c0_ports:setShow(false);
    else
        hw_i2c0_ports:setShow(true);
    end
end);

-- 子项显示
local hw_i2c0_ports_view = cfg:hBox {
            cfg:stLabel(hw_i2c0_ports.name),
            cfg:enumView(hw_i2c0_ports),
            cfg:stSpacer(),
};

--[[=============================== 配置子项1-3: hw_iic_port ================================--]]
local hw_i2c0_clk = cfg:i32("硬件 IIC_0 频率配置:" .. TAB_TABLE[1], 0)
hw_i2c0_clk:setOSize(4)
hw_i2c0_clk:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_HW_I2C0_CLK" .. TAB_TABLE[5] .. hw_i2c0_clk.val .. NLINE_TABLE[2]
		end
	end
)
hw_i2c0_clk:addConstraint(
	function () 
		return hw_i2c0_clk.val < 400000000 or "i2c控制频率不能大于400M";
	end
)
hw_i2c0_clk:addDeps{hw_i2c0_en}
hw_i2c0_clk:setDepChangeHook(function ()
    if (hw_i2c0_en.val == 0) then
        hw_i2c0_clk:setShow(false);
    else
        hw_i2c0_clk:setShow(true);
    end
end);

-- 子项显示
local hw_i2c0_clk_view = cfg:hBox {
            cfg:stLabel(hw_i2c0_clk.name),
            cfg:inputView(hw_i2c0_clk),
            cfg:stSpacer(),
};

--[[=============================== 配置子项2-0: sw_iic_en ================================--]]
local sw_i2c0_en = cfg:enum("软件 IIC_0 使能开关:" .. TAB_TABLE[1], ENABLE_SWITCH, 0)
sw_i2c0_en:setOSize(1)
sw_i2c0_en:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_SW_I2C0_ENABLE" .. TAB_TABLE[5] .. ENABLE_SWITCH_TABLE[sw_i2c0_en.val] .. NLINE_TABLE[1]
		end
	end
)

-- 子项显示
local sw_i2c0_en_view = cfg:hBox {
            cfg:stLabel(sw_i2c0_en.name),
            cfg:enumView(sw_i2c0_en),
            cfg:stSpacer(),
};


--[[=============================== 配置子项2-1: sw_iic_clk_port ================================--]]
local sw_i2c0_clk_port = cfg:enum("软件 IIC_0 时钟引脚选择:", PORTS, 0xFF)
sw_i2c0_clk_port:setOSize(1)
sw_i2c0_clk_port:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_SW_I2C0_CLK_PORT" .. TAB_TABLE[4] .. PORTS_TABLE[sw_i2c0_clk_port.val] .. NLINE_TABLE[1]
		end
	end
)
sw_i2c0_clk_port:addDeps{sw_i2c0_en}
sw_i2c0_clk_port:setDepChangeHook(function ()
    if (sw_i2c0_en.val == 0) then
        sw_i2c0_clk_port:setShow(false);
    else
        sw_i2c0_clk_port:setShow(true);
    end
end);

-- 子项显示
local sw_i2c0_clk_view = cfg:hBox {
            cfg:stLabel(sw_i2c0_clk_port.name),
            cfg:enumView(sw_i2c0_clk_port),
            cfg:stSpacer(),
};


--[[=============================== 配置子项2-2: sw_iic_data_port ================================--]]
local sw_i2c0_dat_port = cfg:enum("软件 IIC_0 数据引脚选择:", PORTS, 0xFF)
sw_i2c0_dat_port:setOSize(1)
sw_i2c0_dat_port:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_SW_I2C0_DAT_PORT" .. TAB_TABLE[4] .. PORTS_TABLE[sw_i2c0_dat_port.val] .. NLINE_TABLE[1]
		end
	end
)
sw_i2c0_dat_port:addDeps{sw_i2c0_en}
sw_i2c0_dat_port:setDepChangeHook(function ()
    if (sw_i2c0_en.val == 0) then
        sw_i2c0_dat_port:setShow(false);
    else
        sw_i2c0_dat_port:setShow(true);
    end
end);

-- 子项显示
local sw_i2c0_dat_view = cfg:hBox {
            cfg:stLabel(sw_i2c0_dat_port.name),
            cfg:enumView(sw_i2c0_dat_port),
            cfg:stSpacer(),
};


--[[=============================== 配置子项2-3: sw_iic_delay_cnt ================================--]]
local sw_i2c0_delay_cnt = cfg:i32("软件 IIC_0 延时时钟数目选择:", 50)
sw_i2c0_delay_cnt:setOSize(4)
sw_i2c0_delay_cnt:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define TCFG_SW_I2C0_DELAY_CNT" .. TAB_TABLE[4] ..  sw_i2c0_delay_cnt.val .. NLINE_TABLE[2]
		end
	end
)
sw_i2c0_delay_cnt:addDeps{sw_i2c0_en}
sw_i2c0_delay_cnt:setDepChangeHook(function ()
    if (sw_i2c0_en.val == 0) then
        sw_i2c0_delay_cnt:setShow(false);
    else
        sw_i2c0_delay_cnt:setShow(true);
    end
end);


-- 子项显示
local sw_i2c0_delay_cnt_view = cfg:hBox {
            cfg:stLabel(sw_i2c0_delay_cnt.name),
            cfg:inputView(sw_i2c0_delay_cnt),
            cfg:stSpacer(),
};

--[[===================================================================================
==================================== 模块返回汇总 =====================================
====================================================================================--]]
-- A. 输出htext
local iic_output_htext = {};
insert_item_to_list(iic_output_htext, iic_comment_begin_htext);
insert_item_to_list(iic_output_htext, hw_i2c0_en);

if enable_moudles["iic"] == true then
insert_item_to_list(iic_output_htext, hw_i2c0_mode);
insert_item_to_list(iic_output_htext, hw_i2c0_ports);
insert_item_to_list(iic_output_htext, hw_i2c0_clk);
end

insert_list_to_list(board_output_text_tabs, iic_output_htext);
if enable_moudles["iic"] == false then
    return;
end

-- B. 输出ctext：无

-- C. 输出bin：无

-- E. 默认值
local iic_cfg_default_table = {};

if open_by_program == "create" then
	insert_item_to_list(iic_cfg_default_table, hw_i2c0_en);
end
insert_item_to_list(iic_cfg_default_table, hw_i2c0_mode);
insert_item_to_list(iic_cfg_default_table, hw_i2c0_ports);
insert_item_to_list(iic_cfg_default_table, hw_i2c0_clk);

-- D. 显示
local hw_iic_group_view = cfg:stGroup("硬件 IIC 配置",
        cfg:vBox {
            hw_i2c0_en_view,
            hw_i2c0_mode_view,
            hw_i2c0_ports_view,
            hw_i2c0_clk_view,
        }
);

local sw_iic_group_view = cfg:stGroup("软件 IIC 配置",
        cfg:vBox {
            sw_i2c0_en_view,
            sw_i2c0_clk_view,
            sw_i2c0_dat_view,
            sw_i2c0_delay_cnt_view,
        }
);

local iic_default_button_view = cfg:stButton(" IIC配置恢复默认值 ", reset_to_default(iic_cfg_default_table));
local iic_view_list = cfg:vBox {
    cfg:hBox {
        hw_iic_group_view,
        --sw_iic_group_view,
    },
    iic_default_button_view,
};

local iic_view = {"IIC 配置",
        iic_view_list,
};

if open_by_program == "create" then
    insert_item_to_list(board_view_tabs, iic_view);
end

-- F. bindGroup：无


