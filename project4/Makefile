all: proj4 clean

proj4: main.o memAccess.o paging.o queue.o table.h
	gcc -o proj4 main.o memAccess.o paging.o queue.o -lpthread

main.o: main.c table.h
	gcc -c main.c -lpthread

memAccess.o: memAccess.c table.h
	gcc -c memAccess.c -lpthread

paging.o: paging.c table.h
	gcc -c paging.c -lpthread

queue.o: queue.c table.h
	gcc -c queue.c -lpthread

clean:
	rm *.o
