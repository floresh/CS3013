#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "queueInit.h"

#define THREADS 20
#define TIMESLICE 200

/* Global Variables */
sem_t clusterVal, tsSem, sleepingSem;
sem_t *thread_semaphores[THREADS];
queuePointer *looping_threads[THREADS];
int ready_threads[THREADS] = {0};
int uTickets = 0; int sTickets = 0;

void add_job(queuePointer *sPtr, queuePointer *ePtr, pthread_t newJob, int id, char *class);
queuePointer get_next_job(queuePointer *job);
void add_front(queuePointer *sPtr, pthread_t newJob);
void *create_thread(void *idPtr);
void run_job();
void naptime();
int run_lottery(int tickets);
int check_cluster(queuePointer *one, queuePointer *two);

int main(int argc, char *argv[]){
	/* Initialise the three queues*/
	queuePointer startTSPointer = NULL;
	queuePointer endTSPointer = NULL;

	queuePointer startSPointer = NULL;
	queuePointer endSPointer = NULL;

	queuePointer startUPointer = NULL;
	queuePointer endUPointer = NULL;

	// Seed rand
	srand(time(NULL));

	// Initialise other variables
	int i;
	pthread_t topSecret, secret, unclassified;
	char *tsClass = "TS";
	char *sClass = "S";
	char *uClass = "U";
	int uQueue = 0;
	int sQueue = 0;
	int tsQueue = 0;
	int numArray[THREADS];
	queuePointer *tempNode;

	/* Initialise a number array for thread creation */
	for(i = 0; i < THREADS; i++){
		numArray[i] = i;
	}

	// Initialise thread semaphores
	for(i = 0; i < THREADS; i++){
		thread_semaphores[i] = malloc(sizeof(sem_t));
		looping_threads[i] = malloc(sizeof(queuePointer));
		sem_init(thread_semaphores[i], 0, 0);
	}
	sem_init(&sleepingSem, 0, 0);

	/* Initialise unclassified threads */
	for(i = 0; i < 8; i++){
		pthread_create(&unclassified, NULL, create_thread, &numArray[i]);
		printf("Thread %d created.\n", i);
		add_job(&startUPointer, &endUPointer, unclassified, i, uClass);
		uQueue++;
	}

	/* Initialise secret threads */
	for(i = 0; i < 6; i++){
		pthread_create(&secret, NULL, create_thread, &numArray[i+8]);
		printf("Thread %d created.\n", (i+8));
		add_job(&startSPointer, &endSPointer, secret, (i+8), sClass);
		sQueue++;
	}

	/* Initialise top secret threads */
	for(i = 0; i < 6; i++){
		pthread_create(&topSecret, NULL, create_thread, &numArray[i+14]);
		printf("Thread %d created.\n", (i+14));
		add_job(&startTSPointer, &endTSPointer, topSecret, (i+14), tsClass);
		tsQueue++;
	}

	// Initialise semaphore and cluster variables
	sem_init(&clusterVal, 0, 2);
	sem_init(&tsSem, 0, 0);
	int clusterMode = 1; /* 1 is classified mode, 0 is unclassified */
	int halfOne = 0; int halfTwo = 0; /* 0 if a cluster half is empty, 1 if occupied */
	int jobIDOne, jobIDTwo; /* Record job IDs when in cluster */
	int counter = 0; /* Used to terminate the while loop after a while */
	int semValue, tsValue, jobOneVal, jobTwoVal;
	queuePointer clusterOne, clusterTwo; /* Used to hold jobs*/
	int lotteryNum; /* Used to pick what type of process goes */
	int clusterUsed = 1; /* Prevents printing when cluster was not used that cycle */
	int halfOneUsed = 1; int halfTwoUsed = 1; /* Indicates whether a half has been used or not. */

	/* Program runs indefinitely */
	while(counter < TIMESLICE){
		sem_getvalue(&clusterVal, &semValue); /* Gets cluster value*/
		sem_getvalue(&tsSem, &tsValue); /* Checks to see if a top secret job needs to go next*/

		if(semValue == 2){ // If cluster is empty
			if(tsValue == 2){ // check to see if we were already doing required ts jobs
				clusterOne = get_next_job(&startTSPointer);
				clusterTwo = get_next_job(&startTSPointer);
				printf("Job %d has entered cluster half 1 in classified [Top Secret] mode.\n", clusterOne->pid);
				printf("Job %d has entered cluster half 2 in classified [Top Secret] mode.\n", clusterTwo->pid);
				tsQueue--;
				tsQueue--;
				sem_wait(&clusterVal); // Down semaphore
				sem_wait(&clusterVal); // Down semaphore
				sem_wait(&tsSem); // Down TS waiting semaphore
				sem_wait(&tsSem); // Down TS waiting semaphore
				halfOne = 1; halfTwo = 1; // Indicate both halves are full
			}
			else if(tsValue == 1){
				clusterOne = get_next_job(&startTSPointer);
				printf("Job %d has entered cluster half 1 in classified [Top Secret] mode.\n", clusterOne->pid);
				tsQueue--;
				sem_wait(&clusterVal); // Down semaphore
				sem_wait(&tsSem); // Down TS waiting semaphore
				halfOne = 1;

				if(sQueue > tsQueue){
					clusterTwo = get_next_job(&startSPointer);
					printf("Job %d has entered cluster half 2 in classified [Secret] mode.\n", clusterTwo->pid);
					sQueue--;
					sem_wait(&clusterVal); // Down semaphore
					halfTwo = 1;
				}
				else{
					clusterTwo = get_next_job(&startTSPointer);
					printf("Job %d has entered cluster half 2 in classified [Top Secret] mode.\n", clusterTwo->pid);
					tsQueue--;
					sem_wait(&clusterVal); // Down semaphore
					halfTwo = 1;
				}
			}
			else{
				if(tsQueue >= 3){
					clusterMode = 1; // Ensure that mode is classified
					clusterOne = get_next_job(&startTSPointer);
					clusterTwo = get_next_job(&startTSPointer);
					printf("Job %d has entered cluster half 1 in classified [Top Secret] mode.\n", clusterOne->pid);
					printf("Job %d has entered cluster half 2 in classified [Top Secret] mode.\n", clusterTwo->pid);
					tsQueue--;
					tsQueue--;
					sem_wait(&clusterVal); // Down semaphore
					sem_wait(&clusterVal); // Down semaphore
					halfOne = 1; halfTwo = 1; // Indicate both halves are full
				}
				else{
					if(uQueue + sQueue + tsQueue != 0){ /* Ensures lottery doesn't run needlessly */
						lotteryNum = run_lottery(THREADS);
					}

					if(lotteryNum < 8){ //Unclassified job
						clusterMode = 0;
						if(uQueue >=2){
							clusterOne = get_next_job(&startUPointer);
							clusterTwo = get_next_job(&startUPointer);
							printf("Job %d has entered cluster half 1 in unclassified mode.\n", clusterOne->pid);
							printf("Job %d has entered cluster half 2 in unclassified mode.\n", clusterTwo->pid);
							uQueue--;
							uQueue--;
							sem_wait(&clusterVal); // Down semaphore
							sem_wait(&clusterVal); // Down semaphore
							halfOne = 1; halfTwo = 1; // Indicate both halves are full
						}
						else if(uQueue == 1){
							clusterOne = get_next_job(&startUPointer);
							printf("Job %d has entered cluster half 1 in unclassified mode.\n", clusterOne->pid);
							uQueue--;
							sem_wait(&clusterVal);
							halfOne = 1;

							halfTwoUsed = 0;
						}
						else{
							if(tsQueue + sQueue != 0){ // See if there are any jobs to pull
								clusterMode = 1;

								if(tsQueue + sQueue >= 2){ // Can fill both halves
									if(tsQueue > sQueue){ //Take a job from larger queue
										clusterOne = get_next_job(&startTSPointer);
										printf("Job %d has entered cluster half 1 in classified [Top Secret] mode.\n", clusterOne->pid);
										tsQueue--;
										sem_wait(&clusterVal);
										halfOne = 1;
									}
									else{
										clusterOne = get_next_job(&startSPointer);
										printf("Job %d has entered cluster half 1 in classified [Secret] mode.\n", clusterOne->pid);
										sQueue--;
										sem_wait(&clusterVal);
										halfOne = 1;
									}

									// Take another job from the larger queue
									if(tsQueue > sQueue){ //Take a job from larger queue
										clusterTwo = get_next_job(&startTSPointer);
										printf("Job %d has entered cluster half 2 in classified [Top Secret] mode.\n", clusterTwo->pid);
										tsQueue--;
										sem_wait(&clusterVal);
										halfTwo = 1;
									}
									else{
										clusterTwo = get_next_job(&startSPointer);
										printf("Job %d has entered cluster half 2 in classified [Secret] mode.\n", clusterTwo->pid);
										sQueue--;
										sem_wait(&clusterVal);
										halfTwo = 1;
									}
								}
								else if(tsQueue + sQueue == 1){ // Only one job available to pull
									if(tsQueue > sQueue){ //Take a job from larger queue
										clusterOne = get_next_job(&startTSPointer);
										printf("Job %d has entered cluster half 1 in classified [Top Secret] mode.\n", clusterOne->pid);
										tsQueue--;
										sem_wait(&clusterVal);
										halfOne = 1;
									}
									else{
										clusterOne = get_next_job(&startSPointer);
										printf("Job %d has entered cluster half 1 in classified [Secret] mode.\n", clusterOne->pid);
										sQueue--;
										sem_wait(&clusterVal);
										halfOne = 1;
									}

									halfTwoUsed = 0;
								}

							}
							else{
								clusterUsed = 0;
							}
						}
					}
					else{ //Classified job
						clusterMode = 1;
						
						if(tsQueue + sQueue >= 2){ // Can fill both halves
							if(tsQueue > sQueue){ //Take a job from larger queue
								clusterOne = get_next_job(&startTSPointer);
								printf("Job %d has entered cluster half 1 in classified [Top Secret] mode.\n", clusterOne->pid);
								tsQueue--;
								sem_wait(&clusterVal);
								halfOne = 1;
							}
							else{
								clusterOne = get_next_job(&startSPointer);
								printf("Job %d has entered cluster half 1 in classified [Secret] mode.\n", clusterOne->pid);
								sQueue--;
								sem_wait(&clusterVal);
								halfOne = 1;
							}

							// Take another job from the larger queue
							if(tsQueue > sQueue){ //Take a job from larger queue
								clusterTwo = get_next_job(&startTSPointer);
								printf("Job %d has entered cluster half 2 in classified [Top Secret] mode.\n", clusterTwo->pid);
								tsQueue--;
								sem_wait(&clusterVal);
								halfTwo = 1;
							}
							else{
								clusterTwo = get_next_job(&startSPointer);
								printf("Job %d has entered cluster half 2 in classified [Secret] mode.\n", clusterTwo->pid);
								sQueue--;
								sem_wait(&clusterVal);
								halfTwo = 1;
							}
						}
						else if(tsQueue + sQueue == 1){ // Only one job available to pull
							if(tsQueue > sQueue){ //Take a job from larger queue
								clusterOne = get_next_job(&startTSPointer);
								printf("Job %d has entered cluster half 1 in classified [Top Secret] mode.\n", clusterOne->pid);
								tsQueue--;
								sem_wait(&clusterVal);
								halfOne = 1;
							}
							else{
								clusterOne = get_next_job(&startSPointer);
								printf("Job %d has entered cluster half 1 in classified [Secret] mode.\n", clusterOne->pid);
								sQueue--;
								sem_wait(&clusterVal);
								halfOne = 1;
							}

							halfTwoUsed = 0;
						}
						else{
							if(uQueue != 0){
								clusterMode = 0; //set back to unclassified mode

								if(uQueue >=2){
									clusterOne = get_next_job(&startUPointer);
									clusterTwo = get_next_job(&startUPointer);
									printf("Job %d has entered cluster half 1 in unclassified mode.\n", clusterOne->pid);
									printf("Job %d has entered cluster half 2 in unclassified mode.\n", clusterTwo->pid);
									uQueue--;
									uQueue--;
									sem_wait(&clusterVal); // Down semaphore
									sem_wait(&clusterVal); // Down semaphore
									halfOne = 1; halfTwo = 1; // Indicate both halves are full
								}
								else if(uQueue == 1){
									clusterOne = get_next_job(&startUPointer);
									printf("Job %d has entered cluster half 1 in unclassified mode.\n", clusterOne->pid);
									uQueue--;
									sem_wait(&clusterVal);
									halfOne = 1;

									halfTwoUsed = 0;
								}

							}
							else{
								clusterUsed = 0;
							}
						}
					}
				}
			}



			/* Running jobs and cleaning up here!!! */
			jobIDOne = clusterOne->pid;
			jobIDTwo = clusterTwo->pid;

			// Check validity
			if(halfOne == 1 && halfTwo == 1){
				if(check_cluster(&clusterOne, &clusterTwo) == -1){
					printf("An unclassified job has run with a classified one! Exiting.\n");
					exit(-1);
				}
			}
			
			if(halfOne == 1){
				sem_post(thread_semaphores[jobIDOne]); // Wake up thread so it runs its sleeping job
			}
			if(halfTwo == 1){
				sem_post(thread_semaphores[jobIDTwo]);
			}
			
			if(halfOne + halfTwo == 0){
				//Do not wait
			}
			else if(halfOne + halfTwo == 1){
				sem_wait(&sleepingSem);
			}
			else{
				sem_wait(&sleepingSem); //Cause process to wait until thread runs
				sem_wait(&sleepingSem); //Cause process to wait until thread runs
			}

			sem_getvalue(thread_semaphores[jobIDOne], &jobOneVal);
			sem_getvalue(thread_semaphores[jobIDTwo], &jobTwoVal);
			if(jobOneVal == 0 && clusterUsed == 1 && halfOneUsed == 1){ // Check that threads finished their naptime
				sem_post(&clusterVal);
				*looping_threads[clusterOne->pid] = clusterOne;
				halfOne = 0;
				printf("Job %d has left cluster half 1.\n", clusterOne->pid);
				counter++;
			}
			if(jobTwoVal == 0 && clusterUsed == 1 && halfTwoUsed == 1){
				sem_post(&clusterVal);
				*looping_threads[clusterTwo->pid] = clusterTwo;
				halfTwo = 0;
				printf("Job %d has left cluster half 2.\n", clusterTwo->pid);
				counter++;
			}

			if(clusterUsed == 1 && (jobOneVal == 0 || jobTwoVal == 0)){
				printf("\n"); //Newline for clarity
			}

			clusterUsed = 1;
			halfOneUsed = 1;
			halfTwoUsed = 1;
			/* Done running jobs, ready for next cycle*/



		}
		else if(semValue == 1){ // If one half of cluster is full
			if(clusterMode == 1){ // Checks for unclassified or classified mode
				if(tsQueue != 0 || sQueue != 0){  // Check if there is anything to pull
					if(tsQueue >= 3){
						sem_post(&tsSem);

						if(halfOne == 1){
							clusterTwo = get_next_job(&startTSPointer);
							printf("Job %d has entered cluster half 2 in classified [Top Secret] mode.\n", clusterTwo->pid);
							tsQueue--;
							sem_wait(&clusterVal); // Down semaphore
							halfTwo = 1;
						}
						else{
							clusterOne = get_next_job(&startTSPointer);
							printf("Job %d has entered cluster half 1 in classified [Top Secret] mode.\n", clusterOne->pid);
							tsQueue--;
							sem_wait(&clusterVal); // Down semaphore
							halfOne = 1;
						}
					}
					else{
						if(halfOne == 1){ // Determine which half to put a new job in
							if(tsValue > 0){ // Check if a TS job needs to run
								clusterTwo = get_next_job(&startTSPointer);
								printf("Job %d has entered cluster half 2 in classified [Top Secret] mode.\n", clusterTwo->pid);
								tsQueue--;
								sem_wait(&clusterVal); // Down semaphore
								sem_wait(&tsSem); // Down TS waiting semaphore
								halfTwo = 1;
							}
							else if(sQueue + tsQueue == 0){
								halfTwoUsed = 0;
							}
							else if(sQueue > tsQueue){ // Determine which job to take next
								clusterTwo = get_next_job(&startSPointer);
								printf("Job %d has entered cluster half 2 in classified [Secret] mode.\n", clusterTwo->pid);
								sQueue--;
								sem_wait(&clusterVal); // Down semaphore
								halfTwo = 1;
							}
							else{
								clusterTwo = get_next_job(&startTSPointer);
								printf("Job %d has entered cluster half 2 in classified [Top Secret] mode.\n", clusterTwo->pid);
								tsQueue--;
								sem_wait(&clusterVal); // Down semaphore
								halfTwo = 1;
							}
						}
						else if(halfTwo == 1){
							if(tsValue > 0){ // Check if a TS job needs to run
								clusterOne = get_next_job(&startTSPointer);
								printf("Job %d has entered cluster half 1 in classified [Top Secret] mode.\n", clusterOne->pid);
								tsQueue--;
								sem_wait(&clusterVal); // Down semaphore
								sem_wait(&tsSem); // Down TS waiting semaphore
								halfOne = 1;
							}
							else if(sQueue + tsQueue == 0){
								halfOneUsed = 0;
							}
							else if(sQueue > tsQueue){ // Determine which job to take next
								clusterOne = get_next_job(&startSPointer);
								printf("Job %d has entered cluster half 1 in classified [Secret] mode.\n", clusterOne->pid);
								sQueue--;
								sem_wait(&clusterVal); // Down semaphore
								halfOne = 1;
							}
							else{
								clusterOne = get_next_job(&startTSPointer);
								printf("Job %d has entered cluster half 1 in classified [Top Secret] mode.\n", clusterOne->pid);
								tsQueue--;
								sem_wait(&clusterVal); // Down semaphore
								halfOne = 1;
							}
						}
					}
				}
			}
			else{
				if(uQueue != 0){ // Check if there is anything to pull
					if(halfOne == 1){ // Determine which half to put a new job in
						clusterTwo = get_next_job(&startUPointer);
						printf("Job %d has entered cluster half 2 in unclassified mode.\n", clusterTwo->pid);
						uQueue--;
						sem_wait(&clusterVal); // Down semaphore
						halfTwo = 1;
					}
					else if(halfTwo == 1){
						clusterOne = get_next_job(&startUPointer);
						printf("Job %d has entered cluster half 1 in unclassified mode.\n", clusterOne->pid);
						uQueue--;
						sem_wait(&clusterVal); // Down semaphore
						halfOne = 1;
					}
				}
				else{
					if(halfOne == 1){
						halfTwoUsed = 0;
					}
					else{
						halfOneUsed = 0;
					}
				}
			}



			/* Running jobs and cleaning up here!!! */
			jobIDOne = clusterOne->pid;
			jobIDTwo = clusterTwo->pid;

			// Check validity
			if(halfOne == 1 && halfTwo == 1){
				if(check_cluster(&clusterOne, &clusterTwo) == -1){
					printf("An unclassified job has run with a classified one! Exiting.\n");
					exit(-1);
				}
			}

			if(halfOne == 1){
				sem_post(thread_semaphores[jobIDOne]); // Wake up thread so it runs its sleeping job
			}
			if(halfTwo == 1){
				sem_post(thread_semaphores[jobIDTwo]);
			}

			if(halfOne + halfTwo == 0){
				//Do not wait
			}
			else if(halfOne + halfTwo == 1){
				sem_wait(&sleepingSem);
			}
			else{
				sem_wait(&sleepingSem); //Cause process to wait until thread runs
				sem_wait(&sleepingSem); //Cause process to wait until thread runs
			}

			sem_getvalue(thread_semaphores[jobIDOne], &jobOneVal);
			sem_getvalue(thread_semaphores[jobIDTwo], &jobTwoVal);
			if(jobOneVal == 0 && clusterUsed == 1 && halfOneUsed == 1){ // Check that threads finished their naptime
				sem_post(&clusterVal);
				*looping_threads[clusterOne->pid] = clusterOne;
				halfOne = 0;
				printf("Job %d has left cluster half 1.\n", clusterOne->pid);
				counter++;
			}
			if(jobTwoVal == 0 && clusterUsed == 1 && halfTwoUsed == 1){
				sem_post(&clusterVal);
				*looping_threads[clusterTwo->pid] = clusterTwo;
				halfTwo = 0;
				printf("Job %d has left cluster half 2.\n", clusterTwo->pid);
				counter++;
			}

			if(clusterUsed == 1 && (jobOneVal == 0 || jobTwoVal == 0)){
				printf("\n"); //Newline for clarity
			}

			clusterUsed = 1;
			halfOneUsed = 1;
			halfTwoUsed = 1;
			/* Done running jobs, ready for next cycle*/



		}
		else if(semValue == 0){



			/* Running jobs and cleaning up here!!! */
			jobIDOne = clusterOne->pid;
			jobIDTwo = clusterTwo->pid;

			if(halfOne == 1){
				sem_post(thread_semaphores[jobIDOne]); // Wake up thread so it runs its sleeping job
			}
			if(halfTwo == 1){
				sem_post(thread_semaphores[jobIDTwo]);
			}
			
			// Check validity
			if(halfOne == 1 && halfTwo == 1){
				if(check_cluster(&clusterOne, &clusterTwo) == -1){
					printf("An unclassified job has run with a classified one! Exiting.\n");
					exit(-1);
				}
			}

			if(halfOne + halfTwo == 0){
				//Do not wait
			}
			else if(halfOne + halfTwo == 1){
				sem_wait(&sleepingSem);
			}
			else{
				sem_wait(&sleepingSem); //Cause process to wait until thread runs
				sem_wait(&sleepingSem); //Cause process to wait until thread runs
			}

			sem_getvalue(thread_semaphores[jobIDOne], &jobOneVal);
			sem_getvalue(thread_semaphores[jobIDTwo], &jobTwoVal);
			if(jobOneVal == 0 && clusterUsed == 1 && halfOneUsed == 1){ // Check that threads finished their naptime
				sem_post(&clusterVal);
				*looping_threads[clusterOne->pid] = clusterOne;
				halfOne = 0;
				printf("Job %d has left cluster half 1.\n", clusterOne->pid);
				counter++;
			}
			if(jobTwoVal == 0 && clusterUsed == 1 && halfTwoUsed == 1){
				sem_post(&clusterVal);
				*looping_threads[clusterTwo->pid] = clusterTwo;
				halfTwo = 0;
				printf("Job %d has left cluster half 2.\n", clusterTwo->pid);
				counter++;
			}

			if(clusterUsed == 1 && (jobOneVal == 0 || jobTwoVal == 0)){
				printf("\n"); //Newline for clarity
			}

			clusterUsed = 1;
			halfOneUsed = 1;
			halfTwoUsed = 1;
			/* Done running jobs, ready for next cycle*/



		}

		for(i = 0; i < THREADS; i++){
			if(ready_threads[i] == 1){
				//printf("Thread %d is ready to queue up again!\n", i); // Debug
				tempNode = looping_threads[i];

				if(i < 8){ // Unclassified Job
					add_job(&startUPointer, &endUPointer, (*tempNode)->job, (*tempNode)->pid, (*tempNode)->classification);
					ready_threads[i] = 0;
					uQueue++;
				}
				else if(i >=8 && i < 14){ // Secret Job
					add_job(&startSPointer, &endSPointer, (*tempNode)->job, (*tempNode)->pid, (*tempNode)->classification);
					ready_threads[i] = 0;
					sQueue++;
				}
				else{ //Top Secret Job
					add_job(&startTSPointer, &endTSPointer, (*tempNode)->job, (*tempNode)->pid, (*tempNode)->classification);
					ready_threads[i] = 0;
					tsQueue++;
				}
			}
		}
	}

	return 0;
}

/* Adds a job to the end of the queue. */
void add_job(queuePointer *sPtr, queuePointer *ePtr, pthread_t newJob, int id, char *class){
	queuePointer newPtr;
	queuePointer prevPtr;
	queuePointer currentPtr;

	newPtr = malloc(sizeof(Queue));

	if(newPtr){
		newPtr->job = newJob;
		newPtr->pid = id;
		newPtr->classification = class;
		newPtr->nextPtr = NULL;
		prevPtr = NULL;

		currentPtr = *sPtr;

		while(currentPtr != NULL){
			prevPtr = currentPtr;
			currentPtr = currentPtr->nextPtr;
		}

		if(prevPtr == NULL){
			newPtr->nextPtr = *sPtr;
			*sPtr = newPtr;
		}
		else{
			prevPtr->nextPtr = newPtr;
			newPtr->nextPtr = currentPtr;
		}

		if(newPtr->nextPtr == NULL){
			*ePtr = newPtr;
		}
	}
	else{
		// Failed to add job. Exit function.
		printf("Failed to add job.\n");
	}

	return;
}

/* Adds a job to the front of a queue. */
void add_front(queuePointer *sPtr, pthread_t newJob){
	queuePointer newPtr;
	queuePointer prevPtr;
	queuePointer currentPtr;

	newPtr = malloc(sizeof(Queue));

	if(newPtr){
		newPtr->job = newJob;
		newPtr->nextPtr = NULL;
		prevPtr = NULL;

		currentPtr = *sPtr;

		while(currentPtr != NULL){
			prevPtr = currentPtr;
			currentPtr = currentPtr->nextPtr;
		}

		/* Adds to beginning */
		newPtr->nextPtr = *sPtr;
		*sPtr = newPtr;
	}
	else{
		// Failed to add job. Exit function.
		printf("Failed to add job.\n");
	}

	return;
}

/* Pulls the first job from the queue. */
queuePointer get_next_job(queuePointer *job){
	queuePointer tempPtr;

	if(*job == NULL){
		printf("No process found!\n");

		return NULL;
	}
	else{
		tempPtr = *job;
		*job = (*job)->nextPtr;

		return tempPtr;
	}
}

/* Thread creation function */
void *create_thread(void *idPtr){

	int id = *((int *)idPtr); // Copies ID

	sem_wait(thread_semaphores[id]); // Block thread

	while(1){
		run_job();
		sem_post(&sleepingSem);

		naptime(); // Sleep for an arbitrary amt of time
		ready_threads[id] = 1; // Indicate that it is ready to go back into the queue
		sem_wait(thread_semaphores[id]);
	}
}

/* Simulation for running a job */
void run_job(){
	int randomNumber;

	randomNumber = ((rand()%1750000) + 250000);

	usleep(randomNumber);
	//printf("Thread has finished sleeping.\n"); // Debug
}

/* Makes threads sleep for a while before going back to queue */
void naptime(){
	int randomNum;

	randomNum = ((rand()%5) + 10);

	sleep(randomNum);
}

/* Randomly selects a 'ticket' from the alloted pool */
int run_lottery(int tickets){
	int selected;

	selected = rand()%tickets;

	//printf("The ticket is %d\n", selected);

	if(selected < 8 && (uTickets - sTickets) > 5){
		selected = 0;
	}
	else if(selected > 7 && (sTickets - uTickets) > 5){
		selected = 10;
	}

	if(selected < 8){
		uTickets++;
	}
	else{
		sTickets++;
	}

	return selected;
}

int check_cluster(queuePointer *one, queuePointer *two){
	char *oneClass = (*one)->classification;
	char *twoClass = (*two)->classification;

	if(strcmp(oneClass, "U") == 0){ // Check if half one is running an unclasified job
		if(strcmp(twoClass, "U") != 0){
			return -1;
		}
	}
	else{ // Half one is running a classified job
		if(strcmp(twoClass, "U") == 0){ // Check if half two is running an unclassified job
			return -1;
		}
	}

	return 0;
}