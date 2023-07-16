#!/bin/bash

source "${BASH_SOURCE[0]%/*}"/mount_header.sh

pmem0=/dev/pmem0
pmem1=/dev/pmem1

# If a previous map is mounted, umount it
if grep -qs '/mnt/pmem ' /proc/mounts; then
	umount /mnt/pmem
fi

if [ -b /dev/mapper/striped ] ; then
	# for some reason, we must wait for
	# umount, otherwise the device will
	# seem busy
	sleep 1
	dmsetup remove striped
fi

# If a previous linear map exitst, exit
if [ -b /dev/mapper/linear ] ; then
	# or remove it: dmsetup remove linear
	return
fi

# Information regarding the device mapper
# https://access.redhat.com/documentation/en-us/
# red_hat_enterprise_linux/5/html/
# logical_volume_manager_administration/device_mapper

# Join 2 devices together
# This is done the first time to obtain
# some basic information we need (block
# size, blocks per group, etc) to do
# this again properly later
size_0=`blockdev --getsz $pmem0`
size_1=`blockdev --getsz $pmem1`
# [start in mapping] [length of segment] ...
# ... [mapping type] [starting offset on the device] \n...
echo "0 $size_0 linear $pmem0 0
$size_0 $size_1 linear $pmem1 0" | dmsetup create linear

# Create once to obtain the wanted information
yes | mkfs.ext4 -O ^flex_bg /dev/mapper/linear

# Info for size roundup
device=/dev/mapper/linear
dumpe2fs_output=$(dumpe2fs $device 2> /dev/null)
dumpe2fs_value() { 
	echo "$dumpe2fs_output" |
       	grep "$1" | 
	awk '{print $NF}'
}
round_down() {
	# n = $1 and divider = $2
	echo $(( ($1 / $2) * $2 ))
}
block_size_bytes=$(dumpe2fs_value "Block size")
blocks_per_group=$(dumpe2fs_value "Blocks per group")
bytes_in_group=$(( block_size_bytes * blocks_per_group ))
sector_size_bytes=512
pmem0_bytes=$(( sector_size_bytes * size_0 ))
pmem1_bytes=$(( sector_size_bytes * size_1 ))
# Trim the size of each device to be perfectly divisible in terms of groups
size_0=$(( $(round_down pmem0_bytes bytes_in_group) / sector_size_bytes ))
size_1=$(( $(round_down pmem1_bytes bytes_in_group) / sector_size_bytes ))

# Remake properly the linear mapping, and
# create the filesystem
dmsetup remove linear
echo "0 $size_0 linear $pmem0 0
$size_0 $size_1 linear $pmem1 0" | dmsetup create linear
# yes | mkfs.ext4 -O ^flex_bg -O ^sparse_super -O ^resize_inode /dev/mapper/linear
# yes | mkfs.ext4 -O ^flex_bg -O ^has_journal /dev/mapper/linear
yes | mkfs.ext4 -O ^flex_bg -O has_journal /dev/mapper/linear
