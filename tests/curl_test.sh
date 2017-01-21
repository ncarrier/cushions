#!/bin/bash

set -xeu

wdir=$(basename $0)

on_exit() {
	kill ${pid}
	rm -rf ${wdir}
}

set -xeu

python -m SimpleHTTPServer &

trap on_exit EXIT

pid=$!
mkdir -p ${wdir}
./cpw http://localhost:8000/libcushions.so ${wdir}/libcushions.so
md5sum libcushions.so > ${wdir}/libcushions.so.md5sum
cd ${wdir}
md5sum -c libcushions.so.md5sum
cd -
