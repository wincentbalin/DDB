#
# Makefile for the Disc Data Base project
#

ifdef $(WIN32)
CC=gcc -mno-cygwin
LIBS=-lstdc++
else
CC=gcc
LIBS=-lstdc++ -ldl -lpthread
endif

SQLITE_VERSION=3.6.23.1
SQLITE_SRC=sqlite-$(SQLITE_VERSION)
INCLUDES=-I"." -I$(SQLITE_SRC) 
CFLAGS=-O2 -c $(INCLUDES)
OBJS=ddb.o sqlite3.o

all: ddb

ddb: $(OBJS)
	$(CC) -o ddb $(OBJS) $(LIBS)

ddb.o:	ddb.cpp ddb.hpp
	$(CC) $(CFLAGS) ddb.cpp

sqlite3.o:
	$(CC) $(CFLAGS) $(SQLITE_SRC)/$*.c

clean:
	rm -f ddb ddb.exe *~ *.o

