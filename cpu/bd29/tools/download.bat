@echo off
@echo *********************************************************************
@echo 			                AC630N SDK
@echo *********************************************************************
@echo %date%

cd /d %~dp0

set OBJDUMP=C:\JL\pi32\bin\llvm-objdump.exe
set OBJCOPY=C:\JL\pi32\bin\llvm-objcopy.exe
set INELF=%1.elf

::@echo on

if exist sdk.elf (

%OBJCOPY% -O binary -j .text %INELF% text.bin
%OBJCOPY% -O binary -j .data %INELF% data.bin

%OBJDUMP% -section-headers -address-mask=0x1ffffff %INELF%
%OBJDUMP% -t %INELF% >  symbol_tbl.txt

copy text.bin/b + data.bin/b  app.bin

)

 


isd_download.exe -tonorflash -dev bd29 -boot 0x2000 -div8 -wait 300 -uboot uboot.boot -app app.bin cfg_tool.bin  

@REM 常用命令说明
@rem -format vm         // 擦除VM 区域
@rem -format all        // 擦除所有
@rem -reboot 500        // reset chip, valid in JTAG debug


 

@REM 生成固件升级文件
fw_add.exe -noenc -fw jl_isd.fw  -add ota.bin -type 100 -out jl_isd.fw
@REM 添加配置脚本的版本信息到 FW 文件中
fw_add.exe -noenc -fw jl_isd.fw -add script.ver -out jl_isd.fw

ufw_maker.exe -fw_to_ufw jl_isd.fw
copy jl_isd.ufw update.ufw
del jl_isd.ufw

 

::ping /n 2 127.1>null

if exist sdk.elf (
move sdk.elf* elf/
move sdk.map elf/
move symbol_tbl.txt elf/
move data.bin elf/
move jl_isd.bin elf/
) else (
pause
)

