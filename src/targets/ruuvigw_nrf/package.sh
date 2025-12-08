#!/bin/bash
set -x #echo on
cd "$(dirname "$0")"
NAME="ruuvigw"

VERSION=$(git describe --exact-match --tags HEAD 2>/dev/null || git rev-parse --short HEAD)
if [[ -n $(git status --porcelain) ]]; then
  VERSION="${VERSION}-dirty"
fi

while getopts "n:v:" option;
do
case "${option}"
in
n) NAME=${OPTARG};;
v) VERSION=${OPTARG};;
*)
esac
done

BINNAME=ruuvigw_nrf_armgcc_${NAME}_${VERSION}

rm ruuvigw_nrf*${NAME}*

mergehex -m ../../../nRF5_SDK_15.3.0_59ac345/components/softdevice/s140/hex/s140_nrf52_6.1.1_softdevice.hex _build/nrf52811_xxaa.hex -o packet.hex

# Check if the version was obtained from a git tag
if [[ $VERSION =~ ^v[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  # Version is from a git tag,
  # use generate_uicr_hex.py to generate the UICR HEX file
  python3 ../../../scripts/generate_uicr_hex.py "$VERSION" uicr_data.hex
  mergehex -m uicr_data.hex packet.hex -o packet_with_uicr.hex
  mv packet_with_uicr.hex ${BINNAME}\_full.hex
else
  mv packet.hex ${BINNAME}\_full.hex
fi
mv _build/nrf52811_xxaa.map ${BINNAME}\_app.map
mv _build/nrf52811_xxaa.hex ${BINNAME}\_app.hex
