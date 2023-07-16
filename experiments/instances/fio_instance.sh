#!/bin/bash

# $workdir, $fiobin
source "${BASH_SOURCE[0]%/*}"/../scripts/global.conf

source $scripts_dir/graph/add_measurement.sh
source $scripts_dir/graph/parse.sh
source $scripts_dir/drop/drop_file.sh

# Change these #
# https://stackoverflow.com/questions/2013547/
# VAR=${VAR:-default_value}  # If VAR not set or null, use default value

# Configurable from outside this script
engine=${ENG:-sync}
threads=${THR:-32}
output="${OUT:-output.json}"
loops=${LPS:-1}
fiotype="${TYPE:-simple}"
comment="${COMMENT:-default}"
atype="${ATYPE:-dax}"
size="${SIZE:-256M}"
rw=${RW:-50}
rthreads=${RTHR:-$((threads/2))}
bw_parse_func=${PRS_FUNC:-fio_get_total_bw}

rlat_parse_func=fio_get_read_lat
wlat_parse_func=fio_get_write_lat
ldir=$workdir/daxdir
rdir=$workdir/daxdir
# manual invalidation (0) or through fio (1)
# (should this ever have been an issue?)
invalidate=1  # TODO: change this again !!
num_samples=3 # TODO: change back to 4
dotfiodir=$base_dir/dotfio/$fiotype

#==============#

runfio() {
	# Derive the number of local threads
	lthreads=$((threads - rthreads))

	# Derive the cpumasks
	l=$(( lthreads > 16 ? 16 : lthreads ))
	r=$(( rthreads > 16 ? 16 : rthreads ))
	lcpulist="0-$((l - 1))"
	rcpulist="16-$((16 + r - 1))"

	# export the parameters for fio
	export ENGINE=$engine
	export SIZE=$size RW=$rw INVAL=$invalidate LOOPS=$loops
	export LDIR=$ldir LTHREADS=$lthreads LCPULIST=$lcpulist
	export RDIR=$rdir RTHREADS=$rthreads RCPULIST=$rcpulist

	if [[ $rthreads == 0 ]]; then
		$fiobin --output-format=json $dotfiodir/local.fio
	elif [[ $lthreads == 0 ]]; then
		$fiobin --output-format=json $dotfiodir/remote.fio
	else
		$fiobin --output-format=json $dotfiodir/mixed.fio
	fi
}

if [[ $atype == dax ]]; then 
	rdir=$workdir/daxdir
else 
	# Special case: Normally, all local threads
	# are dax. Despite this, we are also interested
	# in the case when we have no remote threads
	# but we access our files locally with nodax
	# (if nodax is always better somehow, we should know)
	if [[ $rthreads == 0 ]]; then
		ldir=$workdir/nodaxdir
	fi

	rdir=$workdir/nodaxdir

	# TODO: TMP
	# ldir=$workdir/nodaxdir
fi

# (Re)Create the files by executing fio once
# rm $workdir/daxdir/* &> /dev/null
# rm $workdir/nodaxdir/* &> /dev/null
# bw=$(runfio | $parse_func)

for samples in $(seq 1 $num_samples); do
	if [[ $invalidate -eq 0 ]]; then
		dropfiles $workdir/nodaxdir
	fi

	# sleep 5
	fio_out=$(runfio)
	bw=$(echo $fio_out | $bw_parse_func)
	rlat=$(echo $fio_out | $rlat_parse_func)
	wlat=$(echo $fio_out | $wlat_parse_func)
	fio_add_measurement "$comment" $rw $rthreads \
		$threads $rlat $wlat $bw $output
done
