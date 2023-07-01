FIO=fio
# DIR0=/mnt/pmem0/daxdir
# DIR1=/mnt/pmem1/daxdir
DIR=/mnt/pmem/phtof


# previously: invalidate=0
# taskset -c "0-7" gdb --args $FIO \
$FIO --name=job --thread --group_reporting \
     --ioengine=sync --loops=1 --invalidate=1 \
     --rw=randrw --rwmixread=50 --size=16000M \
     --bs=4K --numjobs=4 --percentage_random=0 \
     --directory=$DIR0 --direct=0

exit

$FIO --name=job --thread --group_reporting \
     --ioengine=sync --loops=1 --invalidate=1 \
     --rw=randrw --rwmixread=30 --size=3500M \
     --bs=4K --numjobs=1 --percentage_random=0 \
     --directory=$DIR1 --direct=0 &

# O_DIRECT fixes ratio mismatch between fio and
# the userspace program
