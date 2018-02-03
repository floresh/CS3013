/*
 * Robyn Domanico
 * Heric Flores-Huerta
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

void printStats(struct rusage startusage, struct rusage rusage, double elapsedTime);

int main(int argc, char *argv[])
{
	pid_t child;
	struct timeval beginTime, endTime;
	struct rusage beginStats;
	getrusage(RUSAGE_CHILDREN, &beginStats);

	char buf[256];
	char *tokens;
	char *arguments[36];
	int i = 0;
	int forkSwitch = 1;
	double elapsedTime;
	int execStatus = 0;

	while(1)
	{
		printf("==>");
		//fgets returns stardard input including newline characters
		fgets(buf, sizeof(buf), stdin);

		//feof looks for end of file and newline character to exit shell
		if(feof(stdin) || buf[0] == '\n'){
			exit(0);
		}

		//separate line using space and newline as delimiter
		tokens = strtok(buf, " \n");
		while(tokens != NULL)
		{//while line is not empty
			arguments[i++] = tokens;//put token into arguements buffer
			tokens = strtok(NULL, " \n");//get next token
		}
		arguments[i++] = NULL;//end arguement array with NULL pointer for execvp

		//if command is exit, exit the shell
		if(!strcasecmp(arguments[0], "exit")){
			exit(0);
		}
		else if(!strcasecmp(arguments[0], "cd"))
		{	//if command is cd, use chdir(), and don't fork
			printf("Directory is %s\n", arguments[1]);
			int dirStatus = chdir(arguments[1]);

			if(dirStatus == 0){//chdir returns 0 on success
				forkSwitch = 0;
			}
			else{
				printf("Directory was not changed.\n");
				forkSwitch = 0;
			}
		}

		gettimeofday(&beginTime,NULL);
		if(forkSwitch == 1)//if command requires forking
		{
			child = fork();
			
			if(child == -1)//failed fork returns -1 to the parent
				exit(1);
			else if(child > 0)//successful fork returns pid to parent
			{
				int status;
				waitpid(child,&status,0);

				if(WIFEXITED(status) && WEXITSTATUS(status) !=0){
					forkSwitch = 0;
				}
			}
			else
			{	//use execvp to execute command in the child
				execvp(arguments[0],&arguments[0]);
				printf("Execvp was unable to execute correctly.\n");
				exit(-1);
			}

		}

		gettimeofday(&endTime,NULL);
		//subtract beginTime from endTime to get process time
		double elapsedTime = (((endTime.tv_sec - beginTime.tv_sec) + 
							((endTime.tv_usec - beginTime.tv_usec)/1000000.0)) * 1000.0);
		struct rusage stats;
		getrusage(RUSAGE_CHILDREN, &stats);

		if(forkSwitch == 1 && execStatus != -1){//if fork was used
			printf("\nStats from %s", arguments[0]);
			printStats(beginStats, stats, elapsedTime);
		}
		
		beginStats = stats;
		forkSwitch = 1;
		i = 0;
	}

	return 0;
}

void printStats(struct rusage startusage, struct rusage rusage, double elapsedTime)
{
	//doubles for calculating total user and system CPU time (end_time - start_time)
	double end_userCPUTime = ((rusage.ru_utime.tv_sec + (rusage.ru_utime.tv_usec / 1000000.0)) * 1000.0);
	double end_sysCPUTime = ((rusage.ru_stime.tv_sec + (rusage.ru_stime.tv_usec / 1000000.0)) * 1000.0);
	double start_userCPUTime = ((startusage.ru_utime.tv_sec + (startusage.ru_utime.tv_usec / 1000000.0)) * 1000.0);
	double start_sysCPUTime = ((startusage.ru_stime.tv_sec + (startusage.ru_stime.tv_usec / 1000000.0)) * 1000.0);
	double userCPUTime = end_userCPUTime - start_userCPUTime;
	double sysCPUTime = end_sysCPUTime - start_sysCPUTime;

	//stats extracted from rusage struct
	long involuntaryInterrupt = rusage.ru_nivcsw - startusage.ru_nivcsw;
	long voluntaryInterrupt = rusage.ru_nvcsw - startusage.ru_nvcsw;
	long pageFaults = rusage.ru_majflt - startusage.ru_majflt;
	long satisfiedFaults = rusage.ru_minflt - startusage.ru_minflt;

	printf("\nElapsed Time: %f\n",elapsedTime);
	printf("User CPU Time(ms): %f\n",userCPUTime);
	printf("System CPU Time(ms): %f\n",sysCPUTime);
	printf("#of Involuntary Interrupts: %ld\n",involuntaryInterrupt);
	printf("#of Voluntary Interrupts: %ld\n",voluntaryInterrupt);
	printf("#of Page Faults: %ld\n", (pageFaults + satisfiedFaults));
	printf("#of Satisfied Page Faults: %ld\n",satisfiedFaults);
}
