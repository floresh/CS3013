#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

//Define statements
#define DEBUG 1
#define LRU_ON 0
#define FAST_RUN 0

// Typedef vAddr
typedef signed short vAddr;

// Page information struct
struct pageNode{
	int locked; // 0 if unlocked, 1 if locked
	int allocated; // 0 if unallocated, 1 if allocated
	int modified; // 0 if clean, 1 if dirty
	int accessed; // 0 if not recently, 1 if recently
	int level; // 0 for RAM, 1 for SSD, 2 for Hard Drive (Highest level)
	vAddr location; // Pointer to page in memory
};

typedef struct pageNode pageNode;
typedef pageNode *pagePtr;

// Queue struct
struct queueJob{
	pagePtr page;
	int index;
	struct queueJob *nextPtr; /* Pointer to next job in queue */
};

typedef struct queueJob Queue;
typedef Queue *queuePointer;

// Hash struct for LRU
struct Hash{
	int capacity;
	pagePtr *array;
};
typedef struct Hash Hash;
typedef Hash *hashPtr;


// The created page table and memory arrays
pagePtr pageTable[1000];
vAddr RAM[25];
vAddr SSD[100];
vAddr hDisk[1000];

// Integer array for vAddr correspondence
int addr[1000];

// Algorithm queues
queuePointer sRAMPtr;
queuePointer eRAMPtr;
queuePointer sSSDPtr;
queuePointer eSSDPtr;
queuePointer sHDISKPtr;
queuePointer eHDISKPtr;

// Queue counters
int RAMq;
int SSDq;
int HDiskq;

// Array counters
int RAMFull;
int SSDFull;
int HDiskFull;
int pages;

// Semaphore arrays
sem_t *semRAM[25];
sem_t *semSSD[100];
sem_t *semHDISK[1000];
sem_t *semTABLE[1000];

// Access semaphores
sem_t ram_index;
sem_t ssd_index;
sem_t hdisk_index;
sem_t ram_queue;
sem_t ssd_queue;
sem_t hdisk_queue;
sem_t use_variable;
sem_t make_addr;


// Function Declarations
vAddr allocateNewInt();
int * accessIntPtr(vAddr address);
void unlockMemory(vAddr address);
void freeMemory(vAddr address);
int findIndex(vAddr address, int level);
int find_empty_spot(int level);

// Queue function declarations
void add_job(queuePointer *sPtr, queuePointer *ePtr, pagePtr page, int index);
queuePointer get_next_job(queuePointer *job);
queuePointer pullNode(queuePointer *sPtr, queuePointer *ePtr, vAddr address);