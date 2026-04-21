CXX = g++
CFLAGS = -Wall -Wextra -O2 -g -Wno-deprecated-declarations -std=c++17
LDFLAGS = -pthread

all: demo benchmark dining

demo: demo.o vortex.o scheduler.o sync.o vortex_context.o
	$(CXX) $(CFLAGS) -o demo demo.o vortex.o scheduler.o sync.o vortex_context.o

benchmark: benchmark.o vortex.o scheduler.o sync.o vortex_context.o
	$(CXX) $(CFLAGS) -o benchmark benchmark.o vortex.o scheduler.o sync.o vortex_context.o $(LDFLAGS)

dining: dining.o vortex.o scheduler.o sync.o vortex_context.o
	$(CXX) $(CFLAGS) -o dining dining.o vortex.o scheduler.o sync.o vortex_context.o

demo.o: demo.cpp vortex.h
	$(CXX) $(CFLAGS) -c demo.cpp

benchmark.o: benchmark.cpp vortex.h
	$(CXX) $(CFLAGS) -c benchmark.cpp

dining.o: dining.cpp vortex.h
	$(CXX) $(CFLAGS) -c dining.cpp

vortex.o: vortex.cpp vortex.h vortex_internal.h
	$(CXX) $(CFLAGS) -c vortex.cpp

scheduler.o: scheduler.cpp vortex.h vortex_internal.h
	$(CXX) $(CFLAGS) -c scheduler.cpp

sync.o: sync.cpp vortex.h vortex_internal.h
	$(CXX) $(CFLAGS) -c sync.cpp

vortex_context.o: vortex_context.S
	$(CXX) $(CFLAGS) -c vortex_context.S

clean:
	del /Q *.o demo.exe benchmark.exe dining.exe 2>NUL
