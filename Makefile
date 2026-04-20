CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -Wno-deprecated-declarations
LDFLAGS = -pthread

all: demo benchmark dining

demo: demo.o vortex.o scheduler.o sync.o
	$(CC) $(CFLAGS) -o demo demo.o vortex.o scheduler.o sync.o

benchmark: benchmark.o vortex.o scheduler.o sync.o
	$(CC) $(CFLAGS) -o benchmark benchmark.o vortex.o scheduler.o sync.o $(LDFLAGS)

dining: dining.o vortex.o scheduler.o sync.o
	$(CC) $(CFLAGS) -o dining dining.o vortex.o scheduler.o sync.o

demo.o: demo.c vortex.h
	$(CC) $(CFLAGS) -c demo.c

benchmark.o: benchmark.c vortex.h
	$(CC) $(CFLAGS) -c benchmark.c

dining.o: dining.c vortex.h
	$(CC) $(CFLAGS) -c dining.c

vortex.o: vortex.c vortex.h vortex_internal.h
	$(CC) $(CFLAGS) -c vortex.c

scheduler.o: scheduler.c vortex.h vortex_internal.h
	$(CC) $(CFLAGS) -c scheduler.c

sync.o: sync.c vortex.h vortex_internal.h
	$(CC) $(CFLAGS) -c sync.c

clean:
	rm -f *.o demo benchmark dining
