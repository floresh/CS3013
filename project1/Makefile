all: runCommand shell shell2
	make clean

clean:
	rm *.o

runCommand.o: phase1.c
	gcc -c phase1.c -o runCommand.o

runCommand: runCommand.o
	gcc runCommand.o -o runCommand

shell.o: phase2.c
	gcc -c phase2.c -o shell.o

shell: shell.o
	gcc shell.o -o shell

shell2.o: phase3.c
	gcc -c phase3.c -o shell2.o

shell2: shell2.o
	gcc shell2.o -o shell2
