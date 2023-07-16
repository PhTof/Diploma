fio_get_write_bw() {
	jq -r '.jobs[].write | (.bw|tostring)'
}

fio_get_read_bw() {
	jq -r '.jobs[].read | (.bw|tostring)'
}

fio_get_total_bw() {
	in=$(</dev/stdin)
	rbw=$(echo $in | fio_get_read_bw)
	wbw=$(echo $in | fio_get_write_bw)
	printf "%d" $(( rbw + wbw ))
	# printf "%'d" $(( rbw + wbw ))
}

fio_get_read_lat() {
	jq -r '.jobs[].read | (.clat_ns.mean|tostring)'
}

fio_get_write_lat() {
	jq -r '.jobs[].write | (.clat_ns.mean|tostring)'
}

filebench_parse_ops_per_sec() {
	# input = output of filebench 2> /dev/null
	tail -n 2 | head -n 1 | awk '{ print $6 }'
}
