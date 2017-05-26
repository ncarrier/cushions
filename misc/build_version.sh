#!/bin/bash

if [ ! -d .git ]; then
	echo "not at the root of the libcushions repository"
	exit 1
fi

set -xeu

version=$1
start_dir=$PWD

rm -rf build_version

mkdir build_version

cd build_version
make -f ../Makefile -j $(nproc) CU_NEW_VERSION=${version} version

docker_cmd="docker run --rm -v ${OLDPWD}:/workspace --user $(id -u):$(id -g)"

cd ${start_dir}
for f in $(find docker/ -name Dockerfile); do
	(d=$(echo $f | cut -d / -f 3)
	docker build -t ${d}_for_cushions $(dirname ${f})
	${docker_cmd} ${d}_for_cushions ./misc/make_archive.sh)
done
wait
