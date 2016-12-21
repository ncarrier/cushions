#!/bin/bash

export CUSHION_HANDLERS_PATH=./handlers_dir

on_exit() {
	rm -f tutu.md5 toto.md5 tutu toto tutu.bz2
}

trap on_exit EXIT

set -xeu

dd if=/dev/urandom of=tutu bs=1024 count=1024
bzip2 -fk tutu
./cp bzip2://tutu.bz2 toto
md5sum tutu > tutu.md5
md5sum toto > toto.md5
diff tutu toto
