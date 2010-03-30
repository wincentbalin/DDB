#
# Makefile for the Disc Data Base project
#

CC=gcc -mno-cygwin
SQLITE_VERSION=3.6.23
SQLITE_SRC=sqlite-$(SQLITE_VERSION)
INCLUDES=-I"." -I$(SQLITE_SRC) 
CFLAGS=-O2 -c $(INCLUDES)
OBJS=ddb.o sqlite3.o

all: ddb

ddb: $(OBJS)
	$(CC) -o ddb $(OBJS) -lstdc++

ddb.o:	ddb.cpp ddb.hpp
	$(CC) $(CFLAGS) ddb.cpp

sqlite3.o:
	$(CC) $(CFLAGS) $(SQLITE_SRC)/$*.c

clean:
	rm -f ddb ddb.exe *~ *.o

