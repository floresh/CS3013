make: queue clean

queue: queue.o queueInit.h
	gcc -o queue queue.c -lpthread

queue.o: queue.c queueInit.h
	gcc -c queue.c -lpthread

clean:
	rm *.o