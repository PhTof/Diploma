#!/bin/bash

# numa_get_sb_info
source "${BASH_SOURCE[0]%/*}"/numa_common.sh

function numa_node_of_file {
	first_block=$(numa_get_file_info $1)
	read middle_block blocks_per_group < <(numa_get_sb_info)
	[[ "$first_block" -lt "$middle_block" ]] && echo 0 || echo 1
}

function print_numa_node_of_file {
	first_block=$(numa_get_file_info $1)
	read middle_block blocks_per_group < <(numa_get_sb_info)
	printf "this = %s / middle = %s\n" "$first_block" "$middle_block"
}
