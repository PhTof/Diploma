#!/bin/bash

if [ $(id -u) -ne 0 ]; then 
	echo "$0 must be run as root. Exiting..."
	exit 
fi
