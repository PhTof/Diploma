#!/bin/bash

# $sudo

mount_dir=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)

source $mount_dir/../global.conf

function mount_undo {
	mountpoint="${1:-/mnt/pmem}"

	if grep -qs $mountpoint /proc/mounts; then
		$sudo umount $mountpoint
	fi

	$sudo dmsetup info | grep striped
	if [ $? == 0 ]; then
		$sudo dmsetup remove striped
	fi

	$sudo dmsetup info | grep linear
	if [ $? == 0 ]; then
		$sudo dmsetup remove linear
	fi
}

function mount_single_device {
	dev="${1:-/dev/pmem0}"
	mountpoint="${2:-/mnt/pmem}"
	# $sudo bash -c "yes | mkfs.ext4 -O ^flex_bg -O ^has_journal $dev"
	$sudo bash -c "yes | mkfs.ext4 -O ^flex_bg -O has_journal $dev"
	$sudo mount -o dax=inode $dev $mountpoint
}

function mount_linear_mapping {
	$sudo $mount_dir/mount_create_linear.sh
	$sudo mount -o dax=inode /dev/mapper/linear /mnt/pmem
}

function mount_linear_mapping_numa {
	$sudo $mount_dir/mount_create_linear.sh
	$sudo mount -o dax=inode -o numa /dev/mapper/linear /mnt/pmem
}

function mount_striped_mapping {
	$sudo $mount_dir/mount_create_striped.sh
	$sudo mount -o dax=inode /dev/mapper/striped /mnt/pmem
}
