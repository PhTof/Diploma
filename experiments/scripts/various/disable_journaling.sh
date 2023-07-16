#!/bin/bash

tune2fs -O ^has_journal /dev/pmem0
