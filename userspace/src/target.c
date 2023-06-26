#include <stdlib.h>	// free()

#include "../headers/processes.h"
#include "../headers/cpu.h"
#include "../headers/target.h"
#include "../headers/quickselect.h"
#include "../headers/list.h"
#include "../headers/numa.h"
// #include "../headers/nvmm.h"

// TMP!
#define NONE (-1)

struct list_to_array {
	struct process **array;
	int alloc_size;
	int used_size;
};

static struct list_to_array global_aux = {NULL, 0, 0};

void migrate_thread(int thread_id, int node) {
	numa_set_node(thread_id, node);
}

int target_get_initial(struct process *proc) {
	// only take into account the reads for now
	int i, target = 0;
	u64 *diff_read = proc->diff_numa_read;

	for (i = 0; i < NUM_NODES; i++)
		if (diff_read[target] < diff_read[i])
			target = i;

	for (i = 0; i < NUM_NODES; i++)
		if (diff_read[target] < diff_read[i] + DIFF_FACTOR)
		    if (i != target)
			return NO_TARGET;

	return target;
}

static void prepare_aux_struct(
	struct process_list *l,
	struct list_to_array *a,
	int node)
{
	int alloc_size, i = 0;
	struct process *proc = l->head;

	// extend the aux struct if needed
	if (a->alloc_size < l->size) {
		free(a->array);
		alloc_size = 
			2*(l->size)*sizeof(struct process *);
		a->array = (struct process **) 
			malloc(alloc_size);
		a->alloc_size = 2*(l->size);
	}

	while (proc) {
		if (proc->numa_node == node) {
			proc_write_ratio(proc);
			a->array[i++] = proc;
		}
		proc = proc->next;
	}
	a->used_size = i;
}

void fix_congestion(struct process_list *l) {
	// int nvmm_congested_node = nvmm_contention();
	int nvmm_congested_node = 1;
	// int cpu_congested_node = cpu_contention();
	int cpu_congested_node = 0;
	u64 nodes_BW[NUM_NODES];
	u64 sum_BW = 0, average_BW;
	u64 data_write_target[NUM_NODES];
	u64 total_proc_write;
	struct list_to_array *aux = &global_aux;
	struct process *proc;
	// TODO: handle differently
	int interval = 1;
	int i, n, target;
	float nodes_cpu[NUM_NODES], cpu_usage;
	int min;

	if (list_is_empty(l))
		return;

	if (nvmm_congested_node != NONE) {

		// update auxiliary structure
		prepare_aux_struct(l, aux, nvmm_congested_node);
		// find data migration margins for each node
		for (i = 0; i < NUM_NODES; i++) {
			// TODO: Don't forget to uncomment this
			// nodes_BW[i] = nvmm_get_node_BW(i);
			sum_BW += nodes_BW[i];
		}

		average_BW = sum_BW / NUM_NODES;
		for (n = 0; n < NUM_NODES; n++) {
			data_write_target[n] = (nodes_BW[n] - average_BW)*interval;
		}

		int index = 0;
		index = quickselect(
			aux->array, 0, aux->used_size-1, 
			data_write_target[nvmm_congested_node]);

		// TODO: This needs better planning (what if you cannot
		// move anything based on target alone -> try to minimize 
		// cost when moving things somewhere else from the target)
		for (i = 0; i < index; i++) {
			proc = aux->array[i];
			total_proc_write = 0;
			for (n = 0; n < NUM_NODES; n++)
				total_proc_write += proc->diff_numa_write[n];
			target = target_get_initial(proc);
			
			if (target == NO_TARGET || target == nvmm_congested_node) {
				int min_node = 0;
				for (n = 0; n < NUM_NODES; n++) {
					if (data_write_target[min_node] > data_write_target[n])
						min_node = n;
				}
				data_write_target[min_node] += total_proc_write;
				migrate_thread(proc->tid, min_node);
				continue;
			}
			if (data_write_target[target] + total_proc_write <= 0) {
				data_write_target[target] += total_proc_write;
				migrate_thread(proc->tid, target);
			} else {
				// do something good here
			}
		}

		return;
	}

	if (cpu_congested_node != NONE) {
		proc = l->head;

		for (n = 0; n < NUM_NODES; n++)
			nodes_cpu[n] = cpu_get_usage(n);

		while (nodes_cpu[cpu_congested_node] > CPU_LIMIT && proc) {
			cpu_usage = process_cpu_usage(proc);
			target = target_get_initial(proc);
			if (target == NO_TARGET) {
				min = 0;
				for (n = 0; n < NUM_NODES; n++)
					if (nodes_cpu[n] < nodes_cpu[min])
						min = n;
				if (nodes_cpu[min] + cpu_usage < CPU_LIMIT)
					migrate_thread(proc->tid, target);
			} else if (nodes_cpu[target] + cpu_usage < CPU_LIMIT) {
				migrate_thread(proc->tid, target);
			} else {
				// Again, think of something
			}
			proc = proc->next;
		}
		return;
	}

	// simply move to target
	proc = l->head;
	while (proc) {
		target = target_get_initial(proc);
		if (target != NO_TARGET)
			migrate_thread(proc->tid, target);
		proc = proc->next;
	}
}

void target_fini() {
	free(global_aux.array);
}
