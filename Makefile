#
# Makefile for the Disc Data Base project
#

CC=gcc
CXX=g++

INCLUDES=-I.
CFLAGS=-O2 -c $(INCLUDES)
CXXFLAGS=$(CFLAGS)
OBJS=db.o ddb.o sqlite3.o
LIBS=-lstdc++ -lboost_filesystem -lboost_system

ifeq ($(findstring CYGWIN,$(shell uname)), CYGWIN)
CFLAGS+=-mno-cygwin
LDFLAGS+=-mno-cygwin
endif

ifndef COMSPEC
LIBS+=-ldl -lpthread
endif

all: ddb

ddb: $(OBJS)
	$(CC) $(LDFLAGS) -o ddb $(OBJS) $(LIBS)

db.o:	db.cpp db.hpp
	$(CXX) $(CXXFLAGS) db.cpp

ddb.o:	ddb.cpp ddb.hpp
	$(CXX) $(CXXFLAGS) ddb.cpp

sqlite3.o:
	$(CC) $(CFLAGS) $*.c

clean:
	rm -f ddb ddb.exe *~ *.o

