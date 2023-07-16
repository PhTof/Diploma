aux_create_file() {
	file=$1
	[ ! -f "$file" ] && { echo "{}" > $file; }
}

aux_update_file() {
	jqstr=$1
	file=$2
	jq "$jqstr" $file > $file.tmp && mv $file.tmp $file	
}

fio_add_measurement() {
	type=$1 rw=$2 rthreads=$3 threads=$4
	rlat=$5 wlat=$6 bw=$7 file=$8
	aux_create_file $file
	jqstr='.measurements += [{"type": "'"$type"'", "rw%": '"$rw"', '
	jqstr+='"rthreads": '"$rthreads"', "threads": '"$threads"', '
	jqstr+='"rlat": '"$rlat"', "wlat": '"$wlat"', "bw": '"$bw"'}]' 
	aux_update_file "$jqstr" $file
}

filebench_add_measurement() {
	workload=$1 conf=$2 ops=$3 file=$4
	aux_create_file $file
	jqstr='.measurements += [{"workload": "'"$workload"'", '
	jqstr+='"configuration": "'"$conf"'", "ops bw": '"$ops"'}]'
	aux_update_file "$jqstr" $file
	
}
