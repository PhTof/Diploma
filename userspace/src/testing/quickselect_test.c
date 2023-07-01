#include <stdio.h>
#include <stdlib.h>

#include "../../headers/quickselect.h"

// shamelessly stolen from geeksforgeeks

// Function to swap two elements
void swap(struct process * a, struct process *b)
{
	struct process t = *a;
	*a = *b;
	*b = t;
}

// Partition the array using the last element as the pivot
int _partition(struct process arr[], int low, int high)
{
	// Choosing the pivot
	int pivot = arr[high].diff_numa_write_sum;

	// Index of smaller element and indicates
	// the right position of pivot found so far
	int i = (low - 1);

	for (int j = low; j <= high - 1; j++) {

		// If current element is smaller than the pivot
		if (arr[j].diff_numa_write_sum < pivot) {

			// Increment index of smaller element
			i++;
			swap(&arr[i], &arr[j]);
		}
	}
	swap(&arr[i + 1], &arr[high]);
	return (i + 1);
}

// The main function that implements QuickSort
// arr[] --> Array to be sorted,
// low --> Starting index,
// high --> Ending index
void quickSort(struct process arr[], int low, int high)
{
	if (low < high) {

		// pi is partitioning index, arr[p]
		// is now at right place
		int pi = _partition(arr, low, high);

		// Separately sort elements before
		// partition and after partition
		quickSort(arr, low, pi - 1);
		quickSort(arr, pi + 1, high);
	}
}

// Driver program to test above methods
int main(int argc, char **argv)
{
	struct process arr[10];
	arr[0].diff_numa_write_sum = 85000000;
	arr[0].next = &arr[1];
	arr[1].diff_numa_write_sum = 71000000;
	arr[1].next = &arr[2];
	arr[2].diff_numa_write_sum = 82000000;
	arr[2].next = &arr[3];
	arr[3].diff_numa_write_sum = 79000000;
	arr[3].next = &arr[4];
	arr[4].diff_numa_write_sum = 61000000;
	arr[4].next = &arr[5];
	arr[5].diff_numa_write_sum = 45000000;
	arr[5].next = &arr[6];
	arr[6].diff_numa_write_sum = 81000000;
	arr[6].next = &arr[7];
	arr[7].diff_numa_write_sum = 76000000;
	arr[7].next = &arr[8];
	arr[8].diff_numa_write_sum = 29000000;
	arr[8].next = &arr[9];
	arr[9].diff_numa_write_sum = 46000000;
	arr[9].next = NULL;

	arr[0].numa_node = 0;
	arr[1].numa_node = 0;
	arr[2].numa_node = 0;
	arr[3].numa_node = 0;
	arr[4].numa_node = 0;
	arr[5].numa_node = 0;
	arr[6].numa_node = 0;
	arr[7].numa_node = 0;
	arr[8].numa_node = 0;
	arr[9].numa_node = 0;
	//	struct process *p[10];

	//	for (int i = 0; i < 10; i++) {
	//		p[i] = &arr[i];
	//	}

	int n = 10;
	struct process_list list = (struct process_list) { &arr[0], n };

	u64 k;

	if (argc != 2)
		return 1;

	k = strtoull(argv[1], NULL, 10);

	printf("TEST: Array = ");
	for (int i = 0; i < n; i++)
		printf("%llu ", arr[i].diff_numa_write_sum);
	printf("\n");
	//struct partition_result res = partition(arr, 0, n-1);
	//printf("i = %d sl = %d sr = %d\n", res.index, res.sl, res.sr);

	u64 res = quickselect_get_write_threshold(&list, 0, k);
	printf("TEST: res = %llu\n", res);
	//	int idx = quickselect(p, 0, n - 1, k);
	//	printf("s = %d && idx = %d\n", k, idx);
	quickSort(arr, 0, 9);
	printf("TEST ");
	for (int i = 0; i < n; i++) {
		printf("%llu + ", arr[i].diff_numa_write_sum);
		//		if (i == idx)
		//			printf("(*) ");
	}
	printf("\n");
	return 0;
}
