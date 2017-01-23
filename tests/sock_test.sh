#!/bin/bash

wdir=${PWD}/$(basename $0)/
mkdir -p ${wdir}

on_exit() {
	rm -rf ${wdir}
}

set -xeu

trap on_exit EXIT

dd if=/dev/urandom of=${wdir}tutu bs=1024 count=1024

# server serves file
./cpw ${wdir}tutu ssock://inet:127.0.0.1:56789 &
sleep .05 # a small sleep shortens the test
./cpw csock://inet:127.0.0.1:56789 ${wdir}toto
cmp ${wdir}tutu ${wdir}toto

# client serves file
./cpw ssock://inet:127.0.0.1:56789 ${wdir}toto &
sleep .05 # a small sleep shortens the test
./cpw ${wdir}tutu csock://inet:127.0.0.1:56789
cmp ${wdir}tutu ${wdir}toto

