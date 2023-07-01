// Don't include this for now (may be completely useless)
// #include <libpmem2.h> 	// pmem2_*

#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h> // TMP
#include <errno.h>
#include <numa.h>

// Directly copied from libnuma
#define bitsperlong (8 * sizeof(unsigned long))
#define howmany(x,y) (((x)+((y)-1))/(y))
#define longsperbits(n) howmany(n, bitsperlong)

#include "../headers/numa.h"
#include "../headers/util.h"	// smalloc()

extern struct pthread *__find_thread_by_id (pid_t tid);

// Number of NUMA nodes
static int num_nodes = 0;
// A bitmap for each node
static struct bitmask **bms = NULL;
static int *cpus_per_node = NULL;

unsigned int
numa_bitmask_nbytes(struct bitmask *bmp)
{
        return longsperbits(bmp->size) * sizeof(unsigned long);
}

// TODO: remove this if no longer needed
// Assumes little endian
static inline void printBits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;
    
    for (i = size-1; i >= 0; i--) {
        for (j = 7; j >= 0; j--) {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
}

// TODO: Remove this as well
void print_cpumask(struct bitmask *bms) {

	int i;
	for (i = 0; i < bms->size/bitsperlong; i++) {
		printBits(bitsperlong >> 3, (void *) (bms->maskp + i));
	}
	printf("\n");
}

// Naive implementation
static inline int hamming_weight(unsigned long val) {
	int sum = 0;
	
	while(val) {
		sum += val & 1UL;
		val = val >> 1;	
	}

	return sum;
}

static inline void cpus_per_node_compute() {
	int i, node;

	for (node = 0; node < num_nodes; node++) {
		cpus_per_node[node] = 0;
		for (i = 0; i < bms[node]->size/bitsperlong; i++)
			cpus_per_node[node] +=
				hamming_weight(bms[node]->maskp[i]);
	}
}

void numa_init() {
	int i;
	num_nodes = numa_num_configured_nodes();
	bms = smalloc(sizeof(struct bitmask *)*num_nodes);
	cpus_per_node = smalloc(sizeof(int)*num_nodes);
	for (i = 0; i < num_nodes; i++) {
		/* https://github.com/numactl/numactl/blob/...
		 * master/libnuma.c#L218:
		 * This will exit upon failure by itself */
		bms[i] = numa_allocate_cpumask();
		numa_node_to_cpus(i, bms[i]);
	}
	cpus_per_node_compute();
}

void numa_fini() {
	for(int node = 0; node < num_nodes; node++)
		numa_free_cpumask(bms[node]);

	free(bms);
	free(cpus_per_node);
}

// Get the numa node where file {fd}
// is located
/*
int get_numa_node(int fd) {
	struct pmem2_source *src;
	int numanode;
	
	pmem2_source_from_fd(&src, fd);
	pmem2_source_numa_node(src, &numanode);
	
	return numanode;
}
*/

// Change the numa node on which {pid} runs
void numa_set_node(int tid, int node) {
	struct bitmask *mask;

	mask = numa_allocate_cpumask();
	numa_node_to_cpus(node, mask);
	numa_sched_setaffinity(tid, mask);

	numa_free_cpumask(mask);
}

int numa_num_nodes() {
	return num_nodes;
}

int numa_num_cpus(int node) {
	return cpus_per_node[node];
}

void numa_iostat(int tid, u64 *numa_read, u64 *numa_write) 
{
	#define LN 128
	const char *read_str = "numa%d_read_bytes: %llu";
	const char *write_str = "numa%d_write_bytes: %llu";
	char path[LN], line[LN];
	FILE* file;
	int node;
	u64 val;

	/*  Why not /proc/%d/numa_io?
	 *
	 *  We extended the io file from procfs to what is the numa_io
	 *  file we are using here, so the reasoning below applies.
	 *  Read here: https://man7.org/linux/man-pages/man5/proc.5.html
	 *  /proc/%d/io seems to accumulate the io statistics of each
	 *  thread in the specific TGID. /proc/%d/task/%d/ treats the
	 *  task as a separate entity. */
	snprintf(path, LN, "/proc/%d/task/%d/numa_io", tid, tid);

	file = fopen(path, "r");

	if(!file)
		return;

	while(fgets(line, LN, file)) {
		/**/ if(sscanf(line, read_str, &node, &val) == 2)
			numa_read[node] = val;
		else if(sscanf(line, write_str, &node, &val) == 2)
			numa_write[node] = val;
	}

	fclose(file);
	#undef LN
}
