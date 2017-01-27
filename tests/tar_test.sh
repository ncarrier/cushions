#!/bin/bash

build_dir=${PWD}
wdir=${build_dir}/$(basename $0)/
rm -rf ${wdir}
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
ln ${wdir}test_dir/tata/toto ${wdir}/test_dir/tete
ln -s tata/toto ${wdir}/test_dir/foo
# TODO add test files for :
# TYPE_FLAG_CHAR
# TYPE_FLAG_BLOCK
# TYPE_FLAG_FIFO

# archive extraction, cd to wdir since we don't want absolute dirs in archive
cd ${wdir}
tar cf test_dir.tar test_dir
mkdir extracted_dir
${build_dir}/cpw test_dir.tar tar://extracted_dir
diff test_dir extracted_dir/test_dir

