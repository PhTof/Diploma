#!/bin/bash

source "${BASH_SOURCE[0]%/*}"/../global.conf
source $scripts_dir/numa/numa_node_of_file.sh

filebench_install_dir=$tools/filebench
filebench_work_dir=$workdir/bigfileset

echo 0 > /proc/sys/kernel/randomize_va_space

# /root/github/philipos-5.13/linux-5.13/tools/perf/perf ftrace \
# --inherit -T ext4_*alloc* \
$filebench_install_dir/filebench -f $filebench_install_dir/workloads/varmail.f

sync

for f in $filebench_work_dir/**/*; do
	if [ $((1 + $RANDOM % 10000)) -gt 10 ]; then
		# Skip some files, as we have way too
		# many to check
		continue
	fi
	last_digit=${f: -1}
	parity=$((last_digit % 2))
	res=$(numa_node_of_file $f)
	if [[ "$res" -eq "$parity" ]]; then
		printf "${GREEN}OK!${NC} %s\n" $f
	else
		printf "${RED}WRONG${NC} %s\n" $f
		print_numa_node_of_file $f
	fi
done
