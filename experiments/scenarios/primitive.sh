#!/bin/bash

this_dir="${BASH_SOURCE[0]%/*}"
source $this_dir/scripts/global.conf
source $this_dir/scripts/mount/mount_utilities.sh
source $this_dir/scripts/workspace/workspace_utilities.sh

id=primitive
instance_script=$this_dir/fio_instance.sh
output_json=$this_dir/graphs/json/$id.json
output_graph=$this_dir/graphs/results/$id.png

function run_instances {
	script=$1
	output=$2

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
				$script
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
run_instances $instance_script $output_json
python3 graphs/scripts/$id.py $output_json $output_graph
