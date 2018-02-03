#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <linux/kernel.h>
#include <sys/syscall.h>

int main(int argc, char *argv[]){
	// Initialize variables
	int targetUid = atoi(argv[1]);
	int numPidsSmited = 0;
	int smitedPids[100];
	long smitedStates[100];

	int smiteResult,unsmiteResult; // Return status of smite and unsmite

	printf("User %d is testing smite/unsmite\n", getuid());
	printf("User to smite is %d\n", targetUid);

	// Run smite
	smiteResult = syscall(356, &targetUid, &numPidsSmited, smitedPids, smitedStates);
	if(smiteResult != 0){
		printf("Smite did not return 0, it returned %d instead!\n", smiteResult);
	}

	// Delay to test whether smite actually was effective
	printf("Wait here!\n");
	sleep(15);

	unsmiteResult = syscall(357, &numPidsSmited, smitedPids, smitedStates);
	if(unsmiteResult != 0){
		printf("Unsmite did not return 0, it returned %d instead!\n", unsmiteResult);
	}

	return 0;
}