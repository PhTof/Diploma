#!/bin/bash

workspace_dir=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)

# $sudo
source $workspace_dir/../global.conf

function workspace_clean_results {
	$workspace_dir/workspace_clean_results.sh $1	
}

function workspace_prepare {
	$sudo $workspace_dir/workspace_prepare.sh
}
