CC = gcc
CXX = g++
CFLAGS = -Wall -O3 -g -mcmodel=medium

all:	verkade

verkade:	verkade.o lup.o heap.o mmio.o matrix.o
	$(CXX) $(CFLAGS) -o $@ $^ -lrt


mmio.o:	mmio.c
	$(CC) $(CFLAGS) -c mmio.c

%.o:	%.cpp
	$(CXX) $(CFLAGS) -c $<

clean:
	rm -f *o
	rm -f verkade
