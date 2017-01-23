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

# decompression
bzip2 -fk tutu
cd ${cwd}; ./cpw bzip2://${wdir}/tutu.bz2 ${wdir}/toto; cd -
md5sum tutu > tutu.md5
md5sum toto > toto.md5
diff tutu toto

# compression
cd ${cwd}; ./cpw ${wdir}/tutu bzip2://${wdir}/tutu.bz2; cd -
bzip2 --decompress ${wdir}/tutu.bz2 --stdout > toto
md5sum tutu > tutu.md5
md5sum toto > toto.md5
diff tutu toto

