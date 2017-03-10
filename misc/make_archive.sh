#!/bin/bash

set -xeu

if [ -d .git ]; then
	echo "not at the root of the libcushions repository"
fi

start_dir=$PWD
distrib=$(lsb_release -cs)
build_dir=${start_dir}/build_dir_${distrib}

on_exit() {
	cd ${start_dir}
	rm -rf ${build_dir}
}

trap on_exit EXIT

rm -rf ${build_dir}
mkdir ${build_dir}

cd ${build_dir}
make -f ../Makefile -j$(nproc) world check package
cd -
mkdir -p releases/${distrib}
chmod a+rwx ${build_dir}/*.deb
mv ${build_dir}/*.deb releases/${distrib}

