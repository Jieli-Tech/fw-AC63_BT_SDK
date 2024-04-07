

































rem @echo off
@echo *********************************************************************
@echo AC632N SDK
@echo *********************************************************************
@echo %date%

cd /d %~dp0

set OBJDUMP=C:\JL\pi32\bin\llvm-objdump.exe
set OBJCOPY=C:\JL\pi32\bin\llvm-objcopy.exe
set INELF=sdk.elf
set LZ4_PACKET=lz4_packet.exe

::@echo on

if exist sdk.elf (

%OBJDUMP% -D -address-mask=0x1ffffff -print-dbg %INELF% > sdk.lst
%OBJCOPY% -O binary -j .text %INELF% text.bin
%OBJCOPY% -O binary -j .data %INELF% data.bin
%OBJCOPY% -O binary -j .data_code %INELF% data_code.bin
%OBJCOPY% -O binary -j .overlay_aec %INELF% aec.bin
%OBJCOPY% -O binary -j .overlay_aac %INELF% aac.bin
%OBJCOPY% -O binary -j .overlay_aptx %INELF% aptx.bin

%OBJCOPY% -O binary -j .common %INELF% common.bin


bankfiles=
for /L %%i in (0,1,20) do (
 %OBJCOPY% -O binary -j .overlay_bank%%i %INELF% bank%%i.bin
 set bankfiles=!bankfiles! bank%%i.bin 0x0
)

echo %bank_files
%LZ4_PACKET% -dict text.bin -input common.bin 0 %bankfiles% -o bank.bin

%OBJDUMP% -section-headers -address-mask=0x1ffffff %INELF%
%OBJSIZEDUMP% -lite -skip-zero -enable-dbg-info %INELF% > symbol_tbl.txt

copy /b text.bin+data.bin+data_code.bin+aec.bin+aac.bin+bank.bin+aptx.bin app.bin

del bank*.bin common.bin text.bin data.bin bank.bin aac.bin aec.bin aptx.bin
)
call download/data_trans/download.bat
