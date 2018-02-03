/*
 * Heric Flores-Huerta
 * Robyn Domanico
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

void printStats(struct rusage rusage, double elapsedTime);

int main(int argc, char *argv[])
{

	struct timeval beginTime, endTime;
	gettimeofday(&beginTime,NULL);

	pid_t child;
	child = fork();
	
	if(child == -1)//failed fork returns -1 to the parent
		exit(1);
	else if(child > 0)//succesful for returns pid to parent
	{
		int status;
		waitpid(child,&status,0);

		if(WIFEXITED(status) && WEXITSTATUS(status) !=0){
			exit(-1);
		}
	}
	else
	{	//use execvp to execute command in the child
		execvp(argv[1],&argv[1]);
		printf("Execvp was unable to execute correctly.\n");
		exit(-1);
	}

	gettimeofday(&endTime,NULL);
	//subtract beginTime(in seconds) from endTime(in seconds) to get process time from gettimeofday()
	double elapsedTime = (((endTime.tv_sec - beginTime.tv_sec) + 
						((endTime.tv_usec - beginTime.tv_usec)/1000000.0)) * 1000.0);

	struct rusage stats;
	getrusage(RUSAGE_CHILDREN, &stats);
	printStats(stats, elapsedTime);

	return 0;
}

void printStats(struct rusage rusage, double elapsedTime)
{
	//convert microseconds to seconds and add to seconds, then convert to milliseconds
	double userCPUTime = ((rusage.ru_utime.tv_sec + (rusage.ru_utime.tv_usec / 1000000.0)) * 1000.0);
	double sysCPUTime = ((rusage.ru_stime.tv_sec + (rusage.ru_stime.tv_usec / 1000000.0)) * 1000.0);

	//stats extracted from rusage struct
	long involuntaryInterrupt = rusage.ru_nivcsw;
	long voluntaryInterrupt = rusage.ru_nvcsw;
	long pageFaults = rusage.ru_majflt;
	long satisfiedFaults = rusage.ru_minflt;

	printf("\nElapsed Time: %f\n",elapsedTime);
	printf("User CPU Time(ms): %f\n",userCPUTime);
	printf("System CPU Time(ms): %f\n",sysCPUTime);
	printf("#of Involuntary Interrupts: %ld\n",involuntaryInterrupt);
	printf("#of Voluntary Interrupts: %ld\n",voluntaryInterrupt);
	printf("#of Page Faults: %ld\n", (pageFaults + satisfiedFaults));
	printf("#of Satisfied Page Faults: %ld\n",satisfiedFaults);
}
