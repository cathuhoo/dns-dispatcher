## Makefile for dadder ##

CFLAGS=-c -Wall -g -O0 -DDEBUG
#include Makefile.inc

CC=gcc
LDFLAGS=-lresolv -lpthread
SOURCES=policy.c resolvers.c list.c mystring.c trie.c

OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=dns-dispatch

prefix=/usr/local/$(EXECUTABLE)

all: $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) -o $@ $(OBJECTS)  $(LDFLAGS)

%.o: %.c %.h
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS) 
	rm -f $(EXECUTABLE) 
    
install:
	./install.sh  $(prefix)  $(EXECUTABLE)
