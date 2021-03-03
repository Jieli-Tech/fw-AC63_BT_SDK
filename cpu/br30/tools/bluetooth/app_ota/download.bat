@echo off

cd %~dp0

..\..\json_to_res.exe json.txt

..\..\md5sum.exe app.bin md5.bin

set /p "themd5=" < "md5.bin"

..\..\isd_download.exe isd_config_%1.ini -tonorflash -dev br30 -boot 0x2000 -div8 -wait 300 -uboot ..\..\uboot.boot -app app.bin cfg_tool.bin  -res  config.dat ..\..\p11_code.bin %2 md5.bin -uboot_compress


@REM 常用命令说明
@rem -format vm         // 擦除VM 区域
@rem -format all        // 擦除所有
@rem -reboot 500        // reset chip, valid in JTAG debug


@REM 删除临时文件
if exist *.mp3 del *.mp3 
if exist *.PIX del *.PIX
if exist *.TAB del *.TAB
if exist *.res del *.res
if exist *.sty del *.sty


@REM 生成固件升级文件
..\..\fw_add.exe -noenc -fw jl_isd.fw  -add ..\..\ota.bin -type 100 -out jl_isd.fw
@REM 添加配置脚本的版本信息到 FW 文件中
..\..\fw_add.exe -noenc -fw jl_isd.fw -add script.ver -out jl_isd.fw

if exist *.ufw del *.ufw
..\..\ufw_maker.exe -fw_to_ufw jl_isd.fw
copy jl_isd.ufw update_%themd5%.ufw
del jl_isd.ufw

@REM 生成配置文件升级文件
rem ufw_maker.exe -chip AC630N %ADD_KEY% -output config.ufw -res bt_cfg.cfg

