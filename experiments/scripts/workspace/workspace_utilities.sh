#!/bin/bash

# $sudo
source "${BASH_SOURCE[0]%/*}"/../global.conf

function workspace_clean_results {
	$scripts_dir/workspace/workspace_clean_results.sh $1
}

function workspace_prepare {
	$sudo $scripts_dir/workspace/workspace_prepare.sh
}
