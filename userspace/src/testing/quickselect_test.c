#include <stdio.h>

#include "../../headers/quickselect.h"

// Driver program to test above methods
int main(int argc, char **argv)
{
	struct process arr[10];
	arr[0].write_ratio = 2.3;
	arr[1].write_ratio = 4.3;
	arr[2].write_ratio = 5.6;
	arr[3].write_ratio = 1.9;
	arr[4].write_ratio = 10.4;
	arr[5].write_ratio = 1.0;
	arr[6].write_ratio = 0.6;
	arr[7].write_ratio = 2.2;
	arr[8].write_ratio = 5.6;
	arr[9].write_ratio = 1.8;

	struct process *p[10];

	for (int i = 0; i < 10; i++) {
		p[i] = &arr[i];
	}

	int n = 10;
	float k;

	if (argc != 2)
		return 1;

	k = atof(argv[1]);

	printf("Array = ");
	for (int i = 0; i < n; i++)
		printf("%.1f ", p[i]->write_ratio);
	printf("\n");
	//struct partition_result res = partition(arr, 0, n-1);
	//printf("i = %d sl = %d sr = %d\n", res.index, res.sl, res.sr);

	int idx = quickselect(p, 0, n - 1, k);
	printf("s = %d && idx = %d\n", k, idx);
	for (int i = 0; i < n; i++) {
		printf("%.1f ", p[i]->write_ratio);
		if (i == idx)
			printf("(*) ");
	}
	printf("\n");
	return 0;
}
