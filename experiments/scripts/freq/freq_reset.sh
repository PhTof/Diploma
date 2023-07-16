#!/bin/bash

# $kernel_dir
source "${BASH_SOURCE[0]%/*}"/freq_header.sh

echo 'GOVERNOR="powersave"' | sudo tee /etc/default/cpufrequtils
systemctl enable ondemand
systemctl restart cpufrequtils

export LD_LIBRARY_PATH="$cpupowerdir"
$cpupowerdir/cpupower frequency-set -g powersave # &> /dev/null
$cpupowerdir/cpupower idle-set --enable-all #&> /dev/null
