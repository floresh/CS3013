# Project 4: Paging

 - Authors: Robyn Domanico, Heric Flores-Huerta
 - CS Class: CS3013- Operating Systems
 - Date: 2/17/2015
 - Programming Language: C
 - OS/Hardware Dependencies: Linux


### Problem Description:

The program is intended to simulate a paging system, with 3 levels: RAM, SSD, and the Hard Disk. Program must simulate allocating and swapping/evicting pages.

### Overall Design:

The program contains an api of page functions (allocateNewInt, accessIntPtr, unlockMemory, freeMemory) as well as eviction algorithms for the page fault handler.

### Implementation Details:	

The program creates 1000 pages (200 for each of the 5 threads in multithreaded mode), gets the intPtr of the page, unlocks it, and then frees all the pages at the end. Page eviction is done by using the second chance or least recently used algorithms.

### How to Build the Program:

To build, run the makefile. Program takes two arguments: the first argument is 0 for single threaded, 1 for multi threaded. The second argument only applies when running in single threaded mode: 0 is for the standard test, 1 tests exceeding the amount of pages, and 2 tests trying to create new pages when all pages in a memory hierarchy are locked, and cannot be evicted.

### Additional Notes:

There are no problems with our single threaded execution, but our multithreaded code does not work when accessIntPtr is run (it runs fine when this function is not used) There are also three constants that define how the program is run. these can be found in table.h at lines 7-9 and operate as such:

`DEBUG`: when set to 1, prints information about paging operations. Default state is 1.

`LRU_ON`: determines which algorithm is used. 1 for LRU algorithm, 0 for second chance. 0 is the default state.

`FAST_RUN`: set to 1 to skip usleep statements for fast testing. 0 will run usleep statements and is the default.
