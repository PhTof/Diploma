// https://man7.org/linux/man-pages/man5/proc.5.html

#include <stdio.h>	// FILE, *open(), *scanf(), *printf(), *close()
// #include <errno.h> // error number
// #include <signal.h> // pthread_kill
#include <stdlib.h>	// free()

#include "../headers/cpu.h"
#include "../headers/util.h"	// smalloc()

/* Utility structures (utilization per NUMA node) */
static int num_nodes;
static float *cpu_utilization = NULL;

void cpu_init(int num_numa_nodes) {
	int n = num_numa_nodes * sizeof(float);
	num_nodes = num_numa_nodes;
	cpu_utilization = (float *) smalloc(n);
}

void cpu_reset() {
	for(int n = 0; n < num_nodes; n++)
		cpu_utilization[n] = 0;
}

void cpu_update(int node, float usage) {
	cpu_utilization[node] += usage;
}

float cpu_get_usage(int node) {
	return cpu_utilization[node];
}

// TODO: remove
void cpu_print() {
	int i;
	for(i = 0; i < num_nodes; i++)
		printf("NODE %d = %d\n", i, (int) cpu_utilization[i]);
}

void cpu_fini() {
	free(cpu_utilization);
}

int cpu_contention() {
	/* Condition: node x has ut(x) > CPU_LIMIT and
	 * ut(n) is at most half of ut(x) for every
	 * other node n */
	int res = NO_CPU_CONTENTION, i;
	// Synonym for ease of readability
	float *ut = cpu_utilization;

	for (i = 0; i < num_nodes; i++)
		if (ut[i] > CPU_LIMIT)
			res = i;
	
	if (res == NO_CPU_CONTENTION)
		return NO_CPU_CONTENTION;

	for (i = 0; i < num_nodes; i++)
		if (i != res && 2*ut[i] > ut[res])
			return NO_CPU_CONTENTION;

	return res;
}

void cpu_stat(int tid, int *cpu, u64 *utime, u64 *stime) {
	#define PL 128
	#define LL 1024
	#define UTIME_FIELD_IDX 14
	#define PROCESSOR_FIELD_IDX 39
	char path[PL], line[LL];
	FILE* f;
	// field = 2, because we skip [pid] and [comm]
	int pos, i = -1, field = 2;
	int len = 0;

	/* We account every thread as a separate entity here
	 * as well */
	snprintf(path, PL, "/proc/%d/task/%d/stat", tid, tid);

	f = fopen(path, "r");
	if(!f) return;

	/* We can't just fscanf("%d (%*[^)]) ...", ...),
	 * neither fscanf("%d (%*s) ...", ...), because
	 * the probability of comm having either a space
	 * or a closing parenthesis inside () is high.
	 * [ex, I have seen (tmux: smth) and ((sd-pam)) ] */
	// fscanf(f, "%[^\n]", line); <-- Incredibly stupid bug
	// newline was never saved, everything else failed miserably
	fscanf(f, "%[^\n] %n", line, &len);

	/* Find the last ')' to properly exclude the 
	 * problematic comm field */
	while(++i < len)
		if (line[i] == ')')
			pos = i + 1;

	/* Skip some values, so as not to write
	 * an unreadable scanf format string */
	while(field < UTIME_FIELD_IDX)
		field += (line[pos++] == ' ');

	/* We don't take into consideration the cutime and
	 * cstime values (whatever the children do will be
	 * accounted in their turn) */
	sscanf(line + pos, "%llu %llu", utime, stime);

	/* Continue for the processor field */	
	while(field < PROCESSOR_FIELD_IDX)
		field += (line[pos++] == ' ');

	sscanf(line + pos, "%d", cpu);

	fclose(f);
}

// TODO: add this to ./testing/
/*
void main() {
	cpu_contention_init(2);

	int i = 0;
	while(i++ < 50)
	{
		sleep(1);
		printf("%d\n", cpu_contention());
	}
	
	cpu_contention_fini();
}
*/
