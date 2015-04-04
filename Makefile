all: proxyFilter


CLIBS=-pthread
CC=gcc
CPPFLAGS=
CFLAGS=-g

PROXYOBJS=proxyFilter.o Cache.o SuperFastHash.o Request.o host_connection.o

proxyFilter: $(PROXYOBJS)
	$(CC) -o proxyFilter $(PROXYOBJS)  $(CLIBS)



clean:
	rm -f *.o
	rm -f proxyFilter
