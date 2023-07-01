#ifndef NVMM_H
#define NVMM_H

/* Error handling */
#define malloc_failure() do { \
	fprintf(stderr, "buffer allocation @ %s failed\n", __func__); \
        exit(EXIT_FAILURE); } while(0)

/* Defined constants */
#define BW_R_WEIGHT ((float) 1/3)
#define NVMM_BW_LIMIT 0.8
#define MAX_READ_BW (2 * 1e9) // TODO: 15
#define MAX_WRITE_BW (0.5 * 1e9) // TODO: 5
#define MAX_TOTAL_BW \
	(MAX_READ_BW * BW_R_WEIGHT + MAX_WRITE_BW)
#define PMWATCH_FREQ 1 //SECS
#define NONE (-1)

/* Defined functions */
#define READ_BW(b,N) \
	(PMWATCH_OP_BUF_TOTAL_BYTES_READ(&b[N])/PMWATCH_FREQ)
#define WRITE_BW(b,N) \
	(PMWATCH_OP_BUF_TOTAL_BYTES_WRITTEN(&b[N])/PMWATCH_FREQ)
#define TOTAL_BW(b,N) \
	(READ_BW(b,N) * BW_R_WEIGHT + WRITE_BW(b,N))
#define TOTAL_BW_PROC(r,w) (r * BW_R_WEIGHT + w)

/* Fancy Printing */
#define DEBUG 0
#define KRED  "\x1B[31m"
#define KWHT  "\x1B[37m"

#include "types.h"

void nvmm_init();
void nvmm_fini();
int nvmm_contention();
void nvmm_reset();
void nvmm_update(int node, u64 read_bytes, u64 write_bytes);
u64 nvmm_get_bytes_procfs(int node);
u64 nvmm_get_bytes_counters(int node);

#endif
