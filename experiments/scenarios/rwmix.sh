#!/bin/bash

this_dir="${BASH_SOURCE[0]%/*}"
source $this_dir/../scripts/global.conf
source $this_dir/../scripts/mount/mount_utilities.sh
source $this_dir/../scripts/workspace/workspace_utilities.sh

id=rwmix
fio_instance=$this_dir/../instances/fio_instance.sh
output_json=$this_dir/../graphs/json/$id.json
output_graph=$this_dir/../graphs/results/$id.png

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
