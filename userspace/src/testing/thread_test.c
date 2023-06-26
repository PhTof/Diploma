#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include <sys/stat.h>	// fstat() -- file size
#include <fcntl.h>      // open()

long get_file_size(int fd) {
	struct stat st;
	fstat(fd, &st);
	return st.st_size;
}

void *print_message_function( void *ptr );
void *print_message_function2( void *ptr );

void main()
{
	pthread_t thread1, thread2;
	char *message1 = "Thread 1";
	char *message2 = "Thread 2";
	int  iret1, iret2;

	/* Create independent threads each of which will execute function */

	iret1 = pthread_create( &thread1, NULL, print_message_function, (void*) message1);
	iret2 = pthread_create( &thread2, NULL, print_message_function2, (void*) message2);

	/* Wait till threads are complete before main continues. Unless we  */
	/* wait we run the risk of executing an exit which will terminate   */
	/* the process and all threads before the threads have completed.   */

	pthread_join( thread1, NULL);
	pthread_join( thread2, NULL); 

	printf("Thread 1 returns: %d\n",iret1);
	printf("Thread 2 returns: %d\n",iret2);
	exit(0);
}

void *print_message_function( void *ptr )
{
	char *message;
	message = (char *) ptr;
	printf("%s \n", message);

        int fd = open("sample.txt", O_RDONLY);
	char str[50];
	
        long size = get_file_size(fd);
        posix_fadvise(fd, 0L, size, POSIX_FADV_DONTNEED);

	int remaining = size, ret;

        while (remaining > 0) {
                //ret = read(fd, buffer + size - remaining, blocksize);
                ret = read(fd, str, 50);
                remaining -= ret;
                printf("%s", str);
        }

	close(fd);
	sleep(1e7);
}

void *print_message_function2( void *ptr )
{
	char *message;
	message = (char *) ptr;
	//printf("%s \n", message);

	sleep(1e7);
}
