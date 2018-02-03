all: phase2 clean

phase2: queue.o phase2.o queueInit.h
	gcc -o phase2 phase2.o queue.o -lpthread

queue.o: queue.c queueInit.h
	gcc -c queue.c -lpthread

phase2.o: phase2.c queueInit.h
	gcc -c phase2.c -lpthread

clean:
	rm *.o
