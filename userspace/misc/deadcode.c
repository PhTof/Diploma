// Code borrowed from: https://stackoverflow.com/questions/29991182/
inline static void iostat(int tid, u64 *read, u64 *write) {
	#define LN 128
	char path[LN], line[LN], *p;
	FILE* f;

	/*  Why not /proc/%d/io?
	    Read here: https://man7.org/linux/man-pages/man5/proc.5.html
	    /proc/%d/io seems to accumulate the io statistics of each
	    thread in the specific TGID. /proc/%d/task/%d/ treats the
	    task as a separate entity */
	snprintf(path, LN, "/proc/%d/task/%d/io", tid, tid);

	f = fopen(path, "r");
	if(!f) return;

	while(fgets(line, LN, f)) {
		/*  */ if(!strncmp(line, "read_bytes:", 11)) {
			sscanf(line, "%*s %llu", read);
		} else if(!strncmp(line, "write_bytes:", 12)) {
			sscanf(line, "%*s %llu", write);
		}
	}

	fclose(f);
	#undef LN
}

/* numa.c */

// TODO: remove this if no longer needed
// Assumes little endian
static inline void printBits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;
    
    for (i = size-1; i >= 0; i--) {
        for (j = 7; j >= 0; j--) {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
}

// TODO: Remove this as well
void print_cpumask(struct bitmask **bms, int node) {
	int bitsperlong = sizeof(unsigned long) << 3;

	int i;
	for (i = 0; i < bms[node]->size/bitsperlong; i++) {
		printBits(bitsperlong >> 3, (void *) (bms[node]->maskp + i));
	}
	printf("\n");
}
