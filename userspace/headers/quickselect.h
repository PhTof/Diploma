#ifndef QUICKSELECT_H
#define QUICKSELECT_H

#include "types.h"
#include "processes.h"

struct partition_result {
	int index;
	u64 sl;
	u64 sr;
};

u64 quickselect_get_write_threshold(struct process_list *l,
	numa_node_t node, u64 write_target);

void quickselect_fini();

#endif
