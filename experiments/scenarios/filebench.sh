#!/bin/bash

source "${BASH_SOURCE[0]%/*}"/../scripts/global.conf

source $scripts_dir/numa/numa_utilities.sh
source $scripts_dir/mount/mount_utilities.sh
source $scripts_dir/workspace/workspace_utilities.sh
source $scripts_dir/userspace/userspace_utilities.sh

id=filebench
output_json=$base_dir/graphs/json/$id.json
output_graph=$base_dir/graphs/results/$id.png
filebench_instance=$base_dir/instances/filebench_instance.sh

run_configuration() {
	# for workload in varmail webserver_numa fileserver; do
	for workload in varmail; do
		echo $workload "$1"
		$filebench_instance "$workload" "$1" $output_json
	done
}

$sudo $base_dir/scripts/various/disable_va_rand.sh

if true; then
workspace_clean_results $id

mount_undo
mount_single_device
workspace_prepare
run_configuration "Single device"

mount_undo
mount_linear_mapping
workspace_prepare
run_configuration "Linear map"

mount_undo
mount_striped_mapping
workspace_prepare
run_configuration "Striped map"

mount_undo
mount_linear_mapping_numa
workspace_prepare
run_configuration "Linear map (modified)"
fi
 
# userspace_start
# run_configuration "Linear map (modified) + userspace"
# userspace_stop

python3 $base_dir/graphs/scripts/filebench.py $output_json $output_graph
