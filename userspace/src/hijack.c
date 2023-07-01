#include <stdio.h>  // printf
#include <dlfcn.h>  // dlopen
#include <errno.h>  // errno
#include <string.h> // strerror
#include <stdlib.h> // abort, rand
#include <unistd.h> // read() & close()
#include <numa.h>

// O_CREAT
#include <asm-generic/fcntl.h>

#include "../headers/dax.h"
#include "../headers/numa.h"

// Static initialization of a loaded function symbol
// For example, if symbol = open, this is equivalent to:
// static open_t real_open = NULL;
// if ( !real_open ) real_open = load("open").open
#define INIT_REAL_FUN(symbol) static symbol##_t real_##symbol = NULL; \
	if ( !real_##symbol ) real_##symbol = load(#symbol).symbol;

typedef int (*open_t) (const char *pathname, int flags);
typedef ssize_t (*read_t) (int fd, const void *buf, size_t count);
typedef ssize_t (*write_t) (int fd, const void *buf, size_t count);

union function_t {
	open_t open;
	read_t read;
	write_t write;
};

union function_t load(char *symbol) {
	static void *handle = NULL;
	static int dlopened = 0;
	union function_t function;

	if (!dlopened) {
		handle = dlopen("libc.so.6", RTLD_NOW);
		if (!handle) {
			fprintf(stderr, "Error loading libc.so.6: %s\n",
				strerror(errno));
			abort();
		}
		printf("dlopned!\n");
	}

	if (strcmp(symbol, "open"))
		function.open = (open_t) dlsym(handle, symbol);
	else if (strcmp(symbol, "read"))
		function.read = (read_t) dlsym(handle, symbol);
	else if (strcmp(symbol, "write"))
		function.write = (write_t) dlsym(handle, symbol);
	else
		abort();

	return function;
}

// open syscall wrapper
int open(const char *pathname, int flags) {
	INIT_REAL_FUN(open);

	if (flags & O_CREAT) {
		numa_run_on_node(rand() % 2);
	}

	int fd = real_open(pathname, flags);

//	if (fd >= 0)
//		daxflag(fd, 1);

	return fd;
}

// read syscall wrapper
ssize_t read(int fd, void *buf, size_t count) {
	INIT_REAL_FUN(read);

	return real_read(fd, buf, count);
}

// write syscall wrapper
ssize_t write(int fd, const void *buf, size_t count) {
	INIT_REAL_FUN(write);

	return real_write(fd, buf, count);
}
