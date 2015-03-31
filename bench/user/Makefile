CC=cc
CFLAGS=-g -Wall 
LDFLAGS=-lpthread -lrt -lcpuset
SRCS = bench_example.c libnl.c common.c  
OBJS = $(SRCS:.c=.o)
PGMS = bench

all: 
	$(CC) $(CFLAGS) $(SRCS)  $(LDFLAGS) -o $(PGMS)

clean:
	rm -f $(OBJS) $(PGMS)

