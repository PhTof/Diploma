#!/bin/bash

userspace_dir=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
source $userspace_dir/../global.conf

function userspace_start {
	$sudo $userspace_dir/userspace_start.sh
}

function userspace_stop {
	$sudo $userspace_dir/userspace_stop.sh
}
