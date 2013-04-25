## Makefile for dadder ##

CFLAGS=-c -Wall -g -O0 -DDEBUG
#include Makefile.inc

CC=gcc
LDFLAGS=-lresolv -lpthread -lm
SOURCES=policy.c resolvers.c list.c mystring.c trie.c ip_prefix.c main.c config.c ini.c listener.c common.c


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
