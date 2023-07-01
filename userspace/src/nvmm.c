#include <stdio.h>
#include <unistd.h> // sleep
#include <stdlib.h> // malloc
// Device discovery struct
#include <nvm_management.h>
// #include <nvm_types.h>
#include <signal.h> // handle abort signal

#include "pmw_api.h"
#include "pmw_struct.h"
#include "../headers/nvmm.h"

// [GCC] We need to pass this argument:
// -L $(PMWATCHDIR)/lib64 -l:libpmwcollect.so
extern struct device_discovery* PMW_COMM_Get_DIMM_Topology(void);

// Used by nvmm_contention
int nvmm_num_numa_nodes, num_nvdimms;
u64 *bw_per_numa_counters = NULL;
u64 *bw_per_numa_procfs = NULL;
PMWATCH_OP_BUF pmwatch_op_buf = NULL;
struct device_discovery *dd;

void start_pmwatch()
{
	PMWATCH_CONFIG_NODE conf;
	int ret;

	PMWATCH_CONFIG_INTERVAL(&conf) = PMWATCH_FREQ;
	PMWATCH_CONFIG_COLLECT_HEALTH(&conf) = 0;
	PMWATCH_CONFIG_COLLECT_PERF_METRICS(&conf) = 1; 
	
	ret = PMWAPIStart(conf);

	if (ret) {
                fprintf(stderr, "Unsuccessful call to PMWAPIStart.\n");
                exit(EXIT_FAILURE);
	}	
}

int stop_pmwatch()
{
	return PMWAPIStop();
}

void nvmm_init()
{
	int alloc_size;
	int ret;

	start_pmwatch();

	// Get the number of nvdimms installed	
	ret = PMWAPIGetDIMMCount(&num_nvdimms);
	
	if (ret) {
		fprintf(stderr, "PMWAPIGETDIMMCount Failed\n");
                exit(EXIT_FAILURE);
	}

	// Obtain the topology information
	// It is important here to have started PMWatch
	dd = PMW_COMM_Get_DIMM_Topology();

	// Allocate the buffer thourgh which counters
	// information will be updated
	alloc_size = num_nvdimms * sizeof(PMWATCH_OP_BUF_NODE);
	pmwatch_op_buf = (PMWATCH_OP_BUF) malloc(alloc_size);
	
	if (!pmwatch_op_buf)
		malloc_failure();
	
	// Get the number of numa nodes present in the system
	nvmm_num_numa_nodes = dd[num_nvdimms-1].socket_id + 1;
	
	// Allocate the buffer used to count the total
	// nvmm bandwidth usage of each node
	alloc_size = nvmm_num_numa_nodes * sizeof(u64);
	bw_per_numa_counters = (u64 *) malloc(alloc_size);

	if(!bw_per_numa_counters) {
		free(pmwatch_op_buf);
		malloc_failure();
        }

	bw_per_numa_procfs = (u64 *) malloc(alloc_size);

	if(!bw_per_numa_procfs) {
		free(bw_per_numa_counters);
		free(pmwatch_op_buf);
		malloc_failure();
        }

	memset(bw_per_numa_procfs, 0, alloc_size);
}

void nvmm_fini()
{
	free(bw_per_numa_procfs);
	free(bw_per_numa_counters);
	free(pmwatch_op_buf);
	stop_pmwatch();
}

/* We base this function on the counter
 * readings, as those provide the best
 * available metric for understanding
 * under how much pressure the devices
 * really are */
int nvmm_contention()
{
	// condition: (BW of i) > (NVMM_BW_LIMIT * MAX_BW) &&
	// (BW for any i != j) < 0.5 * (BW of i)	
	int nvdimm, node, numa_node, max_numa_node = 0;
	uint64_t this_node_bw, max_numa_bw = 0;
	u64 *bw_per_numa = bw_per_numa_counters;

	// Update the information buffer
	PMWAPIRead(&pmwatch_op_buf);

	// Record the total BW for each node
	memset(bw_per_numa, 0, nvmm_num_numa_nodes*sizeof(uint64_t));
	
	for (nvdimm = 0; nvdimm < num_nvdimms; nvdimm++) {
		node = dd[nvdimm].socket_id;
		bw_per_numa[node] += TOTAL_BW(pmwatch_op_buf, nvdimm);
	}

	for (numa_node = 0; numa_node < nvmm_num_numa_nodes && DEBUG; numa_node++)
		printf("%s[DEBUG]%s bw_per_numa[%i] = %lld\n", 
			KRED, KWHT, numa_node, bw_per_numa[numa_node]);
	
	// Find the node with the maximum bandwidth
	for (numa_node = 0; numa_node < nvmm_num_numa_nodes; numa_node++) {
		this_node_bw = bw_per_numa[numa_node];
		if (max_numa_bw < this_node_bw) {
			max_numa_bw = this_node_bw;
			max_numa_node = numa_node;
		}
	}

	if (DEBUG)
		printf(	"%s[DEBUG]%s max_numa_node = %d, max_numa_bw = %ld, "
			"other_numa_bw = %lld\n", KRED, KWHT, max_numa_node,
			max_numa_bw, bw_per_numa[1-max_numa_bw]);

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

void nvmm_reset()
{
	int node = 0;
	for (; node < nvmm_num_numa_nodes; node++)
		bw_per_numa_procfs[node] = 0;
}

void nvmm_update(int node, u64 read_bytes, u64 write_bytes)
{
	bw_per_numa_procfs[node] += TOTAL_BW_PROC(read_bytes, write_bytes);
}

u64 nvmm_get_bytes_procfs(int node)
{
	return bw_per_numa_procfs[node];
}

u64 nvmm_get_bytes_counters(int node)
{
	return bw_per_numa_counters[node];
}

/*
void abort_handler()
{
	nvmm_fini();
	stop_pmwatch();
}

void int_handler() {
	nvmm_contention_cleanup();
	stop_pmwatch();
	exit(0);
}


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
