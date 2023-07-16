#!/bin/bash

source "${BASH_SOURCE[0]%/*}"/../scripts/global.conf

source $scripts_dir/mount/mount_utilities.sh
source $scripts_dir/workspace/workspace_utilities.sh

id=test_mods
fio_instance=$base_dir/instances/fio_instance.sh
output_json=$base_dir/graphs/json/$id.json
output_graph=$base_dir/graphs/results/$id.png

function run_instances {
	output=$1
	comment=$2

	export OUT=$output SIZE=256M
	export ENG=sync

	for threads in 4 8 16 32; do
		for rw in 0 50 100; do
			for atype in dax; do
				format="threads = %d rw = %d\n"
				printf "$format" $threads $rw
				#========================================#
				rm -r /mnt/pmem/phtof/daxdir/*
				export THR=$threads RW=$rw
				export ATYPE=$atype COMMENT="$comment"
				$fio_instance
			done
		done
	done
}

workspace_clean_results $id

mount_undo
mount_linear_mapping
workspace_prepare
run_instances $output_json "Linear map (simple)"

mount_undo
mount_linear_mapping_numa
workspace_prepare
run_instances $output_json "Linear map (modified)"

python3 graphs/scripts/$id.py $output_json $output_graph
