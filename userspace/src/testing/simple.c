#include <stdio.h>	// printf()
#include <fcntl.h>	// open(), posix_fadvise()
#include <unistd.h>	// read() & close()
#include <stdlib.h>	// abort()
#include <sys/stat.h>	// fstat() -- file size
#include <sys/time.h>	// for clock_gettime()
#include <locale.h>	// used to set locale for printf formatting

long get_file_size(int fd) {
	struct stat st;
	fstat(fd, &st);
	return st.st_size;
}

void *safe_malloc(size_t n) {
	void *p = malloc(n);
	if (p == NULL) {
		fprintf(stderr, "Fatal: failed to allocate %zu bytes.\n", n);
		abort();
	}
	return p;
}


int main(int argc, char **argv) {
        if (argc != 3) {
                printf("Missing operands\n");
                printf("Usage: ./simple filename blocksize\n");
                abort();
        }

        char *filename = argv[1];
        int fd = open(filename, O_RDONLY);
        // int fd2 = open("anotherfile", O_RDONLY);

        if (fd < 0) {
                printf("Could not open file: %s\n", filename);
                abort();
        }

        int blocksize = atoi(argv[2]);

        if (blocksize <= 0) {
                printf("Invalid blocksize given\n");
                close(fd);
                abort();
        }

        long size = get_file_size(fd);
        long remaining = size;
        // posix_fadvise(fd, 0L, size, POSIX_FADV_DONTNEED);

        char *buffer = (char *) safe_malloc(blocksize);
        long ret;

        struct timeval start, end;
        long secs_used,micros_used;

        gettimeofday(&start, NULL);
        while (remaining > 0) {
                //ret = read(fd, buffer + size - remaining, blocksize);
                ret = read(fd, buffer, blocksize);
                remaining -= ret;
                // printf("%ld - %ld\n", ret, size);
        }
        gettimeofday(&end, NULL);

        secs_used = (end.tv_sec - start.tv_sec);
        micros_used = ((secs_used*1000000) + end.tv_usec) - (start.tv_usec);
        float duration = (float) micros_used / 1000000;

        float bandwidth; // in KBps

        bandwidth = (float) (size >> 10) / duration;

        setlocale(LC_NUMERIC, "");
        printf("%'.0f (KBps)\n", bandwidth);

        free(buffer);
        close(fd);

}

