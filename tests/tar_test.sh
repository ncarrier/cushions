#!/bin/bash

build_dir=${PWD}
wdir=${build_dir}/$(basename $0)/
mkdir -p ${wdir}

on_exit() {
	rm -rf ${wdir}
}

set -xeu

trap on_exit EXIT

# create test tree structure
mkdir ${wdir}test_dir
dd if=/dev/urandom of=${wdir}test_dir/tutu bs=1024 count=1024
mkdir ${wdir}test_dir/tata
echo titi > ${wdir}test_dir/tata/toto
# TODO add test files for :
# TYPE_FLAG_LINK
# TYPE_FLAG_SYMLINK
# TYPE_FLAG_CHAR
# TYPE_FLAG_BLOCK
# TYPE_FLAG_DIRECTORY
# TYPE_FLAG_FIFO

# archive extraction, cd to wdir since we don't want absolute dirs in archive
cd ${wdir}
tar cf test_dir.tar test_dir
mkdir extracted_dir
# TODO dest support isn't yet implement, we're forced to cd to where we want to
# extract
cd extracted_dir
${build_dir}/cpw ../test_dir.tar tar://extracted_dir
cd -
diff test_dir extracted_dir/test_dir

