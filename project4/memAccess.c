#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

#include "table.h"

int findIndex(vAddr address, int level);
int find_empty_spot(int level);

/* Reserves a new memory location of sizeof(int). Creates in RAM, pushes other pages out
 * if needed. Returns -1 if no memory is available. */
vAddr allocateNewInt(){
	vAddr address = 999;
	pagePtr tempNode = malloc(sizeof(pageNode));
	int i;
	int index;

	if(pages >= 1000){
		// There is no space left
		if(DEBUG){
			printf("There is no space! Page not created.\n");
		}
		return -1;
	}

	/*
	sem_wait(&use_variable);
	for(i = 0; i < 1000; i++){
		if(pageTable[i]->location == -1 && i < address){
			address = i;
		}
	}
	sem_post(&use_variable);
	*/

	if(DEBUG){
		sem_wait(&use_variable);
		printf("RAM is %d, SSD is %d, HDISK is %d\n", RAMFull, SSDFull, HDiskFull); // Debug
		sem_post(&use_variable);
	}

	if(RAMFull >= 25){
		if(SSDFull >= 100){
			if(all_locked(&sSSDPtr, &eSSDPtr) == 1){
				if(DEBUG){
					printf("No memory could be evicted, as it is all locked.\n");
				}
				return -1;
			}
			if(all_locked(&sRAMPtr, &eRAMPtr) == 1){
				if(DEBUG){
					printf("No memory could be evicted, as it is all locked.\n");
				}
				return -1;
			}

			if(LRU_ON){
				sem_wait(&ssd_queue);
				sem_wait(&hdisk_queue);
				LRU(&sSSDPtr, &eSSDPtr, &sHDISKPtr, &eHDISKPtr, address, 1);
				sem_post(&hdisk_queue);
				sem_post(&ssd_queue);
				sem_wait(&ram_queue);
				sem_wait(&ssd_queue);
				LRU(&sRAMPtr, &eRAMPtr, &sSSDPtr, &eSSDPtr, address, 0);
				sem_post(&ssd_queue);
				sem_post(&ram_queue);
			}
			else{
				// Pick an SSD page to swap to hard disk
				sem_wait(&ssd_queue);
				sem_wait(&hdisk_queue);
				second_chance(&sSSDPtr, &eSSDPtr, &sHDISKPtr, &eHDISKPtr);
				sem_post(&hdisk_queue);
				sem_post(&ssd_queue);
				sem_wait(&use_variable);
				SSDFull--;
				HDiskFull++;
				sem_post(&use_variable);

				// Pick a RAM page to swap to ssd
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

			sem_wait(&use_variable);
			for(i = 0; i < 1000; i++){
				if(pageTable[i]->location == -1 && i < address){
					address = i;
				}
			}
			sem_post(&use_variable);

			//Initialize tempNode and put page into ram
			tempNode->locked = 0;
			tempNode->allocated = 1;
			tempNode->modified = 0;
			tempNode->accessed = 0;
			tempNode->level = 0;
			tempNode->location = address;
			pageTable[address] = tempNode;

			sem_wait(&ram_index);
			int index = find_empty_spot(0); // Get index for new process
			sem_post(&ram_index);

			sem_wait(semRAM[index]);
			RAM[index] = address; // Add to RAM cache
			sem_post(semRAM[index]);
			sem_wait(&ram_queue);
			add_job(&sRAMPtr, &eRAMPtr, tempNode, index); // Add new page to RAM queue
			sem_post(&ram_queue);
			sem_wait(&use_variable);
			RAMq++;
			RAMFull++; // Increment memory area counter
			sem_post(&use_variable);
		}
		else{
			if(all_locked(&sRAMPtr, &eRAMPtr) == 1){
				if(DEBUG){
					printf("No memory could be evicted, as it is all locked.\n");
				}
				return -1;
			}

			if(LRU_ON){
				sem_wait(&ram_queue);
				LRU(&sRAMPtr, &eRAMPtr, &sSSDPtr, &eSSDPtr, address, 0);
				sem_post(&ram_queue);
			}
			else{
				// Pick a page from RAM to swap into SSD space
				sem_wait(&ram_queue);
				second_chance(&sRAMPtr, &eRAMPtr, &sSSDPtr, &eSSDPtr);
				sem_post(&ram_queue);
				sem_wait(&use_variable);
				RAMFull--;
				SSDFull++;
				sem_post(&use_variable);
			}

			sem_wait(&use_variable);
			for(i = 0; i < 1000; i++){
				if(pageTable[i]->location == -1 && i < address){
					address = i;
				}
			}
			sem_post(&use_variable);

			//Initialize tempNode and put page into ram
			tempNode->locked = 0;
			tempNode->allocated = 1;
			tempNode->modified = 0;
			tempNode->accessed = 0;
			tempNode->level = 0;
			tempNode->location = address;
			pageTable[address] = tempNode;

			sem_wait(&ram_index);
			int index = find_empty_spot(0); // Get index for new process
			sem_post(&ram_index);

			sem_wait(semRAM[index]);
			RAM[index] = address; // Add to RAM cache
			sem_post(semRAM[index]);
			sem_wait(&ram_queue);
			add_job(&sRAMPtr, &eRAMPtr, tempNode, index); // Add new page to RAM queue
			sem_post(&ram_queue);
			sem_wait(&use_variable);
			RAMq++;
			RAMFull++; // Increment memory area counter
			sem_post(&use_variable);
		}
	}
	else{

		sem_wait(&use_variable);
		for(i = 0; i < 1000; i++){
			if(pageTable[i]->location == -1 && i < address){
				address = i;
			}
		}
		sem_post(&use_variable);

		//Initialize tempNode and put page into ram
		tempNode->locked = 0;
		tempNode->allocated = 1;
		tempNode->modified = 0;
		tempNode->accessed = 0;
		tempNode->level = 0;
		tempNode->location = address;
		pageTable[address] = tempNode;

		sem_wait(&ram_index);
		int index = find_empty_spot(0); // Get index for new process
		sem_post(&ram_index);

		sem_wait(semRAM[index]);
		RAM[index] = address; // Add to RAM cache
		sem_post(semRAM[index]);
		sem_wait(&ram_queue);
		add_job(&sRAMPtr, &eRAMPtr, tempNode, index); // Add new page to RAM queue
		sem_post(&ram_queue);
		sem_wait(&use_variable);
		RAMq++;
		RAMFull++; // Increment memory area counter
		sem_post(&use_variable);
	}
	sem_wait(&use_variable);
	pages++;
	sem_post(&use_variable);

	return address;
}

/* Obtains indicated memory page from lower hierarchy levels and returns an integer pointer 
 * to the location in emulated RAM. Page is locked in memory and considered "dirty". Returns
 * NULL if pointer can't be provided (e.g if RAM is locked).*/
int * accessIntPtr(vAddr address){

	pagePtr tempNode = malloc(sizeof(pageNode));
	printf("We're here!\n");

	sem_wait(semTABLE[address]);
	tempNode = pageTable[address];
	sem_post(semTABLE[address]);

	if(pageTable[address]->level == 0){

		sem_wait(semTABLE[address]);
		pageTable[address]->locked = 1;
		pageTable[address]->modified = 1;
		pageTable[address]->accessed = 1;
		sem_post(semTABLE[address]);

		return &(addr[address]);
	}
	else if(pageTable[address]->level == 1){

		if(all_locked(&sRAMPtr, &eRAMPtr) == 1){
			if(DEBUG){
					printf("No memory could be evicted, as it is all locked.\n");
				}
			return NULL;
		}

		if(FAST_RUN == 0){
			usleep(250000);
		}

		sem_wait(&use_variable);
		if(RAMFull >= 25){
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
			
		}
		else{
			sem_post(&use_variable);
		}

		move_page_up(address, 1); // Swap page in
		sem_wait(&use_variable);
		RAMFull++;
		sem_post(&use_variable);

		sem_wait(semTABLE[address]);
		pageTable[address]->locked = 1;
		pageTable[address]->modified = 1;
		pageTable[address]->accessed = 1;
		pageTable[address]->level = 0;
		sem_post(semTABLE[address]);

		//tableIndex = findIndex(address, 0); // Find new index in RAM
		
		return &(addr[address]);
	}
	else if(pageTable[address]->level == 2){
		if(all_locked(&sSSDPtr, &eSSDPtr) == 1){
			if(DEBUG){
					printf("No memory could be evicted, as it is all locked.\n");
				}
			return NULL;
		}
		if(all_locked(&sRAMPtr, &eRAMPtr) == 1){
			if(DEBUG){
					printf("No memory could be evicted, as it is all locked.\n");
				}
			return NULL;
		}

		if(FAST_RUN == 0){
			usleep(2500000);
		}

		if(SSDFull >= 100){
			// Pick an SSD page to swap to hard disk
			if(LRU_ON){
				sem_wait(&ssd_queue);
				LRU(&sSSDPtr, &eSSDPtr, &sHDISKPtr, &eHDISKPtr, address, 1);
				sem_post(&ssd_queue);
			}
			else{
				sem_wait(&ssd_queue);
				second_chance(&sSSDPtr, &eSSDPtr, &sHDISKPtr, &eHDISKPtr);
				sem_post(&ssd_queue);
				sem_wait(&use_variable);
				SSDFull--;
				HDiskFull++;
				sem_post(&use_variable);
			}
		
		}

		move_page_up(address, 2); // Swap page in
		sem_wait(&use_variable);
		SSDFull++;
		sem_post(&use_variable);

		if(RAMFull >= 25){
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
	
		}
		move_page_up(address, 1);
		sem_wait(&use_variable);
		RAMFull++;
		sem_post(&use_variable);

		sem_wait(semTABLE[address]);
		pageTable[address]->locked = 1;
		pageTable[address]->modified = 1;
		pageTable[address]->accessed = 1;
		pageTable[address]->level = 0;
		sem_post(semTABLE[address]);

		//tableIndex = findIndex(address, 0); // Find new index in RAM

		return &(addr[address]);
	}

	return NULL;
}

/* Unlock memory (indicating that it can be swapped out to disk. Note that previous pointers
 * to the memory become invalid and must not be used after running this command. */
void unlockMemory(vAddr address){

	int i;

	// Scan for page
	for(i = 0; i < 1000; i++){
		sem_wait(semTABLE[i]);
		if(pageTable[i]->location == address){
			pageTable[i]->locked = 0;
			pageTable[i]->modified = 0;

			// Access sleep delays
			if(pageTable[i]->level == 0){
				// No delay
			}
			else if(pageTable[i]->level == 1){
				if(FAST_RUN == 0){
					usleep(250000);
				}
			}
			else if(pageTable[i]->level == 2){
				if(FAST_RUN == 0){
					usleep(2500000);
				}
			}
		}
		sem_post(semTABLE[i]);
	}

	if(DEBUG){
		printf("Address %d has been unlocked.\n", address);
	}

}

/* Frees allocated memory. Frees in memory, but also deletes any swapped out copies of the
 * page as well. */
void freeMemory(vAddr address){

	int i;
	int level;
	int index;
	pagePtr tempPtr = malloc(sizeof(pageNode));

	for(i = 0; i < 1000; i++){
		sem_wait(semTABLE[i]);
		if(pageTable[i]->location == address){
			// Copy to a temporary holder and delete the table entry
			tempPtr = pageTable[i];

			// Set to sentinel specifications
			pageTable[i]->locked = 0;
			pageTable[i]->allocated = 0;
			pageTable[i]->modified = 0;
			pageTable[i]->accessed = -1;
			pageTable[i]->level = -1;
			pageTable[i]->location = -1;

			level = tempPtr->level;

			// Delete any indication in RAM/SSD/Hard Disk listings
			if(level == 0){
				sem_wait(semRAM[address]);
				index = findIndex(tempPtr->location, 0);
				RAM[index] = -1;
				sem_post(semRAM[address]);
				sem_wait(&use_variable);
				RAMFull--;
				sem_post(&use_variable);

				// Remove copy from SSD if there is one
				index = findIndex(tempPtr->location, 1);
				if(index != -1){
					sem_wait(semSSD[address]);
					SSD[index] = -1;
					sem_post(semSSD[address]);
					sem_wait(&use_variable);
					SSDFull--;
					sem_post(&use_variable);
				}

				// Remove copy from Hard Disk if there is one
				index = findIndex(tempPtr->location, 2);
				if(index != -1){
					sem_wait(semHDISK[address]);
					hDisk[index] = -1;
					sem_post(semHDISK[address]);
					sem_wait(&use_variable);
					HDiskFull--;
					sem_post(&use_variable);
				}

			}
			else if(level == 1){
				sem_wait(semSSD[address]);
				index = findIndex(tempPtr->location, 1);
				SSD[index] = -1;
				sem_post(semSSD[address]);
				sem_wait(&use_variable);
				SSDFull--;
				sem_post(&use_variable);

				// Remove copy from Hard Disk if there is one
				index = findIndex(tempPtr->location, 2);
				if(index != -1){
					sem_wait(semHDISK[address]);
					hDisk[index] = -1;
					sem_post(semHDISK[address]);
					sem_wait(&use_variable);
					HDiskFull--;
					sem_post(&use_variable);
				}

			}
			else if(level == 2){
				sem_wait(semHDISK[address]);
				index = findIndex(tempPtr->location, 2);
				hDisk[index] = -1;
				sem_post(semHDISK[address]);
				sem_wait(&use_variable);
				HDiskFull--;
				sem_post(&use_variable);
			}
		}
		sem_post(semTABLE[i]);
	}

	if(DEBUG){
		printf("Address %d has been freed.\n", address);
	}
	sem_wait(&use_variable);
	pages--; // Decrement page counter
	sem_post(&use_variable);

}

/* Finds the index of an address stored in a given array. Returns -1 if not found. */
int findIndex(vAddr address, int level){
	int i;
	int index = -1;

	if(level == 0){
		for(i = 0; i < 25; i++){
			if(address == RAM[i]){
				index = i;
			}
		}
	}
	else if(level == 1){
		for(i = 0; i < 100; i++){
			if(address == SSD[i]){
				index = i;
			}
		}
	}
	else if(level == 2){
		for(i = 0; i < 1000; i++){
			if(address == hDisk[i]){
				index = i;
			}
		}
	}

	return index;
}

/* Finds the first empty spot in a memory array. Returns -1 if level is full. */
int find_empty_spot(int level){
	int index;
	int i;

	if(level == 0){
		for(i = 0; i < 25; i++){
			if(RAM[i] == -1){
				index = i;
			}
		}
	}
	else if(level == 1){
		for(i = 0; i < 100; i++){
			if(SSD[i] == -1){
				index = i;
			}
		}
	}
	else if(level == 2){
		for(i = 0; i < 1000; i++){
			if(hDisk[i] == -1){
				index = i;
			}
		}
	}

	return index;
}