#include <stdbool.h>
#include <stdio.h> // NULL, printf
#include <unistd.h>
#include <stdlib.h>
#include <time.h> // timers
#include <pwd.h> // get uid from username
#include <sys/types.h> // DIR type
#include <dirent.h> // directories management
#include <ctype.h> // isdigit()

#define FILENAMELENGTH		50
#define LINELENGTH		2000
typedef unsigned long long u64;

static void profile_forever(char *usr, int interval) {
	struct timespec start, finish;
	struct passwd *user_info;
	long uid, wanted_uid;
	struct dirent* ent;
	DIR* proc, *task;
	FILE* f;
	long tgid;
	char path[64];


	/* This maybe approximates what ps does. I think it uses fprocopen?
	   This has been lazily verified by using strace on "ps -T" */
	
	// Get the given user's ID
	user_info = getpwnam(usr);
	wanted_uid = user_info->pw_uid;


	while(true) {
		clock_gettime(CLOCK_REALTIME, &start);
		proc = opendir("/proc");
		while (ent = readdir(proc)) {
			// Ignore everything that is not a task
			if(!isdigit(*ent->d_name))
				continue;

			tgid = strtol(ent->d_name, NULL, 10);
			snprintf(path, 64, "/proc/%ld/loginuid", tgid);
			f = fopen(path, "r");
			if(!f) continue;
			fscanf(f, "%ld", &uid);
			fclose(f);
		
			if(uid != wanted_uid) continue;
	
			snprintf(path, 64, "/proc/%ld/task/", tgid);
			task = opendir(path);
			
			while(ent = readdir(task)) {
				if(!isdigit(*ent->d_name))
					continue;
				long int tid = strtol(ent->d_name, NULL, 10);
				printf("TID = %ld\n", tid);
			}
			closedir(task);

		}
		closedir(proc);
		clock_gettime(CLOCK_REALTIME, &finish);
		printf("%.5f secs\n\n",
			((double)finish.tv_nsec * 1e-09 + (double) finish.tv_sec) - ((double)start.tv_nsec * 1e-09 + (double) start.tv_sec));
		sleep(1);
	}
}

int main(int argc, char **argv)
{
	char *usr = NULL;
	int c, interval = 1;

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
	profile_forever(usr, interval);
}
