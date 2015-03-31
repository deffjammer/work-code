/*
 *  Copyright (c) 2007 - 2010 Silicon Graphics, Inc.
 *  All rights reserved.
 *
 *  Derek L. Fults <dfults@sgi.com
 */
#ifndef __KERNEL__
#include <sys/socket.h>
#endif
#include <linux/netlink.h>

#define EX_NAME "ex_misc"
#define REG_EI              1
#define UNREG_EI            2
#define KTHREAD_BIND        3
#define NETLINK_TEST        17
#define BENCH_GROUP         1
#define NETLINK_BENCHMARK   21
#define BENCHMARK_NL_EVENT  141
#define MAX_SIZE            65335*8

void set_thread_affinity(int);
void set_process_affinity(int);

int libnetlink_create_socket(int);
int libnetlink_bind(int fd, struct sockaddr_nl *, struct sockaddr_nl *);
int libnetlink_send_netlink_message(int, void *, int);

struct benchmsg {
	int type;
	int size;
	int number;
	long timestamp_sec;
	long timestamp_usec;
};


typedef struct parm_s { 
	unsigned long cmd;
        unsigned long arg1;
        unsigned long random_dat;
	unsigned long ed;
	unsigned long ec;
} parm_t;

typedef signed short    cpuid_t;        /* cpuid */
