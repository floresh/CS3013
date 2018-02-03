#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

#include "table.h"

int pages = 0;

void memoryMaxer();
void exceedingMemoryMaxer();
void lockedMemoryMaxer();
void threadedMemory();
void *create_thread(void *void_ptr);

int main(int argc, char *argv[]){
	int i;

	if(argc != 3){ // Exit if wrong number of arguments
		printf("Invalid argument count! Format is ./proj4 [threadMode] [testMode]\n");
		return -1;
	}

	// All start empty
	RAMFull = 0;
	SSDFull = 0;
	HDiskFull = 0;

	//Init addr ints
	for(i = 0; i < 1000; i++){
		addr[i] = i;
	}

	// Initialize arrays
	for(i = 0; i < 25; i++){
		RAM[i] = -1;
	}
	for(i = 0; i < 100; i++){
		SSD[i] = -1;
	}
	for(i = 0; i < 1000; i++){
		hDisk[i] = -1;
	}

	for(i = 0; i < 1000; i++){
		// Allocate space and set to sentinel specifications
		pageTable[i] = malloc(sizeof(pageNode));
		pageTable[i]->locked = 0;
		pageTable[i]->allocated = 0;
		pageTable[i]->modified = 0;
		pageTable[i]->accessed = -1;
		pageTable[i]->level = -1;
		pageTable[i]->location = -1;
	}

	// Initialize semaphores
	for(i = 0; i < 25; i++){
		semRAM[i] = malloc(sizeof(sem_t));
		sem_init(semRAM[i], 0, 1);
	}
	for(i = 0; i < 100; i++){
		semSSD[i] = malloc(sizeof(sem_t));
		sem_init(semSSD[i], 0, 1);
	}
	for(i = 0; i < 1000; i++){
		semHDISK[i] = malloc(sizeof(sem_t));
		semTABLE[i] = malloc(sizeof(sem_t));
		sem_init(semHDISK[i], 0, 1);
		sem_init(semTABLE[i], 0, 1);
	}

	sem_init(&ram_index, 0, 1);
	sem_init(&ssd_index, 0, 1);
	sem_init(&hdisk_index, 0, 1);
	sem_init(&ram_queue, 0, 1);
	sem_init(&ssd_queue, 0, 1);
	sem_init(&hdisk_queue, 0, 1);
	sem_init(&use_variable, 0, 1);
	sem_init(&make_addr, 0 , 1);


	// Run testing function
	if(atoi(argv[1]) == 0){
		if(atoi(argv[2]) == 0){ // Check testing
			memoryMaxer();
		}
		else if(atoi(argv[2]) == 1){
			exceedingMemoryMaxer();
		}
		else{
			lockedMemoryMaxer();
		}
	}
	else{
		threadedMemory();
	}
	
	return 0;
}

void memoryMaxer(){
	int i;
	vAddr indexes[1000];
	for(i = 0; i < 1000; i++){
		indexes[i] = allocateNewInt();

		if(DEBUG && indexes[i] != -1){
			printf("Address %d has been created.\n", indexes[i]); // Debug
		}

		if(indexes[i] != -1){
			int *value = accessIntPtr(indexes[i]);
			if(value != NULL) {*value = (i * 3);} // Only if an address was returned
			unlockMemory(indexes[i]);
			printf("\n"); //Spacing in debug
		}
	}
	for(i = 0; i < 1000; i++){
		if(indexes[i] != -1){
			freeMemory(indexes[i]);
		}

	}
}

void threadedMemory(){
	pthread_t threads[5];
	int i;

	for(i = 0; i < 5; i++){
		pthread_create(&threads[i], NULL, create_thread, NULL);
	}

	for(i = 0; i < 5; i++){
		pthread_join(threads[i], NULL);
	}
}

void *create_thread(void *void_ptr){
	int i;
	vAddr indexes[200];

	for(i = 0; i < 200; i++){
		sem_wait(&make_addr);
		indexes[i] = allocateNewInt();
		sem_post(&make_addr);

		if(DEBUG && indexes[i] != -1){
			printf("Address %d has been created.\n", indexes[i]); // Debug
		}

		if(indexes[i] != -1){
			int *value = accessIntPtr(indexes[i]);
			if(value != NULL) {*value = (i * 3);} // Only if an address was returned
			unlockMemory(indexes[i]);
			printf("\n"); //Spacing in debug
		}

	}
	for(i = 0; i < 200; i++){
		if(indexes[i] != -1){
			freeMemory(indexes[i]);
		}

	}
}

// Test that it will not make more than 1000 pages
void exceedingMemoryMaxer(){
	int i;
	vAddr indexes[1001];
	for(i = 0; i < 1001; i++){
		indexes[i] = allocateNewInt();

		if(DEBUG && indexes[i] != -1){
			printf("Address %d has been created.\n", indexes[i]); // Debug
		}

		if(indexes[i] != -1){
			int *value = accessIntPtr(indexes[i]);
			if(value != NULL) {*value = (i * 3);} // Only if an address was returned
			unlockMemory(indexes[i]);
			printf("\n"); //Spacing in debug
		}
	}
	for(i = 0; i < 1000; i++){
		if(indexes[i] != -1){
			freeMemory(indexes[i]);
		}

	}
}

// Tests that it will not break if it cannot evict pages (as all are locked)
void lockedMemoryMaxer(){
	int i;
	vAddr indexes[1000];
	for(i = 0; i < 1000; i++){
		indexes[i] = allocateNewInt();

		if(DEBUG && indexes[i] != -1){
			printf("Address %d has been created.\n", indexes[i]); // Debug
		}

		if(indexes[i] != -1){
			int *value = accessIntPtr(indexes[i]);
			if(value != NULL) {*value = (i * 3);} // Only if an address was returned
			//unlockMemory(indexes[i]);
			printf("\n"); //Spacing in debug
		}
	}
	for(i = 0; i < 1000; i++){
		if(indexes[i] != -1){
			freeMemory(indexes[i]);
		}

	}
}