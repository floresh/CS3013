#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>


unsigned long **sys_call_table;
asmlinkage int (*ref_sys_open)(const char *path, int oflags, mode_t mode);
asmlinkage int (*ref_sys_close)(int fd);
asmlinkage ssize_t (*ref_sys_read)(int fd, void *buf, size_t count);

asmlinkage int new_open(const char *path, int oflags, mode_t mode) 
{
	kuid_t pid = current_uid();
	// Check whether user is a regular user
	if(pid.val >= 1000){
		printk(KERN_INFO "User %d is opening file: %s\n", pid.val, path);
	}

	return ref_sys_open(path, oflags, mode);
}

asmlinkage int new_close(int fd){
	kuid_t pid = current_uid();

	//Check whether user is a regular user
	if(pid.val >= 1000){
		printk(KERN_INFO "User %d is closing file descriptor: %d\n", pid.val, fd);
	}

	return ref_sys_close(fd);
}

asmlinkage ssize_t new_read(int fd, void *buf, size_t count){
	ssize_t readResult;
	kuid_t pid = current_uid();
	readResult = ref_sys_read(fd, buf, count);

	if(pid.val >= 1000){
		if(strstr(buf, "virus") != NULL){
			printk(KERN_INFO "User %d read from file descriptor %d, but that read contained a virus!\n", pid.val, fd);
		}
	}

	return readResult;
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
	ref_sys_open = (void *)sys_call_table[__NR_open];
	ref_sys_close = (void *)sys_call_table[__NR_close];
	ref_sys_read = (void *)sys_call_table[__NR_read];
	
	/* Replace the existing system calls */
	disable_page_protection();
	sys_call_table[__NR_open] = (unsigned long *)new_open;
	sys_call_table[__NR_close] = (unsigned long *)new_close;
	sys_call_table[__NR_read] = (unsigned long *)new_read;
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
	sys_call_table[__NR_open] = (unsigned long *)ref_sys_open;
	sys_call_table[__NR_close] = (unsigned long *)ref_sys_close;
	sys_call_table[__NR_read] = (unsigned long *)ref_sys_read;
	enable_page_protection();
	
	printk(KERN_INFO "Unloaded interceptor!");
}
MODULE_LICENSE("GPL");
module_init(interceptor_start);
module_exit(interceptor_end);