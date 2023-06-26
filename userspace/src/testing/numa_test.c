#include <stdio.h>
#include <numa.h>

// Assumes little endian
void printBits(size_t const size, void const * const ptr)
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
    printf("\n");
}

void main() {
	struct bitmask *bmp;
	bmp = numa_allocate_cpumask();
	numa_node_to_cpus(1, bmp);
	int i;
	printf("%ld\n", bmp->size);
	printf("%ld\n", 8*sizeof(unsigned long));
	printf("%ld\n", sizeof(unsigned long));
	for (i = 0; i < bmp->size/(8*sizeof(unsigned long)); i++)	
		printBits(8, (void*)(bmp->maskp + i));
	numa_free_cpumask(bmp);
}
