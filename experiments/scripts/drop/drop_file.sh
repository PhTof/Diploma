#!/bin/bash

dropfile() {
	file=$1
        dd of=$file oflag=nocache conv=notrunc,fdatasync count=0
}

dropfiles() {
	directory=$1
	for file in $directory/*; do
		dropfile $file
	done
}
