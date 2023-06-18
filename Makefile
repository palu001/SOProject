CC=gcc
CCOPTS=--std=gnu99 -Wall -D_LIST_DEBUG_ -g
AR=ar

OBJS=linked_list.o \
     fake_process.o \
     fake_os.o

HEADERS=linked_list.h fake_process.h

BINS=sched_sim

.PHONY: clean all

all: $(BINS)

%.o: %.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@ $<



sched_sim: sched_sim.c $(OBJS)
	$(CC) $(CCOPTS) -o $@ $^

clean:
	rm -rf *.o *~ $(OBJS) $(BINS)

