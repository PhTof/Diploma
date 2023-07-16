#!/bin/bash

device="${1:-/dev/mapper/linear}"
grep $device /proc/mounts
