#!/bin/sh
if [  $# = 0  ] ;then echo Usage $0 xxxx.hex; exit; fi
nrfjprog='/Applications/Nordic Semiconductor/nrfjprog/nrfjprog'

partNum=`"$nrfjprog" --memrd 0x10000100 --w 32 --n 4  | cut -c16-20`    # quotes mitigate space in directoryname
if [ $partNum -ne 52811 ] ; then echo "--  RuuviGateway uses nRF52811. I see nRF$partNum ! Check T-connect cable." ; exit 1 ; fi

#$nrfjprog --eraseall # do not eraseall
"$nrfjprog" --program $1  --sectorerase --verify --fast 

##/Applications/SEGGER/JLink/JLinkExe  -Device NRF52811_XXAA -Autoconnect 1 -if SWD -Speed 4000 
## nrfjprog --pinreset

echo  "  MAC (low 2 bytes):"
"$nrfjprog"  --memrd 0x100000A4  --w 16 --n 2  |cut -f2  -d:

