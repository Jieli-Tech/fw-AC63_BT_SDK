#!/bin/sh

${OBJDUMP} -D -address-mask=0x1ffffff -print-dbg $1.elf > $1.lst
${OBJCOPY} -O binary -j .text $1.elf text.bin
${OBJCOPY} -O binary -j .data $1.elf data.bin

${OBJCOPY} -O binary -j .overlay_wav $1.elf wav.bin
${OBJCOPY} -O binary -j .overlay_ape $1.elf ape.bin
${OBJCOPY} -O binary -j .overlay_flac $1.elf flac.bin
${OBJCOPY} -O binary -j .overlay_m4a $1.elf m4a.bin
${OBJCOPY} -O binary -j .overlay_amr $1.elf amr.bin
${OBJCOPY} -O binary -j .overlay_aec $1.elf aeco.bin
${OBJCOPY} -O binary -j .overlay_dts $1.elf dts.bin
${OBJCOPY} -O binary -j .overlay_fm $1.elf fmo.bin
${OBJCOPY} -O binary -j .overlay_mp3 $1.elf mp3o.bin
${OBJCOPY} -O binary -j .overlay_wma $1.elf wmao.bin


/opt/utils/remove_tailing_zeros -i aeco.bin -o aec.bin -mark ff 
/opt/utils/remove_tailing_zeros -i fmo.bin -o fm.bin -mark ff 
/opt/utils/remove_tailing_zeros -i mp3o.bin -o mp3.bin -mark ff 
/opt/utils/remove_tailing_zeros -i wmao.bin -o wma.bin -mark ff 


#bank_files=
#for i in $(seq 0 20)
#do
#    ${OBJCOPY} -O binary -j .overlay_bank$i $1.elf bank$i.bin
#    if [ ! -s bank$i.bin ] 
#    then
#        break
#    fi
#    bank_files=$bank_files"bank$i.bin 0x0 "
#done
#echo $bank_files
#lz4_packet -dict text.bin -input common.bin 0 $bank_files -o bank.bin
#

${OBJDUMP} -section-headers -address-mask=0x1ffffff $1.elf
${OBJSIZEDUMP} -lite -skip-zero -enable-dbg-info $1.elf | sort -k 1 >  symbol_tbl.txt

cat text.bin data.bin aec.bin wav.bin ape.bin flac.bin m4a.bin amr.bin dts.bin fm.bin  mp3.bin wma.bin > app.bin

#/opt/pi32v2/bin/pi32v2-uclinux-objdump -h cpu/br22/tools/main.or32


if [ -f version ]; then
    host-client -project ${NICKNAME}_${APP_CASE} -f app.bin version $1.elf br23loader.* uboot* -app_ota_${APP_OTA_EN}
else 
    host-client -project ${NICKNAME}_${APP_CASE} -f app.bin $1.elf br23loader.* uboot* -app_ota_${APP_OTA_EN}

fi
