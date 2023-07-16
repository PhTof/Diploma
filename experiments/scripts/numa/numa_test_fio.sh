#!/bin/bash

# $fiobin
source "${BASH_SOURCE[0]%/*}"/../global.conf
source $scripts_dir/numa/numa_node_of_file.sh

rm $workdir/daxdir/*

$fiobin --thread --group_reporting --create_serialize=0 \
	--blocksize=4K --size=256M --ioengine=sync --rw=randrw \
	--rwmixread=50 --directory=$workdir/daxdir \
	--name local --numjobs=16 --cpus_allowed=0-15 \
	--name remote --cpus_allowed=16-31 --numjobs=16

for f in $workdir/daxdir/local*; do 
	res=$(numa_node_of_file $f)
	if [[ "$res" -eq 0 ]]; then
		printf "${GREEN}OK!${NC} %s\n" $f
	else
		printf "${RED}WRONG${NC} %s\n" $f
		print_numa_node_of_file $f
	fi
done

for f in $workdir/daxdir/remote*; do 
	res=$(numa_node_of_file $f)
	if [[ "$res" -eq 1 ]]; then
		printf "${GREEN}OK!${NC} %s\n" $f
	else
		printf "${RED}WRONG${NC} %s\n" $f
		print_numa_node_of_file $f
	fi
done
