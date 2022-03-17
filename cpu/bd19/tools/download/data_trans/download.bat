@echo off

cd %~dp0

copy ..\..\tone.cfg .
copy ..\..\cfg_tool.bin .
copy ..\..\app.bin .
copy ..\..\bd19loader.bin .
copy ..\..\p11_code.bin .
copy ..\..\script.ver .

..\..\isd_download.exe ..\..\isd_config.ini -tonorflash -dev bd19 -boot 0x2000 -div8 -wait 300 -uboot ..\..\uboot.boot -app ..\..\app.bin ..\..\cfg_tool.bin -res ..\..\p11_code.bin -uboot_compress
:: -format all
::-reboot 2500
::..\..\ex_mcu.bin

@rem 删除临时文件-format all
if exist *.mp3 del *.mp3 
if exist *.PIX del *.PIX
if exist *.TAB del *.TAB
if exist *.res del *.res
if exist *.sty del *.sty

@rem generate upgrade file
..\..\fw_add.exe -noenc -fw jl_isd.fw -add ..\..\ota.bin -type 100 -out jl_isd.fw
@rem add ver to jl_isd.fw
..\..\fw_add.exe -noenc -fw jl_isd.fw -add script.ver -out jl_isd.fw

..\..\ufw_maker.exe -fw_to_ufw jl_isd.fw
copy jl_isd.ufw update.ufw
del jl_isd.ufw

@rem 常用命令说明
@rem -format vm        //擦除VM 区域
@rem -format cfg       //擦除BT CFG 区域
@rem -format 0x3f0-2   //表示从第 0x3f0 个 sector 开始连续擦除 2 个 sector(第一个参数为16进制或10进制都可，第二个参数必须是10进制)

ping /n 2 127.1>null
IF EXIST null del null

