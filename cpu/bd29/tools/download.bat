




rem @echo off
@echo *********************************************************************
@echo AC630N SDK
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
%OBJDUMP% -t %INELF% > symbol_tbl.txt

copy text.bin/b + data.bin/b app.bin

)
copy app.bin bluetooth\standard\app.bin
copy bd29loader.bin bluetooth\standard\bd29loader.bin

bluetooth\standard\download.bat
