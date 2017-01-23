#!/bin/bash

wdir=${PWD}/$(basename $0)/
mkdir -p ${wdir}

on_exit() {
	rm -rf ${wdir}
}

set -xeu

trap on_exit EXIT

dd if=/dev/urandom of=${wdir}tutu bs=1024 count=1024

# compression
./cpw ${wdir}tutu lzop://${wdir}tutu.lzo
lzop -dfo ${wdir}toto ${wdir}tutu.lzo
cmp ${wdir}tutu ${wdir}toto

