#include <stdio.h>	// printf
#include <stdlib.h>	// abort()
#include <fcntl.h>	// open() & posix_fadvise
#include <unistd.h>	// read() & close()
#include <sys/stat.h>	// fstat() -- file size
#include <sys/time.h>	// for clock_gettime()
#include <locale.h>	// used to set locale for printf formatting
#include <errno.h>	// report error on failed read
#include <string.h>
#include <getopt.h>	// long arguments
#include <pthread.h>	// posix threads

#define NO_STR ""
#define DEFAULT_FILE_SIZE (256*1024*1024)
#define USECS_IN_SEC 1000000
#define NO_NUM_THREADS 1
#define	NO_DIRECTORY 2
#define NO_JOB_NAME 3
#define NO_FILE_SIZE 4
#define NO_BLOCK_SIZE 5
#define NO_LOOPS 6

enum access_type {
	ACCESS_TYPE_READ,
	ACCESS_TYPE_WRITE
};

struct worker_info {
	pthread_t thread_id;
	int thread_num;
	char *job_name;
	char *directory;
	long file_size;
	int block_size;
	int loops;
	int invalidate;
	enum access_type atype;
};

long get_file_size(int fd) {
	struct stat st;
	fstat(fd, &st);
	return st.st_size;
}

void reset_file_pointer(int fd) {
	lseek(fd, 0, SEEK_SET);
}

void *safe_malloc(size_t n) {
	void *p = malloc(n);
	if (p == NULL) {
		fprintf(stderr, "Fatal: failed to allocate %zu bytes.\n", n);
		abort();
	}
	return p;
}

#define FILENAME_SIZE 50
static void *worker_do(void *arg) {
	int fd;
	struct worker_info *info = arg;

	/* Open the correspoding file */	
	char filename[FILENAME_SIZE];

	sprintf(filename, "%s/%s_%d", 
		info->directory, 
		info->job_name, 
		info->thread_num);

	if (access(filename, F_OK) == 0) {
		// file exists
		fd = open(filename, O_RDWR);
	} else {
		// file doesn't exist
		fd = open(filename, O_CREAT | O_RDWR, 0666);
		ftruncate(fd, info->file_size);
	}


	if (fd < 0) {
		printf("Could not open file: %s\n", filename);
		perror("");
		abort();
	}

	int loops = info->loops;
	int block_size = info->block_size;
	long file_size = get_file_size(fd);

	if (info->invalidate) {
		posix_fadvise(fd, 0L, file_size, POSIX_FADV_DONTNEED);
	}

	char *buffer = (char *) safe_malloc(block_size); 
	
	float *bandwidth = malloc(sizeof(float));
	float duration;
	struct timeval t_start, t_end;
	long long secs_used,micros_used;
	
	gettimeofday(&t_start, NULL);
	while (loops--) {
		long bytes_remaining = file_size;
		long bytes_accessed;
		reset_file_pointer(fd);

		while (bytes_remaining > 0) {
			switch(info->atype) {
				case ACCESS_TYPE_READ:
					bytes_accessed = read(fd, buffer, block_size);
					break;
				case ACCESS_TYPE_WRITE:
					bytes_accessed = write(fd, buffer, block_size);
					break;
				default: ;
			}

			/* "Error handling" */	
			if (bytes_accessed < 0) {
				close(fd);
				printf("%s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}

			bytes_remaining -= bytes_accessed;
		}
	}
	gettimeofday(&t_end, NULL);
	
	secs_used = (t_end.tv_sec - t_start.tv_sec);
	micros_used = secs_used * USECS_IN_SEC + (t_end.tv_usec - t_start.tv_usec);
	duration = (float) micros_used / USECS_IN_SEC;

	*bandwidth = (float) ((info->loops*file_size) >> 10) / duration;

	free(buffer);
	close(fd);
	return bandwidth;
}
#undef FILENAME_SIZE

void print_help() {
	printf(	"./cfio	--num_threads <int>	\n"
		"	--directory <str>	\n"
		"	--job_name <str>	\n"
		"	--file_size <long>	\n"
		"	--block_size <int>	\n"
		"	--loops <int>		\n"
		"	--invalidate <bool>	\n"
		"	--is_write <bool>	\n");
}

void declare_error_and_exit(const char *msg) {
	printf("%s\n", msg);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {

	int num_threads = 1;
	char *directory = strdup("./");
	char *job_name = strdup("JOB");
	long file_size = DEFAULT_FILE_SIZE;
	int block_size = 4096;
	int loops = 1;
	int invalidate = 0;	
	enum access_type atype = ACCESS_TYPE_READ;

	void *res;
	
	while (1) {
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		struct option long_options[] = {
			{"num_threads", required_argument, 0, NO_NUM_THREADS },
			{"directory", required_argument, 0, NO_DIRECTORY },
			{"job_name", required_argument, 0, NO_JOB_NAME },
			{"file_size", required_argument, 0, NO_FILE_SIZE },
			{"block_size", required_argument, 0, NO_BLOCK_SIZE },
			{"loops", required_argument, 0, NO_LOOPS },
			{"invalidate", no_argument, &invalidate, 1 },
			{"is_write", no_argument, (int *) &atype, ACCESS_TYPE_WRITE },
			{0, 0, 0, 0 }
		};

		char c = getopt_long(argc, argv, NO_STR, long_options, &option_index);

		// No more arguments
		if (c == -1)
			break;

		switch(c) {
			case 0:
				continue;
			case NO_NUM_THREADS:
				num_threads = atoi(optarg);
				if (num_threads < 1)
					declare_error_and_exit("num_threads must be positive");
				break;
			case NO_DIRECTORY:
				free(directory);
				directory = strdup(optarg);
				if (!directory)
					declare_error_and_exit("strdup failed");
				break;
			case NO_JOB_NAME:
				free(job_name);
				job_name = strdup(optarg);
				if (!directory)
					declare_error_and_exit("strdup failed");
				break;
			case NO_FILE_SIZE:
				file_size = atol(optarg);
				if (file_size < 1)
					declare_error_and_exit("file_size must be positive");
				break;
			case NO_BLOCK_SIZE:
				block_size = atoi(optarg);
				if (block_size < 1)
					declare_error_and_exit("block_size must be positive");
				break;
			case NO_LOOPS:
				loops = atoi(optarg);
				if (loops < 1)
					declare_error_and_exit("loops must be positive");
				break;
			default:
				print_help();
				exit(EXIT_FAILURE);
		}
	}




	struct worker_info *info = malloc(num_threads*sizeof(*info));

	for (int tnum = 0; tnum < num_threads; tnum++) {
		info[tnum] = (struct worker_info) {
			.thread_num = tnum,
			.job_name = job_name,
			.directory = directory,
			.file_size = file_size,
			.block_size = block_size,
			.loops = loops,
			.invalidate = invalidate,
			.atype = atype
		};

		int s = pthread_create(
				&info[tnum].thread_id,
				NULL,
				&worker_do,
				&info[tnum]
			);

		if (s)
			declare_error_and_exit("thread creation failed");

	}


	float total_bw = 0;


	for (int tnum = 0; tnum < num_threads; tnum++) {

		int s = pthread_join(info[tnum].thread_id, &res);
		
		if (s)
			declare_error_and_exit("pthread_join failed");
		
		float *bw = res;
		total_bw += *bw;
		free(res);
	}

	setlocale(LC_NUMERIC, "");
	printf("BW = %'.0f (KBps)\n", total_bw);

	free(directory);
	free(job_name);
	free(info);

}
