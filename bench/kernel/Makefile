FLAGS += -D__KERNEL__ -DMODULE -g

obj-m += bench_extint_mod.o
bench_extint_mod-objs := bench_extint.o

clean:
	rm -f *.mod.* *.o *.ko *.symvers .bench_extint* 
	rm -rf *.tmp_versions
