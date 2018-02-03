#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

#include "table.h"

#define RAM_SLEEP 0
#define SSD_SLEEP 250000
#define HDISK_SLEEP 2500000

// Use second chance as one algorithm
int second_chance(queuePointer *sOldQueue, queuePointer *eOldQueue,
	queuePointer *sNewQueue, queuePointer *eNewQueue){

	queuePointer tempPtr = malloc(sizeof(Queue));
	int level;
	int index;
	int newLevel;
	int newIndex;
	int timeout = 0; //Timeout after a certain point, all must be locked
	int semLevel; // Used to figure out which semaphores to lock

	int evicted = 0; // Set to 1 once a page has been swapped out.
	vAddr address; // The address that is swapped out

	// Should NEVER try to evict from an empty queue, catch if program does
	if(sOldQueue == NULL){
		return;
	}

	while(!evicted){
		tempPtr = get_next_job(sOldQueue);

		// Check the accessed bit in first page in old queue
		if((tempPtr->page->accessed == 1 || tempPtr->page->locked == 1) && timeout < 1010){
			tempPtr->page->accessed = 0; // Reset accessed bit
			sem_wait(semTABLE[tempPtr->page->location]);
			pageTable[tempPtr->page->location]->accessed = 0; // Update in page table
			sem_post(semTABLE[tempPtr->page->location]);
			add_job(sOldQueue, eOldQueue, tempPtr->page, tempPtr->index); // Add to end
			timeout++;
		}
		else{
			if(timeout >= 1010){
				if(DEBUG){
					printf("The queue timed out!\n");
				}
				return -1;
			}

			add_job(sNewQueue, eNewQueue, tempPtr->page, tempPtr->index); // Add to new queue

			// Get info to move in memory tables and update in page table
			level = tempPtr->page->level;
			index = findIndex(tempPtr->page->location, level);
			address = tempPtr->page->location;

			// Move in memory tables
			if(level == 0){

				newLevel = 1;
				newIndex = find_empty_spot(newLevel);

				if(FAST_RUN == 0){
					usleep(SSD_SLEEP);
				}

				sem_wait(semSSD[newIndex]);
				SSD[newIndex] = address;
				sem_post(semSSD[newIndex]);
				sem_wait(semRAM[index]);
				RAM[index] = -1;
				sem_post(semRAM[index]);

				// Update page table
				sem_wait(semTABLE[address]);
				pageTable[address]->level = 1;
				sem_post(semTABLE[address]);

				if(DEBUG){
					printf("Page %d has been moved down to level %d.\n", address, newLevel); //Debug
				}
			}
			else if(level == 1){
				newLevel = 2;
				newIndex = find_empty_spot(newLevel);

				if(FAST_RUN == 0){
					usleep(SSD_SLEEP + HDISK_SLEEP);
				}

				sem_wait(semHDISK[newIndex]);
				hDisk[newIndex] = address;
				sem_post(semHDISK[newIndex]);
				sem_wait(semSSD[index]);
				SSD[index] = -1;
				sem_post(semSSD[index]);

				// Update page table
				sem_wait(semTABLE[address]);
				pageTable[address]->level = 2;
				sem_post(semTABLE[address]);

				if(DEBUG){
					printf("Page %d has been moved down to level %d.\n", address, newLevel); //Debug
				}
			}

			evicted = 1;
		}

	}

	return 0;
}

// Manages LRU queues and evicts a page when needed.
void LRU(queuePointer *sOldQueue, queuePointer *eOldQueue,
	queuePointer *sNewQueue, queuePointer *eNewQueue, vAddr address, int level){

	queuePointer tempPtr = malloc(sizeof(Queue));
	pagePtr newPage = malloc(sizeof(pageNode));

	int oldIndex;
	int newIndex;

	if(contains(sOldQueue, eOldQueue, address)){
		tempPtr = pullNode(sOldQueue, eOldQueue, address);
	}
	else{
		if(level == 0){
			if(RAMq >= 25){ // Evict from RAM
				// Evict something
				tempPtr = get_next_job(sOldQueue);
				add_job(sNewQueue, eNewQueue, tempPtr->page, tempPtr->index); // Add to end
				sem_wait(&use_variable);
				RAMq--; RAMFull--;
				SSDq++; SSDFull++;
				sem_post(&use_variable);

				if(FAST_RUN == 0){
					usleep(SSD_SLEEP);
				}

				//Alter memory tables
				oldIndex = findIndex(tempPtr->page->location, level);
				newIndex = find_empty_spot(1);

				sem_wait(semSSD[newIndex]);
				SSD[newIndex] = tempPtr->page->location;
				sem_post(semSSD[newIndex]);
				sem_wait(semRAM[oldIndex]);
				RAM[oldIndex] = -1;
				sem_post(semRAM[oldIndex]);

				sem_wait(semTABLE[tempPtr->page->location]);
				pageTable[tempPtr->page->location]->level = 1;
				sem_post(semTABLE[tempPtr->page->location]);

				if(DEBUG){
					printf("Page %d has been moved down to level 1.\n", tempPtr->page->location); //Debug
				}

			}

		}
		else if(level == 1){
			if(SSDq >= 100){ // Evict from SSD
				tempPtr = get_next_job(sOldQueue);
				add_job(sNewQueue, eNewQueue, tempPtr->page, tempPtr->index); // Add to end
				sem_wait(&use_variable);
				SSDq--; SSDFull--;
				HDiskq++; HDiskFull++;
				sem_post(&use_variable);

				if(FAST_RUN == 0){
					usleep(SSD_SLEEP + HDISK_SLEEP);
				}

				//Alter memory tables
				oldIndex = findIndex(tempPtr->page->location, level);
				newIndex = find_empty_spot(2);

				sem_wait(semHDISK[newIndex]);
				hDisk[newIndex] = tempPtr->page->location;
				sem_post(semHDISK[newIndex]);
				sem_wait(semSSD[oldIndex]);
				SSD[oldIndex] = -1;
				sem_post(semSSD[oldIndex]);

				sem_wait(semTABLE[tempPtr->page->location]);
				pageTable[tempPtr->page->location]->level = 2;
				sem_post(semTABLE[tempPtr->page->location]);

				if(DEBUG){
					printf("Page %d has been moved down to level 2.\n", tempPtr->page->location); //Debug
				}

			}

		}
	}
}

// Function to evict pages
// Give address and it evicts the page that is there

// Moves a page up one level in memory. Expects that program will only try to move up
// a page where there is a free space (due to program status checking), but will also
// perform checks and evict pages if the level is full (if need be)
void move_page_up(vAddr address, int level){

	int index;

	if(level == 0){ // Why are you trying to move up a level??
		// Do nothing, you are as high as you can go
	}
	else if(level == 1){
		sem_wait(&use_variable);
		if(RAMFull < 25){ // Move only if there is space in RAM
			sem_post(&use_variable);

			sem_wait(&ram_index);
			index = find_empty_spot(0);
			sem_post(&ram_index);

			if(index != -1){
				sem_wait(semRAM[index]);
				RAM[index] = address;
				sem_post(semRAM[index]);

				sem_wait(semTABLE[address]);
				pageTable[address]->level = 0;
				pageTable[address]->accessed = 1;
				sem_post(semTABLE[address]);
			}
		}
		else{ // Have to evict something
			sem_post(&use_variable);
			if(LRU_ON){
				sem_wait(&ram_queue);
				sem_wait(&ssd_queue);
				LRU(&sRAMPtr, &eRAMPtr, &sSSDPtr, &eSSDPtr, address, 0);
				sem_post(&ssd_queue);
				sem_post(&ram_queue);
			}
			else{
				sem_wait(&ram_queue);
				sem_wait(&ssd_queue);
				second_chance(&sRAMPtr, &eRAMPtr, &sSSDPtr, &eSSDPtr);
				sem_post(&ssd_queue);
				sem_post(&ram_queue);
				sem_wait(&use_variable);
				RAMFull--;
				SSDFull++;
				sem_post(&use_variable);
			}

			sem_wait(&ram_index);
			index = find_empty_spot(0);
			sem_post(&ram_index);

			if(index != -1){
				sem_wait(semRAM[index]);
				RAM[index] = address;
				sem_post(semRAM[index]);

				sem_wait(semTABLE[address]);
				pageTable[address]->level = 0;
				pageTable[address]->accessed = 1;
				sem_post(semTABLE[address]);
			}
		}
	}
	else if(level  == 2){
		sem_wait(&use_variable);
		if(SSDFull < 100){ // Move only if there is space in SSD
			sem_post(&use_variable);

			sem_wait(&ssd_index);
			index = find_empty_spot(1);
			sem_post(&ssd_index);

			if(index != -1){
				sem_wait(semSSD[index]);
				SSD[index] = address;
				sem_post(semSSD[index]);

				sem_wait(semTABLE[address]);
				pageTable[address]->level = 1;
				pageTable[address]->accessed = 1;
				sem_post(semTABLE[address]);
			}
		}
		else{ // Have to evict something
			sem_post(&use_variable);
			if(LRU_ON){
				sem_wait(&ssd_queue);
				sem_wait(&hdisk_queue);
				LRU(&sSSDPtr, &eSSDPtr, &sHDISKPtr, &eHDISKPtr, address, 1);
				sem_post(&hdisk_queue);
				sem_post(&ssd_queue);
			}
			else{
				sem_wait(&ssd_queue);
				sem_wait(&hdisk_queue);
				second_chance(&sSSDPtr, &eSSDPtr, &sHDISKPtr, &eHDISKPtr);
				sem_post(&hdisk_queue);
				sem_post(&ssd_queue);
				sem_wait(&use_variable);
				SSDFull--;
				HDiskFull++;
				sem_post(&use_variable);
			}

			sem_wait(&ssd_index);
			index = find_empty_spot(1);
			sem_post(&ssd_index);

			if(index != -1){
				sem_wait(semSSD[index]);
				SSD[index] = address;
				sem_post(semSSD[index]);

				sem_wait(semTABLE[address]);
				pageTable[address]->level = 1;
				pageTable[address]->accessed = 1;
				sem_post(semTABLE[address]);
			}
		}
	}

	if(DEBUG){
		printf("Page at address %d have been moved up to level %d!\n", address, level-1); //Debug
	}

}