#include <stdlib.h>	// free()

// TODO: REMOVE
#include <stdio.h>
#include <assert.h>

#include "../headers/processes.h"
#include "../headers/cpu.h"
#include "../headers/target.h"
#include "../headers/quickselect.h"
#include "../headers/list.h"
#include "../headers/numa.h"
#include "../headers/nvmm.h"

#define TARGET_DEBUG 0
#define NO_NODE (-1)
#define node_is_defined(node) (node != NO_NODE)
#define migrate_thread(thread,node) numa_set_node(thread, node)
#define minus(x) (-(x))

typedef struct nvmm_status {
	/* BW usage imbalance indicator:
	 * value < 0: underused node, bytes below average
	 * value > 0: overused node, bytes way above average */
	i64 node_bytes_mean_centered[NUM_NODES];
	u64 write_bytes_threshold;
	numa_node_t nvmm_congested_node;
	numa_node_t nvmm_least_used_node;
} nvmm_status;

/* We only take into account the recorded
 * read BW, as NUMA aware ext4 tends to
 * mostly write locally */

#define DIFF_FACTOR (2 << 27) /* 128 MB */
int get_target_node(struct process *proc)
{
	u64 *diff_read = proc->diff_numa_read;
	int node, target_node = 0;

	for (node = 0; node < NUM_NODES; node++)
		if (diff_read[target_node] < diff_read[node])
			target_node = node;

	for (node = 0; node < NUM_NODES; node++)
		if (diff_read[node] + DIFF_FACTOR > diff_read[target_node])
			if (node != target_node)
				return NO_TARGET;

	return target_node;
}

#define WRITE_RATIO_THRESHOLD 0.3
static inline bool is_write_intensive(struct process *proc,
		struct nvmm_status status)
{
	compute_write_ratio(proc);
	return (proc->diff_numa_write_sum > status.write_bytes_threshold &&
		proc->write_ratio > WRITE_RATIO_THRESHOLD);
}

#define CPU_USAGE_THRESHOLD 10
static inline bool moderate_cpu_usage(struct process *proc) {
	return CPU_USAGE_THRESHOLD <= proc->cpu_usage &&
		proc->cpu_usage < CPU_LIMIT;
}

static inline void compute_nvmm_status(struct process_list *l,
		numa_node_t congested_node, nvmm_status *status)
{
	numa_node_t node;
	u64 bytes_average, bytes_sum = 0;

	for (node = 0; node < NUM_NODES && TARGET_DEBUG; node++) {
		printf("PROCFS BYTES (node %d): %llu\n", node,
			nvmm_get_bytes_procfs(node));
		printf("COUNTERS BYTES (node %d): %llu\n", node,
			nvmm_get_bytes_counters(node));
	}

	status->nvmm_congested_node = congested_node;

	for (node = 0; node < NUM_NODES; node++)
		bytes_sum += nvmm_get_bytes_procfs(node);
	bytes_average = bytes_sum / NUM_NODES;

	for (node = 0; node < NUM_NODES; node++) {
		status->node_bytes_mean_centered[node] =
			nvmm_get_bytes_procfs(node) - bytes_average;
	}

	quickselect_get_write_threshold(l, congested_node,
		status->node_bytes_mean_centered[congested_node]);
}


static inline void update_status(struct process *proc,
		numa_node_t target_node, nvmm_status *ns)
{
	i64 *mean_centered = ns->node_bytes_mean_centered;
	u64 proc_write = proc->diff_numa_write_sum;

	mean_centered[proc->numa_node] -= proc_write;
	mean_centered[target_node] += proc_write;
	
	cpu_update(proc->numa_node, minus(proc->cpu_usage));
	cpu_update(target_node, proc->cpu_usage);
}

static inline numa_node_t nvmm_preferred_node(struct nvmm_status ns)
{
	i64 *mean_centered = ns.node_bytes_mean_centered;
	int min_idx = 0, node;

	for (node = 0; node < NUM_NODES; node++)
		if(mean_centered[node] < mean_centered[min_idx])
			min_idx = node;

	return min_idx;
}

static inline numa_node_t cpu_preferred_node()
{
	int min_idx = 0, node;

	for (node = 0; node < NUM_NODES; node++)
		if(cpu_get_usage(node) < cpu_get_usage(min_idx))
			min_idx = node;

	return min_idx;
}

static inline bool nvmm_target_is_suitable(struct process *proc,
		nvmm_status ns)
{
	numa_node_t target_node, congested_node;
	i64 mean_centered;

	target_node = get_target_node(proc);
	congested_node = ns.nvmm_congested_node;
	if (!node_is_defined(target_node) || target_node == congested_node)
		return false;

	mean_centered = ns.node_bytes_mean_centered[target_node];
	return mean_centered + proc->diff_numa_write_sum <= 0;
}

#define CPU_MARGIN 10
static inline bool cpu_target_is_suitable(struct process *proc,
		numa_node_t congested_node)
{
	numa_node_t target_node = get_target_node(proc);
	float projected_usage;

	if (!node_is_defined(target_node) || target_node == congested_node)
		return false;

	projected_usage = cpu_get_usage(target_node) + proc->cpu_usage;
	return projected_usage < (CPU_LIMIT - CPU_MARGIN);
}

/* {nvmm,cpu}_decide_target functions: In case we have more than 2
 * NUMA nodes, we will have to check more than just the optimal node.
 * This is left for now as is for simplicity. */

static inline bool nvmm_decide_target(struct process *proc,
		nvmm_status ns, numa_node_t *node)
{
	numa_node_t nvmm_optimal_node = nvmm_preferred_node(ns);

	/* Avoid future migration due to cpu congestion */
	if (cpu_get_usage(nvmm_optimal_node) + proc->cpu_usage > CPU_LIMIT)
		return false;

	*node = nvmm_optimal_node;
	return true;
}

static inline bool cpu_decide_target(struct process *proc, numa_node_t *node)
{
	numa_node_t cpu_optimal_node = cpu_preferred_node();
	u64 chosen_node_bytes = nvmm_get_bytes_counters(cpu_optimal_node);
	u64 introduced_bytes = proc->diff_numa_write_sum;

	/* Avoid future migration due to nvmm congestion */
	if (chosen_node_bytes + introduced_bytes > MAX_TOTAL_BW)
		return false;

	*node = cpu_optimal_node;
	return true;
}

void fix_congestion(struct process_list *l)
{
	numa_node_t nvmm_congested_node;
	numa_node_t cpu_congested_node;
	numa_node_t target_node, current_node;
	struct nvmm_status status;
	struct process *proc;

	if (list_is_empty(l))
		return;
	
	/* System-wide indicators */
	nvmm_congested_node = nvmm_contention();
	cpu_congested_node = cpu_contention();

	if (node_is_defined(nvmm_congested_node))
		compute_nvmm_status(l, nvmm_congested_node, &status);

	for(proc = l->head; proc; proc = proc->next) {
		current_node = proc->numa_node;

		if (current_node == nvmm_congested_node &&
		    is_write_intensive(proc, status)) {
			if (nvmm_target_is_suitable(proc, status))
				goto migrate;
			if (nvmm_decide_target(proc, status, &target_node))
				goto migrate;
		}

		if (proc->numa_node == cpu_congested_node &&
		    moderate_cpu_usage(proc)) {
			if (cpu_target_is_suitable(proc, cpu_congested_node))
				goto migrate;
			if (cpu_decide_target(proc, &target_node))
				goto migrate;
		}

		target_node = get_target_node(proc);
		if (!node_is_defined(target_node) ||
		    target_node == nvmm_congested_node ||
		    target_node == cpu_congested_node)
			continue;
migrate:
		update_status(proc, target_node, &status);
		migrate_thread(proc->tid, target_node);
	}
}

void target_fini() {
	quickselect_fini();
}
