@echo off

@echo ********************************************************************************
@echo 			SDK BR23			
@echo ********************************************************************************
@echo %date%

cd /d %~dp0

cd tool_resource\app_ota
copy * ..\..\
cd ..\..\

json_to_res.exe json.txt

del isd_config.ini
copy isd_config_app_ota.ini isd_config.ini

isd_download.exe -tonorflash -dev br23 -boot 0x12000 -div8 -wait 300 -uboot uboot.boot -app app.bin cfg_tool.bin -res tone.cfg config.dat 
::-ex_api_bin user_api.bin
@REM 常用命令说明
@rem -format vm         // 擦除VM 区域
@rem -format all        // 擦除所有
@rem -reboot 500        // reset chip, valid in JTAG debug

del isd_config.ini

@REM 删除临时文件
if exist *.mp3 del *.mp3 
if exist *.PIX del *.PIX
if exist *.TAB del *.TAB
if exist *.res del *.res
if exist *.sty del *.sty

if exist json_to_res.exe del json_to_res.exe
if exist json.txt del json.txt
if exist config.dat del config.dat
if exist isd_config_app_ota.ini del isd_config_app_ota.ini

@REM 生成固件升级文件
fw_add.exe -noenc -fw jl_isd.fw  -add ota.bin -type 100 -out jl_isd.fw
@REM 添加配置脚本的版本信息到 FW 文件中
fw_add.exe -noenc -fw jl_isd.fw -add script.ver -out jl_isd.fw

ufw_maker.exe -fw_to_ufw jl_isd.fw
copy jl_isd.ufw update.ufw
del jl_isd.ufw

@REM 生成配置文件升级文件
rem ufw_maker.exe -chip AC693X %ADD_KEY% -output config.ufw -res bt_cfg.cfg

ping /n 2 127.1>null
IF EXIST null del null

pause
