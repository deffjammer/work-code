/*
 *  Copyright (c) 2007 - 2010 Silicon Graphics, Inc.
 *  All rights reserved.
 *
 *  Derek L. Fults <dfults@sgi.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <asm/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sched.h>
#include <linux/netlink.h>
#include <asm/unistd.h>
#include <cpuset.h>
#include <sys/syscall.h>
#include "errors.h"
#include "../include/bench_example.h"

/* name of the device, usually in /dev */
#define MOD_NAME	    "extint0"

/* Saftey measure for experimenting to prevent runaway threads. */
#define MAX_LOOPS           500000   
#define MAX_RUN_TIME        30         /* Run for 30 seconds default */
#define MAX_INDEX           6          /* Change value to increase or decrease matrix size */
#define MAX_NUM_MSG         100        /* Can causes problems if this is too large and extint period
                                        * is too small
					*/
float max_calctime = 0;
float min_calctime = 0x7ffffffffffffff;
unsigned long total_time = MAX_RUN_TIME; /* sec */
unsigned long start;
unsigned long run_time;
int           receiving_data = 1, number_msg = 1; 
int	      data_size = 1024; 
int	      bench_run = 0;  
cpuid_t	      cpu = 0;
int	      memlock = 0;
unsigned int  rcvbufs = 300000;
unsigned int  global_count = 0;
unsigned char buf[MAX_SIZE];
int	      ex_misc_fd;
int	      ncpus;
	
struct timespec tv_start_time;

typedef struct cond_mutex {
  pthread_mutex_t    mutex;       /* Protects access to data */
  pthread_cond_t     cond;        /* Signals change to data  */
  int                matrix_data; /* Access protected by mutex */
}cond_mutex_t;

cond_mutex_t data = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0};

typedef struct  lmatx_struct {
    long lmatx [MAX_INDEX][MAX_INDEX];
    int index; 
} lmatx_t;

char * usage =
"usage: bench [-hm]  [-b # of msgs] [-t seconds] [-s buffer MB] \n"
"	-h  print usage instructions\n"
"	-m  lock memory\n"
"	-b# socket benchmark mode (# of msgs)\n"
"	-s# size of buffers (for benchmark mode)\n"
"	-p# cpu where the process will run\n"
"	-r# cpu where the receive thread will run\n"
"	-w# cpu where the worker thread will run\n"
"	-k# cpu where the kthread will run\n"
"	-t# total run time (secs)\n";

static void
usage_exit(void)
{
        fprintf(stderr, "%s", usage);
        exit(0);
}

void
print_matrix(lmatx_t print_matx)
{
  int out_index = 0;
  int in_index  = 0;

  for (out_index=0; out_index < MAX_INDEX; out_index++) {
	for (in_index=0; in_index < MAX_INDEX; in_index++) {
	     printf(" %ld,",print_matx.lmatx[out_index][in_index]);
	}
	printf("\n");
  }
  
}

lmatx_t 
multiply (lmatx_t multi_a, lmatx_t multi_b ) 
{
  int out_index = 0;
  int in_index  = 0;
  int k =0;
  lmatx_t result;

  /* initialize result matrix */
  for (out_index=0; out_index < MAX_INDEX; out_index++) {
       for (in_index=0; in_index < MAX_INDEX; in_index++) {
	 result.lmatx[out_index][in_index]= 0,0;
       }
  }
 
  for(out_index=0; out_index < MAX_INDEX; out_index++) {
      for(in_index=0; in_index < MAX_INDEX; in_index++) {
	  for(k=0; k < MAX_INDEX; k++) {
		 result.lmatx[out_index][in_index] += (multi_a.lmatx[out_index][k] * multi_b.lmatx[k][in_index]);
	  }
      }
  }
   return result;
}


float 
calc_time(struct timespec *tv_start, struct timespec *tv_end)
{
	float time;
	long sec, nsec;
	
        sec = tv_end->tv_sec - tv_start->tv_sec;
        if (sec == 0) {
                nsec = tv_end->tv_nsec - tv_start->tv_nsec;
		goto out; 
        }

        if (tv_end->tv_nsec < tv_start->tv_nsec) {
                sec--;
                nsec = (tv_end->tv_nsec+1000000000) - (tv_start->tv_nsec);
		if (nsec < 0) printf("BUG: calc_rate\n");
        } else {
                nsec = tv_end->tv_nsec - tv_start->tv_nsec;
        }

out:	time = (float) (sec + (nsec * 0.000000001));

	return time;
}


void 
output_rate_mbits(int size, int numbytes, int drop, int nmsg, float time, char *type)
{
	if ((global_count % 500) == 1) {
	  printf("%s: %d pkts of %d bytes (drop %d) in %f seconds (%.2f Mbits/s) (%.2f pkts/s)\n",
		 type, nmsg, size, drop, time, (numbytes*8/time) / 1024000, nmsg/time);
	}
}

/* routine to do work of receive thread */
void *
worker_routine( void *arg )
{
  long cpu = (long)arg;
  int status;
  int inside = 0; 
  int outside = 0;
  int matrix_full;
  float  matx_calctime = 0;
  struct timespec tv_matx_stime, tv_matx_etime;

  if (cpu >= 0) {
	/* Could use cpu_sysrt_add and cpu_sysrt_runon(cpu) */
	set_thread_affinity(cpu);
  }

  tv_matx_stime.tv_sec  = 0;
  tv_matx_stime.tv_nsec = 0;

  tv_matx_etime.tv_sec  = 0;
  tv_matx_etime.tv_nsec = 0;

  /*
   * matx_multiplier[0] ==  1st matrix multiplier (A)
   * matx_multiplier[1] ==  2nd matrix multiplier (B)
   * matx_multiplier[2] ==  result (A) * (B)
   * 
   */
  lmatx_t  matx_multiplier[3];
  int stx = 0; /* 0-2 to control struct index */

  /*receive not done and matrix not full */
 KEEP_LOOPING:

  do {
      if(!bench_run) {
          matrix_full=0;
	  status = pthread_mutex_lock(&data.mutex);
	  if (status !=0) 
		err_abort (status, "Lock mutex");
	  

	  status = pthread_cond_wait(&data.cond, &data.mutex);
	  if (status !=0) 
		  err_abort (status, "Cond Wait");
	  
	  matx_multiplier[stx].lmatx[outside][inside] = data.matrix_data;	  
	  inside++;

	  status = pthread_cond_signal (&data.cond);
	  if (status !=0) 
		err_abort (status, "Signal condition");
	  

	  status = pthread_mutex_unlock(&data.mutex);
	  if (status !=0) 
		err_abort (status, "Unlock mutex");
	  
	  /* 
	   * Time how long it takes to do the multiplication of the
           * the matrices.  Change MAX_INDEX to increase the size
           * of the matrices.  The multiply function could be done
           * via a FPGA then see how much faster it does the multiplication.
           */
	    if (inside == MAX_INDEX ) {
              inside = 0;
	      outside++;
	      if (outside == MAX_INDEX) { 
		matrix_full = 1;
		if (matx_multiplier[stx].index ==  0) {
			/* 
			 * Don't print first matrix if time has expired.
			 * There is no matrix to multiply it to
			 */
			if (run_time < total_time ) {
			  print_matrix(matx_multiplier[stx]);
			  printf("      X \n");
			}
			stx++;
			matx_multiplier[stx].index = 1;
		} else {
			print_matrix(matx_multiplier[stx]);
			/* Use Matrix Data */
			printf("===================================== \n");

			clock_gettime(CLOCK_REALTIME,&tv_matx_stime);

			matx_multiplier[2] =  multiply(matx_multiplier[0],matx_multiplier[1]);
			matx_multiplier[stx].index = 0;
			stx--;

			clock_gettime(CLOCK_REALTIME,&tv_matx_etime);

			/* calculation time is in nsec */
			matx_calctime = calc_time(&tv_matx_stime, &tv_matx_etime); 

			if (matx_calctime > max_calctime) {
			  max_calctime = matx_calctime;
			}
			if (matx_calctime < min_calctime) {
			  min_calctime = matx_calctime;
			}
			/* print result matrix */
			print_matrix(matx_multiplier[2]);
			printf("Calculation time for matrix multiplication %f seconds.\n",matx_calctime );
			printf("//////////////////////////////////////////////////////////\n");
		  }
		  outside = 0;
	      }
	    }
      } /* bench_run */ 
	  if (global_count > MAX_LOOPS)
		goto done;

  }  while ( run_time < total_time);
 
  if(!bench_run) {
    if (!matrix_full) 
	goto KEEP_LOOPING;
  }

 done:
  printf("sends = %d, with %d msgs per send in %ld seconds.\n",global_count, number_msg, total_time);  
  printf("Worker routine..done recieving\n");
  receiving_data = 0;
  pthread_exit(NULL);
}


/*
 * kernel_netlink_send - receive netlink messages from kernel space
 * @fd: socket descriptor
 * @peer: socket structure which contains information about the peer
 * @msg: benchmark message
 * @debug_level: print debug if needed
 */

int
netlink_kernel_to_user(int fd, struct sockaddr_nl *peer, struct benchmsg *msg)
{	
	struct timeval tv;
	struct timespec tv_bench1, tv_bench2, tv_current;
	socklen_t addrlen = sizeof(*peer);
	int status, bytes, numbytes = 0, drop = 0, i;
	fd_set read_fds,netlink;
	float time = 0;
	int nmsg = msg->number;
	int size = msg->size;
	struct nlmsghdr *nlh;

        FD_ZERO(&netlink);
	FD_SET(fd, &netlink);

	/* hey kernel, start sending me packets! */
	if (libnetlink_send_netlink_message(fd, msg, sizeof(*msg)) < 0) {
		perror("send_netlink:");
		exit(-1);
	}

	if(bench_run) { 
	  /* start the timer... */
	  clock_gettime(CLOCK_REALTIME,&tv_bench1); 
	}

	for (i=0; i< nmsg; i++) {

		tv.tv_sec = 1;
		tv.tv_usec = 0;
		read_fds = netlink;

		/* we have a winner */
		if (select(fd+1, &read_fds, NULL, NULL, &tv) < 0) {
			printf("error en select\n");
			return -1;
		}

                if (!FD_ISSET(fd, &read_fds)) {
			/* 1 packet drop means 1 second */
			printf("packet drop! (%d)\n", i);
			drop++; 
			continue;
		}

		/* receiving packets! */
		/* we receive the packet + nlmsghdr which is 16 bytes */
		if ((bytes = recvfrom(fd, buf, MAX_SIZE-1, 0,(struct sockaddr *)&peer, &addrlen)) <0) {
			perror("recvfrom");
			return -1;
		}
		nlh = (struct nlmsghdr *) buf;
		bytes -= sizeof(struct nlmsghdr);
		numbytes += bytes;

		/* 
		 * Calculate condition to signal worker that shared data is ready  
		 * And signal worker pthread 
		 */

		clock_gettime(CLOCK_REALTIME, &tv_current);
  
		/* elapsed time so far is in sec */
		run_time =  calc_time(&tv_start_time, &tv_current);
		if (!bench_run) {
		  status = pthread_mutex_lock(&data.mutex);
		  if (status !=0) 
			  err_abort (status, "Lock mutex");

		  /* 
		   * Get data from driver and signal
		   * woker thread to manipulate it  
		   */
		  data.matrix_data =atoi(NLMSG_DATA(nlh));   

		  status = pthread_cond_signal (&data.cond);
		  if (status !=0) 
			err_abort (status, "Signal condition");
		
		
		  status = pthread_mutex_unlock(&data.mutex);
		  if (status !=0) 
			err_abort (status, "Unlock mutex");
		}
	}

	if(bench_run) {
	  /* stopping timer */
	  clock_gettime(CLOCK_REALTIME,&tv_bench2); 
	  time = calc_time(&tv_bench1, &tv_bench2);
	  /* Only print out every 500 times */
	  output_rate_mbits(size, numbytes, drop, nmsg, time, "kernel -> user"); 
	}  
	return 0;
}


/* 
 * This routine receives the data from the driver 
 * and signals the worker thread w/ the data
 */
void * 
netlink_receive(void *arg)
{
	long cpu = (long)arg;
	int sockfd, addrlen;
	struct sockaddr_nl local, peer;
	struct benchmsg msg;
 
	if (cpu >= 0){
		/* Could use cpu_sysrt_add and cpu_sysrt_runon(cpu) */
		set_thread_affinity(cpu);
	}
	/* 
	 * Give worker a chance to get started. 
	 * If extint period is set too fast,
	 * this thread will be receiving and worker
	 * doesn't get a chance to start
	 */
	printf("Waiting...\n");
	sleep(1);

	sockfd = libnetlink_create_socket(NETLINK_BENCHMARK);
	if (sockfd < 0) {
		perror("socket:");
		exit(0);
	}

	if (libnetlink_bind(sockfd, &local, &peer) < 0) {
		printf("cannot bind\n");
		exit(-1);
	}

	addrlen = sizeof(peer);

	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (void *) &rcvbufs, sizeof(rcvbufs)) < 0) {
		perror("setsockopt");
		exit(-1);
	}
	
	msg.type = 0;
	msg.size = data_size;
	msg.number = number_msg;
	msg.timestamp_sec = 0;
	msg.timestamp_usec = 0;

	do{
	   if (global_count > MAX_LOOPS)
		/* This will allow worker to exit safely */
		run_time = MAX_RUN_TIME;

	  global_count++;
	  netlink_kernel_to_user(sockfd, &peer, &msg);	  
	} while(receiving_data); 

	printf("netlink_receive thread exit\n");
	pthread_exit(NULL);
 
}

int
main(int argc, char **argv)
{
	int extint_fd, status, thread_policy, max_priority, min_priority, opt;
	int bind_thread = 0;
	pthread_t pt_receive, pt_worker;
	pthread_attr_t thread_attr;
	struct sched_param thread_param;
	long	worker_cpu = -1;
	long	receive_cpu = -1;
	int	process_cpu = -1;

        static char* opt_string = "mb:t:s:k:p:w:r:h";
        while ((opt = getopt(argc, argv, opt_string)) >= 0) {
                switch (opt){
                case 'm':
			memlock = 1;
                        break;
                case 'b':
			bench_run = 1;
			number_msg = atoi(optarg);
			if (number_msg > MAX_NUM_MSG)
				number_msg = MAX_NUM_MSG;
                        break;
                case 't':
                        total_time = atoi(optarg);
			if (total_time > MAX_RUN_TIME)
				total_time = MAX_RUN_TIME;  
                        break;
		case 's':
			data_size = atoi(optarg);
                        break;
		case 'k':
			bind_thread = 1;
			cpu = atoi(optarg);
                        break;
		case 'p':
			process_cpu = atoi(optarg);
                        break;
		case 'w':
			worker_cpu = atoi(optarg);
                        break;
		case 'r':
			receive_cpu = atoi(optarg);
                        break;
                case 'h':
                default:
                        usage_exit();
                }
        }
	ncpus = sysconf(_SC_NPROCESSORS_ONLN);

	if (process_cpu >=0){
		/* Could use cpu_sysrt_add and cpu_sysrt_runon(cpu) */
		set_process_affinity(process_cpu);
	}

	/* Get the file descriptor for extint, pass to driver */
	if ((extint_fd = open("/dev/"MOD_NAME,O_RDONLY)) == -1) {
	  perror("open /dev/extint0");
	  exit(1);
	}

	/* Open the module device */
	if ((ex_misc_fd = open("/dev/"EX_NAME,O_RDWR)) == -1) {
	  perror("open /dev/ex_misc");
	  exit(1);
	}
	/* Do not allow paging if selected */
        if (memlock &&
            (mlockall(MCL_CURRENT | MCL_FUTURE) < 0)) {
                perror("memlock");
                exit(1);
        }
	/* Register the external interrupt */
	if (ioctl(ex_misc_fd, REG_EI, extint_fd) < 0) {
		close(extint_fd);
		close(ex_misc_fd);
		perror("ioctl /dev/ex_misc");
		exit(1);
	}
	/* Bind kernel thread to a cpu if passed in */
	if (bind_thread) {
		if (ioctl(ex_misc_fd, KTHREAD_BIND, cpu) < 0)
			perror("ioctl /dev/ex_misc");
	}

	clock_gettime(CLOCK_REALTIME,&tv_start_time);

	/* Get info about policies and create Receive thread */
	status = pthread_attr_init(&thread_attr);
	status = pthread_attr_getschedpolicy (&thread_attr, &thread_policy);
	if(status != 0)
		err_abort (status, "Get policy");
	
	status = pthread_attr_getschedparam (&thread_attr, &thread_param);
	if(status != 0) 
		err_abort (status, "Get sched param");
	
	printf ("Default policy is %s, priority is %d\n", 
		(thread_policy == SCHED_FIFO ? "FIFO":
		 (thread_policy == SCHED_RR ? "RR" :
		  (thread_policy == SCHED_OTHER ? "OTHER":
		 "unknown"))), thread_param.sched_priority);
	
	status = pthread_attr_setschedpolicy (&thread_attr, SCHED_FIFO);
	if(status != 0)
		err_abort (status, "Unable to set SCHED_FIFO policy.\n");
	

	min_priority = sched_get_priority_min (SCHED_FIFO);
	if (min_priority == -1) 
		errno_abort ("Get SCHED_FIFO min priority");
	

	max_priority = sched_get_priority_max (SCHED_FIFO);
	if (max_priority == -1)
		errno_abort ("Get SCHED_FIFO max priority");
	
	thread_param.sched_priority = (min_priority + max_priority)/3;

	printf("SCHED_FIFO priority range is %d to %d: using %d\n",
	       min_priority,
	       max_priority,
	       thread_param.sched_priority);

	status = pthread_attr_setschedparam (&thread_attr, &thread_param);
	if(status != 0) 
		err_abort (status, "Set params");
	
	printf ("Creating thread at FIFO/%d\n", thread_param.sched_priority);

	status = pthread_attr_setinheritsched (&thread_attr, PTHREAD_EXPLICIT_SCHED);
	if(status != 0)
		err_abort (status, "Set inherit");
	
	status = pthread_create(&pt_receive, &thread_attr, netlink_receive, (void *)receive_cpu);
	if (status != 0)
		err_abort(status,"Create thread");
	
	/* Create worker thread */
	status = pthread_attr_init(&thread_attr);
	status = pthread_attr_setschedpolicy (&thread_attr, SCHED_RR);
	if(status != 0)
		err_abort (status, "Unable to set SCHED_RR policy.\n");
	
	thread_param.sched_priority = min_priority;

	status = pthread_create(&pt_worker, &thread_attr, worker_routine, (void *)worker_cpu);
	if (status != 0)
		err_abort(status,"Create thread");

	status = pthread_join(pt_worker,NULL);
	if (status != 0)
		err_abort(status,"Join thread send");

	pthread_cancel(pt_receive);
	if (status != 0)
		err_abort(status,"Cancel thread");

	status = pthread_join(pt_receive,0);
	if (status != 0)
		err_abort(status,"Join thread receive");

	/* Unregister the external interrupt */
	if (ioctl(ex_misc_fd, UNREG_EI, 0) < 0)
		perror("ioctl /dev/ex_misc");

	close(extint_fd);
	close(ex_misc_fd);
	
	return 0;
}
