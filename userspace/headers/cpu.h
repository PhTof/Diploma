#ifndef CPU_H
#define CPU_H

#include "types.h"

#define NO_CPU_CONTENTION (-1)
#define CPU_LIMIT 90

void cpu_init(int num_nodes);
void cpu_fini();
void cpu_reset();
void cpu_update(int node, float usage);
// TODO: remove
void cpu_print();
int cpu_contention();
float cpu_get_usage(int node);
void cpu_stat(int tid, int *cpu, u64 *utime, u64 *stime);

#endif
