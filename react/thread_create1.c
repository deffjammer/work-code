#define LOOPS_TO_RUN    50000000


#define FIRST_RT_CPU	19
#define RT_CPUS 		21		/* FIRST_RT_CPU -> (RT_CPUS + FIRST_RT_CPU - 1) */

#define OS_CPUSET_DIR		"/boot"
#define RT_CPUSET_ROOT_DIR	"/rtcpus"
#define RT_CPUSET_DIR		"/rtcpu"


#if 1
#define SHIELD  1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <sched.h>
#include <sys/mman.h>
#include <react.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <sys/resource.h>
#include <math.h>

#define _GNU_SOURCE
#include <sched.h>

void *rt_main (void *);
int MaxPriority;

typedef struct
{
	int noSp;
} ST_T_ARG;

pthread_t tid[256+1];
int StartLoop = 0;


pid_t _gettid(){
  return syscall(__NR_gettid);
}


int main (int argc, char **argv)
{
	char HostName[256];
	int	i, j, err;
	ST_T_ARG arg;
	struct sched_param sched_rtkernel;

	printf ("main PID: %d\n", getpid ());
	printf ("Real-time cpus: %d -> %d\n\n", FIRST_RT_CPU, RT_CPUS + FIRST_RT_CPU - 1);

#ifdef SHIELD	
	printf ("main: using sgi-shield");
	printf ("\nmain: start interrupts\n\n");
	fflush (stdout);

	for (i = FIRST_RT_CPU; i <= (RT_CPUS + FIRST_RT_CPU - 1 ); i++)
		cpu_shield (SHIELD_START_INTR, i);
#endif

	mlockall (MCL_CURRENT | MCL_FUTURE);

	MaxPriority = sched_get_priority_max (SCHED_FIFO) - 1;
	sched_rtkernel.sched_priority = MaxPriority;

	if (sched_setscheduler (0, SCHED_FIFO, &sched_rtkernel) < 0)
		perror (" sched_setscheduler SCHED_FIFO");

	/* real-time cpus = FIRST_RT_CPU a (RT_CPUS + FIRST_RT_CPU - 1) */

	for (i = FIRST_RT_CPU; i <= (RT_CPUS + FIRST_RT_CPU - 1 ); i++)
	{
		usleep (100000);
		arg.noSp = i;

		if (pthread_create (&tid[i], NULL, rt_main, (void *) &arg))
		{
			perror (" pthread_create rt_main");
			exit (EXIT_FAILURE);
		}
	}

	fflush (stdout);
	sleep (1);

#ifdef SHIELD

	for (i = FIRST_RT_CPU; i <= (RT_CPUS + FIRST_RT_CPU - 1 ); i++)
	{
		err = cpu_shield (SHIELD_STOP_INTR, i);

		if (err)
		{
			printf ("\nmain: error stop interrupts: CPU %d:", i);
			perror (" cpu_shield");
			fflush (stdout);
		}
	}
	
#endif

	sleep (1);
	StartLoop = 1;

	printf ("\nmain: loops to run = %d", LOOPS_TO_RUN);
	fflush (stdout);

	printf ("\nmain: pthread_join");
	fflush (stdout);

	for (i = FIRST_RT_CPU; i <= (RT_CPUS + FIRST_RT_CPU - 1 ); i++)
		pthread_join (tid[i], NULL);

#ifdef SHIELD	
	printf ("\nmain: start interrupts");
	fflush (stdout);

	for (i = FIRST_RT_CPU; i <= (RT_CPUS + FIRST_RT_CPU - 1 ); i++)
		cpu_shield (SHIELD_START_INTR, i);
#endif

	printf ("\nmain: exit \n");
	fflush (stdout);
	return (0);
}


/*==================================================================

rt_main: main program for slave real-time processors

====================================================================*/

void *rt_main(void *thread_arg)
{
	volatile int step = 0;
	int	err, cpuNum;
	struct sched_param sched_master;
	volatile int i, j, k;
	char rtCpuSet[64];

	cpuNum = ((ST_T_ARG *) thread_arg)->noSp;
	usleep (10000);

	printf ("rt_main CPU %d TID: %d\n", cpuNum, _gettid ());
	fflush (stdout);

	/* determiner le cpuset correspondant au processeur */

	sprintf (rtCpuSet, "%s%s%d", RT_CPUSET_ROOT_DIR, RT_CPUSET_DIR, cpuNum);

	/* forcer l'execution du thread courant sur le processeur */

	err = cpuset_move (syscall (__NR_gettid), rtCpuSet);

	if (err)
	{
		printf ("\nrt_main: error: CPU %d:", cpuNum);
		fflush (stdout);
		perror (" cpuset_move");
		fflush (stdout);
	}

	printf ("MustRun: CPU %d: rtCpuSet = %s%s", cpuNum,
			RT_CPUSET_ROOT_DIR, rtCpuSet);
	fflush (stdout);

	sched_master.sched_priority = MaxPriority;

	if (sched_setscheduler(0, SCHED_FIFO, &sched_master) < 0)
		perror(" rt_main: error: sched_setscheduler");

	printf ("----------------------------------------------");
	printf ("\nInit CPU %d", cpuNum);
	printf ("\n----------------------------------------------\n\n");
	fflush (stdout);

	while (! StartLoop);

	for (i = 0; i < LOOPS_TO_RUN; i++)
	{
		j = i * 2.0;
		for (k = 0; k < (100); ++k);
	}
			
	for (i = 0; i < (1000000 * cpuNum); ++i);

	printf ("\nrt_main CPU %d: done", cpuNum);
	fflush (stdout);

	pthread_exit (NULL);
}

