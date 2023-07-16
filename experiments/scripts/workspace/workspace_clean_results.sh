#!/bin/bash

source "${BASH_SOURCE[0]%/*}"/../global.conf

read -p "Are you sure you want to delete previous results? " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]
then
	exit
fi

rm $base_dir/graphs/json/$1*
rm $base_dir/graphs/results/$1*
