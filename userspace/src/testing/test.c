#define _GNU_SOURCE	// sched.h

#include <fcntl.h>	// open()
#include <unistd.h>	// read() & close()
#include <stdio.h>
#include <libpmem2.h>

#include <sched.h>	// sched_getcpu
#include <numa.h>	// numa_node_of_cpu
#include <unistd.h>	// getpid()

#include "../headers/dax.h"
#include "../headers/numa.h"

int main(int argc, char **argv) {
	if (argc == 1) {
		printf("No argument provided\n");
		return 0;
	}

	int fd = open(argv[1], O_RDONLY);

	printf("%d\n", getnumanode(fd));

	int cpu, node;
	
	setnumanode(getpid(), 0);
	
	//getcpu(&cpu, &node);
	cpu = sched_getcpu();
	node = numa_node_of_cpu(cpu);
	printf("cpu = %d\n", cpu);
	printf("node = %d\n", node);

	long double a[4];

	// daxflag(fd, 0);
	close(fd);
}
