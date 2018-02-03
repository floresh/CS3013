# Project 1: Shell 

 - Authors: Heric Flores-Huerta, Robyn Domanico
 - CS Class: CS3013- Operating Systems
 - Date: 1/27/2015
 - Programming Language: C
 - OS/Hardware Dependencies: Linux


### Problem Description:

The assignment was to write 3 programs, the first of which reads in a command via command line argument and executes it. The second program executes commands in a shell environment, supporting the non-forked commands exit and cd. The third is similar to the second, but also supports the jobs command and has backgrounding capabilities.

### Overall Design:

In phase 1, the program reads in the command line arguments from `argv[]` and forks: the child process then passes the command to `execvp()`, where the command is run. Phases 2 and 3 operate very similar to each other: the user is given a prompt, and the input is parsed into tokens and placed in an array similar to `argv[]` in phase 1. The input is checked for non-forking commands (exit and cd in phase 2, with the added command jobs in phase 3)- the process will fork otherwise, and execute the given command through `execvp()`. The programs then calculate and print statistics.

Phase 3 also supports backgrounding through the use of the `&` command. The program will wait for the child with the `WNOHANG` option, allowing the user to type other commands. The program will check for any commands that also finish, printing their statistics and removing them from the jobs list. If there are no finished child processes, the program will then prompt the user again.

### Program Assumptions:

Phase 1 assumes that the command is given as a command line argument. Phases 2 and 3 will handle both input from a user and from a file. All phases can handle an illegal command (for example, the command asdf, which does not exist)- phases 2 and 3 will print an error and give the prompt to the user again. Phases 2 and 3 assume no more than 256 characters per input line, and no more than 36 arguments per input line. The jobs list in phase 3 has a limit of 32 places, meaning that no more than 32 different background jobs may be run at once.

### Interfaces:

All input is through the command line. Program interaction is similar to a bash shell (e.g can run commands like make or execute programs)

### Implementation Details:

All phases fork to create a new child process. If the child pid = 0, we are in the child and run execvp. If we are not in the child, we wait for the child (this is different for backgrounding). Stats are printed afterward. 

For backgrounding processes, the program checks for the `&` argument. If this is found, we increase the number of processes by 1. The process forks, and the command is added to the jobs list (by pid and name, such as pwd or ls). The parent waits for the child with the `WNOHANG` option, and the parent proceeds. After we print statistics for any foreground process that was run, we check to see if `wait3` has returned the pid of any children. While this value is not 0 or -1, we continue to loop to check for children, removing them from the jobs list, decrementing the number of background processes, and printing the statistics for the finished process. We then check `wait3` again to make sure that there are no other finished processes waiting.

### How to Build the Program:

To build all 3 phases, run `make`. Individual phases can be built using `make <name of program>` (ex. make shell).
