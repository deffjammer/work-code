/*
 *  Copyright (c) 2007 - 2010 Silicon Graphics, Inc.
 *  All rights reserved.
 *
 *  Derek L. Fults <dfults@sgi.com
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/cdev.h>
#include <linux/cpumask.h>
#include <linux/miscdevice.h>
#include <linux/version.h>
#include <asm/atomic.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif
#include <asm/delay.h>
#include <asm/uaccess.h>

#include "/usr/include/sn/extint.h"
#include "../include/bench_example.h"
#include <linux/skbuff.h>
#include <linux/capability.h>
#include <linux/socket.h>
#include <linux/ip.h>
#include <net/scm.h>
#include <linux/netlink.h>
#include <net/sock.h>
#include <linux/random.h>

MODULE_LICENSE("GPL");
MODULE_INFO(supported, "external");

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
typedef unsigned char *sk_buff_data_t;
#endif

unsigned char buf[MAX_SIZE];
int extint_run_count;
int thread_run_count;
int sending_errors;

unsigned int int_buf;

static struct nlmsghdr *nlh;
static	struct benchmsg msg;
static int pid;

/* We might store more information here if we were measuring response
 * times at different points.
 */
static struct sock *benchsk;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)
static DECLARE_MUTEX(benchmark_sem);
#else
static DEFINE_SEMAPHORE(benchmark_sem);
#endif
static struct task_struct *ex_task;

int ex_ioctl(struct inode *, struct file *, unsigned int , unsigned long);
void extint_run(void *);
static int ex_sock_thread(void *nothing);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
static void benchmark_rcv_sk(struct sock *sk, int len);
#else
static void benchmark_rcv_sk(struct sk_buff *);
#endif 

static struct file_operations ex_fops = {
	owner: THIS_MODULE,
	.ioctl = ex_ioctl,
};

/* The register structure for /dev/ex_misc */
static struct miscdevice ex_miscdev = {
	MISC_DYNAMIC_MINOR,
	EX_NAME,
	&ex_fops,
};
struct extint_device * store_ed;	
struct extint_callout * store_ec;

int 
extint_register(parm_t * ex_args, int fd) {

	struct file * filp;
        struct extint_device * ed;
        struct extint_callout * ec;

        if (!(ec = kmalloc(sizeof(struct extint_callout), GFP_KERNEL))) {
                return ENOMEM;
        }

        /* Get a reference to the EI file */
        filp = fget(fd);
        if (!filp) {
                kfree(ec);
                return EINVAL;
        }

        /* Get a reference to its EI */
        ed = file_to_extint_device(filp);
        fput(filp);
        if (ed==(void *)-EINVAL) {
                kfree(ec);
                return EINVAL;
        }

        /* Request the EI */
        ec->owner = THIS_MODULE;
        ec->function = extint_run;
        ec->data = ex_args;
        if (extint_callout_register(ed, ec)) {
                kfree(ec);
                return EINVAL;
        }
        /* Store a link to the EI for later deregistration */
        store_ed = ed;
        store_ec = ec;

        return 0;
}

int
ex_ioctl(struct inode *inode, struct file *file,	
		 unsigned int ioctl_num, unsigned long ioctl_param)
{
	int error = 0;
	parm_t *ex_args = NULL; 

	switch (ioctl_num) {
	case REG_EI:
	  	extint_run_count=0;
		thread_run_count=0;
		sending_errors=0;

		if (ex_task) {
		  /* 
		   * Application died before it had a chance to clean up.
		   * Handle clean-up, then it can be run again 
		   *
		   */  
			printk(KERN_ERR "bench_exintd exited abnormally\n");
			error = -EAGAIN;
			goto out;
		}

		ex_task = kthread_create(ex_sock_thread, NULL, "bench_exintd");
		if (IS_ERR(ex_task)) {
			printk(KERN_ERR "bench_exint: Failed to start exintd\n");
			return PTR_ERR(ex_task);
		}

		benchsk = (struct sock *) 
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
		  netlink_kernel_create(NETLINK_BENCHMARK, 0, 
					benchmark_rcv_sk, THIS_MODULE);
#else
		  netlink_kernel_create(&init_net, NETLINK_BENCHMARK, 0, 
					(void *) benchmark_rcv_sk, 
					NULL, THIS_MODULE);
#endif

		if (!benchsk) {
			printk("netlinkbech: Cannot create netlink socket\n");
			error = -ENOMEM;
			goto out;
		}

		ex_args = kmalloc(sizeof(parm_t ), GFP_KERNEL);
		if (ex_args == NULL) {
			error = -ENOMEM;
			goto out;
		}
		ex_args->cmd = 0;
		ex_args->arg1 = (unsigned long)file;
		if ((error = extint_register(ex_args, ioctl_param))) {
			printk(KERN_INFO "error registering bench_extint\n");
			return error;
		}
		break;
	case UNREG_EI:
out:
		printk(KERN_INFO "bench_extint ran %d, thread ran %d dropped msgs %d\n",
		       extint_run_count, thread_run_count, sending_errors);
		
		if (ex_task) 
			kthread_stop(ex_task);
		
		ex_task = NULL;

		if (benchsk) 
			sock_release(benchsk->sk_socket);

		if ((store_ed != NULL) && (store_ed != NULL)){
			printk(KERN_INFO "ioctl unregister bench_extint\n");
			extint_callout_unregister(store_ed, store_ec);
		}
		if (ex_args) 
			kfree(ex_args);
		
		break;
	case KTHREAD_BIND:
		kthread_bind(ex_task, ioctl_param);
		break;
	}
	return error;
}

static int
send_netlink_unicast(int size, void *payload, int pid)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	char *data;
	sk_buff_data_t old_tail;
	int *i;

	skb = alloc_skb(NLMSG_SPACE(size), GFP_ATOMIC);
	
	/* possible cache effect: use skb as random number */
	i = (int *)skb;

	old_tail = skb->tail;

	nlh = NLMSG_PUT(skb, 0, 0, BENCHMARK_NL_EVENT, size - sizeof(*nlh));

	data = NLMSG_DATA(nlh);

	if (payload != NULL) 
		memcpy(data, payload, size);
	else
		memset(data, *i, size);

	nlh->nlmsg_len = skb->tail - old_tail;

	return(netlink_unicast(benchsk, skb, pid, 0));

nlmsg_failure:
	if (skb)
		kfree(skb);
	printk("Error sending netlink packet: too exhausted to deal?\n");
	return(-1);
} 

static inline void
benchmark_rcv_skb(struct sk_buff *skb)
{
	nlh = (struct nlmsghdr *) skb->data;
	pid = nlh->nlmsg_pid;
	memcpy(&msg, NLMSG_DATA(nlh), sizeof(struct benchmsg));

}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
static void
benchmark_rcv_sk(struct sock *sk, int len)
{

	do {
	 	struct sk_buff *skb;

		down_trylock(&benchmark_sem);

		while ((skb = skb_dequeue(&sk->sk_receive_queue)) != NULL) {
			benchmark_rcv_skb(skb);
			kfree_skb(skb);
	        }

                up(&benchmark_sem);

	} while (benchsk && benchsk->sk_receive_queue.qlen);
	
}

#else
static void
benchmark_rcv_sk(struct sk_buff *skb)
{

	do {
		down_trylock(&benchmark_sem);
		benchmark_rcv_skb(skb);
		up(&benchmark_sem);

	} while (benchsk && benchsk->sk_receive_queue.qlen);
	
}
#endif
static int
ex_sock_thread(void *nothing)
{
  	int i;

	while (!kthread_should_stop()){
		__set_current_state(TASK_RUNNING);
		thread_run_count++;
		for (i=0;i< msg.number;i++) {
			get_random_bytes(&int_buf, sizeof(int_buf));
			int_buf %= 10;
			sprintf(buf, "%u", int_buf);
			if (send_netlink_unicast(msg.size, buf, pid) < 0) {
				sending_errors++;
			}
		}
		__set_current_state(TASK_INTERRUPTIBLE);
		schedule();
	}

	return 0;
}
int 
ex_init(void)
{
	int res;

	/* Create the /dev/ex-misc entry */
	if ((res = misc_register(&ex_miscdev)) < 0) {
		printk(KERN_ERR "%s: failed to register device, %d\n",
			EX_NAME, res);
		return res;
	}

	printk(KERN_INFO "bench_extint init\n");
	return 0;
}
void
ex_exit(void)
{
	misc_deregister(&ex_miscdev);
	printk(KERN_INFO "exit\n");
}
void
extint_run(void * arg)
{

	/*
	 * External interrupt handler.  Wakes the sending thread
	 * and keeps a running total of how many times its been called.
	 * A count of how many times the thread actually runs is kept
	 * as well.  If the extint runs more then the thread, consider
	 * slowing down the period of the extint.
	 */

	extint_run_count++;
	if (ex_task)
	  	wake_up_process(ex_task);

}
module_init(ex_init);
module_exit(ex_exit);
