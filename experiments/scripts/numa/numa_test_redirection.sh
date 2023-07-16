#!/bin/bash

#################
#   !WARNING!   #
#################

# This test is not supposed to be passed
# at the moment. The redirection operator
# seems to handle IO directly through a
# kworker, so success here may depend on
# what has been spawned

# Fancy printing
source "${BASH_SOURCE[0]%/*}"/../global.conf
# numa_get_middle_block && numa_get_file_info
source "${BASH_SOURCE[0]%/*}"/numa_common.sh

# PARAMETERS
files_per_node=10
delete_files=true
create_files=true
exit_on_fail=false
directory=phtof
bigfile=$scripts_dir/various/bigfile.sh

if [ $(id -u) -ne 0 ]; then 
	echo "$0 must be run as root. Exiting..."
	exit 
fi

# dumpe2fs info
read middle_block blocks_per_group < <(numa_get_sb_info)

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

# Just to be sure
mkdir /mnt/pmem/$directory &> /dev/null
# Legacy code: I experimented with this once
# echo 0 > /sys/fs/ext4/dm-0/inode_goal

if [ "$delete_files" = true ]; then
	# Cleanup any previously created files
	rm /mnt/pmem/$directory/file* &> /dev/null
fi

if [ "$create_files" = true ]; then
	printf "Creating $((2*files_per_node)) test files...\n"

	for i in $(seq 1 $files_per_node); do
		taskset -c 0 bash -c "$bigfile > /mnt/pmem/$directory/file_0_$i"
		taskset -c 16 bash -c "$bigfile > /mnt/pmem/$directory/file_1_$i"
		report_progress $i $files_per_node
	done
fi

# Make sure that everything 
# is written to disk first
sync

printf "\n\nTesting if everything has been placed properly\n"

for node in 0 1; do
	for file_num in $(seq 1 $files_per_node); do 
		file=file_${node}_${file_num}
		first_block=$(numa_get_file_info "/mnt/pmem/$directory/$file")
		if [[ "$i" -eq 0 && "$first_block" -ge "$middle_block" ]] || 
		   [[ "$i" -eq 1 && "$first_block" -lt "$middle_block" ]]; then
			printf "${RED}ERROR:${NC} "
			printf "file %s with first block %d (group %d) seems misplaced \n" $file \
				$first_block $(( first_block / blocks_per_group ))
			found_error=true
			# https://unix.stackexchange.com/questions/114402/
			[[ $exit_on_fail = true ]] && exit
			continue
		fi
		printf "${GREEN}OK:${NC} "
		printf "file %s with first block %d (group %d) seems properly placed \n" $file $first_block $(( first_block / blocks_per_group ))
	done
done

[[ $found_error = false ]] && printf "${GREEN}SUCCESS:${NC} Everything seems ok\n"
