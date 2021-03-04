require("lang_en");
require("user_cfg");

-------------------- 设置芯片信息 ----------------------
cfg:addKeyInfo("chipname", "AC632N");

-------------------- 设置默认加载的fw文件 --------------
cfg:addKeyInfo("default_fwfile", cfg.projDir .. '/conf/output/default/default_cfg.fw');
cfg:addKeyInfo("ota.bin:path", cfg.projDir .. "/../ota.bin");

-------------------- 设置配置项输出格式 ----------------
cfg:setBinaryFormat("vm"); --配置项与vm格式相同

package.path = package.path .. ';' .. cfg.projDir .. '/?.lua'

projdir = cfg.projDir;  --入口文件所在目录

local download_path;
local text_output_path;
local state_output_path;

if config_status == "develop" then
package.path = package.path .. ';' .. cfg.projDir .. '/conf/source/?.lua'  --添加lua文件搜索路径
srcdir = cfg.projDir .. "/conf/source/";

download_path = projdir .. '/../'; --下载目录的路径
state_output_path = projdir .. '/conf/output/';

text_output_path = state_output_path;
c_out_path = text_output_path;
h_out_path = text_output_path;
state_out_path = state_output_path;
default_out_path = state_output_path;

bin_out_path = download_path;
ver_out_path = download_path;
tone_out_path = download_path;
ini_out_path = download_path;

else

package.path = package.path .. ';' .. cfg.projDir .. '/conf/source/?.lua'  --添加lua文件搜索路径
srcdir = cfg.projDir .. "/conf/source/";

download_path = projdir .. '/download/'; --下载目录的路径
state_output_path = projdir .. '/conf/output/';
text_output_path = state_output_path;
c_out_path = text_output_path;
h_out_path = text_output_path;
state_out_path = state_output_path;
default_out_path = state_output_path;

bin_out_path = download_path;
ver_out_path = download_path;
tone_out_path = download_path;
ini_out_path = download_path;

end

------------------- 设置文件加载的默认路径 --------------------
cfg:addKeyInfo("ota.bin:path", download_path .. 'ota.bin');                     -- 设置 ota.bin 的路径
cfg:addKeyInfo("default.tone:dir", projdir .. '/conf/source/tone_file/');       -- 默认选择提示音的目录
cfg:addKeyInfo("default.tone_cfg:dir", download_path);                          -- 默认加载提示音 tone.tone 的目录
cfg:addKeyInfo("default.bin_cfg:dir", download_path);                           -- 默认打开 bin 配置的目录

require("conf.output.default.default_cfg"); -- 参数的默认值

