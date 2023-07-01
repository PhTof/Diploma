/* quickselect.c: Overly complicated way to find a write threshold
 * for target.c in linear time relative to how many processes we have
 * accounted for this time quantum. We run an alteration of quickselect
 * to find a process with a diff_numa_write_sum (WS) such that the sum
 * of every WS of processes with WS higher or equal to this is adequate
 * to cover the required write_target bytes. */

/* Simpler alternatives: sort by WS and find the wanted process */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "../headers/quickselect.h"
#include "../headers/processes.h"

#define INFINITE_THRESHOLD ((u64) -1)
#define NO_THRESHOLD ((u64) 0)

#define BW_R_WEIGHT ((float) 1/3)
#define TOTAL_BW(p,n) \
	(p->diff_numa_read[n] * BW_R_WEIGHT + p->diff_numa_write[n])

// Vector like structure to avoid
// some memory allocations
typedef struct auxiliary_array {
	struct process **array;
	int alloc_size;
	int size;
	u64 total_write_sum;
} aux_arr;

static aux_arr global_aux_arr = {NULL, 0, 0};

static inline void swap(struct process **a, struct process **b) 
{
	struct process *tmp = *a;
	*a = *b;
	*b = tmp;
}

#define WRITE_THRESHOLD (2 << 20) // 1MB
static inline bool not_negligible(struct process *proc) {
	return (proc->diff_numa_write_sum > WRITE_THRESHOLD);
}

static void
prepare_aux_arr(struct process_list *l, aux_arr *a, numa_node_t node)
{
	int alloc_size, count = 0;
	struct process *proc = l->head;

	// extend the aux array if needed
	if (a->alloc_size < l->size) {
		free(a->array);
		alloc_size = 2*(l->size)*sizeof(struct process *);
		a->array = (struct process **) malloc(alloc_size);
		a->alloc_size = 2*(l->size);
	}

	a->total_write_sum = 0;
	while (proc) {
		if (proc->numa_node == node && not_negligible(proc)) {
			compute_write_ratio(proc);
			a->array[count++] = proc;
			a->total_write_sum += proc->diff_numa_write_sum;
		}
		proc = proc->next;
	}
	a->size = count;
}

/* Functions keep_process and compute_sum, as simple as they seem, are
 * defined mostly for ease of experimenting with different criteria */

static inline bool keep_process(struct process *proc, struct process *pivot)
{
	return proc->diff_numa_write_sum >= pivot->diff_numa_write_sum;
}

static inline u64 compute_sum(struct process *proc)
{
	// return TOTAL_BW(proc, 0) + TOTAL_BW(proc, 1);
	return proc->diff_numa_write_sum;
}

static struct partition_result 
partition(struct process **arr, int l, int r)
{
	int index = l;
	int ri = l + rand() % (r - l + 1);
	struct process *pivot = arr[ri];
	u64 sum_l = 0, sum_r = 0, sum;

	swap(&arr[ri], &arr[r]);

	for (int j = l; j <= r - 1; j++) {
		sum = compute_sum(arr[j]);
		if (keep_process(arr[j], pivot)) {
			sum_l += sum;
			swap(&arr[index], &arr[j]);
			index++;
		} else {
			sum_r += sum;
		}
	}
	swap(&arr[index], &arr[r]);
	return (struct partition_result) {index, sum_l, sum_r};
}

int quickselect(struct process **arr, int l, int r, u64 target)
{
	struct partition_result res = partition(arr, l, r);
	int index = res.index;
	u64 index_write = compute_sum(arr[index]);

	if (res.sl <= target && target <= res.sl + index_write)
		return index;

	if (target < res.sl)
		return quickselect(arr, l, index - 1, target);

	return quickselect(arr, index + 1, r,
		target - index_write - res.sl);
}

u64 quickselect_get_write_threshold(struct process_list *l,
		numa_node_t node, u64 write_target)
{
	int index, left, right;
	aux_arr *aux = &global_aux_arr;

	prepare_aux_arr(l, aux, node);

	// Not a single suitable process found
	if (aux->size == 0)
		return INFINITE_THRESHOLD;

	if (aux->total_write_sum < write_target)
		return NO_THRESHOLD;

	left = 0;
	right = aux->size - 1;
	index = quickselect(aux->array, left, right, write_target);

	printf("index = %d\n", index);
	for (int i = 0; i < aux->size; i++)
		printf("(%d) %llu + ", i, aux->array[i]->diff_numa_write_sum);
	printf("\n");

	return aux->array[index]->diff_numa_write_sum;
}

void quickselect_fini() {
	free(global_aux_arr.array);
}
