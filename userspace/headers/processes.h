#ifndef PROCESSES_H
#define PROCESSES_H

#include <unistd.h>	// (at least) sysconf
#include <time.h>	// struct timespec
#include "numa.h"

#define FILENAMELENGTH 50
#define LINELENGTH 2000
#define NSECS_IN_SEC 1000000000
#define TIME_SEC_SINGLE(t) ((double) t.tv_nsec*1.0e-9 + (double) t.tv_sec)
// There is no real reason to use this, but why not?
#define unlikely(x) __builtin_expect((x),0)
#define TIME_SEC(s, e) (TIME_SEC_SINGLE(e) - TIME_SEC_SINGLE(s))
#define TIME_NSEC(s, e) ((e.tv_nsec + e.tv_sec*NSECS_IN_SEC) - (s.tv_nsec + s.tv_sec*NSECS_IN_SEC))

#define FANCY_PRINTING false

typedef unsigned long long u64;

extern int *cpus_per_node;

struct process {
	int tid;
	int cpu;
	int numa_node;
	u64 logical_timestamp;
	u64 total_numa_read[NUM_NODES];
	u64 total_numa_write[NUM_NODES];
	u64 total_clock_ticks;
	u64 diff_numa_read[NUM_NODES];
	u64 diff_numa_write[NUM_NODES];
	u64 diff_clock_ticks;
	float write_ratio;
	struct timespec prev_timestamp;
	struct timespec curr_timestamp;
	struct process *next;
};

struct process_list {
	struct process *head;
	int size;
};

inline static void proc_write_ratio(struct process *proc) {
	u64 acc_diff_read = 0;
	u64 acc_diff_write = 0;
	u64 denominator;

	for(int i = 0; i < NUM_NODES; i++) {
		acc_diff_read += proc->diff_numa_read[i];
		acc_diff_write += proc->diff_numa_write[i];
	}

	denominator = acc_diff_read + acc_diff_write;

	if (denominator == 0)
		proc->write_ratio = 0;
	else
		proc->write_ratio = (float) acc_diff_write / denominator;
}

// TODO: What about hyperthreading? Is it a good practice to
// sum the hyperthread cores when computing usage = time / (interval * cores)?
inline static float process_cpu_usage(struct process* proc) {
	long number_of_processors = numa_num_cpus(proc->numa_node);
	// https://stackoverflow.com/questions/19919881/
	long clock_ticks_per_sec = sysconf(_SC_CLK_TCK);
	float secs_consumed = ((float)proc->diff_clock_ticks/clock_ticks_per_sec) ;
	float elapsed_interval = TIME_SEC(proc->prev_timestamp,proc->curr_timestamp);
	// Alternative: adjusted_interval = interval * number_of_processors;
	float adjusted_interval = elapsed_interval * number_of_processors;
	return (secs_consumed / adjusted_interval) * 100;
}

#endif
