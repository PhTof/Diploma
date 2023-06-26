#ifndef QUICKSELECT_H
#define QUICKSELECT_H

#include "processes.h"

struct partition_result {
	int index;
	float sl;
	float sr;
};

int quickselect(
	struct process **arr,
	int l, int r, float target
);

#endif
