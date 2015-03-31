/*
 *  Copyright (c) 2007 - 2010 Silicon Graphics, Inc.
 *  All rights reserved.
 *
 *  Derek L. Fults <dfults@sgi.com
 */

#define _GNU_SOURCE
#include <sched.h>

#include <stdlib.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <cpuset.h>
#include <react.h>
#include "errors.h"
#include "../include/bench_example.h"

#define CPUSET_ROOT "/dev/cpuset"
#define BITS_PER_LONG (sizeof(unsigned long) * 8)

extern int ncpus;

pid_t _gettid(){
	return syscall(__NR_gettid);
}

void set_pthread_affinity(int cpu) {

	cpu_set_t cpus;
	pid_t tid = _gettid();
	
	if (cpu > (ncpus-1)) {
		printf("set_pthread_affinity: Invalid cpu %d\n",cpu);
		return;
	}
	CPU_ZERO(&cpus);
	CPU_SET(cpu, &cpus);

	if (sched_setaffinity(tid, sizeof(cpus), &cpus)) {
		printf("Could not move thread to cpu %d, continuing normally.\n",cpu);
	}


}

void do_thread_affinity(int cpu) {

        char path[50],fullpath[50];

	sprintf(path, "/rtcpus/rtcpu%d", cpu);
	sprintf (fullpath, CPUSET_ROOT "/rtcpus/rtcpu%d",cpu); 	

	if (access(fullpath, F_OK) != 0) {
		/* no cpuset, so try moving it without */
		set_pthread_affinity(cpu);
		return;
	}

	/* Move the process into the cpuset */
	if (cpuset_move(_gettid(), path) == -1) {
		printf("Could not move thread to cpu %d, continuing normally.\n",cpu);
	}
}


void set_thread_affinity(int cpu) {

        char path[50],fullpath[50];

	sprintf(path, "/rtcpus/rtcpu%d", cpu);
	sprintf (fullpath, CPUSET_ROOT "/rtcpus/rtcpu%d",cpu); 	

	if (access(fullpath, F_OK) != 0) {
		/* no cpuset, so try moving it without */
		do_thread_affinity(cpu);
		return;
	}

	/* Move the process into the cpuset */
	if (cpuset_move(_gettid(), path) == -1) {
		perror("cpuset_move");
		exit(1);
	}
}



/* Set the current proc to run on cpu <cpu>. */
void set_process_affinity(int cpu) {

	cpu_set_t cpus;

	if (cpu > (ncpus-1)) {
		printf("set_process_affinity: Invalid cpu %d\n",cpu);
        }
        CPU_ZERO(&cpus);
        CPU_SET(cpu, &cpus);

	if (sched_setaffinity(getpid(), sizeof(cpus), &cpus)) {
		perror("set_process_affinity");
	}

	return;

}
