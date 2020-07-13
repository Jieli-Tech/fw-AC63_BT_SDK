




##!/bin/sh
${OBJDUMP} -D -address-mask=0x1ffffff -print-dbg $1.elf > $1.lst
${OBJCOPY} -O binary -j .text $1.elf text.bin
${OBJCOPY} -O binary -j .data $1.elf data.bin

${OBJDUMP} -section-headers -address-mask=0x1ffffff $1.elf
${OBJSIZEDUMP} -lite -skip-zero -enable-dbg-info $1.elf | sort -k 1 > symbol_tbl.txt

cat text.bin data.bin > app.bin

files="app.bin bd29loader.bin uboot.boot uboot.boot_debug uboot_no_ota.boot uboot_no_ota.boot_debug ota.bin isd_config.ini isd_download.exe fw_add.exe ufw_maker.exe"
NICKNAME="bd29_sdk"
cp bluetooth/standard/isd_config.ini ./




host-client -project ${NICKNAME}$2 -f ${files} $1.elf
