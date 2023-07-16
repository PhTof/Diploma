#!/bin/bash

# $sudo, $perf, $tools
source "${BASH_SOURCE[0]%/*}"/../global.conf

flamegraphs=$tools/FlameGraph

$sudo $perf record -F 997 -ag --call-graph dwarf $base_dir/instances/filebench_instance.sh varmail $1 tmp.json
$sudo $perf script | $flamegraphs/stackcollapse-perf.pl > out.perf-folded
cat out.perf-folded | $flamegraphs/flamegraph.pl > ~/experiments/graphs/$1.svg
$sudo rm out.perf-folded perf.data
