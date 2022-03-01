INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= 

HFILES= 
CFILES= hw2.c
SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	hw2

clean:
	-rm -f *.o *.d
	-rm -f pthread

hw2: hw2.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o -lpthread -lrt -pthread

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
