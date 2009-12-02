# Makefile for pypurple
#

LIBS = `pkg-config --libs purple`
INCS = `pkg-config --cflags purple` -I/usr/include/python2.5
CC = gcc -c -g -fPIC
LINK = gcc -g

pypurple: pypurple.o ioloop.o pypurple_wrap.o 
	$(LINK) -shared pypurple.o pypurple_wrap.o ioloop.o -o _pypurple.so $(LIBS) -Xlinker -export-dynamic -fPIC

test:
	$(LINK) $(INCS) $(LIBS) -DPYPURPLE_TEST pypurple.c -o pypurple

pypurple.o: pypurple.c
	$(CC) $(INCS) pypurple.c
ioloop.o: ioloop.c
	$(CC) $(INCS) ioloop.c
pypurple_wrap.o: pypurple_wrap.c
	$(CC) $(INCS) pypurple_wrap.c

pypurple_wrap.c: pypurple.i
	swig -python pypurple.i

clean:
	rm *.o


