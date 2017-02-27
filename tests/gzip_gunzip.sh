#!/bin/bash

set -xeu

on_exit() {
	rm -f test_file test_file.backup test_file.gz
}

trap on_exit EXIT

dd if=/dev/urandom of=test_file bs=1024 count=1024

./gzip test_file
mv test_file{,.backup}
./gunzip test_file.gz

cmp test_file test_file.backup
