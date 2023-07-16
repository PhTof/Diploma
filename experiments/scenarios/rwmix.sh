#!/bin/bash

source "${BASH_SOURCE[0]%/*}"/../scripts/global.conf

source $scripts_dir/mount/mount_utilities.sh
source $scripts_dir/workspace/workspace_utilities.sh

id=rwmix
fio_instance=$base_dir/instances/fio_instance.sh
output_json=$base_dir/graphs/json/$id.json
output_graph=$base_dir/graphs/results/$id.png

function run_instances {
	output=$1

	export OUT=$output SIZE=256M
	export ENG=sync

	threads=32
	for rthreads in 0 16 32; do
		for rw in 0 50 100; do
			for atype in dax; do
				format="threads = %d rthreads = %d rw = %d\n"
				printf "$format" $threads $rthreads $rw
				#========================================#
				rm -r /mnt/pmem/phtof/daxdir/*
				export THR=$threads RTHR=$rthreads RW=$rw
				export ATYPE=$atype COMMENT=$atype
				$fio_instance
			done
		done
	done
}

workspace_clean_results $id
mount_undo
mount_single_device
workspace_prepare
run_instances $output_json
python3 graphs/scripts/$id.py $output_json $output_graph
