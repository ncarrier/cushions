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
# these 3 tests don't work since they confuse diff plus mknod requires cap_mknod
# mkfifo ${wdir}/test_dir/fifo
# sudo mknod ${wdir}/test_dir/block b 1 2
# sudo mknod ${wdir}/test_dir/char c 3 4
touch -t 198302080000 ${wdir}test_dir/tutu
touch -t 198212080000 ${wdir}test_dir/tata

# archive extraction, cd to wdir since we don't want absolute dirs in archive
cd ${wdir}
tar cf test_dir.tar test_dir
mkdir extracted_dir
${build_dir}/cpw test_dir.tar tar://extracted_dir
diff test_dir extracted_dir/test_dir

