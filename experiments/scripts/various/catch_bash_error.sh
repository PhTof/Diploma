#!/bin/bash

set -eE -o functrace

failure() {
	local lineno=$1
	local msg=$2
	echo "Failed at $lineno: $msg"
}

trap 'failure ${LINENO} "$BASH_COMMAND"' ERR

# do something error prone here
