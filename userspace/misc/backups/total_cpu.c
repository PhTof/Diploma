/* FUN FACT: None of this was useful... */

// https://man7.org/linux/man-pages/man5/proc.5.html

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h> // thread management
#include <errno.h> // error number
#include <signal.h> // pthread_kill

// https://stackoverflow.com/questions/257418/do-while-0-what-is-it-good-for
#define handle_error_en(en, msg) \
	do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

static void *thread_fn(void *arg);

long double loadavg = 0;
pthread_t thread_id;
// TODO: change this to something better
int terminate = 0;

void cpu_contention_prepare() {
	int s;

	s = pthread_create(&thread_id, NULL, &thread_fn, &loadavg);
	if (s != 0)
        	handle_error_en(s, "pthread_create");
}

void cpu_contention_cleanup() {
	int s;
	void *res;

	terminate = 1;
	s = pthread_join(thread_id, &res);
	if (s != 0)
		handle_error_en(s, "pthread_join");
}

int cpu_contention() {
	return (loadavg > 0.8);
}

static void *
thread_fn(void *arg)
{
	long double *loadavg = (long double *) arg;
	long double user, nice, system, idle;
	long double active_a, total_a;
	long double active_b, total_b;
	FILE *fp;
	char dump[50];

	while(!terminate)
	{
		fp = fopen("/proc/stat","r");
		fscanf(fp,"%*s %Lf %Lf %Lf %Lf", &user, &nice, &system, &idle);
		fclose(fp);

		active_a = user + nice + system;
		total_a = active_a + idle;

		sleep(1);

		fp = fopen("/proc/stat","r");
		fscanf(fp,"%*s %Lf %Lf %Lf %Lf", &user, &nice, &system, &idle);
		fclose(fp);

		active_b = user + nice + system;
		total_b = active_b + idle;

		*loadavg = (active_b - active_a) / (total_b - total_a);
		//printf("The current CPU utilization is : %Lf\n",loadavg);
	}
	return NULL;
}



void main() {
	cpu_contention_prepare();

	int i = 0;
	while(i++ < 5)
	{
		sleep(1);
		printf("%Lf\n", loadavg);
		printf("%d\n", cpu_contention());
	}
	
	cpu_contention_cleanup();
}
