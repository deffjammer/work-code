/*
 * =====================================================================================
 *
 *       Filename:  mq.c
 *
 *    Description:  posix message test case
 *
 *    compile server:
 *        gcc -DMQ_SERVER -g -O2 -o mq_server mq.c -lrt
 *
 *    compile client:
 *        gcc -DMQ_CLIENT -g -O2 -o mq_client mq.c -lrt
 *
 * =====================================================================================
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <linux/unistd.h>
#include <time.h>
#include <malloc.h>
#include <mqueue.h>
#define _GNU_SOURCE
#include <sched.h>

#define MQ_SERVER_SIZE             (64)
#define MQ_SERVER_PATH             "/server_mq.1234"
#define MQ_SERVER_RES_PATH         "/server_mq_res.1234"

typedef struct mq_server_s
{
    unsigned long long int send_stamp;
    unsigned long long int rcvd_stamp;
    unsigned int cmd;
    unsigned int unused0;
    unsigned long long int unused1;
} mq_server_t;

typedef enum mq_cmd_s
{
    MQ_CMD_NOTHING = 0,
    MQ_CMD_GET_TS,
    MQ_CMD_EXIT
} mq_cmd_t;


static unsigned long long int get_current_timestamp(void)
{
    struct timespec ts;
	
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return(((unsigned long long)ts.tv_sec) * 1000000000ULL + ((unsigned long long)ts.tv_nsec));
}

static int priority = 10; /* for FIFO scheduling class */


#if defined(MQ_SERVER)
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  mq_server 
 *  Description:  creates 2 posix message queue, waits for msgs on one of them
 *                executes what is received, sends back the result via the other mq
 * =====================================================================================
 */
int main(int argc, char **argv)
{
    mqd_t mq_fd, mq_res_fd;
    struct mq_attr mq_attributes;
    mq_server_t mq_msg;
    int len;
    struct sched_param schedp;
    unsigned int mq_msg_prio = 1;
    /* int policy = SCHED_FIFO; */

    if (geteuid())
    {
	fprintf(stderr, "you should be root\n");
	exit(-1);
    }

    /* the server has higher prio */
    priority++;

    /* create 2 posix message queues */
    mq_attributes.mq_flags = 0;
    mq_attributes.mq_maxmsg = MQ_SERVER_SIZE;
    mq_attributes.mq_msgsize = sizeof(mq_server_t);
    mq_attributes.mq_curmsgs = 0;

    mq_fd = mq_open(MQ_SERVER_PATH, O_RDWR | O_CREAT,  S_IWUSR | S_IRUSR, &mq_attributes);
    mq_res_fd = mq_open(MQ_SERVER_RES_PATH, O_RDWR | O_CREAT,  S_IWUSR | S_IRUSR, &mq_attributes);

    memset(&schedp, 0, sizeof(schedp));
    /*
     * schedp.sched_priority = priority;
     * sched_setscheduler(0, policy, &schedp);	
     */
    
    fprintf(stderr, "mq_server started (pid: %d prio: %d)\n", getpid(), schedp.sched_priority);
    
    while(1)
    {
	len = mq_receive(mq_fd, (char *) &mq_msg, sizeof(mq_msg), &mq_msg_prio);
	    
	if(len == sizeof(mq_msg))
	{
	    switch(mq_msg.cmd)
	    {
		case MQ_CMD_NOTHING:
		    break;
		case MQ_CMD_GET_TS:
		    mq_msg.rcvd_stamp = get_current_timestamp();
		    /* send with received  prio */
		    mq_send(mq_res_fd, (char *) &mq_msg, sizeof(mq_msg), mq_msg_prio);
		    /* printf("got ts req send: 0x%016llx now: 0x%016llx \n", mq_msg.send_stamp, now ); */
		    break;
		case MQ_CMD_EXIT:
		    goto OUT;
		default:
		    break;
	    }
	}
    }

OUT:
    /* let the client finish */
    sleep(1);

    fprintf(stderr, "mq_server exiting\n");

    mq_close(mq_fd);
    mq_unlink(MQ_SERVER_PATH);
    mq_close(mq_res_fd);
    mq_unlink(MQ_SERVER_RES_PATH);

    return 0;
}


#elif defined(MQ_CLIENT)

static int timeout = 1000000; /* usec;  1 sec as default */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  mq_client
 *  Description:  opens 2 posix message queue, send out jobs on one mq
 *                collects the result via the other mq and prints the result
 * =====================================================================================
 */
int main(int argc, char **argv)
{
    int i, len;
    mqd_t mq_fd, mq_res_fd;
    struct mq_attr mq_attributes;
    unsigned int mq_msg_prio = 1;
    mq_server_t mq_msg;
    struct sched_param schedp;
    /* int policy = SCHED_FIFO; */
    struct timespec ts_wait;

    if (geteuid())
    {
	fprintf(stderr, "you should be root\n");
	exit(-1);
    }

    ts_wait.tv_sec  =  timeout / 1000000;
    ts_wait.tv_nsec = (timeout % 1000000) * 1000;

    fprintf(stderr, "wait time: %ld seconds %ld nsec\n",  ts_wait.tv_sec,  ts_wait.tv_nsec);

    mq_fd = mq_open(MQ_SERVER_PATH, O_RDWR,  S_IWUSR | S_IRUSR, &mq_attributes);
    mq_res_fd = mq_open(MQ_SERVER_RES_PATH, O_RDWR,  S_IWUSR | S_IRUSR, &mq_attributes);
    
    memset(&schedp, 0, sizeof(schedp));
    /*
     * schedp.sched_priority = priority;
     * sched_setscheduler(0, policy, &schedp);	
     */

    fprintf(stderr, "mq_test started (pid: %d prio: %d)\n", getpid(), schedp.sched_priority);

    for(i = 0; i < 3; i++)
    {
	nanosleep(&ts_wait, NULL);
	/* sleep(1); */

	/* query ts service from server */
	mq_msg.cmd = MQ_CMD_GET_TS;
	mq_msg.send_stamp = get_current_timestamp();
	mq_send(mq_fd, (char *) &mq_msg, sizeof(mq_msg), mq_msg_prio);

	/* got response */
	len = mq_receive(mq_res_fd, (char *) &mq_msg, sizeof(mq_msg), &mq_msg_prio);

	if (len == sizeof(mq_msg))
	{
	    if(mq_msg.cmd == MQ_CMD_GET_TS)
	    {
		fprintf(stderr, "measured mq latency %lld nsec (i: %d)\n",
			mq_msg.rcvd_stamp - mq_msg.send_stamp, i);
	    }
	}
    } /* while (true) */

    fprintf(stderr, "measure loop exited\n");

    /* shut down server */
    mq_msg.cmd = MQ_CMD_EXIT;
    mq_send(mq_fd, (char *) &mq_msg, sizeof(mq_msg), mq_msg_prio);

    mq_close(mq_res_fd);
    mq_close(mq_fd);

    return 0;
}
#else
#error "MQ_SERVER or MQ_CLIENT should be defined"
#endif  /* MQ_SERVER */
