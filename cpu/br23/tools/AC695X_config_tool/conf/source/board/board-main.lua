
board_view_tabs = {}
board_output_text_tabs = {}
board_output_bin_tabs = {}

require("board.uart_v1")
require("board.i2c_v1")
require("board.spi_v1")
require("board.sd_v1")
require("board.key_common")
require("board.adkey_v1")
require("board.iokey_v1")
require("board.audio_v1")
require("board.chargestore_v1")
require("board.charge_v1")
require("board.irflt_v1")
require("board.pulse_v1")
require("board.pwmled_v1")
--require("board.pwr_v1")
require("board.redc_v1")
require("board.clock_v1")
require("board.lowpower_v1")

--[[
cfg:addGlobalConstraint(function()
    --return not(adkey_en.val == 1 and iokey_en.val ==1) or "IOKEY 和 ADKEY 不能同时使能";
end
);
--]]


board_view = {"板级配置",
	cfg:stTab(board_view_tabs);
};




