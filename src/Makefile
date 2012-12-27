CC=gcc
CFLAGS=-g -Wall -ansi -pedantic -fpic
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)
SUBDIRS=shell
LIB=libchess.a

all: $(LIB) $(SUBDIRS)

$(LIB): $(OBJS)
	libtool -static -o $@ $(OBJS) 2>/dev/null || ar crus $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean: $(SUBDIRS)
	rm -f *.o $(LIB)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKEOPTS) $(MAKECMDGOALS)

test:
	$(MAKE) -C $@ $(MAKEOPTS)
	

.PHONY: clean $(SUBDIRS) test
