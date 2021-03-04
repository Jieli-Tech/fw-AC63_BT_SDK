






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


files="app.bin  p11_code.bin ota.bin ota_debug.bin uboot.boot isd_config.ini isd_download.exe bd19loader.bin"
NICKNAME="bd19_sdk"
cp bluetooth/standard/isd_config.ini ./
cp bluetooth/standard/download.bat ./



host-client -project ${NICKNAME}$2 -f ${files} $1.elf
