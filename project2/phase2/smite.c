/*
 *Heric Flores-Huerta
 *Robyn Domanico
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/cred.h>


unsigned long **sys_call_table;
asmlinkage long (*ref_sys_cs3013_syscall2)(unsigned short *target_uid, int *num_pids_smited,
	int *smited_pids, long *pid_states);
asmlinkage long (*ref_sys_cs3013_syscall3)(int *num_pids_smited,
int *smited_pids, long *pid_states);

// Smite function
asmlinkage long smite(unsigned short *target_uid, int *num_pids_smited,
	int *smited_pids, long *pid_states) 
{
	//Variables where user arguments are copied
	int smitedPids[100];
	long pidStates[100];

	// Initialize variables
	kuid_t uid = current_uid();
	struct task_struct *tasks;
	int counter = 0;
	int taskPid;
	long taskState; 

	//Check for root access or not
	if(uid.val == 0){

		for_each_process(tasks){ // Iterate through processes
			if(counter >= 100){

				// Write smite lists and counter back to user space
				if(copy_to_user(smited_pids, smitedPids, sizeof(int)*100)){
					return EFAULT;
				}
				if(copy_to_user(pid_states, pidStates, sizeof(long)*100)){
					return EFAULT;
				}
				if(copy_to_user(num_pids_smited, &counter, sizeof(counter))){
					return EFAULT;
				}

				return 0; // Indicate successful return
			}
			else if(tasks->real_cred->uid.val == *target_uid){ //Check if process has the same uid as target
				taskPid = tasks->pid;
				taskState = tasks->state;

				//Check if process is already unrunnable
				if(taskState != 0){
					// Move on to next process
				}
				else{ //Process is not already unrunnable, make it unrunnable
					smitedPids[counter] = taskPid;
					pidStates[counter] = taskState;

					// Print to syslog
					printk(KERN_INFO "User %d is smiting process %d [%s]\n", uid.val, taskPid, tasks->comm);

					// Make process unrunnable
					tasks->state = TASK_UNINTERRUPTIBLE;

					// Increment counter
					counter++;
				}
			}
		}
	}
	else{
		//User is not root, do nothing
		printk(KERN_INFO "A user tried to smite someone, but they are not root!\n");
	}

	// Write smite lists back to user space, if there were not 100 processes
	if(copy_to_user(smited_pids, smitedPids, sizeof(int)*100)){
		return EFAULT;
	}
	if(copy_to_user(pid_states, pidStates, sizeof(long)*100)){
		return EFAULT;
	}
	if(copy_to_user(num_pids_smited, &counter, sizeof(counter))){
		return EFAULT;
	}
	
	return 0;
}

//Unsmite function
asmlinkage long unsmite(int *num_pids_smited,
int *smited_pids, long *pid_states) 
{
	//Initialize variables
	kuid_t pid = current_uid();
	int i;
	int taskPid;
	long taskState;
	struct task_struct *processTask;

	//Variables where user arguments are copied
	int smitedPids[100];
	long pidStates[100];
	int numPidsSmited;

	//Copy all arguments into kernel space.
	if(copy_from_user(smitedPids, smited_pids, sizeof(int)*100)){
		return EFAULT;
	}
	if(copy_from_user(pidStates, pid_states, sizeof(long)*100)){
		return EFAULT;
	}
	if(copy_from_user(&numPidsSmited, num_pids_smited, sizeof(int))){
		return EFAULT;
	}

	//Check for root
	if(pid.val == 0){
		for(i = 0; i < numPidsSmited; i++){
			//Get copy of pid and state of process to restore
			taskPid = smitedPids[i];
			taskState = pidStates[i];

			//Get task_struct of process to restore, and restore state
			for_each_process(processTask){ //Loop through processes
				if(processTask->pid == taskPid){ // Check if pid is desired process
					wake_up_process(processTask);
					printk(KERN_INFO "The process task state of %d [%s] is %ld\n", processTask->pid, processTask->comm, processTask->state);
				}
			}

			//Clear entry in smited lists
			smitedPids[i] = 0;
			pidStates[i] = 0;
		}
	}
	else{
		//User is not root, do nothing
		printk(KERN_INFO "A user tried to unsmite someone, but they are not root!\n");
	}
	
	return 0; //Indicate successful return
}

static unsigned long **find_sys_call_table(void) 
{
	unsigned long int offset = PAGE_OFFSET;
	unsigned long **sct;
	while (offset < ULLONG_MAX) 
	{
		sct = (unsigned long **)offset;

		if (sct[__NR_close] == (unsigned long *) sys_close) 
		{
		printk(KERN_INFO "Interceptor: Found syscall table at address: 0x%02lX",(unsigned long) sct);
		return sct;
		}
		offset += sizeof(void *);
	}
	
	return NULL;
}


static void disable_page_protection(void) 
{
	/*
	Control Register 0 (cr0) governs how the CPU operates.
	Bit #16, if set, prevents the CPU from writing to memory marked as
	read only. Well, our system call table meets that description.
	But, we can simply turn off this bit in cr0 to allow us to make
	changes. We read in the current value of the register (32 or 64
	bits wide), and AND that with a value where all bits are 0 except
	the 16th bit (using a negation operation), causing the write_cr0
	value to have the 16th bit cleared (with all other bits staying
	the same. We will thus be able to write to the protected memory.
	It’s good to be the kernel!
	*/
	write_cr0 (read_cr0 () & (~ 0x10000));
}


static void enable_page_protection(void) 
{
	/*
	See the above description for cr0. Here, we use an OR to set the
	16th bit to re-enable write protection on the CPU.
	*/
	write_cr0 (read_cr0 () | 0x10000);
}


static int __init interceptor_start(void) 
{
	/* Find the system call table */
	if(!(sys_call_table = find_sys_call_table())) {
		/* Well, that didn’t work.
		Cancel the module loading step. */
		return -1;
	}

	/* Store a copy of all the existing functions */
	ref_sys_cs3013_syscall2 = (void *)sys_call_table[__NR_cs3013_syscall2];
	ref_sys_cs3013_syscall3 = (void *)sys_call_table[__NR_cs3013_syscall3];
	
	/* Replace the existing system calls */
	disable_page_protection();
	sys_call_table[__NR_cs3013_syscall2] = (unsigned long *)smite;
	sys_call_table[__NR_cs3013_syscall3] = (unsigned long *)unsmite;
	enable_page_protection();
	
	/* And indicate the load was successful */
	printk(KERN_INFO "Loaded interceptor!");
	
	return 0;
}

static void __exit interceptor_end(void) {

	/* If we don’t know what the syscall table is, don’t bother. */
	if(!sys_call_table)
		return;
	
	/* Revert all system calls to what they were before we began. */
	disable_page_protection();
	sys_call_table[__NR_cs3013_syscall2] = (unsigned long *)ref_sys_cs3013_syscall2;
	sys_call_table[__NR_cs3013_syscall3] = (unsigned long *)ref_sys_cs3013_syscall3;
	enable_page_protection();
	
	printk(KERN_INFO "Unloaded interceptor!");
}
MODULE_LICENSE("GPL");
module_init(interceptor_start);
module_exit(interceptor_end);