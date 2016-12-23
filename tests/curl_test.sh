#!/bin/bash

set -xeu

on_exit() {
	kill ${pid}
	rm -f toto.so
}

set -xeu

python -m SimpleHTTPServer &

trap on_exit EXIT

pid=$!

cp http://libcushions.so toto.so
