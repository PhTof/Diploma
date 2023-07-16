#!/bin/bash

source "${BASH_SOURCE[0]%/*}"/mount_header.sh

pmem0=/dev/pmem0
pmem1=/dev/pmem1

# If a previous linear map is mounted, umount it
if grep -qs '/mnt/pmem ' /proc/mounts; then
	umount /mnt/pmem
fi

if [ -b /dev/mapper/linear ] ; then
	sleep 1
	dmsetup remove linear
fi

# If a previous striped map exitst, exit
if [ -b /dev/mapper/striped ] ; then
	return
fi

size_0=`blockdev --getsz $pmem0`
size_1=`blockdev --getsz $pmem1`
echo "0 $((size_0 + size_1)) striped 2 4096 $pmem0 0 $pmem1 0" | dmsetup create striped
# yes | mkfs.ext4 -O ^flex_bg -O ^has_journal /dev/mapper/striped
yes | mkfs.ext4 -O ^flex_bg -O has_journal /dev/mapper/striped
