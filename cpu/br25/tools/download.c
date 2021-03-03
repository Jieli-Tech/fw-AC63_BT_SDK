// *INDENT-OFF*
#include "app_config.h"

#ifdef __SHELL__

##!/bin/sh
${OBJDUMP} -D -address-mask=0x1ffffff -print-dbg $1.elf > $1.lst
${OBJCOPY} -O binary -j .text $1.elf text.bin
${OBJCOPY} -O binary -j .data $1.elf data.bin
${OBJCOPY} -O binary -j .data_code $1.elf data_code.bin

${OBJCOPY} -O binary -j .overlay_aec $1.elf aeco.bin
${OBJCOPY} -O binary -j .overlay_wav $1.elf wavo.bin
${OBJCOPY} -O binary -j .overlay_ape $1.elf apeo.bin
${OBJCOPY} -O binary -j .overlay_flac $1.elf flaco.bin
${OBJCOPY} -O binary -j .overlay_m4a $1.elf m4ao.bin
${OBJCOPY} -O binary -j .overlay_amr $1.elf amro.bin
${OBJCOPY} -O binary -j .overlay_dts $1.elf dtso.bin
${OBJCOPY} -O binary -j .overlay_fm $1.elf fmo.bin
${OBJCOPY} -O binary -j .overlay_mp3 $1.elf mp3o.bin
${OBJCOPY} -O binary -j .overlay_wma $1.elf wmao.bin



/opt/utils/remove_tailing_zeros -i aeco.bin -o aec.bin -mark ff
/opt/utils/remove_tailing_zeros -i wavo.bin -o wav.bin -mark ff
/opt/utils/remove_tailing_zeros -i apeo.bin -o ape.bin -mark ff
/opt/utils/remove_tailing_zeros -i flaco.bin -o flac.bin -mark ff
/opt/utils/remove_tailing_zeros -i m4ao.bin -o m4a.bin -mark ff
/opt/utils/remove_tailing_zeros -i amro.bin -o amr.bin -mark ff
/opt/utils/remove_tailing_zeros -i dtso.bin -o dts.bin -mark ff
/opt/utils/remove_tailing_zeros -i fmo.bin -o fm.bin -mark ff
/opt/utils/remove_tailing_zeros -i mp3o.bin -o mp3.bin -mark ff
/opt/utils/remove_tailing_zeros -i wmao.bin -o wma.bin -mark ff


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


${OBJDUMP} -section-headers -address-mask=0x1ffffff $1.elf
${OBJSIZEDUMP} -lite -skip-zero -enable-dbg-info $1.elf | sort -k 1 >  symbol_tbl.txt

cat text.bin data.bin data_code.bin \
	aec.bin \
	wav.bin \
	ape.bin \
	flac.bin \
	m4a.bin \
	amr.bin \
	dts.bin \
	fm.bin \
	mp3.bin \
	wma.bin \
	bank.bin \
	> app.bin


files="app.bin br25loader.bin br25loader.uart uboot.boot uboot.boot_debug uboot_no_ota.boot uboot_no_ota.boot_debug uboot_lrc.boot uboot_lrc.boot_debug ota_lrc.bin ota.bin isd_config.ini isd_download.exe fw_add.exe ufw_maker.exe"

#if(CONFIG_SPP_AND_LE_CASE_ENABLE || CONFIG_HID_CASE_ENABLE || CONFIG_MESH_CASE_ENABLE || CONFIG_GAMEBOX_CASE)
#if RCSP_UPDATE_EN

NICKNAME="br25_app_ota"
cp bluetooth/app_ota/isd_config.ini ./
#else

NICKNAME="br25_sdk"
cp bluetooth/standard/isd_config.ini ./
#endif

#endif

#if CONFIG_SOUNDBOX_CASE_ENABLE

#ifdef CONFIG_APP_BT_ENABLE
#if CONFIG_DOUBLE_BANK_ENABLE
NICKNAME="br25_ai_double_bank"
cp soundbox/ai_double_bank/isd_config.ini ./
#else
NICKNAME="br25_ai_single_bank"
cp soundbox/ai_single_bank/isd_config.ini ./
#endif

#else

#ifdef CONFIG_SOUNDBOX_FLASH_256K
NICKNAME="br25_standard_2m_flash"
cp soundbox/standard_2m_flash/isd_config.ini ./
#elif defined CONFIG_SOUNDBOX_608N
NICKNAME="br_standard_608n"
cp soundbox/standard_608n/isd_config.ini ./
#else
NICKNAME="br25_standard"
cp soundbox/standard/isd_config.ini ./
#endif

#endif
#endif


host-client -project ${NICKNAME}$2 -f ${files} $1.elf

#else


rem @echo off

@echo *****************************************************************
@echo 			SDK BR25
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
%OBJDUMP% -t %ELFFILE% >  symbol_tbl.txt

copy /b text.bin+data.bin+data_code.bin+aec.bin+wav.bin+ape.bin+flac.bin+m4a.bin+amr.bin+dts.bin+fm.bin+mp3.bin+wma.bin+bank.bin app.bin

#if CONFIG_SPP_AND_LE_CASE_ENABLE || CONFIG_HID_CASE_ENABLE || CONFIG_MESH_CASE_ENABLE || CONFIG_GAMEBOX_CASE
#if RCSP_UPDATE_EN

#if TCFG_KWS_VOICE_RECOGNITION_ENABLE
set kws_cfg=..\..\jl_kws.cfg
#endif

copy app.bin bluetooth\app_ota\app.bin
copy br25loader.bin bluetooth\app_ota\br25loader.bin

bluetooth\app_ota\download.bat %kws_cfg%
#else
copy app.bin bluetooth\standard\app.bin
copy br25loader.bin bluetooth\standard\br25loader.bin

bluetooth\standard\download.bat %kws_cfg%
#endif

#endif      //endif CONFIG_SPP_AND_LE_CASE_ENABLE || CONFIG_HID_CASE_ENABLE || CONFIG_MESH_CASE_ENABLE || CONFIG_GAMEBOX_CASE

#if CONFIG_SOUNDBOX_CASE_ENABLE

#ifdef CONFIG_APP_BT_ENABLE
#if CONFIG_DOUBLE_BANK_ENABLE
copy app.bin soundbox\ai_double_bank\app.bin
copy br25loader.bin soundbox\ai_double_bank\br25loader.bin
soundbox\ai_double_bank\download.bat
#else
copy app.bin soundbox\ai_single_bank\app.bin
copy br25loader.bin soundbox\ai_single_bank\br25loader.bin
soundbox\ai_single_bank\download.bat
#endif      //CONFIG_DOUBLE_BANK_ENABLE
#else
#ifdef CONFIG_SOUNDBOX_FLASH_256K
copy app.bin soundbox\standard_2m_flash\app.bin
copy br25loader.bin soundbox\standard_2m_flash\br25loader.bin

soundbox\standard_2m_flash\download.bat
#elif defined CONFIG_SOUNDBOX_608N
copy app.bin soundbox\standard_608n\app.bin
copy br25loader.bin soundbox\standard_608n\br25loader.bin

soundbox\standard_608n\download.bat
#else
copy app.bin soundbox\standard\app.bin
copy br25loader.bin soundbox\standard\br25loader.bin

soundbox\standard\download.bat
#endif
#endif

#endif      //endif CONFIG_SOUNDBOX_CASE_ENABLE



#endif

