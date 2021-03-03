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
#else
cp br30_p11_code.bin p11_code.bin
#endif /* #ifdef CONFIG_BR30_C_VERSION */

#ifdef CONFIG_BR30_C_VERSION
cp br30c_ota.bin ota.bin
cp br30c_ota_debug.bin ota_debug.bin
#else
cp br30_ota.bin ota.bin
cp br30_ota_debug.bin ota_debug.bin
#endif /* #ifdef CONFIG_BR30_C_VERSION */

files="app.bin br30loader.bin br30loader.uart uboot.boot uboot.boot_debug uboot_no_ota.boot uboot_no_ota.boot_debug p11_code.bin ota.bin ota_debug.bin isd_config.ini isd_download.exe fw_add.exe ufw_maker.exe"

#ifdef CONFIG_EARPHONE_CASE_ENABLE


#ifdef CONFIG_APP_BT_ENABLE

#if CONFIG_DOUBLE_BANK_ENABLE
NICKNAME="br30_ai_double_bank"
/* cp earphone/ai_double_bank/isd_config.ini ./isd_config.ini */
#else
NICKNAME="br30_ai_single_bank"
/* cp earphone/ai_single_bank/isd_config_AC897N.ini ./isd_config.ini */
#endif
#elif TCFG_AUDIO_ANC_ENABLE
NICKNAME="br30_ANC"
#ifdef CONFIG_ANC_30C_ENABLE
/* cp earphone/ANC/isd_config_AC699N.ini ./isd_config.ini */
#else
/* cp earphone/ANC/isd_config_AC897N.ini ./isd_config.ini */
#endif

#else

NICKNAME="br30_standard"
chip_name=CONFIG_CHIP_NAME
/* cp earphone/standard/isd_config_$chip_name.ini ./isd_config.ini */
#endif

#endif


#if(CONFIG_SPP_AND_LE_CASE_ENABLE || CONFIG_HID_CASE_ENABLE || CONFIG_MESH_CASE_ENABLE || CONFIG_GAMEBOX_CASE)
#if RCSP_UPDATE_EN
NICKNAME="br30_single_bank"
/* cp bluetooth/ai_single_bank/isd_config_AD697N.ini ./isd_config.ini */
#else
NICKNAME="br30_standard"
/* cp bluetooth/standard/isd_config_AD697N.ini ./isd_config.ini */
#endif
#endif      //通用蓝牙设备


#ifdef CONFIG_QCY_CASE_ENABLE

#ifdef CONFIG_BOARD_AC6972A_ALI_T8
#if CONFIG_DOUBLE_BANK_ENABLE
NICKNAME="T8_AL_double_bank"
cp QCY/isd_config_d.ini ./isd_config.ini
#else
NICKNAME="T8_AL_single_bank"
cp QCY/isd_config_s.ini ./isd_config.ini
#endif //CONFIG_DOUBLE_BANK_ENABLE
#endif //CONFIG_BOARD_AC6972A_ALI_T8

#ifdef CONFIG_BOARD_AC6972A_QCY_T8
#if CONFIG_DOUBLE_BANK_ENABLE
NICKNAME="T8_QCY_double_bank"
cp QCY/isd_config_QCY_T8.ini ./isd_config.ini
#else
NICKNAME="T8_QCY_single_bank"
cp QCY/isd_config_s.ini ./isd_config.ini
#endif //CONFIG_DOUBLE_BANK_ENABLE
#endif //CONFIG_BOARD_AC6972A_QCY_T8

#ifdef CONFIG_BOARD_AC6976A_QCY_AT02
NICKNAME="AT02_QCY"
cp QCY/isd_config_AT02.ini ./isd_config.ini
#endif //CONFIG_BOARD_AC6976A_QCY_AT02

#ifdef CONFIG_BOARD_AC6976A_ALI_AT02
NICKNAME="AT02_ALI"
cp QCY/isd_config_AT02.ini ./isd_config.ini
#endif //CONFIG_BOARD_AC6976A_ALI_AT02

#ifdef CONFIG_BOARD_AC6972A_QCY_T5
NICKNAME="T5"
cp QCY/isd_config_T5.ini ./isd_config.ini
#endif //CONFIG_BOARD_AC6972A_QCY_T5

#ifdef CONFIG_BOARD_AC8976A_NPU_T8
#if CONFIG_DOUBLE_BANK_ENABLE
NICKNAME="NPU_T8_double_bank"
cp QCY/isd_config_QCY_T8.ini ./isd_config.ini
#else
NICKNAME="NPU_T8_single_bank"
cp QCY/isd_config_s.ini ./isd_config.ini
#endif //CONFIG_DOUBLE_BANK_ENABLE
#endif /* #ifdef CONFIG_BOARD_AC6976A_NPU_T8 */

#ifdef CONFIG_BOARD_AC8976A_NPU_XM
NICKNAME="NPU_XM"
cp QCY/isd_config_XM.ini ./isd_config.ini
#endif /* #ifdef CONFIG_BOARD_AC8976A_NPU_XM */


#endif //CONFIG_QCY_CASE_ENABLE

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

#if TCFG_KWS_VOICE_RECOGNITION_ENABLE
set kws_cfg=..\..\jl_kws.cfg
#endif

#ifdef CONFIG_BR30_C_VERSION
copy br30c_p11_code.bin p11_code.bin
copy br30c_ota.bin ota.bin
cp br30c_ota_debug.bin ota_debug.bin
#else
copy br30_p11_code.bin p11_code.bin
copy br30_ota.bin ota.bin
cp br30_ota_debug.bin ota_debug.bin
#endif /* #ifdef CONFIG_BR30_C_VERSION */

#if CONFIG_EARPHONE_CASE_ENABLE
#ifdef CONFIG_APP_BT_ENABLE
#if CONFIG_DOUBLE_BANK_ENABLE
copy app.bin earphone\ai_double_bank\app.bin
copy br30loader.bin earphone\ai_double_bank\br30loader.bin
copy br30loader.uart earphone\ai_double_bank\br30loader.uart

earphone\ai_double_bank\download.bat CONFIG_CHIP_NAME %kws_cfg%
#else
copy app.bin earphone\ai_single_bank\app.bin
copy br30loader.bin earphone\ai_single_bank\br30loader.bin
copy br30loader.uart earphone\ai_single_bank\br30loader.uart

earphone\ai_single_bank\download.bat CONFIG_CHIP_NAME %kws_cfg%
#endif/*CONFIG_DOUBLE_BANK_ENABLE*/

#elif TCFG_AUDIO_ANC_ENABLE /*ANC下载目录*/
copy app.bin earphone\ANC\app.bin
copy br30loader.bin earphone\ANC\br30loader.bin
copy br30loader.uart earphone\ANC\br30loader.uart

earphone\ANC\download.bat CONFIG_CHIP_NAME %kws_cfg%
#else
copy app.bin earphone\standard\app.bin
copy br30loader.bin earphone\standard\br30loader.bin
copy br30loader.uart earphone\standard\br30loader.uart

earphone\standard\download.bat CONFIG_CHIP_NAME %kws_cfg%
#endif

#endif //CONFIG_EARPHONE_CASE_ENABLE

#if CONFIG_SPP_AND_LE_CASE_ENABLE || CONFIG_HID_CASE_ENABLE || CONFIG_MESH_CASE_ENABLE || CONFIG_GAMEBOX_CASE
#if RCSP_UPDATE_EN
copy app.bin bluetooth\app_ota\app.bin
copy br30loader.bin bluetooth\app_ota\br30loader.bin

bluetooth\app_ota\download.bat CONFIG_CHIP_NAME %kws_cfg%
#else
copy app.bin bluetooth\standard\app.bin
copy br30loader.bin bluetooth\standard\br30loader.bin

bluetooth\standard\download.bat CONFIG_CHIP_NAME %kws_cfg%
#endif

#endif      //endif CONFIG_SPP_AND_LE_CASE_ENABLE || CONFIG_HID_CASE_ENABLE || CONFIG_MESH_CASE_ENABLE || CONFIG_GAMEBOX_CASE




#endif
