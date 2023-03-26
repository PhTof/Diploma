#!/bin/sh

pmem0=/dev/pmem0
pmem1=/dev/pmem1

# https://access.redhat.com/documentation/en-us/
# red_hat_enterprise_linux/5/html/
# logical_volume_manager_administration/device_mapper

# Join 2 devices together
size1=`blockdev --getsz $pmem0`
size2=`blockdev --getsz $pmem1`
# [start in mapping] [length of segment] ...
# ... [mapping type] [starting offset on the device] \n...
echo "0 $size1 linear $pmem0 0
$size1 $size2 linear $pmem1 0" | dmsetup create linear

yes | mkfs.ext4 -O ^flex_bg /dev/mapper/linear

mount -o dax=inode /dev/mapper/linear /mnt/pmem
