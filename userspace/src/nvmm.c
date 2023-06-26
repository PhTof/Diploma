#include <stdio.h>
#include <unistd.h> // sleep
#include <stdlib.h> // malloc
// Device discovery struct
#include <nvm_management.h>
// #include <nvm_types.h>
#include <signal.h> // handle abort signal

#include "pmw_struct.h"
#include "pmw_api.h"

// Defined constants
#define BW_R_WEIGHT ((float) 1/3)
#define NVMM_BW_LIMIT 0.8
#define MAX_READ_BW (6.8 * 1e8)
#define MAX_WRITE_BW (1.85 * 1e8)
#define MAX_TOTAL_BW \
	(MAX_READ_BW * BW_R_WEIGHT + MAX_WRITE_BW)
#define FREQ 1
#define NONE (-1)

// Defined functions
#define READ_BW(b,N) \
	(PMWATCH_OP_BUF_TOTAL_BYTES_READ(&b[N])/FREQ)
#define WRITE_BW(b,N) \
	(PMWATCH_OP_BUF_TOTAL_BYTES_WRITTEN(&b[N])/FREQ)
#define TOTAL_BW(b,N) \
	(READ_BW(b,N) * BW_R_WEIGHT + WRITE_BW(b,N))

// Fancy Printing
#define DEBUG 1
#define KRED  "\x1B[31m"
#define KWHT  "\x1B[37m"

typedef unsigned long long u64;

// [GCC] We need to pass this argument:
// -L $(PMWATCHDIR)/lib64 -l:libpmwcollect.so
extern struct device_discovery* PMW_COMM_Get_DIMM_Topology(void);

// Used by nvmm_contention
int nvmm_num_numa_nodes, num_nvdimms;
uint64_t *bw_per_numa = NULL;
PMWATCH_OP_BUF pmwatch_op_buf = NULL;
struct device_discovery *dd;

void nvmm_contention_prepare() {
	int ret;

	// Get the number of nvdimms installed	
	ret = PMWAPIGetDIMMCount(&num_nvdimms);
	
	if (ret) {
		fprintf(stderr, "PMWAPIGETDIMMCount Failed\n");
		abort();
	}

	// Obtain the topology information
	dd = PMW_COMM_Get_DIMM_Topology();

	// Allocate the buffer thourgh which counters
	// information will be updated
	if (!pmwatch_op_buf)
		pmwatch_op_buf = 
			(PMWATCH_OP_BUF) malloc(num_nvdimms*sizeof(PMWATCH_OP_BUF_NODE));
	
	if (!pmwatch_op_buf) {
		fprintf(stderr, "buffer allocation @ %s failed\n", __func__);
		abort();
	}
	
	// Get the number of numa nodes present in the system
	nvmm_num_numa_nodes = dd[num_nvdimms-1].socket_id + 1;
	
	// Allocate the buffer used to count the total
	// nvmm bandwidth usage of each node
	if(!bw_per_numa)	
		bw_per_numa = (uint64_t *) malloc(nvmm_num_numa_nodes*sizeof(uint64_t));

	if(!bw_per_numa) {
                fprintf(stderr, "buffer allocation @ %s failed\n", __func__);
                abort();
        }
}

void nvmm_contention_cleanup() {
	if (pmwatch_op_buf)
		free(pmwatch_op_buf);
	if (bw_per_numa)
		free(bw_per_numa);
}

int nvmm_congestion() {
	// condition: (BW of i) > (NVMM_BW_LIMIT * MAX_BW) &&
	// (BW for any i != j) < 0.5 * (BW of i)
	
	int nvdimm, node, numa_node, max_numa_node = 0;
	uint64_t this_node_bw, max_numa_bw = 0;

	// Update the information buffer
	PMWAPIRead(&pmwatch_op_buf);
	
	// Record the total BW for each node
	memset(bw_per_numa, 0, nvmm_num_numa_nodes*sizeof(uint64_t));
	
	for (nvdimm = 0; nvdimm < num_nvdimms; nvdimm++) {
		node = dd[nvdimm].socket_id;
		bw_per_numa[node] += TOTAL_BW(pmwatch_op_buf, nvdimm);
	}
	
	for (numa_node = 0; numa_node < nvmm_num_numa_nodes; numa_node++) {
		printf("%s[DEBUG]%s bw_per_numa[%i] = %ld\n", 
			KRED, KWHT, numa_node, bw_per_numa[numa_node]);
	}
	
	// Find the node with the maximum bandwidth
	for (numa_node = 0; numa_node < nvmm_num_numa_nodes; numa_node++) {
		this_node_bw = bw_per_numa[numa_node];
		if (max_numa_bw < this_node_bw) {
			max_numa_bw = this_node_bw;
			max_numa_node = numa_node;
		}
	}

	if (DEBUG)
		printf("%s[DEBUG]%s max_numa_node = %d, max_numa_bw = %ld, other_numa_bw = %ld\n",
			KRED, KWHT, max_numa_node, max_numa_bw, bw_per_numa[1]);

	// Check first part of the condition
	if (max_numa_bw < NVMM_BW_LIMIT * MAX_TOTAL_BW)
		return NONE;

	// Check second part of the condition
	for (numa_node = 0; numa_node < nvmm_num_numa_nodes; numa_node++)
		if (numa_node != max_numa_node)
			if (bw_per_numa[numa_node] >= max_numa_bw >> 1)
				return NONE;

	return max_numa_node;
}

u64 nvmm_get_node_BW(int node) {
	return bw_per_numa[node];
}

void startPMWatch() {
	int ret;
	PMWATCH_CONFIG_NODE conf;

	PMWATCH_CONFIG_INTERVAL(&conf) = FREQ;
	PMWATCH_CONFIG_COLLECT_HEALTH(&conf) = 0;
	PMWATCH_CONFIG_COLLECT_PERF_METRICS(&conf) = 1; 
	
	ret = PMWAPIStart(conf);

	if (ret) {
                fprintf(stderr, "Unsuccessful call to PMWAPIStart.\n");
                abort();
	}
	
}

int stopPMWatch() {
	return PMWAPIStop();
}

void abort_handler() {
	nvmm_contention_cleanup();
	stopPMWatch();
}

void int_handler() {
	nvmm_contention_cleanup();
	stopPMWatch();
	exit(0);
}

/*

int main() {
	int i = 0;

	signal(SIGABRT, abort_handler);
	signal(SIGINT, int_handler);
	
	startPMWatch();
	nvmm_contention_prepare();
	
	while (i++ < 100000) {
		sleep(1);
		printf("contention = %d\n", 
			nvmm_congestion());
	}

	nvmm_contention_cleanup();
	(void) stopPMWatch();

	return 0;
}

*/
