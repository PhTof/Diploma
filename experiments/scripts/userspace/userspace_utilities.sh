#!/bin/bash

source "${BASH_SOURCE[0]%/*}"/../global.conf

function userspace_start {
	$sudo $scripts_dir/userspace/userspace_start.sh
}

function userspace_stop {
	$sudo $scripts_dir/userspace/userspace_stop.sh
}
