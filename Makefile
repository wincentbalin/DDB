#
# Makefile for the Disc Data Base project
#

CC=gcc

INCLUDES=-I.
CFLAGS=-O2 -c $(INCLUDES)
OBJS=ddb.o sqlite3.o

ifeq ($(findstring CYGWIN,$(shell uname)), CYGWIN)
CFLAGS+=-mno-cygwin
LDFLAGS+=-mno-cygwin
endif

ifdef COMSPEC
LIBS=-lstdc++
else
LIBS=-lstdc++ -ldl -lpthread
endif

all: ddb

ddb: $(OBJS)
	$(CC) $(LDFLAGS) -o ddb $(OBJS) $(LIBS)

ddb.o:	ddb.cpp ddb.hpp
	$(CC) $(CFLAGS) ddb.cpp

sqlite3.o:
	$(CC) $(CFLAGS) $*.c

clean:
	rm -f ddb ddb.exe *~ *.o

