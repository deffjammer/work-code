/*
 * allocate a transparent huge page and connect with xpmem
 * cpw       5/2011
 *
 * # ln -s libgru.so.0.0.0 libgru.so
 * cc -I. -Wl,-L/lib64 -Wl,-lxpmem -Wl,-L/usr/lib64 -Wl,-lgru -o hugexpmem hugexpmem.c
 * ls -la hugexpmem
 * ldd hugexpmem
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <malloc.h>
#include <sys/wait.h>
#include <uv/gru/gru.h>
#include <uv/gru/gru_instructions.h>
#include <errno.h>
#include <sn/xpmem.h>
#include <signal.h>

extern int optind, opterr;
extern char *optarg;
#define HSIZE  2097152  // 2M
#define KEYVAL 123

/* the boundary; at this point an address may only be addressed with the GRU */
#define GRU_SPACE                       (1UL << 48)
#define GRU_SPACE_SHARED                (1UL << 63)

int	verbose=0, pflag=0, tflag=0, Mflag=0, dflag=0, aflag=0, Pflag=0;
int	Aflag=0, Bflag=0, Sflag=0, gflag=0, Fflag=0, Lflag=0, cflag=0, Cflag=0;
int 	num_attaches=0, num_attach_sizes=0;
long	atoffset[10];
long	atsize[10];
char	*used_hint[10];
long	used_offset[10];
long	used_size[10];
long	hintoffset = 0;
long	hintoffsetchild = -1;
char	*athint = 0;
long	seghandle;
long	hbytes=HSIZE, pagesize, segoffset=0, segbytes;
char	*cmdname;
char	*at_vaddr;
void	usage();
void	get_options(int, char *[]);
long	scaled_atol(char *);
void	on_sigbus (int);
char	ansline[100];
gru_segment_t *gseg;
gru_cookie_t cookie;
gru_control_block_t *cb = NULL;
struct sigaction action;

/*
 * overwrite an area with the GRU, assuming gru context is already set up
 * base is the virtual address of the areas
 * size is its size in bytes
 * value is the data to write into each 8-byte word
 */
void
vset_area(void *base, long size, gru_control_block_t *cb, unsigned long value)
{
	int nelem;
	int ret;
	long num_longs;

	num_longs = size / sizeof(long);
	nelem = size / 64; /* cache lines */
	if (verbose) {
		printf("%d child overwriting area %p size %#lx with gru\n",
			getpid(), base, size);
		printf("%d child %d cache lines,  cb %p\n",
			getpid(), nelem, cb);
	}
	gru_vset(cb, base, value, XTYPE_CL, nelem, 1, 0);
	// void gru_vset(gru_control_block_t *cb, gru_addr_t dest,
	//    unsigned long value, unsigned char xtype, unsigned long nelem,
	//    unsigned long stride, unsigned long hints)

	if ((ret = gru_wait(cb))) {
		printf("wait failed\n");
		return;
	}
	if (verbose)
		printf("%d child vset done\n", getpid());
}

/*
 * this process will attempt to attach to the given process with xpmem
 *
 *   handle is that of the entire huge page
 *   atoffset is that withing the segment to the area we are to attach to
 *   size is that of the area we are to attach to (ie the attachment size)
 *   p is the virtual address by which the parent knows the area
 */
static void
do_attach(__s64 id, long atoffset, long size, char *hint)
{
	int i;
	long *lp, numlongs;
	char	*loc_at_vaddr; /* the local (to this function) attach vaddr */

	if (Bflag) {
		printf("%d child pausing before xpmem_attach\n", getpid());
		pause();
	}
	// ask for the same vaddr (hint), else NULL lets it find any available
	if (verbose)
		printf(
	     "%d child try attach id %#llx atoffset %#lx size %#lx hint %p\n",
			getpid(), id, atoffset, size, hint);
	if (gflag) {
		loc_at_vaddr = xpmem_attach_high_2(id, atoffset, size, hint);
		if (loc_at_vaddr == (void *)-1) {
		 	printf("xpmem_attach_high_2() failed, errno = %d\n",
				errno);
			return;
		}
		at_vaddr = loc_at_vaddr; /* set the global too */
	} else {
		loc_at_vaddr = xpmem_attach_2(id, atoffset, size, hint);
		if (loc_at_vaddr == (void *)-1) {
			printf("xpmem_attach_2() failed, errno = %d\n", errno);
			return;
		}
		at_vaddr = loc_at_vaddr; /* set the global too */
	}

	if (verbose)
		printf("%d child address of attach point: %p for %ld (%#lx)\n",
			getpid(), at_vaddr, size, size);
	if (Aflag) {
		printf("%d child pausing after xpmem_attach\n", getpid());
		pause();
	}

	if (!Mflag) {
		// if (pflag) {
		// 	printf(
		//	  "%d child spinning before madvise continue? [y] \n",
 		//		getpid());
		//	ansline[0] = '\0';
		//	while (ansline[0] == 'n') {fgets (ansline, 25, stdin);}
        	// }
		// ask the kernel to fill in our pte's 
		// int xpmem_madvise_2(void * start, size_t length, int advice)
		//  'advice' is not used
		if (verbose)
			printf(
			"%d child madvise loc_at_vaddr %p size %#lx\n",
				getpid(), loc_at_vaddr, size);
		if (xpmem_madvise_2(loc_at_vaddr, size, 0)) {
			printf("xpmem_madvise_2() failed, errno = %d\n", errno);
			exit(1);
		}
	}
	
	numlongs = size / sizeof(long);
	if (gflag) {
		vset_area(loc_at_vaddr, size, cb, KEYVAL);
		lp = (long *)(loc_at_vaddr + size - sizeof(long));
	} else {
		if (verbose)
			printf("%d child begin write of %p-%p\n",
					getpid(), loc_at_vaddr,
					loc_at_vaddr+size);
		for (i=0, lp=(long *)loc_at_vaddr; i < numlongs; i++, lp++) {
			*lp = KEYVAL;
		}
		lp = (long *)loc_at_vaddr;
	}
	if (verbose)
		printf("%d child data: %p\n", getpid(), loc_at_vaddr);
	if (verbose)
		printf("%d child wrote key to %p\n",
			getpid(), loc_at_vaddr + size - 8);
}

int
main(int argc, char *argv[])
{
	char *hugep, *segp, *seg_hugep, *hugep_end, *hp, *old_hugep;
	char *attp;
	char *hint;
	int pid;
	int attach;
	int status;
	int ret;
	long *lp, sum;
	long numlongs, i, attbytes;
	unsigned long ul;
	__s64 handle;
	__s64 id;

	get_options(argc, argv);

	/* xpmem will cause a SIGBUS error when a huge page is mapped on
	   a huge page boundary and also on a non-huge page boundary */
	action.sa_handler = on_sigbus;
	action.sa_flags   = 0;
	if (sigaction(SIGBUS, &action, 0)) {
		printf ("sigaction failed\n");
		exit(1);
	}

	segbytes = hbytes - segoffset;
	// atoffset is the attachment offset into the segment
	for (i=0; i<num_attaches; i++) {
		if (atoffset[i] > segbytes) {
			printf("error; att offset %ld > %ld\n",
				atoffset[i], segbytes);
			exit(1);
		}
		if (atsize[i] % 4096) {
			printf("error; att size %ld not full pages\n",
				atoffset[i]);
			exit(1);
		}
	}

	if (cflag) {
		// attach specified handle
		// allow only one -o/-s
		attbytes = atsize[0];
		if (verbose)
			printf(
   "%d new: attaching to handle %#lx; offset %#lx attbytes %#lx, hint %p\n",
				getpid(), seghandle, atoffset[i], attbytes,
				athint);
		id = xpmem_get_2(seghandle, XPMEM_RDWR, XPMEM_PERMIT_MODE,
				NULL);
		do_attach(id, atoffset[i], attbytes, athint);
		if (pflag) {
			printf("new %d: pausing\n", getpid());
                        printf(" vm %d | grep %lx\n", getpid(), at_vaddr);
			printf("    should be %p-%p\n", at_vaddr,
				at_vaddr + attbytes);
			printf(" first:  pagetable -t %d -a %p\n",
				getpid(), at_vaddr);
			printf(" last:   pagetable -t %d -a %p\n",
					getpid(),
					at_vaddr + attbytes - 8);
			pause();
		}
		exit(0);
	}

	if (verbose)
		printf("%d parent using mmap\n", getpid());

	hugep = mmap(0, hbytes, PROT_READ | PROT_WRITE,
				MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	// MAP_FAILED is -1
	if (!(hbytes & (HSIZE-1)) && hugep != (char *)MAP_FAILED &&
	    				(size_t)hugep & (HSIZE-1)) {
		// not on a huge page boundary
		munmap(hugep, hbytes);
		hugep = mmap (0, hbytes + HSIZE-1,
				PROT_READ | PROT_WRITE,
				MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		old_hugep = hugep;
		hugep = (char *)(((size_t)hugep + HSIZE - 1) &
							 ~(HSIZE - 1));
		if (old_hugep != hugep) {
			munmap(old_hugep, hugep-old_hugep);
		}
		if (hugep != old_hugep + HSIZE-1) {
			munmap(hugep + hbytes, old_hugep + HSIZE - 1
							- hugep);
		}
	}
	// Mark the huge page as VM_DONTCOPY so that it does not appear
	// in the child's address space.
	madvise(hugep, hbytes, MADV_DONTFORK); 

	// segoffset is the segment offset into the huge page area
	segp = hugep + segoffset;
	if (verbose)
		printf("%d parent hugep %p-%p segp %p-%p\n",
			getpid(), hugep, hugep+hbytes, segp, segp+segbytes);

	if (pflag | Aflag | Bflag) {
		printf("parent segment:\n");
		printf(" pagetable -t %d -a %p\n", getpid(), hugep);
	}

	if (tflag) {
		hugep_end = hugep + hbytes;
		i = 0;
		hp = hugep;
		while (hp < hugep_end) {
			printf("touching hugepage[%ld] at %p\n", i, hp);
			*hp = 0;
			i++;
			hp += 0x200000;
		}
	}

	// note: patched libxpmem will touch the thp on the xpmem_make
	// fill with zeros
	if (Fflag) {
		numlongs = segbytes / sizeof(long);
		for (i=0, lp=(long *)segp; i < numlongs; i++, lp++) {
			*lp = 0;
		}
		if (verbose)
			printf("%d parent done zeroing %p-%p\n",
				getpid(), segp, segp + segbytes);
	} else {
		if (verbose)
			printf("%d parent not zeroing the segment\n", getpid());
	}
	if (Pflag) {
		if (pflag) {
			if (!Fflag)
				printf("%d parent did not fill the segment\n",
					getpid());
			printf("%d parent pause before xpmem_make_2\n",
				getpid());
			pause();
		} else
			printf("%d parent exit before xpmem_make_2\n",
				getpid());
		exit(0);
	}

	// create an xpmem handle for the parent's area
	// handle = xpmem_make_2(segp, segbytes, XPMEM_PERMIT_MODE | XPMEM_TRY_THP,
	handle = xpmem_make_2(segp, segbytes, XPMEM_PERMIT_MODE,
					 (void *)0600);
	if (handle < 0) {
		printf("handle %#llx  xpmem_make_2() failed, errno = %d\n",
			handle, errno);
		exit(1);
	}
	if (verbose)
		printf("%d parent segment handle for %p is %#llx\n",
					getpid(), segp, handle);
	
	if (verbose)
		printf("%d parent begin %d attaches\n\n",
			getpid(), num_attaches);
	/* calculate before forking so any child can report them*/
	for (attach = 0; attach < num_attaches; attach++) {
		attp = segp + atoffset[attach];
		if (atsize[attach])
			attbytes = atsize[attach];
		else
			attbytes = segbytes - atoffset[attach];
		if (hintoffsetchild >= 0) {
			if (hintoffsetchild == attach)
				hint = attp + hintoffset;
			else
				hint = attp;
		} else
			hint = attp + hintoffset;
		if (gflag) {
			ul = (unsigned long)hint;
			ul |= GRU_SPACE;
			hint = (char *)ul;
		}
		if (aflag)
			hint = 0;
		used_hint[attach] = hint;
		used_size[attach] = attbytes;
		used_offset[attach] = atoffset[attach];
	}

	if (Sflag) {
		// single child does the attaches
		pid = fork();
		if (pid == 0) {
			if (gflag) {
				/* only the child should create the cb, as it
				 * is mapped to /dev/gru and is lost across
				 * a fork
				 */
				if (gru_create_context(&cookie, 0, 1, 0, 1,
					GRU_OPT_MISS_USER_POLL) < 0) {
					/* cookie: to be returned
					   start:  for mmap
					   1: number of control_blocks
					   0: data segment bytes 
					   1: maximum thread count
					   option: using user polling option */
					printf(
					"gru_create_context failed; errno:%d\n",
					 	errno);
					exit(1);
				}
				if ((gseg = gru_get_thread_gru_segment(
							cookie, 0)) == NULL) {
					printf(
					"gru_get_thread_gru_segment failed\n");
					exit(1);
				}
				cb = gru_get_cb_pointer(gseg, 0);
			}

			id = xpmem_get_2(handle, XPMEM_RDWR,
				XPMEM_PERMIT_MODE, NULL);
			if (id == -1) {
				printf("xpmem_get_2() failed, errno = %d\n",
						errno);
				exit(1);
			}
			if (verbose)
				printf(
				      "%d child id for handle %#llx is %#llx\n",
					getpid(), handle, id);

			for (attach = 0; attach < num_attaches; attach++) {
				attp = segp + atoffset[attach];
				if (atsize[attach]) {
					if (atsize[attach] + atoffset[attach] >
								segbytes) {
						printf(
				      "error; atoffset %ld + size %ld > %ld\n",
					    atoffset[attach], atsize[attach],
						segbytes);
						exit(1);
					}
					attbytes = atsize[attach];
				} else {
			/* default size to all the hugepages after att offset */
					attbytes = segbytes - atoffset[attach];
				}
				if (verbose)
					printf(
				   "%d child begin do_attach to area %p-%p\n",
					getpid(), attp, attp+attbytes);

				/* -H does not apply with only 1 child */
				hint = attp + hintoffset;

				if (gflag) {
					// use a GRU-only addressable space
					ul = (unsigned long)hint;
					ul |= GRU_SPACE;
					hint = (char *)ul;
				}

				if (aflag) {
					// no address hint, 'any' address
					// mmap'd space already released
					do_attach(id, atoffset[attach],
								attbytes, 0);
				} else {
					do_attach(id, atoffset[attach],
								attbytes, hint);
				}
				if (dflag) {
					if (verbose)
						printf(
					       "%d child doing xpmem_detach\n",
							getpid());
						xpmem_detach(at_vaddr);
				}
			}

			if (pflag | Aflag | Bflag) {
				printf("child %d: pausing\n", getpid());
                        	printf(" vm %d | grep %lx\n",
					getpid(), at_vaddr);
				printf("    should be %p-%p\n", at_vaddr,
					at_vaddr + attbytes);
				printf(" first:  pagetable -t %d -a %p\n",
					getpid(), at_vaddr);
				printf(" last:   pagetable -t %d -a %p\n",
					getpid(),
					at_vaddr + attbytes - 8);
				pause();
			}
			exit(0);
		}

		if (Aflag || Bflag) {
			printf("%d parent pausing after fork\n", getpid());
				pause();
		}

		if (pflag | Aflag | Bflag) {
                	printf("parent segment:\n");
                	printf(" pagetable -t %d -a %p\n", getpid(), hugep);
        	}
		/* parent pauses during connection */
		if (verbose)
			printf("%d parent waiting for %d attaches\n",
				getpid(), num_attaches);
		ret = waitpid(-1, &status, 0);
		if (verbose)
			printf("%d parent pid %d, exit status %d\n",
				getpid(), ret, WEXITSTATUS(status));
		
		/* areas the child attached to */
		for (attach = 0; attach < num_attaches; attach++) {
			attp = segp + atoffset[attach];
			if (atsize[attach])
				attbytes = atsize[attach];
			else
				attbytes = segbytes - atoffset[attach];
			numlongs = attbytes / sizeof(long);
			lp = (long *)attp + (numlongs-1);
			if (verbose)
				printf("%d parent attach[%d] %p\n",
					getpid(), attach, attp);
			if (*lp != KEYVAL) {
				printf(
				  "%d parent test failed: no key value at %p\n",
						getpid(), lp);
				continue;
			}
		
			if (verbose)
				printf("%d parent sees key at %p\n",
					getpid(), lp);
			if (verbose)
				printf("%d parent adding up area %p-%p\n",
					getpid(), attp, attp + attbytes);
			sum = 0;
			for (i=0, lp=(long *)attp; i < numlongs; i++, lp++) {
				sum += *lp;
			}
		
			if (sum == numlongs * KEYVAL) {
				printf("%d parent test passed %d of %d\n",
					getpid(), attach+1, num_attaches);
			} else {
				printf(
			 "%d parent test failed [%d] %ld vs %ld (%ld * %d)\n\n",
					getpid(), attach, sum,
					numlongs * KEYVAL, numlongs, KEYVAL);
			}
		}
	} else {
		// different child for each attach
		for (attach = 0; attach < num_attaches; attach++) {
			attp = segp + atoffset[attach];
			if (atsize[attach]) {
				if (atsize[attach] + atoffset[attach] >
								segbytes) {
					printf(
				      "error; atoffset %ld + size %ld > %ld\n",
					    atoffset[attach], atsize[attach],
						segbytes);
						exit(1);
				}
				attbytes = atsize[attach];
			} else {
			/* default size to all the hugepages after att offset */
				attbytes = segbytes - atoffset[attach];
			}
		
			if (verbose)
				printf(
				   "%d parent begin do_attach to area %p-%p\n",
					getpid(), attp, attp+attbytes);
	
			pid = fork();
			if (pid == 0) {
				if (gflag) {
				/* only the child should create the cb, as it
				 * is mapped to /dev/gru and is lost across
				 * a fork
				 */
					if (gru_create_context(&cookie, 0, 1,
					  0, 1, GRU_OPT_MISS_USER_POLL) < 0) {
					/* cookie: to be returned
					   start:  for mmap
					   1: number of control_blocks
					   0: data segment bytes 
					   1: maximum thread count
					   option: using user polling option */
						printf(
					"gru_create_context failed; errno:%d\n",
					 		errno);
						exit(1);
					}
					if ((gseg = gru_get_thread_gru_segment(
							cookie, 0)) == NULL) {
						printf(
					"gru_get_thread_gru_segment failed\n");
						exit(1);
					}
					cb = gru_get_cb_pointer(gseg, 0);
				}

				id = xpmem_get_2(handle, XPMEM_RDWR,
					XPMEM_PERMIT_MODE, NULL);
				if (id == -1) {
					printf(
					  "xpmem_get_2() failed, errno = %d\n",
						errno);
					exit(1);
				}
				if (verbose)
					printf(
				      "%d child id for handle %#llx is %#llx\n",
						getpid(), handle, id);
				if (verbose)
					printf(
		    "%d child do_attach attbytes %#lx segp %p atoffset %#lx\n",
						getpid(), attbytes, segp,
						atoffset[attach]);
					/* this is the child, which
					   should try to attach */

				/* -H tells us which child gets the -h offset */
				if (hintoffsetchild >= 0) {
					if (hintoffsetchild == attach) {
						if (verbose)
							printf(
			    "%d child applying hintoffset %#lx for child %ld\n",
							  getpid(), hintoffset,
							  hintoffsetchild);
						hint = attp + hintoffset;
					} else {
						hint = attp;
					}
				} else {
					/* any child get any -h offset */
					if (verbose)
						printf(
					"%d child setting hint to %p + %#lx\n",
						getpid(), attp, hintoffset);
					hint = attp + hintoffset;
				}

				if (gflag) {
					// use a GRU-only addressable space
					ul = (unsigned long)hint;
					ul |= GRU_SPACE;
					hint = (char *)ul;
				}

				if (aflag) {
					// no address hint, 'any' address
					do_attach(id, atoffset[attach],
								attbytes, 0);
				} else {
					do_attach(id, atoffset[attach],
								attbytes, hint);
				}
				if (dflag) {
					if (verbose)
						printf(
					        "%d child doing xpmem_detach\n",
							getpid());
						xpmem_detach(at_vaddr);
				}
				if (pflag | Aflag | Bflag) {
					printf("child %d: pausing\n", getpid());
                        		printf(" vm %d | grep %lx\n",
						getpid(), at_vaddr);
					printf(
					   "    should be %p-%p\n", at_vaddr,
						at_vaddr + attbytes);
					printf(
					   " first:  pagetable -t %d -a %p\n",
						getpid(), at_vaddr);
					printf(
					    " last:   pagetable -t %d -a %p\n",
						getpid(),
						at_vaddr + attbytes - 8);
					pause();
				}
				exit(0);
			}
			if (Aflag || Bflag) {
				printf("%d parent pausing after fork\n",
					getpid());
				pause();
			}
		
			seg_hugep = segp + atoffset[attach];
				/* area the child attached to */
			numlongs = attbytes / sizeof(long);
			if (verbose)
				printf(
				    "%d parent address of attach point: %p\n",
						getpid(), seg_hugep);
		
			lp = (long *)attp + numlongs - 1;
			/* parent pauses during connection */
			/* but with -L we let the attaches run in parallel */
			if (!Lflag) {
				if (verbose)
					printf(
				   "%d parent waiting for key value at %p\n",
						getpid(), lp);
				waitpid(-1, &status, 0);
				if (verbose)
					printf(
					  "%d parent pid %d, exit status %d\n",
						getpid(), ret,
						WEXITSTATUS(status));
		
				if (*lp != KEYVAL) {
					printf(
				  "%d parent test failed: no key value at %p\n",
						getpid(), lp);
					if (pflag) {
						printf("%d parent pausing\n",
							getpid());
						pause();
					}
					continue;
				}
			
				if (verbose)
					printf("%d parent sees key at %p\n",
						getpid(), lp);
				if (verbose)
					printf(
					"%d parent adding up area %p-%p\n",
						getpid(), seg_hugep,
						seg_hugep + attbytes);
				sum = 0;
				for (i=0, lp=(long *)seg_hugep; i < numlongs;
								i++, lp++) {
					sum += *lp;
				}
			
				if (sum == numlongs * KEYVAL) {
					printf("%d parent test passed\n",
						getpid());
				} else {
					printf(
			"%d parent test failed %ld vs %ld (%ld * %d)\n\n",
						getpid(), sum,
						numlongs * KEYVAL,
						numlongs, KEYVAL);
				}
			}
		}

		if (Lflag) {
			// above ran the multiple attaches in parallel
			if (verbose)
				printf(
				"%d parent waiting for all children\n",
					getpid());

			do {
				ret = waitpid(-1, &status, 0);
				if (verbose && ret > 0)
					printf(
					  "%d parent pid %d, exit status %d\n",
						getpid(), ret,
						WEXITSTATUS(status));
			} while (ret > 0);

			// to test -L results use the first test, as if there
			// there was an alignment conflict the second fails
			numlongs = used_size[0] / 8;
			attp = hugep + atoffset[0];

			lp = (long *)attp + numlongs - 1;
			if (*lp != KEYVAL) {
				printf(
				 "%d parent test failed: no key value at %p\n",
						getpid(), lp);
			} else {
				if (verbose)
					printf("%d parent sees key at %p\n",
						getpid(), lp);
				if (verbose)
					printf(
					"%d parent adding up area %p-%p\n",
						getpid(), attp,
						attp + attbytes);
				sum = 0;
				for (i=0, lp=(long *)attp; i < numlongs;
								i++, lp++) {
					sum += *lp;
				}
			
				if (sum == numlongs * KEYVAL) {
					printf("%d parent test passed\n",
						getpid());
				} else {
					printf(
			     "%d parent test failed %ld vs %ld (%ld * %d)\n\n",
						getpid(), sum,
						numlongs * KEYVAL,
						numlongs, KEYVAL);
				}
			}
		}
	}

	if (pflag) {
		printf("%d parent pausing\n", getpid());
		pause();
	}

	return 0;
}

/*
 * scaled_atol - string to ascii with abbreviations:
 *      k = 1000
 *      K = 1024
 *      m = 1000000
 *      M = 1024*1024
 *      g = 1000000000
 *      G = 1024*1024*1024
 *      t = 1000000000000
 *      T = 1024*1024*1024*1024
 *      p - system pagesize
 *      P - system pagesize
 */
long
scaled_atol(char *p)
{
	long val;
	char *pe;

	val = strtol(p, &pe, 0);
	switch (*pe) {
	case 'P':
	case 'p':
		val *= getpagesize();
		break;
	case 't':
		val *= 1000;
	case 'g':
		val *= 1000;
	case 'm':
		val *= 1000;
	case 'k':
		val *= 1000;
		break;
	case 'T':
		val *= 1024;
	case 'G':
		val *= 1024;
	case 'M':
		val *= 1024;
	case 'K':
		val *= 1024;
		break;
	}
	return val;
}

void
get_options(int argc, char *argv[])
{
	int i, c, error=0;

	cmdname = *argv;
	if (strchr(cmdname,'/')) { /* isolate the last component */
		i = strlen (cmdname);                         
		for (cmdname=cmdname+i; *(cmdname-1) != '/'; cmdname--);    
        }   

	opterr = 1;
	pagesize = getpagesize();
	while ((c = getopt(argc, argv, "c:C:tmMPLABFSpgdab:r:o:h:H:O:s:vz")) != EOF)
		switch (c) {
		case 'c':
			cflag++;
			seghandle = scaled_atol(optarg);
			continue;
		case 'C':
			Cflag++;
			athint = (char *)scaled_atol(optarg);
			continue;
		case 'b':
			hbytes = scaled_atol(optarg);
			continue;
		case 'O':
			segoffset = scaled_atol(optarg);
			continue;
		case 'o':
			atoffset[num_attaches] = scaled_atol(optarg);
			num_attaches++;
			continue;
		case 'h':
			hintoffset = scaled_atol(optarg);
			continue;
		case 'H':
			hintoffsetchild = scaled_atol(optarg);
			continue;
		case 's':
			atsize[num_attach_sizes] = scaled_atol(optarg);
			num_attach_sizes++;
			continue;
		case 'P':
			Pflag++;
			break;
		case 'L':
			Lflag++;
			break;
		case 'g':
			gflag++;
			break;
		case 'A':
			Aflag++;
			break;
		case 'F':
			Fflag++;
			break;
		case 'B':
			Bflag++;
			break;
		case 'S':
			Sflag++;
			break;
		case 'd':
			dflag++;
			break;
		case 'a':
			aflag++;
			break;
		case 'p':
			pflag++;
			break;
		case 't':
			tflag++;
			break;
		case 'M':
			Mflag++;
			break;
		case 'v':
			verbose++;
			break;
		case 'z':
			error++;
			break;
		case '?':
			error = 1;
			break;
	}

	if (gflag) { // don't do an madvise on a GRU-addressable attachment
		Mflag = 1; 
	}
	
	if (num_attaches != num_attach_sizes) {
		printf("-o's and -s's must be paired (equal number of each)\n");
		error++;
	}
	if (!num_attaches) {
		// set defaults without any -o or -s
		num_attaches = 1;
		num_attach_sizes = 1;
		atoffset[0] = 0;
		atsize[0] = 0;
	}

	if (error) {
		usage();
		exit(1);
	}
}

void
usage()
{
	printf("usage: %s -b bytes [-O offset] [-o offset] [-s size] [-h offset] [-H task] [-a] [-d] [-P] [-L] [-c handle] [-C hint] [-v] [-z]\n", cmdname);
	printf("  -b bytes to malloc (default %d)\n", HSIZE);
	printf("   (valid suffixes for -b,-o,-s: k=1000 K=1024 m=1000000\n");
	printf("          M=1024*1024 g=1000000000 G=1024*1024*1024\n");
	printf("          t=1000000000000 T=1024*1024*1024*1024\n");
	printf("          p-system pagesize P-system pagesize)\n");
	printf("  -O offset  offset of segment on the huge page, default 0\n");

	printf("\n");
	printf(" can have several attachments:\n");
	printf("  -o offset  offset arg to xpmem_attach (att's offset), default 0\n");
	printf("  -s size    size to xpmem_attach, default bytes-offset\n");

	printf("\n");
	printf("  -L      children attaches run in parallel (no pass/fail; not -S)\n");
	printf("  -c handle attach to an existing segment (give seg handle)\n");
	printf("  -C hint  address hint for the -c attach (default to 0);\n");
	printf("  -h offset  offset to add to the hint for each attach\n");
	printf("  -H task  use -h on only the -H'th task; not w/ -S\n");
	printf("  -d      do an xpmem_detach (not just exit processing)\n");
	printf("  -a      attach at any address, no address hint)\n");

	printf("  -p      both parent and child pause [after test]\n");
	printf("  -A      child pause after attach\n");
	printf("  -B      child pause before attach\n");
	printf("  -g      use the GRU for attachment accesses (implies -M too)\n");
	printf("  -S      a single child does all the attaches\n");
	printf("  -P      if -p, pause before the attach\n");
	printf("  -F      parent to fill the buffer\n");
	printf("  -t      trigger some misc. custom test\n");
	printf("  -M      use no madvise\n");
	printf("  -v      verbose\n");
	printf("  -z      help\n");
	return;
}

void
on_sigbus (int signo)
{
	int id, i, status, signal;

	if (signo == SIGBUS) {
		action.sa_handler = on_sigbus;
		action.sa_flags   = 0;
		if (sigaction(SIGBUS, &action, 0)) {
			printf ("sigaction failed\n");
			exit(1);
		}
		printf("%s process %d exiting on SIGBUS; probably a huge ",
			cmdname, getpid());
		printf("page mapping conflict\n");
		printf("  one of %d attaches:\n", num_attaches);
		for (i=0; i<num_attaches; i++) {
			printf("   hint %p offset %#lx size %#lx\n",
				used_hint[i], used_offset[i],
				used_size[i]);
                }
		if (pflag) {
			printf("%d child pausing\n", getpid());
			pause();
		}
		exit(1);
	}
	printf ("pid %d  signal %d unexpected; exiting\n", getpid(), signo);
	exit(1);
}
