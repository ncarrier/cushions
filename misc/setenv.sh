#!/bin/bash

LD_LIBRARY_PATH=$(realpath .) CUSHIONS_HANDLERS_PATH=$(realpath handlers_dir) CUSHIONS_LOG_LEVEL=3 "$@"
