#!/bin/bash

source "${BASH_SOURCE[0]%/*}"/../scripts/global.conf

source $scripts_dir/graph/add_measurement.sh
source $scripts_dir/graph/parse.sh

# Configurable from outside this script
workload=${1:-fileserver}
configuration="${2:-single_device}"
output="${3:-output.json}"
parse_func=${PRS_FUNC:-filebench_parse_ops_per_sec}
num_samples=1
ops_per_sec_sum=0

#==============#

run_filebench() {
	rm -r /mnt/pmem/phtof/daxdir/*
	$tools/filebench/filebench -f $tools/filebench/workloads/$workload.f 2> /dev/null
}


for samples in $(seq 1 $num_samples); do
	ops_per_sec=$(run_filebench | $parse_func)
	echo $ops_per_sec
	ops_per_sec=${ops_per_sec%.*} # float to int
	ops_per_sec_sum=$((ops_per_sec_sum + ops_per_sec))
done

ops_per_sec_mean=$((ops_per_sec_sum / num_samples))

filebench_add_measurement "$workload" "$configuration" "$ops_per_sec_mean" $output
