#!/bin/bash

perf=$(echo $HOME/linux-5.13/tools/perf/perf)
func=submit_bio
file=/mnt/pmem0/nodax
blocksize=$((1024*1024))

while : ; do
	echo $func
	echo 3 > /proc/sys/vm/drop_caches
	func=$($perf ftrace -T "$func" ./simple $file $blocksize | grep simple | tail -1 | awk '{print $NF}' | cut -c 3-)
	if [[ -z "$func" || "$func" == "do_syscall_64" ]]; then
		break
	fi
done
