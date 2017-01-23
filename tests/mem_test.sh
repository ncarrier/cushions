#!/bin/bash

wdir=${PWD}/$(basename $0)/
mkdir -p ${wdir}

on_exit() {
	rm -rf ${wdir}
}

set -xeu

trap on_exit EXIT

dd if=/dev/urandom of=${wdir}tutu bs=1024 count=1024
./cpw ${wdir}tutu mem://${wdir}toto
cmp ${wdir}tutu ${wdir}toto

