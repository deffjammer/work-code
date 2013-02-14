/*
 *  Copyright (c) 2011 Silicon Graphics, Inc.
 *  All rights reserved.
 *
 *  Derek L. Fults <dfults@sgi.com
 */


#include <stdio.h>
#include <stdlib.h>
#include <bitmask.h>
#include <cpuset.h>
#include <react.h>

/* 
 * Set to 1 for cpu_sysrt_irq example. 
 * Additional IRQ setup info needed below
 */
#define IRQ_TEST 0

static char *bmp_to_list(const struct bitmask *bmp)
{
	char *buf = NULL;
	int buflen;
	char c;

	/* First bitmask_displaylist() call just to get the length */
	buflen = bitmask_displaylist(&c, 1, bmp) + 1;	/* "+ 1" for nul */
	if ((buf = malloc(buflen)) == NULL)
		return NULL;

	bitmask_displaylist(buf, buflen, bmp);
	return buf;
}

void display_react_info(void)
{
	struct bitmask *i_cpus = NULL;
	char *dp;

	if ((i_cpus = bitmask_alloc(cpuset_cpus_nbits())) == NULL) {
		perror("cpuset: bitmask alloc failed:");
		exit (1);
	}

	if (cpu_sysrt_info(&i_cpus, QBOOTCPUS)){
		perror("cpu_sysrt_info failed:");
	}
	dp = bmp_to_list(i_cpus);
	printf("/boot cpus %s, ",dp);
	free(dp);

	if (cpu_sysrt_info(&i_cpus, QBOOTMEMS)){
		perror("cpu_sysrt_info failed:");
	}
	dp = bmp_to_list(i_cpus);
	printf("/boot mems %s\n",dp);
	free(dp);
	
	if (cpu_sysrt_info(&i_cpus, QRTCPUS)){
		perror("cpu_sysrt_info failed:");
	}
	dp = bmp_to_list(i_cpus);
	printf("/rtcpus cpus %s, ",dp);
	free(dp);

	if (cpu_sysrt_info(&i_cpus, QRTMEMS)){
		perror("cpu_sysrt_info failed:");
	}
	dp = bmp_to_list(i_cpus);
	printf("/rtcpus mems %s\n",dp);
	free(dp);
	
	bitmask_free(i_cpus);
	
}

int main(int argc, char **argv)
{
	struct bitmask *cpus = NULL;

	/* List of new rtcpus, [0] = #of rtcpus in array */
	int cpulist[10] = {2,2,3};
	int i, cpu_to_runon = 2;

	if ((cpus = bitmask_alloc(cpuset_cpus_nbits())) == NULL) {
		perror("cpuset: bitmask alloc failed:");
		exit (1);
	}
  
	printf("Original REACT setup.\n");
	printf("==============================\n");
	display_react_info();
	printf("==============================\n");

	for (i = 1; i <= cpulist[0]; i++) {
		bitmask_setbit(cpus, cpulist[i]);
	}

	/* Add rtcpus */
	if (cpu_sysrt_add(cpus, RT_WAIT)){
		perror("cpu_sysrt_add failed:");
	}

	/* Set permissions of REACT bits*/
	gid_t           group_id = 117; /* group id or PARAMETER_UNCHANGED, READ_FROM_FILE */
	mode_t          mode     = 01644; /* permissions or PARAMETER_UNCHANGED, READ_FROM_FILE*/
	unsigned long   mask     = RT_NO_WAIT;    /* or RT_WAIT  */
  
	if (cpu_sysrt_perm(group_id, mode,  mask) < 0){
		perror("Permissions failed");	   
	}

#ifdef IRQ_TEST
	/* IRQ */
	char user_irq_input_buf[45] = "86:2,89:1,87:3,18:4,88:6";
 
	if (cpu_sysrt_irq(user_irq_input_buf, RT_WAIT)){
		perror("cpu_sysrt_irq failed");
	}
#endif
	/* Move ourselves to an rtcpu */
	cpu_sysrt_runon(cpu_to_runon);

	printf("\nProgram modified REACT setup\n");
	printf("==============================\n");
	display_react_info();
	printf("==============================\n");
	printf("\nNow running on /rtcpus/rtcpu%d, press 'Enter' to cont..\n",cpu_to_runon);
	getchar();
	

	/* Delete rtcpus that were added */
	if (cpu_sysrt_delete(cpus, RT_WAIT)){
		perror("cpu_sysrt_del failed:");
	}

	bitmask_free(cpus);

	return 0; 
}
