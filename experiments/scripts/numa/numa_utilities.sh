#!/bin/bash

numa_dir=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)

source $numa_dir/../global.conf

function numa_node_of_file {
	$sudo bash -c \"source $numa_dir/numa_node_of_file.sh \; numa_node_of_file $1\"
}

function print_numa_node_of_file {
	$sudo bash -c \"source $numa_dir/numa_node_of_file.sh \; print_numa_node_of_file $1\"
}

function numa_test_awareness {
	echo "TEST 1: fio"
	$sudo $numa_dir/numa_test_fio.sh
	echo "TEST 2: Modified filebench"
	$sudo $numa_dir/numa_test_filebench.sh
	echo "TEST 3: Redirection operator [WILL PROPABLY FAIL]"
	$sudo $numa_dir/numa_test_redirection.sh
}
