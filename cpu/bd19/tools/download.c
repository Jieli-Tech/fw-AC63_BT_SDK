// *INDENT-OFF*
#include "app_config.h"

#ifdef __SHELL__

##!/bin/sh

${OBJDUMP} -D -address-mask=0x1ffffff -print-dbg $1.elf > $1.lst
${OBJCOPY} -O binary -j .text $1.elf text.bin
${OBJCOPY} -O binary -j .data $1.elf data.bin
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
${OBJSIZEDUMP} -lite -skip-zero -enable-dbg-info $1.elf | sort -k 1 > symbol_tbl.txt

cat text.bin data.bin data_code.bin aec.bin aac.bin bank.bin aptx.bin > app.bin

/opt/utils/strip-ini -i isd_config.ini -o isd_config.ini

/* files="app.bin ${CPU}loader.* uboot*  ota*.bin p11_code.bin isd_config.ini isd_download.exe fw_add.exe ufw_maker.exe" */
files="app.bin ${CPU}loader.* uboot*  ota*.bin p11_code.bin isd_config.ini"
NICKNAME="${CPU}_sdk"

host-client -project ${NICKNAME}$2 -f ${files} $1.elf

#else

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

#ifdef CONFIG_WATCH_CASE_ENABLE
call download/watch/download.bat
#elif defined(CONFIG_SOUNDBOX_CASE_ENABLE)
call download/soundbox/download.bat
#elif defined(CONFIG_EARPHONE_CASE_ENABLE)
call download/earphone/download.bat
#elif defined(CONFIG_HID_CASE_ENABLE) ||defined(CONFIG_SPP_AND_LE_CASE_ENABLE)||defined(CONFIG_MESH_CASE_ENABLE)||defined(CONFIG_DONGLE_CASE_ENABLE)    //数传
call download/data_trans/download.bat
#else
//to do other case
#endif  //endif app_case

#endif
