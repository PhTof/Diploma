#!/bin/bash

# $kernel_dir
source "${BASH_SOURCE[0]%/*}"/../global.conf
cpupowerdir=$kernel_dir/tools/power/cpupower

if [ ! -f $cpupowerdir/libcpupower.so ]; then
	make -C $cpupowerdir
fi

if [ $(id -u) -ne 0 ]; then 
	echo "$0 must be run as root. Exiting..."
	exit 
fi
