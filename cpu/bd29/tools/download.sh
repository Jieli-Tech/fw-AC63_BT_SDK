#!/bin/sh

${OBJDUMP} -D -address-mask=0x1ffffff -print-dbg $1.elf > $1.lst
${OBJCOPY} -O binary -j .text $1.elf text.bin
${OBJCOPY} -O binary -j .data $1.elf data.bin

${OBJDUMP} -section-headers -address-mask=0x1ffffff $1.elf
${OBJSIZEDUMP} -lite -skip-zero -enable-dbg-info $1.elf | sort -k 1 >  symbol_tbl.txt

cat text.bin data.bin  > app.bin

#/opt/pi32v2/bin/pi32v2-uclinux-objdump -h cpu/br22/tools/main.or32


if [ -f version ]; then
    host-client -project ${NICKNAME}$2 -f app.bin version $1.elf bd29loader.bin uboot* -app_ota_${APP_OTA_EN} 
else 
    host-client -project ${NICKNAME}$2 -f app.bin $1.elf bd29loader.bin uboot* -app_ota_${APP_OTA_EN} 

fi
