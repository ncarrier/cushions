#!/bin/bash

wdir=${PWD}/$(basename $0)/
mkdir -p ${wdir}

on_exit() {
	rm -rf ${wdir}
}

set -eu

cp=${CP_COMMAND:-./cpw}

trap on_exit EXIT

dd if=/dev/urandom of=${wdir}tutu bs=1024 count=1024

# decompression
gzip -fck ${wdir}tutu > ${wdir}tutu.gz
${cp} gzip://${wdir}tutu.gz ${wdir}toto
cmp ${wdir}tutu ${wdir}toto
rm ${wdir}tutu.gz

# compression
${cp} ${wdir}tutu gzip://${wdir}tutu.gz
gunzip ${wdir}tutu.gz --stdout > ${wdir}toto
cmp ${wdir}tutu ${wdir}toto

