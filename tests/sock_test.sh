#!/bin/bash

cwd=${PWD}
wdir=${cwd}/$(basename $0)

on_exit() {
	rm -rf ${wdir}
}

set -xeu

trap on_exit EXIT

mkdir -p ${wdir}

cd ${wdir}
dd if=/dev/urandom of=tutu bs=1024 count=1024

export LD_LIBRARY_PATH=${cwd}
export CUSHIONS_HANDLERS_PATH=${cwd}/handlers_dir

# server serves file
${cwd}/cpw tutu ssock://inet:127.0.0.1:56789 &
sleep .05 # a small sleep shortens the test
${cwd}/cpw csock://inet:127.0.0.1:56789 toto
cmp tutu toto

# client serves file
${cwd}/cpw ssock://inet:127.0.0.1:56789 toto &
sleep .05 # a small sleep shortens the test
${cwd}/cpw tutu csock://inet:127.0.0.1:56789
cmp tutu toto

