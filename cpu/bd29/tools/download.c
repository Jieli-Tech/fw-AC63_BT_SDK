// *INDENT-OFF*
#include "app_config.h"

#ifdef __SHELL__

##!/bin/sh
${OBJDUMP} -D -address-mask=0x1ffffff -print-dbg $1.elf > $1.lst
${OBJCOPY} -O binary -j .text $1.elf text.bin
${OBJCOPY} -O binary -j .data $1.elf data.bin

${OBJDUMP} -section-headers -address-mask=0x1ffffff $1.elf
${OBJSIZEDUMP} -lite -skip-zero -enable-dbg-info $1.elf | sort -k 1 >  symbol_tbl.txt

cat text.bin data.bin  > app.bin

files="app.bin bd29loader.bin uboot.boot uboot.boot_debug uboot_no_ota.boot uboot_no_ota.boot_debug ota.bin isd_config.ini isd_download.exe fw_add.exe ufw_maker.exe"

#if CONFIG_SPP_AND_LE_CASE_ENABLE || CONFIG_GAMEBOX_CASE || CONFIG_HID_CASE_ENABLE
#if RCSP_UPDATE_EN
NICKNAME="bd29_app_ota"
cp bluetooth/app_ota/isd_config.ini ./
#else
NICKNAME="bd29_sdk"
cp bluetooth/standard/isd_config.ini ./
#endif
#endif

host-client -project ${NICKNAME}$2 -f ${files} $1.elf

#else

rem @echo off
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


#if CONFIG_SPP_AND_LE_CASE_ENABLE || CONFIG_HID_CASE_ENABLE || CONFIG_MESH_CASE_ENABLE
#if RCSP_UPDATE_EN
copy app.bin bluetooth\app_ota\app.bin
copy bd29loader.bin bluetooth\app_ota\bd29loader.bin

bluetooth\app_ota\download.bat
#else
copy app.bin bluetooth\standard\app.bin
copy bd29loader.bin bluetooth\standard\bd29loader.bin

bluetooth\standard\download.bat
#endif

#endif





#endif

