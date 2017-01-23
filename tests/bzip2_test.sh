#!/bin/bash

wdir=${PWD}/$(basename $0)/
mkdir -p ${wdir}

on_exit() {
	rm -rf ${wdir}
}

set -xeu

trap on_exit EXIT

dd if=/dev/urandom of=${wdir}tutu bs=1024 count=1024

# decompression
bzip2 -fck ${wdir}tutu > ${wdir}tutu.bz2
./cpw bzip2://${wdir}tutu.bz2 ${wdir}toto
cmp ${wdir}tutu ${wdir}toto

# compression
./cpw ${wdir}tutu bzip2://${wdir}tutu.bz2
bzip2 --decompress ${wdir}tutu.bz2 --stdout > ${wdir}toto
cmp ${wdir}tutu ${wdir}toto

