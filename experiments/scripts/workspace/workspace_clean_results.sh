#!/bin/bash

read -p "Are you sure you want to delete previous results? " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]
then
	exit
fi

pushd $(dirname -- "$0") > /dev/null

rm ../../graphs/json/$1*
rm ../../graphs/results/$1*

popd
