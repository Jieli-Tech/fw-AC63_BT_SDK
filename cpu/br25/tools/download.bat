

@echo off

@echo *****************************************************************
@echo SDK BR25
@echo *****************************************************************
@echo %date%

cd %~dp0


set OBJDUMP=C:\JL\pi32\bin\llvm-objdump.exe
set OBJCOPY=C:\JL\pi32\bin\llvm-objcopy.exe
set ELFFILE=sdk.elf

REM %OBJDUMP% -D -address-mask=0x1ffffff -print-dbg $1.elf > $1.lst
%OBJCOPY% -O binary -j .text %ELFFILE% text.bin
%OBJCOPY% -O binary -j .data %ELFFILE% data.bin
%OBJCOPY% -O binary -j .data_code %ELFFILE% data_code.bin

%OBJDUMP% -section-headers -address-mask=0x1ffffff %ELFFILE%
%OBJDUMP% -t %ELFFILE% > symbol_tbl.txt

copy /b text.bin+data.bin+data_code.bin+bank.bin app.bin
copy app.bin bluetooth\standard\app.bin
copy br25loader.bin bluetooth\standard\br25loader.bin

bluetooth\standard\download.bat
