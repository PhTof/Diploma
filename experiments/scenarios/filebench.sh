#!/bin/bash

this_dir="${BASH_SOURCE[0]%/*}"
source $this_dir/../scripts/global.conf
source $this_dir/../scripts/numa/numa_utilities.sh
source $this_dir/../scripts/mount/mount_utilities.sh
source $this_dir/../scripts/workspace/workspace_utilities.sh
source $this_dir/../scripts/userspace/userspace_utilities.sh

id=filebench
output_json=$this_dir/../graphs/json/$id.json
output_graph=$this_dir/../graphs/results/$id.png
filebench_instance=$this_dir/../instances/filebench_instance.sh

run_configuration() {
	# for workload in varmail_numa webserver_numa fileserver; do
	for workload in varmail; do
		echo $workload "$1"
		$filebench_instance "$workload" "$1" $output_json
	done
}

$sudo $this_dir/../scripts/various/disable_va_rand.sh

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

python3 $this_dir/../graphs/scripts/filebench.py $output_json $output_graph
