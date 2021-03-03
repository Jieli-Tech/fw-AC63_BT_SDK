


rem @echo off

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


%OBJCOPY% -O binary -j .overlay_aec %ELFFILE% aeco.bin
%OBJCOPY% -O binary -j .overlay_wav %ELFFILE% wavo.bin
%OBJCOPY% -O binary -j .overlay_ape %ELFFILE% apeo.bin
%OBJCOPY% -O binary -j .overlay_flac %ELFFILE% flaco.bin
%OBJCOPY% -O binary -j .overlay_m4a %ELFFILE% m4ao.bin
%OBJCOPY% -O binary -j .overlay_amr %ELFFILE% amro.bin
%OBJCOPY% -O binary -j .overlay_dts %ELFFILE% dtso.bin
%OBJCOPY% -O binary -j .overlay_fm %ELFFILE% fmo.bin
%OBJCOPY% -O binary -j .overlay_mp3 %ELFFILE% mp3o.bin
%OBJCOPY% -O binary -j .overlay_wma %ELFFILE% wmao.bin


remove_tailing_zeros -i aeco.bin -o aec.bin -mark ff
remove_tailing_zeros -i wavo.bin -o wav.bin -mark ff
remove_tailing_zeros -i apeo.bin -o ape.bin -mark ff
remove_tailing_zeros -i flaco.bin -o flac.bin -mark ff
remove_tailing_zeros -i m4ao.bin -o m4a.bin -mark ff
remove_tailing_zeros -i amro.bin -o amr.bin -mark ff
remove_tailing_zeros -i dtso.bin -o dts.bin -mark ff
remove_tailing_zeros -i fmo.bin -o fm.bin -mark ff
remove_tailing_zeros -i mp3o.bin -o mp3.bin -mark ff
remove_tailing_zeros -i wmao.bin -o wma.bin -mark ff



%OBJDUMP% -section-headers -address-mask=0x1ffffff %ELFFILE%
%OBJDUMP% -t %ELFFILE% > symbol_tbl.txt

copy /b text.bin+data.bin+data_code.bin+aec.bin+wav.bin+ape.bin+flac.bin+m4a.bin+amr.bin+dts.bin+fm.bin+mp3.bin+wma.bin+bank.bin app.bin
copy app.bin bluetooth\standard\app.bin
copy br25loader.bin bluetooth\standard\br25loader.bin

bluetooth\standard\download.bat %kws_cfg%
