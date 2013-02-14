
//
// Print the cpus and nodes associated with the containing cpuset
//
//

#include "assert.h"
#include "cpuset.h"
#include "bitmask.h"
#include "stdio.h"

main()
{
	struct cpuset * machine;
	struct cpuset * mycpuset;
	int rv;
	int node;
	int i;
	int phys_ncpus, phys_nnodes, ratio;
	int ncpus, nnodes;

	machine = cpuset_alloc();
	assert(machine);
	mycpuset = cpuset_alloc();
	assert(mycpuset);

	/* count cpus and mems on whole system */
	rv = cpuset_query(machine, "/");
	assert(rv == 0);
	phys_ncpus = cpuset_cpus_weight(machine);
	phys_nnodes = cpuset_mems_weight(machine);
	ratio = phys_ncpus/ phys_nnodes;
	printf("This system has %d cpus and %d nodes\n",
	       phys_ncpus, phys_nnodes);
	
	/* count the cpus and nodes in containing cpuset */
	rv = cpuset_cpusetofpid(mycpuset, 0);
	assert(rv == 0);
	ncpus = cpuset_cpus_weight(mycpuset);
	nnodes = cpuset_mems_weight(mycpuset);

	char buf[30];
	cpuset_getcpusetpath(0, buf, sizeof(buf));
	printf("cpuset %s has %d cpus and %d mems\n", buf, ncpus, nnodes);

	for (i=0; i<ncpus; i++) {
	    int rcpu = i;
            int pcpu = cpuset_c_rel_to_sys_cpu(mycpuset, rcpu);
	    int rnode = cpuset_cpu2node(rcpu);
	    int pnode = cpuset_c_rel_to_sys_mem(mycpuset, rnode);
	    char * sss;

	    sss = "";

	    printf("cpu %d is physcpu %d  on node %d physnode %d   %s\n", 
		rcpu, pcpu, rnode, pnode, sss);
	}

	/*
	 * Print out the nodes.
	 */
	for (i=0; i<nnodes; i++) {
            struct bitmask *cpus = NULL, *mems = NULL;
	    cpus = bitmask_alloc(ncpus);
	    mems = bitmask_alloc(nnodes);
	    bitmask_setbit(mems, i);
	    cpuset_localcpus(mems, cpus);
	    int firstcpu = bitmask_first(cpus);
	    bitmask_free(cpus);
	    bitmask_free(mems);

	    printf("rnode %d is phys %d has cpu %d  phys cpu %d\n", 
		i,
		cpuset_c_rel_to_sys_mem(mycpuset, i),
		firstcpu,
		cpuset_c_rel_to_sys_cpu(mycpuset, firstcpu));
	}

	/*
	 * Reverse lookup.  Find cpus for a give node.
	 */

	
	return 0;
}
