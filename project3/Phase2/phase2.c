#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "queueInit.h"

// Defined Constants
#define NW 0
#define NE 1
#define SE 2
#define SW 3

#define NORTH 0
#define SOUTH 1
#define EAST 2
#define WEST 3

#define THREADS 20
#define TIMESLICE 50

#define TURN_LEFT 2
#define TURN_RIGHT 0
#define TURN_STRAIGHT 1

// Global Variables
pthread_mutex_t nw;
pthread_mutex_t ne;
pthread_mutex_t se;
pthread_mutex_t sw;

// Initialize the four direction queues
queuePointer sNptr = NULL;
queuePointer eNptr = NULL;

queuePointer sSptr = NULL;
queuePointer eSptr = NULL;

queuePointer sEptr = NULL;
queuePointer eEptr = NULL;

queuePointer sWptr = NULL;
queuePointer eWptr = NULL;

queuePointer sentinel;
queuePointer *held_cars[THREADS];
int ready_cars[THREADS] = {0};

// Condition variable setup
int approach_cars[THREADS] = {0};
pthread_cond_t signal_cond;
pthread_mutex_t signal_lock;

// Various counters and holder variables
int nQueue = 0; int sQueue = 0; int eQueue = 0; int wQueue = 0;
int numArray[THREADS];
queuePointer intersection[2][2];
queuePointer nApproach, sApproach, eApproach, wApproach;
int i = 0; // Counter

//Fuction Declarations
void initialize_values();
void loop();
void make_cars();
int run_lottery(int tickets);
void naptime();
void *create_thread(void *idPtr);
int check_priority();
int check_deadlockCondition();
int drive(queuePointer car, int approach, int direction);

int main(){

	queuePointer tempPtr = malloc(sizeof(queuePointer));

	pthread_cond_init(&signal_cond, NULL);
	pthread_mutex_init(&signal_lock, NULL);

	srand(time(NULL)); // Seed random

	//Create cars and initialize values
	initialize_values();
	//make_cars();

	//int i = 0;
	int approach;
	int direction;
	pthread_t car;

	// Create cars
	for(i = 0; i < THREADS-5; i++){
		approach = run_lottery(4);
		direction = run_lottery(3);
		pthread_create(&car, NULL, create_thread, &numArray[i]);

		if(approach == NORTH){
			add_job(&sNptr, &eNptr, car, i, 0, NORTH, direction);
			nQueue++;
		}
		else if(approach == SOUTH){
			add_job(&sSptr, &eSptr, car, i, 0, SOUTH, direction);
			sQueue++;
		}
		else if(approach == EAST){
			add_job(&sEptr, &eEptr, car, i, 0, EAST, direction);
			eQueue++;
		}
		else if(approach == WEST){
			add_job(&sWptr, &eWptr, car, i, 0, WEST, direction);
			wQueue++;
		}

		//printf("Made car %d with approach %d, in direction %d.\n", i, approach, direction);
	}

	// Create motorcade cars
	approach = run_lottery(4);
	direction = run_lottery(3);
	for(i = 15; i < THREADS; i++){
		pthread_create(&car, NULL, create_thread, &numArray[i]);

		if(approach == NORTH){
			add_job(&sNptr, &eNptr, car, i, 1, NORTH, direction);
			nQueue++;
		}
		else if(approach == SOUTH){
			add_job(&sSptr, &eSptr, car, i, 1, SOUTH, direction);
			sQueue++;
		}
		else if(approach == EAST){
			add_job(&sEptr, &eEptr, car, i, 1, EAST, direction);
			eQueue++;
		}
		else if(approach == WEST){
			add_job(&sWptr, &eWptr, car, i, 1, WEST, direction);
			wQueue++;
		}
	}

	sleep(1);

	int priorityPresent;
	int deadlockPresent = 0;
	int nID, sID, eID, wID;

	while(i < TIMESLICE){

		priorityPresent = check_priority();
		deadlockPresent = check_deadlockCondition();

		//Check to see if there is an emergency vehicle or motorcade
		if(priorityPresent != -1){
			if(priorityPresent == NORTH){
				tempPtr = get_next_job(&sNptr);
				nApproach = tempPtr;
				nQueue--;
				printf("A priority car [#%d] is approaching from the North, and turning %d\n", nApproach->pid, nApproach->direction);
			}
			else if(priorityPresent == SOUTH){
				tempPtr = get_next_job(&sSptr);
				sApproach = tempPtr;
				sQueue--;
				printf("A priority car [#%d] is approaching from the South, and turning %d\n", sApproach->pid, sApproach->direction);
			}
			else if(priorityPresent == EAST){
				tempPtr = get_next_job(&sEptr);
				eApproach = tempPtr;
				eQueue--;
				printf("A priority car [#%d] is approaching from the East, and turning %d\n", eApproach->pid, eApproach->direction);
			}
			else{
				tempPtr = get_next_job(&sWptr);
				wApproach = tempPtr;
				wQueue--;
				printf("A priority car [#%d] is approaching from the West, and turning %d\n", wApproach->pid, wApproach->direction);
			}

		}
		else{
			//printf("Deadlock state is %d\n\n", deadlockPresent); //Debug

			// Check for deadlock conditions
			if(deadlockPresent == -1){

				// Pick a lane to wait
				int pickedLane = run_lottery(4);

				// Add cars for all lanes except one designated to wait
				if(pickedLane != NORTH){
					tempPtr = get_next_job(&sNptr);
					nApproach = tempPtr;
					nQueue--;
					printf("A car [#%d] is approaching from the North, and turning %d\n", nApproach->pid, nApproach->direction);
				}
				if(pickedLane != SOUTH){
					tempPtr = get_next_job(&sSptr);
					sApproach = tempPtr;
					sQueue--;
					printf("A car [#%d] is approaching from the South, and turning %d\n", sApproach->pid, sApproach->direction);
				}
				if(pickedLane != EAST){
					tempPtr = get_next_job(&sEptr);
					eApproach = tempPtr;
					eQueue--;
					printf("A car [#%d] is approaching from the East, and turning %d\n", eApproach->pid, eApproach->direction);
				}
				if(pickedLane != WEST){
					tempPtr = get_next_job(&sWptr);
					wApproach = tempPtr;
					wQueue--;
					printf("A car [#%d] is approaching from the West, and turning %d\n", wApproach->pid, wApproach->direction);
				}

			}
			else{
				// Add a car for all lanes
				if(nQueue > 0){
					tempPtr = get_next_job(&sNptr);
					nApproach = tempPtr;
					nQueue--;
					printf("A car [#%d] is approaching from the North, and turning %d\n", nApproach->pid, nApproach->direction);
				}
				
				if(sQueue > 0){
					tempPtr = get_next_job(&sSptr);
					sApproach = tempPtr;
					sQueue--;
					printf("A car [#%d] is approaching from the South, and turning %d\n", sApproach->pid, sApproach->direction);
				}
				
				if(eQueue > 0){
					tempPtr = get_next_job(&sEptr);
					eApproach = tempPtr;
					eQueue--;
					printf("A car [#%d] is approaching from the East, and turning %d\n", eApproach->pid, eApproach->direction);
				}
				
				if(wQueue > 0){
					tempPtr = get_next_job(&sWptr);
					wApproach = tempPtr;
					wQueue--;
					printf("A car [#%d] is approaching from the West, and turning %d\n", wApproach->pid, wApproach->direction);
				}
			}
		}

		// Run cars that have been moved to the approach point.
		nID = nApproach->pid; sID = sApproach->pid; eID = eApproach->pid; wID = wApproach->pid;
		
		// Signal the north car's thread
		if(nID > -1){
			pthread_mutex_lock(&signal_lock);
			approach_cars[nID] = 1;
			//pthread_cond_broadcast(&signal_cond);
			pthread_mutex_unlock(&signal_lock);
		}

		// Signal the south car's thread
		if(sID > -1){
			pthread_mutex_lock(&signal_lock);
			approach_cars[sID] = 1;
			//pthread_cond_broadcast(&signal_cond);
			pthread_mutex_unlock(&signal_lock);
		}

		// Signal the east car's thread
		if(eID > -1){
			pthread_mutex_lock(&signal_lock);
			approach_cars[eID] = 1;
			//pthread_cond_broadcast(&signal_cond);
			pthread_mutex_unlock(&signal_lock);
		}

		// Signal the west car's thread
		if(wID > -1){
			pthread_mutex_lock(&signal_lock);
			approach_cars[wID] = 1;
			//pthread_cond_broadcast(&signal_cond);
			pthread_mutex_unlock(&signal_lock);
		}

		// Broadcast
		pthread_cond_broadcast(&signal_cond);

		sleep(1);

		// Check for any cars that are ready to queue again
		loop();

		i++; // Increment cycle
	}

	return 0;
}

void initialize_values(){
	int i = 0; int j = 0;// Counter

	// Initialize mutexes for intersection quadrants
	pthread_mutex_init(&nw, NULL);
	pthread_mutex_init(&ne, NULL);
	pthread_mutex_init(&se, NULL);
	pthread_mutex_init(&sw, NULL);

	// Create number array for ID numbers
	for(i = 0; i < THREADS; i++){
		numArray[i] = i;
		held_cars[i] = malloc(sizeof(queuePointer));
		*held_cars[i] = sentinel;
	}

	// Allocate intersection spaces
	for(i = 0; i < 2; i++){
		for(j = 0; j < 2; j++){
			intersection[i][j] = malloc(sizeof(queuePointer));
			intersection[i][j] = sentinel;
		}
	}

	// Initialize the sentinel pointer
	sentinel = malloc(sizeof(queuePointer));
	sentinel->pid = -1;
	sentinel->priority = -1;
	sentinel->approach = -1;
	sentinel->direction = -1;
	sentinel->nextPtr = NULL;

	// Initialize waiting spot for cars about to go
	nApproach = malloc(sizeof(queuePointer));
	sApproach = malloc(sizeof(queuePointer));
	eApproach = malloc(sizeof(queuePointer));
	wApproach = malloc(sizeof(queuePointer));

	nApproach = sentinel;
	sApproach = sentinel;
	eApproach = sentinel;
	wApproach = sentinel;
}

// Checks for cars that are ready to enter queue again
void loop(){
	int i;
	int approach;
	int direction;

	queuePointer tempPtr;

	// Loops for all normal cars + one emergency vehicle
	for(i = 0; i < THREADS-5; i++){
		if(ready_cars[i] == 1){
			//printf("Car %d is ready to queue up again!\n", i); // Debug

			tempPtr = *held_cars[i]; // Get car to modify

			// Gets new approach and direction and sets it
			approach = run_lottery(4);
			direction = run_lottery(3);

			tempPtr->approach = approach;
			tempPtr->direction = direction;

			// Puts in proper queue
			if(approach == NORTH){
				add_job(&sNptr, &eNptr, tempPtr->job, tempPtr->pid,
					0, NORTH, direction);
				nQueue++;
			}
			else if(approach == SOUTH){
				add_job(&sSptr, &eSptr, tempPtr->job, tempPtr->pid,
					0, SOUTH, direction);
				sQueue++;
			}
			else if(approach == EAST){
				add_job(&sEptr, &eEptr, tempPtr->job, tempPtr->pid,
					0, EAST, direction);
				eQueue++;
			}
			else if(approach == WEST){
				add_job(&sWptr, &eWptr, tempPtr->job, tempPtr->pid,
					0, WEST, direction);
				wQueue++;
			}

			// Clear ready values
			*held_cars[i] = sentinel;
			ready_cars[i] = 0;
		}
	}

	int motorcade_approach = run_lottery(4);
	int motorcade_direction = run_lottery(3);

	//Manage motorcades, minus car 15, which will become an emergency vehicle after the 1st time
	for(i = 15; i < THREADS; i++){
		if(ready_cars[i] == 1){
			tempPtr = *held_cars[i]; // Get car

			// Puts in proper queue
			if(motorcade_approach == NORTH){
				add_job(&sNptr, &eNptr, tempPtr->job, i,
					1, NORTH, motorcade_direction);
				nQueue++;
			}
			else if(motorcade_approach == SOUTH){
				add_job(&sSptr, &eSptr, tempPtr->job, i,
					1, SOUTH, motorcade_direction);
				sQueue++;
			}
			else if(motorcade_approach == EAST){
				add_job(&sEptr, &eEptr, tempPtr->job, i,
					1, EAST, motorcade_direction);
				eQueue++;
			}
			else if(motorcade_approach == WEST){
				add_job(&sWptr, &eWptr, tempPtr->job, i,
					1, WEST, motorcade_direction);
				wQueue++;
			}
		}

		*held_cars[i] = sentinel;
		ready_cars[i] = 0;
	}
}

// Creates car nodes/threads
void make_cars(){
	int i = 0;
	int approach;
	int direction;
	pthread_t car;

	for(i = 0; i < THREADS-5; i++){
		approach = run_lottery(4);
		direction = run_lottery(3);
		pthread_create(&car, NULL, create_thread, &numArray[i]);

		if(approach == NORTH){
			add_job(&sNptr, &eNptr, car, i, 0, NORTH, direction);
			nQueue++;
		}
		else if(approach == SOUTH){
			add_job(&sSptr, &eSptr, car, i, 0, SOUTH, direction);
			sQueue++;
		}
		else if(approach == EAST){
			add_job(&sEptr, &eEptr, car, i, 0, EAST, direction);
			eQueue++;
		}
		else if(approach == WEST){
			add_job(&sWptr, &eWptr, car, i, 0, WEST, direction);
			wQueue++;
		}

		//printf("Made car %d with approach %d, in direction %d.\n", i, approach, direction);
	}

	// Create motorcade cars
	approach = run_lottery(4);
	direction = run_lottery(3);
	for(i = 15; i < THREADS; i++){
		pthread_create(&car, NULL, create_thread, &numArray[i]);

		if(approach == NORTH){
			add_job(&sNptr, &eNptr, car, i, 1, NORTH, direction);
			nQueue++;
		}
		else if(approach == SOUTH){
			add_job(&sSptr, &eSptr, car, i, 1, SOUTH, direction);
			sQueue++;
		}
		else if(approach == EAST){
			add_job(&sEptr, &eEptr, car, i, 1, EAST, direction);
			eQueue++;
		}
		else if(approach == WEST){
			add_job(&sWptr, &eWptr, car, i, 1, WEST, direction);
			wQueue++;
		}
	}
}

/* Randomly selects a 'ticket' from the alloted pool */
int run_lottery(int tickets){
	int selected;

	selected = rand()%tickets;

	//printf("The ticket is %d\n", selected);

	return selected;
}

/* Thread creation function */
void *create_thread(void *idPtr){

	int id = *((int *)idPtr); // Copies ID
	queuePointer threadPtr;

	// Block until signaled
	pthread_mutex_lock(&signal_lock);
	while(approach_cars[id] == 0){ // Wait until car approaches intersection
		pthread_cond_wait(&signal_cond, &signal_lock);
	}
	//

	while(1){
		// Check which direction its car is approaching from
		if(nApproach->pid == id){
			threadPtr = nApproach;
		}
		else if(sApproach->pid == id){
			threadPtr = sApproach;
		}
		else if(eApproach->pid == id){
			threadPtr = eApproach;
		}
		else if(wApproach->pid == id){
			threadPtr = wApproach;
		}

		// Drive function here
		printf("Car %d is entering the intersection from direction %d and turning %d.\n",
			threadPtr->pid, threadPtr->approach, threadPtr->direction);
		drive(threadPtr, threadPtr->approach, threadPtr->direction);

		pthread_mutex_unlock(&signal_lock);

		// Update car ready value
		naptime();
		ready_cars[id] = 1;

		// Wait until signaled to go again
		approach_cars[id] = 0;
		pthread_mutex_lock(&signal_lock);
		while(approach_cars[id] == 0){ // Wait until car approaches intersection
			pthread_cond_wait(&signal_cond, &signal_lock);
		}
		//pthread_mutex_unlock(&signal_lock);
	}
}

/* Makes threads sleep for a while before going back to queue */
void naptime(){
	int randomNum;

	randomNum = ((rand()%5) + 10);

	sleep(randomNum);
}

// Checks if the first car in any queue is a priority car
// Returns -1 if none, or the number of the queue it is in
int check_priority(){
	if(nQueue == 0 && sQueue == 0 && eQueue == 0 && wQueue == 0){
		return -1;
	}

	if(nQueue > 0){
		if(sNptr->priority > 0){
			return NORTH;
		}
	}
	
	if(sQueue > 0){
		if(sSptr->priority > 0){
			return SOUTH;
		}
	}

	if(eQueue > 0){
		if(sEptr->priority > 0){
			return EAST;
		}
	}

	if(wQueue > 0){
		if(sWptr->priority > 0){
			return WEST;
		}
	}

	return -1;
}

// Returns 0 if there is no deadlock situation, -1 if a deadlock is imminent
int check_deadlockCondition(){
	/* We have deadlock conditions when all the following occur:
	 *     -There are 4 cars
	 *     -No cars are trying to make a right turn (only left/straight turns)
	 */
	 if(nQueue == 0 || sQueue == 0 || eQueue == 0 || wQueue == 0){
	 	return 0;
	 }
	 else{
	 	if(sNptr->direction != TURN_RIGHT && sSptr->direction != TURN_RIGHT &&
	 		sEptr->direction != TURN_RIGHT && sWptr->direction != TURN_RIGHT){
	 		return -1;
	 	}
	 }

	return 0;
}

// Returns 0 if car has finished driving, -1 if the car could not drive
// Note: MUST check for a deadlock situation BEFORE sending a car to drive!
int drive(queuePointer car, int approach, int direction){
	int whileSwitch = 1;

	// Determine approach direction
	if(approach == NORTH){
		// Determine where car is turning
		if(direction == TURN_LEFT){
			while(whileSwitch){ // Tries to enter intersection
				if(pthread_mutex_trylock(&nw) == 0){
					whileSwitch = 0;
					intersection[0][0] = car;
					nApproach = sentinel;
				}
			}

			whileSwitch = 1; // Reset switch

			// Tries to enter next quadrant
			while(whileSwitch){
				if(pthread_mutex_trylock(&sw) == 0){
					whileSwitch = 0;
					intersection [1][0] = car;
					intersection[0][0] = sentinel;

					pthread_mutex_unlock(&nw);
				}
			}

			whileSwitch = 1;

			while(whileSwitch){
				if(pthread_mutex_trylock(&se) == 0){
					whileSwitch = 0;
					intersection[1][1] = car;
					intersection[1][0] = sentinel;

					pthread_mutex_unlock(&sw);
				}
			}

			// Leaves intersection
			*held_cars[car->pid] = car;
			intersection[1][1] = sentinel;
			pthread_mutex_unlock(&se);
		}

		else if(direction == TURN_RIGHT){
			while(whileSwitch){ // Try to enter intersection, wait until it can
				if(pthread_mutex_trylock(&nw) == 0){
					whileSwitch = 0;
					intersection[0][0] = car;
					nApproach = sentinel;
				}
			}

			// Move out of intersection and unlock quadrant
			*held_cars[car->pid] = car;
			intersection[0][0] = sentinel;
			pthread_mutex_unlock(&nw);
		}

		else if(direction == TURN_STRAIGHT){
			while(whileSwitch){ // Tries to enter intersection
				if(pthread_mutex_trylock(&nw) == 0){
					whileSwitch = 0;
					intersection[0][0] = car;
					nApproach = sentinel;
				}
			}

			whileSwitch = 1; // Reset switch

			// Tries to enter next quadrant
			while(whileSwitch){
				if(pthread_mutex_trylock(&sw) == 0){
					whileSwitch = 0;
					intersection [1][0] = car;
					intersection[0][0] = sentinel;

					pthread_mutex_unlock(&nw);
				}
			}

			// Leaves intersection
			*held_cars[car->pid] = car;
			intersection[1][0] = sentinel;
			pthread_mutex_unlock(&sw);
		}

		printf("Car %d [Entered NORTH, turned direction %d] has left the intersection.\n", car->pid, direction);
	}


	else if(approach == SOUTH){
		// Determine where car is turning
		if(direction == TURN_LEFT){
			while(whileSwitch){ // Tries to enter intersection
				if(pthread_mutex_trylock(&se) == 0){
					whileSwitch = 0;
					intersection[1][1] = car;
					sApproach = sentinel;
				}
			}

			whileSwitch = 1; // Reset switch

			// Tries to enter next quadrant
			while(whileSwitch){
				if(pthread_mutex_trylock(&ne) == 0){
					whileSwitch = 0;
					intersection [0][1] = car;
					intersection[1][1] = sentinel;

					pthread_mutex_unlock(&se);
				}
			}

			whileSwitch = 1;

			while(whileSwitch){
				if(pthread_mutex_trylock(&nw) == 0){
					whileSwitch = 0;
					intersection[0][0] = car;
					intersection[0][1] = sentinel;

					pthread_mutex_unlock(&ne);
				}
			}

			// Leaves intersection
			*held_cars[car->pid] = car;
			intersection[0][0] = sentinel;
			pthread_mutex_unlock(&nw);
		}

		else if(direction == TURN_RIGHT){
			while(whileSwitch){ // Try to enter intersection, wait until it can
				if(pthread_mutex_trylock(&se) == 0){
					whileSwitch = 0;
					intersection[1][1] = car;
					sApproach = sentinel;
				}
			}

			// Move out of intersection and unlock quadrant
			*held_cars[car->pid] = car;
			intersection[1][1] = sentinel;
			pthread_mutex_unlock(&se);
		}

		else if(direction == TURN_STRAIGHT){
			while(whileSwitch){ // Tries to enter intersection
				if(pthread_mutex_trylock(&se) == 0){
					whileSwitch = 0;
					intersection[1][1] = car;
					sApproach = sentinel;
				}
			}

			whileSwitch = 1; // Reset switch

			// Tries to enter next quadrant
			while(whileSwitch){
				if(pthread_mutex_trylock(&ne) == 0){
					whileSwitch = 0;
					intersection [0][1] = car;
					intersection[1][1] = sentinel;

					pthread_mutex_unlock(&se);
				}
			}

			// Leaves intersection
			*held_cars[car->pid] = car;
			intersection[1][0] = sentinel;
			pthread_mutex_unlock(&ne);
		}

		printf("Car %d [Entered SOUTH, turned direction %d] has left the intersection.\n", car->pid, direction);
	}


	else if(approach == EAST){
		// Determine where car is turning
		if(direction == TURN_LEFT){
			while(whileSwitch){ // Tries to enter intersection
				if(pthread_mutex_trylock(&ne) == 0){
					whileSwitch = 0;
					intersection[0][1] = car;
					eApproach = sentinel;
				}
			}

			whileSwitch = 1; // Reset switch

			// Tries to enter next quadrant
			while(whileSwitch){
				if(pthread_mutex_trylock(&nw) == 0){
					whileSwitch = 0;
					intersection [0][0] = car;
					intersection[0][1] = sentinel;

					pthread_mutex_unlock(&ne);
				}
			}

			whileSwitch = 1;

			while(whileSwitch){
				if(pthread_mutex_trylock(&sw) == 0){
					whileSwitch = 0;
					intersection[1][0] = car;
					intersection[0][0] = sentinel;

					pthread_mutex_unlock(&nw);
				}
			}

			// Leaves intersection
			*held_cars[car->pid] = car;
			intersection[1][0] = sentinel;
			pthread_mutex_unlock(&sw);
		}

		else if(direction == TURN_RIGHT){
			while(whileSwitch){ // Try to enter intersection, wait until it can
				if(pthread_mutex_trylock(&ne) == 0){
					whileSwitch = 0;
					intersection[0][1] = car;
					eApproach = sentinel;
				}
			}

			// Move out of intersection and unlock quadrant
			*held_cars[car->pid] = car;
			intersection[0][1] = sentinel;
			pthread_mutex_unlock(&ne);
		}

		else if(direction == TURN_STRAIGHT){
			while(whileSwitch){ // Tries to enter intersection
				if(pthread_mutex_trylock(&ne) == 0){
					whileSwitch = 0;
					intersection[0][1] = car;
					eApproach = sentinel;
				}
			}

			whileSwitch = 1; // Reset switch

			// Tries to enter next quadrant
			while(whileSwitch){
				if(pthread_mutex_trylock(&nw) == 0){
					whileSwitch = 0;
					intersection [0][0] = car;
					intersection[0][1] = sentinel;

					pthread_mutex_unlock(&ne);
				}
			}

			// Leaves intersection
			*held_cars[car->pid] = car;
			intersection[0][0] = sentinel;
			pthread_mutex_unlock(&nw);
		}

		printf("Car %d [Entered EAST, turned direction %d] has left the intersection.\n", car->pid, direction);
	}
	

	else if(approach == WEST){
		// Determine where car is turning
		if(direction == TURN_LEFT){
			while(whileSwitch){ // Tries to enter intersection
				if(pthread_mutex_trylock(&sw) == 0){
					whileSwitch = 0;
					intersection[1][0] = car;
					wApproach = sentinel;
				}
			}

			whileSwitch = 1; // Reset switch

			// Tries to enter next quadrant
			while(whileSwitch){
				if(pthread_mutex_trylock(&se) == 0){
					whileSwitch = 0;
					intersection [1][1] = car;
					intersection[1][0] = sentinel;

					pthread_mutex_unlock(&sw);
				}
			}

			whileSwitch = 1;

			while(whileSwitch){
				if(pthread_mutex_trylock(&ne) == 0){
					whileSwitch = 0;
					intersection[0][1] = car;
					intersection[1][1] = sentinel;

					pthread_mutex_unlock(&se);
				}
			}

			// Leaves intersection
			*held_cars[car->pid] = car;
			intersection[1][1] = sentinel;
			pthread_mutex_unlock(&ne);
		}

		else if(direction == TURN_RIGHT){
			while(whileSwitch){ // Try to enter intersection, wait until it can
				if(pthread_mutex_trylock(&sw) == 0){
					whileSwitch = 0;
					intersection[1][0] = car;
					wApproach = sentinel;
				}
			}

			// Move out of intersection and unlock quadrant
			*held_cars[car->pid] = car;
			intersection[1][0] = sentinel;
			pthread_mutex_unlock(&sw);
		}

		else if(direction == TURN_STRAIGHT){
			while(whileSwitch){ // Tries to enter intersection
				if(pthread_mutex_trylock(&sw) == 0){
					whileSwitch = 0;
					intersection[1][0] = car;
					wApproach = sentinel;
				}
			}

			whileSwitch = 1; // Reset switch

			// Tries to enter next quadrant
			while(whileSwitch){
				if(pthread_mutex_trylock(&se) == 0){
					whileSwitch = 0;
					intersection [1][1] = car;
					intersection[1][0] = sentinel;

					pthread_mutex_unlock(&sw);
				}
			}

			// Leaves intersection
			*held_cars[car->pid] = car;
			intersection[1][1] = sentinel;
			pthread_mutex_unlock(&se);
		}

		printf("Car %d [Entered WEST, turned direction %d] has left the intersection.\n", car->pid, direction);
	}

	return 0;
}