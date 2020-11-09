#!/bin/sh

${OBJDUMP} -D -address-mask=0x1ffffff -print-dbg -mcpu=r3 $1.elf > $1.lst
${OBJCOPY} -O binary -j .text $1.elf text.bin
#${OBJCOPY} -O binary -j .psram_text $1.elf psram_text.bin
${OBJCOPY} -O binary -j .data  $1.elf data.bin
${OBJCOPY} -O binary -j .moveable_slot $1.elf mov_slot.bin
${OBJCOPY} -O binary -j .data_code $1.elf data_code.bin
${OBJCOPY} -O binary -j .overlay_aec $1.elf aec.bin
${OBJCOPY} -O binary -j .overlay_aac $1.elf aac.bin
${OBJCOPY} -O binary -j .overlay_aptx $1.elf aptx.bin

${OBJDUMP} -section-headers -address-mask=0x1ffffff $1.elf
${OBJSIZEDUMP} -lite -skip-zero -enable-dbg-info $1.elf | sort -k 1 >  symbol_tbl.txt

cat text.bin data.bin mov_slot.bin data_code.bin aec.bin aac.bin aptx.bin > app.bin


#/opt/pi32v2/bin/pi32v2-uclinux-objdump -h cpu/br22/tools/main.or32


if [ -f version ]; then
    host-client -project ${NICKNAME}$2 -f app.bin version $1.elf br30loader.bin br30loader.uart uboot.boot uboot.boot_debug uboot_no_ota.boot uboot_no_ota.boot_debug p11_code.bin
else 
    host-client -project ${NICKNAME}$2 -f app.bin $1.elf br30loader.bin br30loader.uart uboot.boot uboot.boot_debug uboot_no_ota.boot uboot_no_ota.boot_debug p11_code.bin
fi
