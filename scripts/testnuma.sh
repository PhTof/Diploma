#!/bin/bash

# PARAMETERS
files_per_node=950
delete_files=true
create_files=true
exit_on_fail=false
debug=false

# dumpe2fs info
device=/dev/mapper/linear
dumpe2fs_output=$(dumpe2fs $device 2> /dev/null)
dumpe2fs_value() { 
	echo "$dumpe2fs_output" |
       	grep "$1" | 
	awk '{print $NF}'
}
round_up() {
	# n = $1 and divider = $2
	echo $(( ($1 + $2 - 1) / $2 ))
}
total_blocks=$(dumpe2fs_value "Block count")
blocks_per_group=$(dumpe2fs_value "Blocks per group")
ngroups=$(round_up total_blocks blocks_per_group)
first_remote_group=$(((ngroups / 2) + (ngroups % 2)))
middle_block=$(( first_remote_group * blocks_per_group ))

# Colours for fancy printing
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# Other variables
found_error=false

report_progress() {
	done=$1
	total=$2
	bar_len=60
	done=$(((bar_len*done)/total))
	printf '\r' # clear line
	printf "["
	if [[ "$done" -ne 0 ]]; then
		printf "#%.0s" $(seq 0 $((done-1)))
	fi
	if [ "$done" -ne "$bar_len" ]; then
		printf "_%.0s" $(seq 1 $((bar_len - done)))
	fi
	printf "]"
}

if [ "$debug" = true ]; then
	rm /mnt/pmem/file_0_{35..51}
	for i in {35..51}; do
		taskset -c 0 bash -c "./scripts/bigfile.sh > /mnt/pmem/file_0_${i}"
	done
	sync
	for i in {35..51}; do
		file=file_0_${i}
		block_info=$(debugfs -R "blocks $file" $device 2> /dev/null)
		first_block=$(echo $block_info | awk '{ print $1 }')
		if [[ "$first_block" -ge "$middle_block" ]]; then
			printf "${RED}ERROR:${NC} "
			printf "file %s with first block %d seems misplaced\n" $file $first_block
			found_error=true
			# https://unix.stackexchange.com/questions/114402/
			[[ $exit_on_fail = true ]] && exit
		fi
	done
	exit
fi

# Just to be sure
echo 0 > /sys/fs/ext4/dm-0/inode_goal

if [ "$delete_files" = true ]; then
	# Cleanup any previously created files
	rm /mnt/pmem/file* &> /dev/null
fi

if [ "$create_files" = true ]; then
	printf "Creating the $((2*files_per_node)) test files...\n"

	for i in $(seq 1 $files_per_node); do
		taskset -c 0 bash -c "./scripts/bigfile.sh > /mnt/pmem/file_0_$i"
		report_progress $i $((2*files_per_node))
	done

	for i in $(seq 1 $files_per_node); do
		taskset -c 2 bash -c "./scripts/bigfile.sh > /mnt/pmem/file_1_$i"
		report_progress $((files_per_node + i)) $((2*files_per_node))
	done
fi

# Make sure that everything 
# is written to disk first
sync

printf "\n\nTesting if everything has been placed properly\n"

for i in 0 1; do
	for j in $(seq 1 $files_per_node); do 
		file=file_${i}_${j}
		block_info=$(debugfs -R "blocks $file" $device 2> /dev/null)
		first_block=$(echo $block_info | awk '{ print $1 }')
		if [[ "$i" -eq 0 && "$first_block" -ge "$middle_block" ]] || 
		   [[ "$i" -eq 1 && "$first_block" -lt "$middle_block" ]]; then
			printf "${RED}ERROR:${NC} "
			printf "file %s with first block %d seems misplaced\n" $file $first_block
			found_error=true
			# https://unix.stackexchange.com/questions/114402/
			[[ $exit_on_fail = true ]] && exit
		fi
	done
done

[[ $found_error = false ]] && printf "${GREEN}SUCCESS:${NC} Everything seems ok\n"
