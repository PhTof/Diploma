#!/bin/bash

# $kernel_dir
source "${BASH_SOURCE[0]%/*}"/freq_header.sh

# Set the frequency to a constant value
echo 'GOVERNER="performance"' | tee /etc/default/cpufrequtils
systemctl disable ondemand
systemctl restart cpufrequtils

export LD_LIBRARY_PATH="$cpupowerdir"
$cpupowerdir/cpupower frequency-set -g performance # &> /dev/null
$cpupowerdir/cpupower idle-set --disable-by-latency 0 #&> /dev/null
