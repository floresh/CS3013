#include <stdio.h>
#include <stdlib.h>

struct queueJob{
	pthread_t job; /* Thread that contains the job*/
	int pid;
	int priority;
	int approach;
	int direction;
	struct queueJob *nextPtr; /* Pointer to next job in queue */
};

typedef struct queueJob Queue;
typedef Queue *queuePointer;

// Queue function declarations
void add_job(queuePointer *sPtr, queuePointer *ePtr, pthread_t newJob, int id, int priority,
	int approach, int direction);
queuePointer get_next_job(queuePointer *job);