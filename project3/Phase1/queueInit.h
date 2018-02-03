#include <stdio.h>
#include <stdlib.h>

struct queueJob{
	pthread_t job; /* Thread that contains the job*/
	int pid;
	char *classification;
	struct queueJob *nextPtr; /* Pointer to next job in queue */
};

typedef struct queueJob Queue;
typedef Queue *queuePointer;