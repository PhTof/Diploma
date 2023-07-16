#!/bin/bash

source "${BASH_SOURCE[0]%/*}"/../scripts/global.conf

source $scripts_dir/mount/mount_utilities.sh
source $scripts_dir/workspace/workspace_utilities.sh

id=primitive
instance_script=$base_dir/instances/fio_instance.sh
output_json=$base_dir/graphs/json/$id.json
output_graph=$base_dir/graphs/results/$id.png

function run_instances {
	output=$1

	export OUT=$output SIZE=4G
	export ENG=sync ATYPE=dax

	for threads in $(seq 1 16); do
		for rthreads in 0 $threads; do
			for rw in 0 100; do
				format="threads = %d rthreads = %d rw = %d\n"
				printf "$format" $threads $rthreads $rw
				#========================================#
				rm -r /mnt/pmem/phtof/daxdir/*
				export THR=$threads RTHR=$rthreads RW=$rw
				$instance_script
			done
		done
	done
}

workspace_clean_results $id
mount_undo

# NOVA and WineFS experimentation
# $sudo mount -t NOVA -o init /dev/pmem0 /mnt/pmem
# $sudo mount -t winefs -o init,strict /dev/pmem0 /mnt/pmem

mount_single_device
workspace_prepare
run_instances $output_json
python3 graphs/scripts/$id.py $output_json $output_graph
