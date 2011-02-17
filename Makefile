FTRACE=ftrace
FTRACE_CFLAGS=-g -Wall -DFTRACE_DEBUG
FTRACE_OBJS=task.o ftrace.o

all:$(FTRACE) Makefile

$(FTRACE):$(FTRACE_OBJS)
	gcc $(FTRACE_OBJS) -o $@

%.o:%.c
	gcc $(FTRACE_CFLAGS) -c $< -o $@

clean:
	rm -rf *.o $(FTRACE)

