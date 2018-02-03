#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "table.h"

/* Adds a job to the end of the queue. */
void add_job(queuePointer *sPtr, queuePointer *ePtr, pagePtr page, int index){
	queuePointer newPtr;
	queuePointer prevPtr;
	queuePointer currentPtr;

	newPtr = malloc(sizeof(Queue));

	if(newPtr){
		newPtr->page = page;
		newPtr->index = index;
		newPtr->nextPtr = NULL;
		prevPtr = NULL;

		currentPtr = *sPtr;

		while(currentPtr != NULL && page->location != currentPtr->page->location){
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

queuePointer pullNode(queuePointer *sPtr, queuePointer *ePtr, vAddr address){
	queuePointer prevPtr;
	queuePointer currentPtr;
	queuePointer tempPtr;

	currentPtr = *sPtr;

	// Get to node we want to remove
	while(currentPtr != NULL && address != currentPtr->page->location){
		prevPtr = currentPtr;
		currentPtr = currentPtr->nextPtr;
	}

	if(prevPtr == NULL){ // If at beginning
		tempPtr = *sPtr; //Copy beginning pointer
		*sPtr = currentPtr->nextPtr; // Set beginning pointer to next pointer
	}
	else{ // Remove from within the queue
		tempPtr = currentPtr;
		prevPtr->nextPtr = currentPtr->nextPtr;
		tempPtr->nextPtr = NULL;
		currentPtr->nextPtr = NULL;
	}

	return tempPtr;
}

int contains(queuePointer *sPtr, queuePointer *ePtr, vAddr address){
	queuePointer prevPtr;
	queuePointer currentPtr;
	queuePointer tempPtr;

	currentPtr = *sPtr;

	while(currentPtr != NULL && address != currentPtr->page->location){
		prevPtr = currentPtr;
		currentPtr = currentPtr->nextPtr;
	}

	if(currentPtr == NULL){
		return 0;
	}

	return 1;
}

int all_locked(queuePointer *sPtr, queuePointer *ePtr){
	queuePointer prevPtr;
	queuePointer currentPtr;
	queuePointer tempPtr;

	currentPtr = *sPtr;

	while(currentPtr != NULL && currentPtr->page->locked == 1){
		prevPtr = currentPtr;
		currentPtr = currentPtr->nextPtr;
	}

	if(currentPtr == NULL){
		return 1;
	}

	return 0;
}