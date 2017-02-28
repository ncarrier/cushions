#!/bin/bash

wdir=${PWD}/$(basename $0)/
mkdir -p ${wdir}

on_exit() {
	rm -rf ${wdir}
}

set -xeu

cp=${CP_COMMAND:-./cpw}

trap on_exit EXIT

dd if=/dev/urandom of=${wdir}tutu bs=1024 count=1024

# compression
${cp} ${wdir}tutu gzip://${wdir}tutu.gz
gunzip ${wdir}tutu.gz --stdout > ${wdir}toto
cmp ${wdir}tutu ${wdir}toto

