#!/bin/bash


../misc/setenv.sh valgrind --leak-check=full \
		--show-leak-kinds=all \
		--suppressions=../misc/valgrind.supp \
		--suppressions=../misc/curl_ssh2_valgrind.supp \
		--track-fds=yes "$@"
