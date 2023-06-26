#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "../headers/quickselect.h"
#include "../headers/processes.h"

static inline void swap(
	struct process **a,
	struct process **b) 
{
	struct process *tmp = *a;
	*a = *b;
	*b = tmp;
}

struct partition_result 
partition(struct process **arr, int l, int r)
{
	int i = l;
	int ri = l + rand() % (r - l + 1);
	int pivot = arr[ri]->write_ratio;
	float sl = 0, sr = 0;

	swap(&arr[ri], &arr[r]);

	for (int j = l; j <= r - 1; j++) {
		if (arr[j]->write_ratio >= pivot) {
			sl += arr[j]->write_ratio;
			swap(&arr[i], &arr[j]);
			i++;
		} else {
			sr += arr[j]->write_ratio;
		}
	}
	swap(&arr[i], &arr[r]);
	return (struct partition_result) {i, sl, sr};
}

int quickselect(
	struct process **arr,
	int l, int r, float target)
{
	struct partition_result res = partition(arr, l, r);
	int index = res.index;
	float index_wratio = arr[index]->write_ratio;

	if (res.sl + index_wratio + res.sr < target)
		return r;

	if (res.sl <= target && target <= res.sl + index_wratio)
		return index;

	if (target < res.sl)
		return quickselect(arr, l, index - 1, target);

	return quickselect(arr, index + 1, r,
		target - index_wratio - res.sl);
}
