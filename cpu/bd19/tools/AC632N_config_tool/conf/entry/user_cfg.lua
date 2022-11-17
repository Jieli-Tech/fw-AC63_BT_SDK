-------------------- 设置FW版本信息 --------------------
cfg:addKeyInfo("script_version", "AC632N-v0.01-cfg_tool-v0.05");

-------------------- 设置应用名称 --------------------
product_name = "AC632N"; --此名称将显示于配置工具入口界面

-------------------- 设置配置工具开发状态 --------------
    -- develop: 开发状态, 用于开发SDK使用
    -- release: 发布状态, 用于发布上传使用
config_status = "develop";
--config_status = "release";

-------------------- 设置eq工具是否打开 --------------
--eq_tool_button_show = true;
eq_tool_button_show = false;

-------------------- 设置编译前工具是否打开 ----------
fw_create_button_show = true;
--fw_create_button_show = false;

-------------------- 设置模块显示使能 --------------
-- true:  模块配置显示使能;
-- false: 模块配置不显示;
-- 注意: adkey 和 iokey 不能同时为true;
enable_moudles = {
	["isdtool"] = false,
    ["audio"]   = false,
    ["charge"]  = true,
    ["status"]  = false,
    ["tone"]    = true,
    ["bluetooth"]   = true,
    ["ble_config"] = false,
    ["key_msg"] = {enable = false, num = 10},
};

-------------------- 设置应用详细信息: 请在app_log.md文件中填写 --------------------

