#!/bin/bash

USR=phtof

# https://superuser.com/questions/443406/
for i in {1..32}; do while : ; do : ; done & done

sleep 10

pids=$(ps -u $USR -T --no-headers | grep fake | awk '{print $1}')

# Kill all the created threads, except the current one ($$)
pids=(${pids//$$/ })
unset 'pids[${#pids[@]}-1]' # Remove the ps PID (last one in the array)
kill -9 ${pids[@]}
