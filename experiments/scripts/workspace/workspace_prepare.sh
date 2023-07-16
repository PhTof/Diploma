#!/bin/bash

# $sudo, $tools and $scripts_dir
source "${BASH_SOURCE[0]%/*}"/../global.conf

mountpoint=/mnt/pmem
workdir=$mountpoint/phtof
linkdir=$scripts_dir/..
# fiobin=$tools/fio/fio

if ! grep -qs $mountpoint /proc/mounts; then
	echo "Nothing mounted at $mountpoint. Exiting..."
	exit
fi

if [ ! -d $workdir ]; then
	$sudo mkdir $workdir
	$sudo chown -R phtof:users $workdir
fi

# Create symlinks to the fio
# executable and the workspace
# [ ! -d $linkdir/pmem ] && ln -fs $workdir $linkdir/pmem
# [ ! -f $linkdir/fio  ] && ln -fs $fiobin  $linkdir/fio

# Create files
touch $workdir/nodax
touch $workdir/dax
xfs_io -c 'chattr +x' $workdir/dax

# Create directories
mkdir -p $workdir/nodaxdir
mkdir -p $workdir/daxdir
xfs_io -c 'chattr +x' $workdir/daxdir/

# Fix ownership
$sudo chown -R phtof:users $workdir

# Set the frequency to a constant value
$sudo $scripts_dir/freq/freq_set.sh

# Drop the page cache for the flags to take effect
# (not needed for linux version >= 5.13)
# $sudo $scripts_dir/drop/drop_caches.sh
