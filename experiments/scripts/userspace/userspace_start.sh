#!/bin/bash

# $tools
source "${BASH_SOURCE[0]%/*}"/../global.conf

$userspace_dir/processes &

sleep 1
