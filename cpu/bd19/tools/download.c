// *INDENT-OFF*
#include "app_config.h"

#ifdef __SHELL__

##!/bin/sh
${OBJDUMP} -D -address-mask=0x1ffffff -print-dbg $1.elf > $1.lst
${OBJCOPY} -O binary -j .text $1.elf text.bin
${OBJCOPY} -O binary -j .data  $1.elf data.bin
${OBJCOPY} -O binary -j .data_code $1.elf data_code.bin
${OBJCOPY} -O binary -j .overlay_aec $1.elf aec.bin
${OBJCOPY} -O binary -j .overlay_aac $1.elf aac.bin
${OBJCOPY} -O binary -j .overlay_aptx $1.elf aptx.bin

${OBJCOPY} -O binary -j .common $1.elf common.bin

bank_files=
for i in $(seq 0 20)
do
    ${OBJCOPY} -O binary -j .overlay_bank$i $1.elf bank$i.bin
    if [ ! -s bank$i.bin ]
    then
        break
    fi
    bank_files=$bank_files"bank$i.bin 0x0 "
done
echo $bank_files
lz4_packet -dict text.bin -input common.bin 0 $bank_files -o bank.bin

${OBJDUMP} -section-headers -address-mask=0x1ffffff $1.elf
${OBJSIZEDUMP} -lite -skip-zero -enable-dbg-info $1.elf | sort -k 1 >  symbol_tbl.txt

cat text.bin data.bin data_code.bin aec.bin aac.bin bank.bin aptx.bin > app.bin


files="app.bin  p11_code.bin ota.bin ota_debug.bin uboot.boot isd_config.ini isd_download.exe bd19loader.bin"

/* #files="app.bin bd19loader.bin uboot.boot uboot.boot_debug uboot_no_ota.boot uboot_no_ota.boot_debug ota.bin isd_config.ini isd_download.exe fw_add.exe ufw_maker.exe" */

#if CONFIG_SPP_AND_LE_CASE_ENABLE || CONFIG_GAMEBOX_CASE || CONFIG_HID_CASE_ENABLE || CONFIG_MESH_CASE_ENABLE
#if RCSP_UPDATE_EN
NICKNAME="bd19_app_ota"
cp bluetooth/app_ota/isd_config.ini ./
cp bluetooth/app_ota/download.bat ./
#else
NICKNAME="bd19_sdk"
cp bluetooth/standard/isd_config.ini ./
cp bluetooth/standard/download.bat ./
#endif
#endif

host-client -project ${NICKNAME}$2 -f ${files} $1.elf

#else

rem @echo off
@echo *********************************************************************
@echo 			                AC632N SDK
@echo *********************************************************************
@echo %date%

cd /d %~dp0

set OBJDUMP=C:\JL\pi32\bin\llvm-objdump.exe
set OBJCOPY=C:\JL\pi32\bin\llvm-objcopy.exe
set INELF=sdk.elf
set LZ4_PACKET=lz4_packet

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
%LZ4_PACKET% -dict text.bin -input common.bin 0 !bankfiles! -o bank.bin

%OBJDUMP% -section-headers -address-mask=0x1ffffff %INELF%
%OBJSIZEDUMP% -lite -skip-zero -enable-dbg-info %INELF% > symbol_tbl.txt

copy /b text.bin+data.bin+data_code.bin+aec.bin+aac.bin+bank.bin+aptx.bin app.bin

del bank*.bin common.bin text.bin data.bin bank.bin aac.bin aec.bin aptx.bin
)


#if CONFIG_SPP_AND_LE_CASE_ENABLE || CONFIG_HID_CASE_ENABLE || CONFIG_MESH_CASE_ENABLE
#if RCSP_UPDATE_EN
copy app.bin bluetooth\app_ota\app.bin
copy bd19loader.bin bluetooth\app_ota\bd19loader.bin

bluetooth\app_ota\download.bat
#else
copy app.bin bluetooth\standard\app.bin
copy bd19loader.bin bluetooth\standard\bd19loader.bin

bluetooth\standard\download.bat
#endif

#endif





#endif


