

require("const-def")
require("typeids")


generic_def_module = {}

local MOUDLE_COMMENT_BEGIN = "//                                 配置开始                                        //"

local comment_begin = cfg:i32("generic注释开始", 0)
comment_begin:setTextOut(
	function (ty) 
		if (ty == 3) then
			return COMMMENT_LINE .. NLINE_TABLE[1] .. MOUDLE_COMMENT_BEGIN .. NLINE_TABLE[1] .. COMMMENT_LINE .. NLINE_TABLE[1]
		end
	end
)

local MOUDLE_COMMENT_END = "//                                 配置结束                                        //"

local comment_end = cfg:i32("generic注释结束", 0)
comment_end:setTextOut(
	function (ty) 
		if (ty == 3) then
			return COMMMENT_LINE .. NLINE_TABLE[1] .. MOUDLE_COMMENT_END .. NLINE_TABLE[1] .. COMMMENT_LINE .. NLINE_TABLE[1]
		end
	end
)




local no_cfg_port = cfg:i32("IO口未设置", 0)
no_cfg_port:setOSize(1)
no_cfg_port:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define NO_CONFIG_PORT" .. TAB_TABLE[6] .. "(" .. -1 .. ")"  .. NLINE_TABLE[2]
		end
	end
)

local moudle_enable = cfg:i32("模块使能", 1)
moudle_enable:setOSize(1)
moudle_enable:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define ENABLE_THIS_MODULE" .. TAB_TABLE[5] .. (1) .. NLINE_TABLE[1]
		end
	end
)

local moudle_disable = cfg:i32("模块不使能", 0)
moudle_disable:setOSize(1)
moudle_disable:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define DISABLE_THIS_MODULE" .. TAB_TABLE[5] .. (0) .. NLINE_TABLE[2]
		end
	end
)


local function_enable = cfg:i32("功能开启", 1)
function_enable:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define ENABLE" .. TAB_TABLE[8] .. (1) .. NLINE_TABLE[1]
		end
	end
)

local function_disable = cfg:i32("功能不开启", 0)
function_disable:setTextOut(
	function (ty) 
		if (ty == 3) then
			return "#define DISABLE" .. TAB_TABLE[8] .. (0) .. NLINE_TABLE[2]
		end
	end
)


--[[============================== 输出到 text 汇总 ================================--]]
generic_def_output_text_begin_tabs = {
    comment_begin,
    moudle_enable,
    moudle_disable,
    function_enable,
    function_disable,
    no_cfg_port,
    --comment_end,
};
generic_def_output_text_end_tabs = {
    comment_end,
};

--[[============================== 模块返回  ================================--]]

return generic_def_module







