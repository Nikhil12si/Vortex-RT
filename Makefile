CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -Wno-deprecated-declarations
LDFLAGS = -pthread

all: demo benchmark

demo: demo.o vortex.o scheduler.o
	$(CC) $(CFLAGS) -o demo demo.o vortex.o scheduler.o

benchmark: benchmark.o vortex.o scheduler.o
	$(CC) $(CFLAGS) -o benchmark benchmark.o vortex.o scheduler.o $(LDFLAGS)

demo.o: demo.c vortex.h
	$(CC) $(CFLAGS) -c demo.c

benchmark.o: benchmark.c vortex.h
	$(CC) $(CFLAGS) -c benchmark.c

vortex.o: vortex.c vortex.h vortex_internal.h
	$(CC) $(CFLAGS) -c vortex.c

scheduler.o: scheduler.c vortex.h vortex_internal.h
	$(CC) $(CFLAGS) -c scheduler.c

clean:
	rm -f *.o demo benchmark
