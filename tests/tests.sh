#!/bin/bash

on_exit() {
	rm -f tutu.md5 toto.md5 tutu toto tutu.bz2
}

trap on_exit EXIT

set -xeu

script_dir=$(dirname $0)

${script_dir}/bzip2_test.sh
${script_dir}/curl_test.sh
