#ifndef NUMA_H
#define NUMA_H

#include "types.h"

#define NUM_NODES 2

void numa_init();
void numa_fini();
// TODO: remove or keep
// int get_numa_node(int fd);
void numa_set_node(int pid, int node);
int numa_num_nodes();
int numa_num_cpus(int node);
void numa_iostat(int tid, u64 *numa_read, u64 *numa_write); 

#endif
