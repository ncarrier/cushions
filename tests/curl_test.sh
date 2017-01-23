#!/bin/bash

wdir=${PWD}/$(basename $0)/
mkdir -p ${wdir}

on_exit() {
	rm -rf ${wdir}
	kill ${pid}
}

set -xeu

python -m SimpleHTTPServer &
pid=$!

trap on_exit EXIT

./cpw http://localhost:8000/libcushions.so ${wdir}libcushions.so
cmp libcushions.so ${wdir}libcushions.so

