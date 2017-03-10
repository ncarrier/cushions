#!/bin/bash

if [ ! -d .git ]; then
	echo "not at the root of the libcushions repository"
	exit 1
fi

set -xeu

version=$1

rm -rf build_version

mkdir build_version

cd build_version
make -f ../Makefile -j $(nproc) CU_NEW_VERSION=${version} version

docker_cmd="docker run --rm -v ${OLDPWD}:/workspace"

for d in trusty xenial precise; do
	${docker_cmd} ${d}_for_cushions ./misc/make_archive.sh &
done
wait
