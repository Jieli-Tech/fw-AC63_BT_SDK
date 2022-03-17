// *INDENT-OFF*
#include "app_config.h"

#ifdef __SHELL__

##!/bin/sh
${OBJDUMP} -D -address-mask=0x1ffffff -print-dbg -mcpu=r3 $1.elf > $1.lst
${OBJCOPY} -O binary -j .text $1.elf text.bin
${OBJCOPY} -O binary -j .data  $1.elf data.bin
${OBJCOPY} -O binary -j .moveable_slot $1.elf mov_slot.bin
${OBJCOPY} -O binary -j .data_code $1.elf data_code.bin
${OBJCOPY} -O binary -j .overlay_aec $1.elf aec.bin
${OBJCOPY} -O binary -j .overlay_aac $1.elf aac.bin
${OBJCOPY} -O binary -j .overlay_aptx $1.elf aptx.bin

${OBJDUMP} -section-headers -address-mask=0x1ffffff $1.elf
${OBJSIZEDUMP} -lite -skip-zero -enable-dbg-info $1.elf | sort -k 1 >  symbol_tbl.txt

cat text.bin data.bin mov_slot.bin data_code.bin aec.bin aac.bin aptx.bin > app.bin

#ifdef CONFIG_BR30_C_VERSION
cp br30c_p11_code.bin p11_code.bin
cp br30c_ota.bin ota.bin
cp br30c_ota_debug.bin ota_debug.bin
#else
cp br30_p11_code.bin p11_code.bin
cp br30_ota.bin ota.bin
cp br30_ota_debug.bin ota_debug.bin
#endif

/opt/utils/strip-ini -i isd_config.ini -o isd_config.ini

files="app.bin br30loader.bin br30loader.uart uboot.boot uboot.boot_debug uboot_no_ota.boot uboot_no_ota.boot_debug p11_code.bin ota.bin ota_debug.bin isd_config.ini isd_download.exe fw_add.exe ufw_maker.exe"


host-client -project ${NICKNAME}$2 -f ${files} $1.elf

#else


rem @echo off

@echo *****************************************************************
@echo 			SDK BR30
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
%OBJCOPY% -O binary -j .overlay_aec %ELFFILE% aec.bin
%OBJCOPY% -O binary -j .overlay_aac %ELFFILE% aac.bin

%OBJDUMP% -section-headers -address-mask=0x1ffffff %ELFFILE%
%OBJDUMP% -t %ELFFILE% >  symbol_tbl.txt

copy /b text.bin+data.bin+data_code.bin+aec.bin+aac.bin app.bin


#ifdef CONFIG_BR30_C_VERSION
copy br30c_p11_code.bin p11_code.bin
copy br30c_ota.bin ota.bin
copy br30c_ota_debug.bin ota_debug.bin
#else
copy br30_p11_code.bin p11_code.bin
copy br30_ota.bin ota.bin
copy br30_ota_debug.bin ota_debug.bin
#endif

#ifdef CONFIG_WATCH_CASE_ENABLE
call download/watch/download.bat
#elif defined(CONFIG_SOUNDBOX_CASE_ENABLE)
call download/soundbox/download.bat
#elif defined(CONFIG_EARPHONE_CASE_ENABLE)
#if (CONFIG_APP_BT_ENABLE == 0)
call download/earphone/download.bat
#else
call download/earphone/download_app_ota.bat
#endif
#elif defined(CONFIG_HID_CASE_ENABLE) ||defined(CONFIG_SPP_AND_LE_CASE_ENABLE)||defined(CONFIG_MESH_CASE_ENABLE)||defined(CONFIG_DONGLE_CASE_ENABLE)    //数传
call download/data_trans/download.bat
#else
//to do other case
#endif  //endif app_case

/* isd_download.exe isd_config.ini -tonorflash -dev br30 -boot 0x2000 -div8 -wait 300 -uboot uboot.boot -app app.bin cfg_tool.bin  -res  p11_code.bin %tone_file% %kws_cfg% -uboot_compress */

#endif
