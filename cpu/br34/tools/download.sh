#!/bin/sh

${OBJDUMP} -D -address-mask=0x1ffffff -print-dbg -mcpu=r3 $1.elf > $1.lst
${OBJCOPY} -O binary -j .text $1.elf text.bin
#${OBJCOPY} -O binary -j .psram_text $1.elf psram_text.bin
${OBJCOPY} -O binary -j .data  $1.elf data.bin
#${OBJCOPY} -O binary -j .moveable_slot $1.elf mov_slot.bin
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
lz4_packet -dict text.bin -input common.bin 0 aec.bin 0 aac.bin 0 $bank_files -o bank.bin

${OBJDUMP} -section-headers -address-mask=0x1ffffff $1.elf
${OBJSIZEDUMP} -lite -skip-zero -enable-dbg-info $1.elf | sort -k 1 >  symbol_tbl.txt

cat text.bin data.bin data_code.bin bank.bin > app.bin

#/opt/pi32v2/bin/pi32v2-uclinux-objdump -h cpu/br22/tools/main.or32

files="app.bin br34loader.bin br34loader.uart uboot.boot p11_code.bin ota.bin ota_debug.bin"

host-client -project ${NICKNAME}$2 -f ${files} $1.elf

