#!/bin/bash

wdir=${PWD}/$(basename $0)/
mkdir -p ${wdir}

on_exit() {
	set +e
	rm -rf ${wdir}
	kill -15 ${pid}
	wait ${pid}
	echo "http server exited with status $?"
}

set -eu
if [ "${CUSHIONS_LOG_LEVEL}" -gt 2 ]; then
	set -x
fi

cp=${CP_COMMAND:-./cpw}

python -m SimpleHTTPServer &
pid=$!

trap on_exit EXIT

${cp} http://localhost:8000/world.d ${wdir}world.d
cmp world.d ${wdir}world.d

