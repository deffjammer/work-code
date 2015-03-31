This is an example of a multithreaded application that demonstrates using
external interrupts and other aspects of REACT. It uses netlink sockets to
communicate from kernel space to user space. You can use it as a performance
benchmark to compare between machines or settings within REACT, such as for
external interrupts, cpusets, and CPU isolation.

For detailed instructions see the REACT Manual Appendix A.

# Ensure that the sgi-extint-kmp-modvers rpm is installed.

     $ cp /usr/share/extint/`uname -r`/Module.symvers /usr/share/react/samples/bench/kernel/.


# To build and load the kernel module in bench_example/kernel/

     $ make -C /lib/modules/`uname -r`/build SUBDIRS=$PWD modules

     $ cp bench_extint_mod.ko /lib/modules/`uname -r`

     $ depmod

     $ modprobe bench_extint_mod


# To build the user application in bench_example/user/

     $ make


# Load the ioc4_extint module:

     $ modprobe ioc4_extint


# Insert the required information into the source, mode, and period
files in the /sys/class/extint/extint0/ directory. For example:

     $ echo loopback >/sys/class/extint/extint0/source
     $ echo toggle >/sys/class/extint/extint0/mode
     $ echo 1000000 >/sys/class/extint/extint0/period

or you can used the script 'extint <period>' in bench_example/kernel/



The bench command has the following options:
-h	 Prints usage instructions

-m	 Locks memory

-b<msgs> Socket benchmark mode. 	

-s<size> Specifies the size of buffers in bytes (for network socket bench mode). 
	 The default is 1024. You can vary the size of the buffers to see the impact on performance.

-p<cpu>	 Specifies the CPU where the bench process will run.

-r<cpu>	 Specifies the CPU where the receive thread will run.

-w<cpu>	 Specifies the CPU where the worker thread will run.

-k<cpu>	 Specifies the CPU where the kthread will run.

-t<sec>	 Specifies the total run time in seconds, with a maximum of 30 seconds.
	 The default is 30. 
