# Project 3:

 - Author: Robyn Domanico, Heric Flores-Huerta
 - CS Class: CS3013- Operating Systems
 - Date: 2/17/2015
 - Programming Language: C
 - OS/Hardware Dependencies: Linux


### Problem Description:

Part 1 is simulating the SecuriOS cluster, and essentially runs a series of jobs in two cluster halves at the same time, not allowing unclassified jobs and classified jobs to mix. Part 2 is simulating an intersection, and must schedule the cars in  order to prevent deadlock or starvation of a lane.

### Overall Design:

In part 1, we create 20 threads that each manage a job. The threads have security levels cannot run with conflicting levels. The program uses semaphores to prevent race conditions. In part 2, we create 20 car threads that are each individually managed by a thread. The cars must be schedules so as to prevent deadlock, and use mutexes/condition variables to prevent race conditions.

### Interfaces:

All input is through the command line.

### Implementation Details:	

Phase 1 creates 20 threads and places them in one of three queues based on their classification level. If there are 3 or more top secret jobs waiting, at least 2 are run first. Otherwise, the cluster will try to fill empty spots if it can. If the cluster is empty, the program uses a pseudo-random function to decide which types of jobs the cluster will take next. When jobs are run, a semaphore that the respective thread is waiting on is upped and the thread runs its job. Threads are able to loop, allowing the program to run indefinitely. Phase 2 creates 20 threads and places them in one of four queues, based on a pseudo-randomly generated number. 5 priority vehicles are also initialized in a motorcade and placed in the front on one queue. The program then checks to see if a priority vehicle needs to go , and allows it to go if there is. If not, the program checks for a deadlock condition. If a potential deadlock is found, one randomly selected lane waits. Otherwise, a car from all 4 lanes is allowed to go. The threads that run these are signaled by a condition variable, allowing the specified car to proceed. The cars then wait a specified amount of time before receiving a new direction and approach value, and then requeues.

### How to Build the Program:

To build phase 1, run the makefile in the phase 1 folder. To build phase 2, run the makefile in the phase 2 folder.

### Additional Notes:

Our phase 1 does not appear to have any problems, but our phase 2 has some issues: 5 cars in a motorcade are created, but because not all of the motorcade cars finish waiting at the same time, they are reassigned different values and become more than one motorcade, or one or two emergency vehicles.
