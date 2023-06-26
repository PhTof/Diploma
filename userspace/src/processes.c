#include <stdbool.h>
#include <stdio.h>	// NULL, printf
#include <stdlib.h>	// malloc()
#include <numa.h>	// numa_node_of_cpu()
#include <time.h>

#include "../headers/numa.h"
#include "../headers/cpu.h"
#include "../headers/processes.h"
#include "../headers/target.h"

int interval = 1;
u64 current_logical_timestamp = 0;

static struct process *process_initialize_stats(int tid) {
	int cpu = 0;
	u64 clock_ticks;
	u64 numa_read[NUM_NODES];
	u64 numa_write[NUM_NODES];
	u64 utime = 0, stime = 0;
	struct process *proc;

	proc = (struct process *) malloc(sizeof(struct process));
	// If the initialization failed, we will try
	// again during the next loop
	if (!proc)
		return proc;
	
	numa_iostat(tid, numa_read, numa_write);
	cpu_stat(tid, &cpu, &utime, &stime);
	clock_ticks = utime + stime;

	proc->prev_timestamp = (struct timespec) {0, 0};	
	clock_gettime(CLOCK_REALTIME, &proc->curr_timestamp);
	proc->tid = tid;
	proc->cpu = cpu;
	proc->numa_node = numa_node_of_cpu(proc->cpu);
	proc->logical_timestamp = current_logical_timestamp;
	#define UPDATE(field) \
		proc->diff_##field = 0; \
		proc->total_##field = field
	UPDATE(clock_ticks);
	for (int i = 0; i < NUM_NODES; i++) {
		UPDATE(numa_read[i]);
		UPDATE(numa_write[i]);
	}
	#undef UPDATE
	return proc;
}

static void process_update_stats(struct process *proc) {
	int cpu = 0;
	// The initializations 1 and 2 are important:
	// If the task has terminated, iostat/cpustat will
	// return shortly, and the read/write/cpu_usage 
	// values will be garbage
	float cpu_usage = 0;
	u64 utime = 0, stime = 0; /* 1 */
	u64 numa_read[NUM_NODES];
	u64 numa_write[NUM_NODES];
	u64 clock_ticks = proc->total_clock_ticks;
	
	for (int i = 0; i < NUM_NODES; i++) {
		numa_read[i] = proc->total_numa_read[i];
		numa_write[i] = proc->total_numa_write[i];
	}

	numa_iostat(proc->tid, numa_read, numa_write);
	cpu_stat(proc->tid, &cpu, &utime, &stime);
	// https://stackoverflow.com/questions/16726779/
	clock_ticks = utime + stime;

	proc->prev_timestamp = proc->curr_timestamp;
	clock_gettime(CLOCK_REALTIME, &proc->curr_timestamp);
	/* This may not be necessary to update here (at least *
	 * if we are sure that no one else is moving threads  *
	 * around) */
	proc->cpu = cpu;
	proc->numa_node = numa_node_of_cpu(proc->cpu);
	/*----------------------------------------------------*/
	proc->logical_timestamp = current_logical_timestamp;
	#define UPDATE(field) \
		proc->diff_##field = field - proc->total_##field; \
		proc->total_##field = field
	UPDATE(clock_ticks);
	for (int i = 0; i < NUM_NODES; i++) {
		UPDATE(numa_read[i]);
		UPDATE(numa_write[i]);
	}
	#undef UPDATE
	
	cpu_usage = process_cpu_usage(proc);
	cpu_update(proc->numa_node, cpu_usage);
}

static void add_pid_to_list(int tid, struct process_list *l) {
	struct process *proc = l->head, *new = NULL;

	// If the list is NULL, initialize it
	if (!proc)
		goto allocate_process;

	// See if the process already exists
	// in the list
	while (proc) {
		if (proc->tid == tid)
			break;
		proc = proc->next;
	}

	// If it does, update it
	if (proc) {
		process_update_stats(proc);
		return;
	}

allocate_process:
	// Elsewise, create a new node and
	// initialize it
	new = process_initialize_stats(tid);

	if (new) {
		new->next = l->head;
		l->head = new;
		(l->size)++;
	}
}

static void remove_expired_processes(struct process_list *l) {
	struct process *prev, *curr, *del;

	prev = NULL;
	curr = l->head;

	if (curr == NULL)
		return;

	while(curr) {
		if (curr->logical_timestamp < current_logical_timestamp) {
			del = curr;
			if (prev)
				prev->next = curr->next;
			else
				l->head = curr->next;
			curr = curr->next;
			(l->size)--;
			free(del);
		} else {
			prev = curr;
			curr = curr->next;
		}
	}
}

static void log_process_info(struct process *head) {
	struct process *proc = head;

	while (proc) {
		// temporarily
		//if (process_cpu_usage(proc) > 0.1)
		printf( "TID: %6d "
			"[cpu: %.3f]" //"[ratio: %.3f]"
			"[dr0: %lld][dw0: %lld]"
			"[dr1: %lld][dw1: %lld] (INIT_TARGET = %d)\n", 
			proc->tid, 
			process_cpu_usage(proc),
			proc->diff_numa_read[0],
			proc->diff_numa_write[0],
			proc->diff_numa_read[1],
			proc->diff_numa_write[1],
			target_get_initial(proc)
		);
		proc = proc->next;
	}
}

static void profile_forever(char *usr) {
	FILE *fp;
	struct process_list l = {NULL, 0};
	char line[LINELENGTH], command[LINELENGTH];
	struct timespec start, finish;
	int num_nodes;

	/* Make the interval between loops
	 * average to roughly {interval} secs */
	long interval_nsec = interval * NSECS_IN_SEC;
	long offset_nsec = 0;

	numa_init(); // TODO: this should be moved to main
	num_nodes = numa_num_nodes();
	cpu_init(num_nodes);	

	// -u: processes of specified user
	// -T: show threads instead of just processes
	// --no-headers: don't show the header (easier parsing)
	sprintf(command, "ps -u %s -T --no-headers", usr);

	while(true) {
		/* Start timing */
		clock_gettime(CLOCK_REALTIME, &start);
		
		fp = popen(command, "r");
		
		if (fp == NULL) {
			printf("Could not get process list\n");
			exit(EXIT_FAILURE);
		}

		cpu_reset();

		while (fgets(line, 100, fp) != NULL) {
			int tid;

			/* Ignore background deamons */
			if ( strstr(line, "sshd")  || 
			     strstr(line, " bash") ||
			     // These two are caused by popen
			     strstr(line, " sh")   ||
			     strstr(line, " ps")   )
				continue;

			sscanf(line, "%*d %d", &tid);
			add_pid_to_list(tid, &l);
		}
		remove_expired_processes(&l);
		if (FANCY_PRINTING)
			printf("\e[1;1H\e[2J");
		log_process_info(l.head);
		pclose(fp);

		cpu_print();	
		fix_congestion(&l);

		// TMP
		printf("CONT = %d\n", cpu_contention());
	
		/* Stop timing */	
		clock_gettime(CLOCK_REALTIME, &finish);

		/* Wait for the rest of the interval */	
		long computation_nsec = TIME_NSEC(start, finish);
		long sleep_interval_nsec = interval_nsec - computation_nsec - offset_nsec;
		usleep(sleep_interval_nsec/1000);
	
		// Update the logical timestamp	
		current_logical_timestamp += 1;
		// Correct the offset parameter
		clock_gettime(CLOCK_REALTIME, &finish);
		printf("%.5f secs\n\n", TIME_SEC(start, finish));
		offset_nsec += TIME_NSEC(start, finish) - interval_nsec;
	}
	// TODO: put this inside a signal handler
	cpu_fini();
	numa_fini();
}


int main(int argc, char **argv)
{
	char *usr = NULL;
	int c;

	while ((c = getopt(argc, argv, "i:u:")) != -1) {
		switch(c) {
			case 'u':
				usr = optarg;
				break;
			case 'i':
				interval = atoi(optarg);
				break;
			default:
				printf("Usage: %s [-u username] [-i interval]\n", argv[0]);
				exit(EXIT_FAILURE);
		}
	}
	if (usr == NULL) {
		usr = malloc(sizeof(LINELENGTH));
		if (getlogin_r(usr, LINELENGTH)) {
			printf("Could not retrieve login name.\n");
			exit(EXIT_FAILURE);
		}
	}
	profile_forever(usr);
}
