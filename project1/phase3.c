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
void printBgStats(struct rusage rusage, double elapsedTime);
void jobs(pid_t pids[], char *pnames[], int size);
char* remJobs(pid_t pids[], char *pnames[],pid_t job, int size);
int findHighestProcessNum(pid_t pids[], int size);

int main(int argc, char *argv[])
{
	pid_t child;
	struct timeval beginTime, endTime;
	struct rusage beginStats, beginBgStats;
	struct rusage bgStats;
	getrusage(RUSAGE_CHILDREN, &beginStats);

	pid_t pids[32];
	char *pnames[32];

	char buf[256];
	char *tokens;
	char *arguments[36];
	int i = 0;
	int forkSwitch = 1;
	int status, bgStatus;
	double elapsedTime;
	int bgSwitch = 0;
	int numProcesses = 0;
	int jobNum = 0;
	pid_t bgPid = 0;
	int exitSwitch = 0;

	for(i = 0; i < 32; i++){
		pids[i] = 0;
		pnames[i] = malloc(100);
	}
	i = 0;

	while(1)
	{
		printf("==>");
		//fgets returns stardard input including newline characters
		fgets(buf, sizeof(buf), stdin);

		//feof looks for end of file and newline character to exit shell
		if(buf[0] == '\n'){
			if(numProcesses > 0){
				printf("There are still child processes running.\n");
				// wait;
				// exit(0);
				exitSwitch = 1;
				forkSwitch = 0;
			}
			else{
				exit(0);
			}
		}
		else if(feof(stdin)){
			exit(0);
		}

		//////////////////////////////////////////////////////////////////////////////
		//								Tokenizing									//
		//////////////////////////////////////////////////////////////////////////////

		if(exitSwitch == 0)
		{
			//separate line using space and newline as delimiter
			tokens = strtok(buf, " \n");
			while(tokens != NULL)
			{//while line is not empty

				if(strcmp(tokens, "&") == 0){
					bgSwitch = 1;
					numProcesses++;
					break;
				}
				else{
					arguments[i++] = tokens;//put token into arguements buffer
				}

				tokens = strtok(NULL, " \n");//get next token
			}
			arguments[i++] = NULL;//end arguement array with NULL pointer for execvp

			//////////////////////////////////////////////////////////////////////////////
			//							Non-Forking Commands							//
			//////////////////////////////////////////////////////////////////////////////

			//if command is exit, exit the shell
			if(!strcasecmp(arguments[0], "exit")){
				if(numProcesses > 0){
					printf("There are still child processes running.\n");
					// wait;
					// exit(0);
					forkSwitch = 0;
					exitSwitch = 1;
				}
				else{
					exit(0);
				}
			}
			else if(!strcasecmp(arguments[0], "cd"))
			{	//if command is cd, use chdir(), and don't fork
				int dirStatus = chdir(arguments[1]);

				if(dirStatus == 0){//chdir returns 0 on success
					forkSwitch = 0;
				}
				else{
					printf("Directory was not changed.\n");
					forkSwitch = 0;
				}
			}
			else if(!strcasecmp(arguments[0], "jobs")){
				jobs(pids, pnames, 32);
				forkSwitch = 0;
			}
		}

		//////////////////////////////////////////////////////////////////////////////
		//								Forking Commands							//
		//////////////////////////////////////////////////////////////////////////////

		gettimeofday(&beginTime,NULL);
		if(forkSwitch == 1)//if command requires forking
		{
			child = fork();
			
			if(child == -1)//failed fork returns -1 to the parent
				exit(1);
			else if(child > 0)//successful fork returns pid to parent
			{

				if(bgSwitch == 1){
					jobNum = findHighestProcessNum(pids, 32);

					pids[jobNum + 1] = child;
					strcpy(pnames[jobNum + 1], arguments[0]);

					waitpid(child,&bgStatus,WNOHANG);
				}
				else{
					waitpid(child,&status,0);
				}

				if(WIFEXITED(status) && WEXITSTATUS(status) !=0){
					forkSwitch = 0;
				}
			}
			else
			{	//use execvp to execute command in the child
				printf("\n");
				execvp(arguments[0],&arguments[0]);
				printf("Execvp was unable to execute correctly.\n");
				exit(-1);
			}

		}

		//////////////////////////////////////////////////////////////////////////////
		//							Process Statistics								//
		//////////////////////////////////////////////////////////////////////////////

		gettimeofday(&endTime,NULL);
		//subtract beginTime from endTime to get process time
		double elapsedTime = (((endTime.tv_sec - beginTime.tv_sec) + 
							((endTime.tv_usec - beginTime.tv_usec)/1000000.0)) * 1000.0);
		struct rusage stats;
		getrusage(RUSAGE_CHILDREN, &stats);

		if(forkSwitch == 1 && bgSwitch == 0){//if fork was used
			printf("\nStats from %s", arguments[0]);
			printStats(beginStats, stats, elapsedTime);
		}

		bgPid = wait3(&bgStatus, WNOHANG, &bgStats);
		while(bgPid != 0 && bgPid != -1)
		{
			char *jobName;
			jobName = remJobs(pids, pnames, bgPid, 32);
			printf("\nJob %d [%s] has finished.\n", bgPid, jobName);
			printBgStats(bgStats, elapsedTime);
			numProcesses--;
			bgPid = wait3(&bgStatus,WNOHANG,&bgStats);
		}

		beginStats = stats;
		forkSwitch = 1;
		exitSwitch = 0;
		bgSwitch = 0;
		i = 0;
	}

	return 0;
}

		//////////////////////////////////////////////////////////////////////////////
		//							Helper Functions								//
		//////////////////////////////////////////////////////////////////////////////

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
	printf("#of Page Faults: %ld\n",(pageFaults + satisfiedFaults));
	printf("#of Satisfied Page Faults: %ld\n",satisfiedFaults);
}

void printBgStats(struct rusage rusage, double elapsedTime){
	//doubles for calculating total user and system CPU time (end_time - start_time)
	double end_userCPUTime = ((rusage.ru_utime.tv_sec + (rusage.ru_utime.tv_usec / 1000000.0)) * 1000.0);
	double end_sysCPUTime = ((rusage.ru_stime.tv_sec + (rusage.ru_stime.tv_usec / 1000000.0)) * 1000.0);
	double userCPUTime = end_userCPUTime;
	double sysCPUTime = end_sysCPUTime;

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
	printf("#of Page Faults: %ld\n",pageFaults);
	printf("#of Satisfied Page Faults: %ld\n",satisfiedFaults);
}

void jobs(pid_t pids[], char *pnames[], int size){
	int i;
	for(i = 0; i < size; i++){
		if(pids[i] != 0){
			printf("[%d] %d %s\n", i, pids[i], pnames[i]);
		}
	}

	return;
}

char* remJobs(pid_t pids[], char *pnames[], pid_t job, int size){
	int i;
	char *jobName;
	for(i = 0; i < size; i++){
		if(pids[i] == job){
			pids[i] = 0;
			strcpy(jobName, pnames[i]);
		}
	}

	return jobName;
}

int findHighestProcessNum(pid_t pids[], int size){
	int i;
	int max = 0;
	for(i = 0; i < size; i++){
		if(pids[i] != 0){
			max = i;
		}
	}

	return max;
}
