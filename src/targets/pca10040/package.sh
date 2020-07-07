#!/bin/bash
cd "$(dirname "$0")"
NAME="ruuvigw"
VERSION=$(git describe --exact-match --tags HEAD)
if [ -z "$VERSION" ]; then
  VERSION=$(git rev-parse --short HEAD)
fi

while getopts "n:v:" option;
do
case "${option}"
in
n) NAME=${OPTARG};;
v) VERSION=${OPTARG};;
esac
done
BINNAME=pca10040\_armgcc\_${NAME}\_${VERSION}

rm pca10040_armgcc*${NAME}*

mergehex -m ../../../nRF5_SDK_15.3.0_59ac345/components/softdevice/s132/hex/s132_nrf52_6.1.1_softdevice.hex  _build/nrf52832_xxaa.hex -o packet.hex
mv packet.hex ${BINNAME}\_full.hex
mv _build/nrf52832_xxaa.map ${BINNAME}\_app.map
mv _build/nrf52832_xxaa.hex ${BINNAME}\_app.hex
