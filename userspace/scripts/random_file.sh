#/bin/bash

if [ $# -eq 0 ]; then
	echo "No arguments supplied"
	exit
fi

head -c 1G < /dev/urandom > $1
