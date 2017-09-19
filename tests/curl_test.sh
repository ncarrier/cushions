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

i=10
while [ $i -gt 0 ]; do
	# retry until the http server is up, or a timeout of 1 second
	# because cp can't (yet) detect errors, we rely on cmp to know if we can
	# halt the loop
	${cp} http://localhost:8000/world.d ${wdir}world.d
	cmp world.d ${wdir}world.d && break
	# test failed, buy the simple HTTP server some time in case it wasn't ready
	sleep .1
	i=$(($i - 1))
	rm ${wdir}world.d
done

# we redo one more cmp, because the previous' one status code has been consumed
# by the &&
cmp world.d ${wdir}world.d
