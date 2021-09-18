@echo off

cd %~dp0

copy ..\..\tone.cfg .
copy ..\..\br23loader.bin .
copy ..\..\script.ver

..\..\isd_download.exe  ..\..\isd_config.ini -tonorflash -dev br23 -boot 0x12000 -div8 -wait 300 -uboot ..\..\uboot.boot -app ..\..\app.bin ..\..\cfg_tool.bin -ota_file ..\..\ota_all.bin  -format all 
:: -format all
::-reboot 2500

@rem 删除临时文件-format all
if exist *.mp3 del *.mp3 
if exist *.PIX del *.PIX
if exist *.TAB del *.TAB
if exist *.res del *.res
if exist *.sty del *.sty

@rem add ver to jl_isd.fw
@rem generate upgrade file
..\..\fw_add.exe -noenc -fw jl_isd.fw  -add ..\..\ota_all.bin -name ota.bin -type 100 -out jl_isd_all.fw
..\..\fw_add.exe -noenc -fw jl_isd.fw  -add ..\..\ota_nor.bin -name ota.bin -type 100 -out jl_isd_nor.fw
@rem add ver to jl_isd.fw
..\..\fw_add.exe -noenc -fw jl_isd_all.fw -add script.ver -out jl_isd_all.fw
..\..\fw_add.exe -noenc -fw jl_isd_nor.fw -add script.ver -out jl_isd_nor.fw

..\..\ufw_maker.exe -fw_to_ufw jl_isd_all.fw
..\..\ufw_maker.exe -fw_to_ufw jl_isd_nor.fw

copy jl_isd_all.ufw update.ufw
copy jl_isd_nor.ufw nor_update.ufw
copy jl_isd_all.fw jl_isd.fw
del jl_isd_all.ufw jl_isd_nor.ufw jl_isd_all.fw jl_isd_nor.fw

@REM 生成配置文件升级文件
::ufw_maker.exe -chip AC800X %ADD_KEY% -output config.ufw -res bt_cfg.cfg

::IF EXIST jl_696x.bin del jl_696x.bin 

@rem 常用命令说明
@rem -format vm        //擦除VM 区域
@rem -format cfg       //擦除BT CFG 区域
@rem -format 0x3f0-2   //表示从第 0x3f0 个 sector 开始连续擦除 2 个 sector(第一个参数为16进制或10进制都可，第二个参数必须是10进制)

ping /n 2 127.1>null
IF EXIST null del null

