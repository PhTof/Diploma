#!/bin/bash

# Global variable
device=/dev/mapper/linear

round_up() {
	# n = $1 and divider = $2
	echo $(( ($1 + $2 - 1) / $2 ))
}

function numa_get_file_info {
	file="${1#"/mnt/pmem"}"
	block_info=$(debugfs -R "blocks $file" $device 2> /dev/null)
	first_block=$(echo $block_info | awk '{ print $1 }')
	echo "$first_block"
}

function numa_get_sb_info {
	# dumpe2fs info
	dumpe2fs_output=$(dumpe2fs -h $device 2> /dev/null)
	dumpe2fs_value() { 
		echo "$dumpe2fs_output" |
		grep "$1" | 
		awk '{print $NF}'
	}
	total_blocks=$(dumpe2fs_value "Block count")
	blocks_per_group=$(dumpe2fs_value "Blocks per group")
	ngroups=$(round_up total_blocks blocks_per_group)
	first_remote_group=$(((ngroups / 2) + (ngroups % 2)))
	middle_block=$(( first_remote_group * blocks_per_group ))

	echo $middle_block $blocks_per_group
}
