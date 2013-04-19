## Makefile for dns-dispatcher ##

CFLAGS=-c -Wall -g -O0 -DDEBUG
#include Makefile.inc

CC=gcc
LDFLAGS=-lresolv -lpthread
SOURCES=*.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=dns-dispatcher

prefix=/usr/local/$(EXECUTABLE)

all: $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) -o $@ $(OBJECTS)  $(LDFLAGS)

list.o: list.h list.c
	$(CC) $(CFLAGS) $< -o $@  

mystring.o: mystring.c
	$(CC) $(CFLAGS) $< -o $@  

resolvers.o: resolvers.c resolvers.h list.h 
	$(CC) $(CFLAGS) $< -o $@  

    
%.o: %.c %.h
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS) 
	rm -f $(EXECUTABLE) 
    
install:
	./install.sh  $(prefix)  $(EXECUTABLE)
