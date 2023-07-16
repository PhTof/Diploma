#!/bin/bash

source "${BASH_SOURCE[0]%/*}"/../global.conf

source $scripts_dir/global.conf

function numa_node_of_file {
	$sudo bash -c \"source $scripts_dir/numa/numa_node_of_file.sh \; numa_node_of_file $1\"
}

function print_numa_node_of_file {
	$sudo bash -c \"source $scripts_dir/numa/numa_node_of_file.sh \; print_numa_node_of_file $1\"
}

function numa_test_awareness {
	echo "TEST 1: fio"
	$sudo $scripts_dir/numa/numa_test_fio.sh
	echo "TEST 2: Modified filebench"
	$sudo $scripts_dir/numa/numa_test_filebench.sh
	echo "TEST 3: Redirection operator [WILL PROPABLY FAIL]"
	$sudo $scripts_dir/numa/numa_test_redirection.sh
}
