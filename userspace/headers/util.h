#include <stdio.h>	// perror(), sprintf()
#include <stdlib.h>	// exit()

// https://stackoverflow.com/questions/257418/
#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define smalloc(n) safe_malloc(n, __func__)

static inline void* safe_malloc(size_t n, const char* func) {
	void* p = malloc(n);

	if (!p) {
		char msg[64];
		sprintf(msg, "FAILURE: %s@%s\n", func,  __FILE__);
		handle_error(msg);
	}

	return p;
}
